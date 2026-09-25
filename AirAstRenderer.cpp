/*
* Copyright 2026 Rochus Keller <mailto:me@rochus-keller.ch>
*
* This file is part of the AIR (Algol Intermediate Representation) library.
*
* The following is the license that applies to this copy of the
* file. For a license to use the file under conditions
* other than those described here, please email to me@rochus-keller.ch.
*
* GNU General Public License Usage
* This file may be used under the terms of the GNU General Public
* License (GPL) versions 2.0 or 3.0 as published by the Free Software
* Foundation and appearing in the file LICENSE.GPL included in
* the packaging of this file. Please review the following information
* to ensure GNU General Public Licensing requirements will be met:
* http://www.fsf.org/licensing/licenses/info/GPLv2.html and
* http://www.gnu.org/copyleft/gpl.html.
*/

#include <Algol60/AirAstRenderer.h>
#include <Algol60/AirValidator.h>
using namespace Air;
using Alg::RowCol;

static QByteArray format(const Quali& q)
{
    if( q.first.isEmpty() )
        return q.second;
    return q.first + "!" + q.second;
}

static QByteArray format(const Trident& t)
{
    return format(t.first) + "." + t.second;
}

AstRenderer::AstRenderer(AstModel* mdl, bool runValidator):
    mdl(mdl),module(0),done(0),curProc(0),curType(0),runValidator(runValidator),toDelete(false)
{
    Q_ASSERT( mdl );
}

AstRenderer::~AstRenderer()
{
}

void AstRenderer::beginModule(const QByteArray& moduleName, const QString& sourceFile)
{
    Q_ASSERT( module == 0 );
    errors.clear();
    unresolved.clear();
    toDelete = false;
    module = new Declaration();
    module->kind = Declaration::Module;
    module->name = moduleName;
    module->pos = curPos;
    module->getMd()->source = sourceFile;
    if( !mdl->addModule(module) )
    {
        error("a module of this name already exists in the model");
        toDelete = true;
    }
}

void AstRenderer::endModule()
{
    Q_ASSERT( module != 0 );
    module->getMd()->end = curPos;
    resolveAll(true);
    resolveLater();

    if( toDelete )
    {
        delete module;
        module = 0;
        return;
    }

    if( !errors.isEmpty() )
        module->hasErrors = true;
    else if( runValidator )
    {
        Validator v(mdl);
        if( !v.validate(module) )
        {
            module->hasErrors = true;
            errors << v.errors;
        }
    }
    done = module;
    module = 0;
}

Declaration* AstRenderer::addDecl(quint8 declKind, const QByteArray& name, bool isPublic)
{
    Q_ASSERT( module );
    Declaration* d = new Declaration();
    d->kind = Declaration::Kind(declKind);
    d->name = name;
    d->public_ = isPublic;
    d->pos = curPos;
    module->appendSub(d);
    return d;
}

void AstRenderer::addImport(const QByteArray& moduleName)
{
    Q_ASSERT( module );
    Declaration* import = addDecl(Declaration::Import, moduleName, false);
    import->imported = mdl->findModuleByName(moduleName);
    if( import->imported == 0 )
    {
        // the imported module is not in the model
        // represent it by a stub so that references into it can be resolved and rendered again
        Declaration* stub = new Declaration();
        stub->kind = Declaration::Module;
        stub->name = moduleName;
        stub->stub = true;
        import->imported = stub;
        import->stub = true;
    }
}

void AstRenderer::addVariable(const Quali& typeRef, const QByteArray& name, bool isPublic)
{
    Declaration* var = addDecl(Declaration::VarDecl, name, isPublic);
    var->setType(derefType(typeRef));
}

void AstRenderer::addConst(const Quali& typeRef, const QByteArray& name, const QVariant& val)
{
    Declaration* co = addDecl(Declaration::ConstDecl, name, true);
    co->setType(derefType(typeRef));
    co->c = Constant::fromVariant(val);
    if( co->c->kind == Constant::Invalid )
        error(QString("invalid constant value of '%1'").arg(name.constData()));
}

void AstRenderer::beginType(const QByteArray& name, bool isPublic, quint8 typeKind, const Quali& base)
{
    Q_ASSERT( curType == 0 );
    Declaration* decl = addDecl(Declaration::TypeDecl, name, isPublic);
    curType = new Type();
    curType->pos = curPos;
    decl->setType(curType);
    curType->decl = decl;
    switch( typeKind )
    {
    case EmiTypes::Struct:
        curType->kind = Type::Struct;
        break;
    case EmiTypes::Record:
        curType->kind = Type::Record;
        if( !base.second.isEmpty() )
            curType->setType(derefType(base));
        break;
    case EmiTypes::CStruct:
        curType->kind = Type::CStruct;
        break;
    case EmiTypes::CUnion:
        curType->kind = Type::CUnion;
        break;
    default:
        error(QString("invalid type kind of '%1'").arg(name.constData()));
        break;
    }
}

void AstRenderer::endType()
{
    curType = 0;
    resolveAll();
}

void AstRenderer::addType(const QByteArray& name, bool isPublic, const Quali& baseType,
                          quint8 typeKind, quint32 len)
{
    Q_ASSERT( curType == 0 );
    Declaration* decl = addDecl(Declaration::TypeDecl, name, isPublic);
    Type* t = new Type();
    t->pos = curPos;
    decl->setType(t);
    t->decl = decl;
    switch( typeKind )
    {
    case EmiTypes::Alias:
        t->kind = Type::NameRef;
        t->quali = new Quali(baseType);
        unresolved << t;
        break;
    case EmiTypes::Pointer:
    case EmiTypes::CPointer:
        t->kind = typeKind == EmiTypes::Pointer ? Type::Pointer : Type::CPointer;
        t->setType(derefType(baseType));
        break;
    case EmiTypes::Array:
    case EmiTypes::CArray:
        t->kind = typeKind == EmiTypes::Array ? Type::Array : Type::CArray;
        t->setType(derefType(baseType));
        t->len = len; // after setType because len and quali share the union
        break;
    default:
        error(QString("invalid type kind of '%1'").arg(name.constData()));
        break;
    }
}

void AstRenderer::addField(const QByteArray& fieldName, const Quali& typeRef, bool isPublic)
{
    if( curType == 0 )
    {
        addVariable(typeRef, fieldName, isPublic);
        return;
    }
    Declaration* field = new Declaration();
    field->kind = Declaration::Field;
    field->name = fieldName;
    field->public_ = isPublic;
    field->pos = curPos;
    field->outer = curType->decl;
    field->setType(derefType(typeRef));
    curType->subs.append(field);
}

void AstRenderer::addProcedure(const ProcData& proc)
{
    Q_ASSERT( module );
    if( proc.kind == ProcData::Invalid )
        return;

    resolveAll();

    const bool isType = proc.kind == ProcData::ProcType || proc.kind == ProcData::BoundProcType;

    Declaration* decl = new Declaration();
    decl->kind = isType ? Declaration::TypeDecl : Declaration::Procedure;
    decl->name = proc.name;
    decl->public_ = proc.isPublic;
    decl->pos = curPos;
    curProc = decl;

    Type* pt = 0;
    if( isType )
    {
        pt = new Type();
        pt->kind = Type::Proc;
        pt->pos = curPos;
        pt->typebound = proc.kind == ProcData::BoundProcType;
        decl->setType(pt);
        pt->decl = decl;
        pt->setType(derefType(proc.retType));
    }else
    {
        decl->setType(derefType(proc.retType));
        switch( proc.kind )
        {
        case ProcData::Init:
            decl->init = true;
            break;
        case ProcData::Extern:
            decl->extern_ = true;
            decl->getPd()->externalName = proc.binding;
            break;
        case ProcData::Foreign:
            decl->foreign_ = true;
            decl->getPd()->externalName = proc.binding;
            break;
        }
    }

    foreach( const ProcData::Var& param, proc.params )
    {
        Declaration* p = new Declaration();
        p->kind = Declaration::ParamDecl;
        p->name = param.name;
        p->varParam = param.isVarParam;
        p->pos = setLine(param.line);
        p->outer = decl;
        p->setType(derefType(param.type));
        if( pt )
            pt->subs.append(p);
        else
            decl->appendSub(p);
    }

    if( isType )
    {
        module->appendSub(decl);
        curProc = 0;
        return;
    }

    foreach( const ProcData::Var& local, proc.locals )
    {
        Declaration* l = new Declaration();
        l->kind = Declaration::LocalDecl;
        l->name = local.name;
        l->pos = setLine(local.line);
        l->setType(derefType(local.type));
        decl->appendSub(l);
    }

    if( !proc.binding.isEmpty() && (proc.kind == ProcData::Normal || proc.kind == ProcData::Init) )
    {
        // a bound procedure belongs to the record type it is bound to, not to the module
        Declaration* receiver = module->findSubByName(proc.binding);
        Type* rt = receiver && receiver->kind == Declaration::TypeDecl ? receiver->getType() : 0;
        if( rt )
            rt = rt->deref();
        if( rt == 0 || !rt->isRecord() )
        {
            error(QString("invalid receiver: %1").arg(proc.binding.constData()));
            delete decl;
            curProc = 0;
            return;
        }
        decl->typebound = true;
        decl->outer = receiver;
        rt->subs.append(decl);
        // keep the position in the module for the serializer
        Declaration* ph = addDecl(Declaration::Placeholder, proc.name, proc.isPublic);
        ph->imported = decl;
    }else
        module->appendSub(decl);

    if( proc.endLine )
        decl->getPd()->end = RowCol(proc.endLine, 1);

    if( !decl->extern_ && !decl->foreign_ )
    {
        curPos = decl->pos; // the lines of the params and locals don't apply to the body
        int pc = 0;
        decl->body = translateStat(proc.body, pc);
    }
    curProc = 0;
}

void AstRenderer::line(const RowCol& pos)
{
    curPos = pos;
}

RowCol AstRenderer::setLine(quint32 row)
{
    if( row )
        curPos = RowCol(row, 1);
    return curPos;
}

Type* AstRenderer::derefType(const Quali& q)
{
    if( q.second.isEmpty() )
        return 0;
    // a declared type shadows a predeclared type name
    Declaration* d = resolve(q, Declaration::TypeDecl);
    if( d && d->getType() )
        return d->getType();
    if( q.first.isEmpty() )
    {
        const quint8 k = AstModel::basicTypeKind(q.second);
        if( k != Type::Undefined )
            return mdl->getBasicType(k);
    }
    // not yet declared, or a stub; represent by a named reference and resolve later
    Type* ref = new Type();
    ref->kind = Type::NameRef;
    ref->pos = curPos;
    ref->quali = new Quali(q);
    unresolved << ref;
    return ref;
}

Declaration* AstRenderer::importOf(const QByteArray& moduleName)
{
    Declaration* d = module->findSubByName(moduleName);
    if( d && d->kind == Declaration::Import )
        return d->imported;
    return 0;
}

Declaration* AstRenderer::stubOf(Declaration* m, const QByteArray& name, quint8 declKind)
{
    Q_ASSERT( m && m->stub );
    Declaration* d = new Declaration();
    d->kind = declKind == Declaration::NoMode ? Declaration::VarDecl : Declaration::Kind(declKind);
    d->name = name;
    d->stub = true;
    d->public_ = true;
    if( d->kind == Declaration::TypeDecl )
    {
        Type* t = new Type();
        t->kind = Type::Undefined;
        d->setType(t);
        t->decl = d;
    }
    m->appendSub(d);
    return d;
}

Declaration* AstRenderer::resolve(const Quali& q, quint8 declKind)
{
    if( q.second.isEmpty() )
        return 0;
    if( q.first.isEmpty() || q.first == module->name )
    {
        Declaration* d = module->findSubByName(q.second);
        if( d == 0 )
            d = mdl->getGlobals()->findSubByName(q.second);
        return d;
    }
    Declaration* m = importOf(q.first);
    if( m == 0 )
        return 0;
    Declaration* d = m->findSubByName(q.second);
    if( d == 0 && m->stub )
        d = stubOf(m, q.second, declKind);
    return d;
}

Declaration* AstRenderer::resolve(const Trident& t, quint8 declKind)
{
    Declaration* d = resolve(t.first, Declaration::TypeDecl);
    if( d == 0 || d->kind != Declaration::TypeDecl || d->getType() == 0 )
        return 0;
    Type* ty = d->getType()->deref();
    Declaration* sub = ty->findSubByName(t.second);
    if( sub == 0 && d->stub )
    {
        sub = new Declaration();
        sub->kind = declKind == Declaration::NoMode ? Declaration::Field : Declaration::Kind(declKind);
        sub->name = t.second;
        sub->stub = true;
        sub->outer = d;
        ty->subs.append(sub);
    }
    return sub;
}

void AstRenderer::resolveAll(bool reportErrors)
{
    for( int i = 0; i < unresolved.size(); i++ )
    {
        Type* t = unresolved[i];
        Q_ASSERT( t && t->kind == Type::NameRef && t->quali );
        if( t->getType() )
            continue;
        Declaration* d = resolve(*t->quali, Declaration::TypeDecl);
        if( d && d->kind == Declaration::TypeDecl && d->getType() )
            t->setType(d->getType());
        else if( reportErrors )
        {
            if( d == 0 )
                error(QString("cannot resolve type reference: %1").arg(format(*t->quali).constData()));
            else
                error(QString("not a type declaration: %1").arg(format(*t->quali).constData()));
        }
    }
    if( reportErrors )
        unresolved.clear();
}

void AstRenderer::later(Declaration** slot, const QVariant& arg, quint8 declKind,
                        bool trident, int pc, const char* what)
{
    Pending p;
    p.slot = slot;
    p.arg = arg;
    p.proc = curProc;
    p.what = what;
    p.declKind = declKind;
    p.trident = trident;
    p.pc = pc;
    pending << p;
}

void AstRenderer::resolveLater()
{
    Declaration* outer = curProc;
    for( int i = 0; i < pending.size(); i++ )
    {
        const Pending& p = pending[i];
        Declaration* d = p.trident ? resolve(p.arg.value<Trident>(), p.declKind)
                                   : resolve(p.arg.value<Quali>(), p.declKind);
        *p.slot = d;
        if( d == 0 )
        {
            curProc = p.proc;
            const QByteArray name = p.trident ? format(p.arg.value<Trident>())
                                              : format(p.arg.value<Quali>());
            error(QString("cannot resolve %1: %2").arg(p.what).arg(name.constData()), p.pc);
        }
    }
    curProc = outer;
    pending.clear();
}

void AstRenderer::error(const QString& msg, int pc)
{
    Error e;
    e.msg = msg;
    e.where = curProc ? curProc->toPath() : ( module ? module->name : QByteArray() );
    e.pc = pc < 0 ? 0 : pc;
    errors << e;
}

bool AstRenderer::expect(const QList<ProcData::Op>& ops, int pc, quint8 op)
{
    if( pc >= ops.size() )
    {
        error(QString("expecting '%1' at the end of the operation sequence").arg(s_opName[op]), pc);
        return false;
    }
    if( ops[pc].op != op )
    {
        error(QString("expecting '%1' instead of '%2'").arg(s_opName[op]).
              arg(s_opName[ops[pc].op]), pc);
        return false;
    }
    return true;
}

Statement* AstRenderer::translateStat(const QList<ProcData::Op>& ops, int& pc)
{
    Statement* res = 0;
    while( pc < ops.size() )
    {
        const Op op = (Op)ops[pc].op;

        if( op == op_LINE )
        {
            const quint32 packed = ops[pc].arg.toUInt();
            curPos.setRowCol(RowCol::unpackRow2(packed), RowCol::unpackCol2(packed));
            pc++;
            continue;
        }

        switch( op )
        {
        case op_CASE:
        case op_DO:
        case op_UNTIL:
        case op_THEN:
        case op_ELSE:
        case op_END:
            return res;
        }

        Statement* tmp = new Statement();
        tmp->kind = op;
        tmp->pos = curPos;
        if( res )
            res->append(tmp);
        else
            res = tmp;

        if( isExprOp(op) )
        {
            tmp->kind = Op(Statement::ExprStat);
            tmp->e = translateExpr(ops, pc);
            if( tmp->e )
                tmp->pos = tmp->e->pos;
            continue;
        }

        switch( op )
        {
        case op_WHILE:
            pc++;
            tmp->e = translateExpr(ops, pc);
            if( !expect(ops, pc, op_DO) )
                return res;
            pc++;
            tmp->body = translateStat(ops, pc);
            if( !expect(ops, pc, op_END) )
                return res;
            break;
        case op_REPEAT:
            pc++;
            tmp->body = translateStat(ops, pc);
            if( !expect(ops, pc, op_UNTIL) )
                return res;
            pc++;
            tmp->e = translateExpr(ops, pc);
            if( !expect(ops, pc, op_END) )
                return res;
            break;
        case op_LOOP:
            pc++;
            tmp->body = translateStat(ops, pc);
            if( !expect(ops, pc, op_END) )
                return res;
            break;
        case op_IF:
            pc++;
            tmp->e = translateExpr(ops, pc);
            if( !expect(ops, pc, op_THEN) )
                return res;
            pc++;
            tmp->body = translateStat(ops, pc);
            if( pc < ops.size() && ops[pc].op == op_ELSE )
            {
                pc++;
                tmp = new Statement();
                tmp->kind = op_ELSE;
                tmp->pos = curPos;
                res->append(tmp);
                tmp->body = translateStat(ops, pc);
            }
            if( !expect(ops, pc, op_END) )
                return res;
            break;
        case op_SWITCH:
            pc++;
            tmp->e = translateExpr(ops, pc);
            while( pc < ops.size() && ops[pc].op == op_CASE )
            {
                const CaseLabelList labels = ops[pc].arg.value<CaseLabelList>();
                if( labels.isEmpty() )
                {
                    error("empty case label list", pc);
                    return res;
                }
                pc++;
                tmp = new Statement();
                tmp->kind = op_CASE;
                tmp->pos = curPos;
                res->append(tmp);
                tmp->labels = new CaseLabelList(labels);
                tmp->body = translateStat(ops, pc);
            }
            if( pc < ops.size() && ops[pc].op == op_ELSE )
            {
                pc++;
                tmp = new Statement();
                tmp->kind = op_ELSE;
                tmp->pos = curPos;
                res->append(tmp);
                tmp->body = translateStat(ops, pc);
            }
            if( !expect(ops, pc, op_END) )
                return res;
            break;
        case op_goto:
        case op_label:
            tmp->name = ops[pc].arg.toByteArray();
            break;
        case op_starg:
        case op_stloc:
            tmp->id = ops[pc].arg.toUInt();
            break;
        case op_copy:
        case op_stelem:
        case op_stind:
            later(&tmp->d, ops[pc].arg, Declaration::TypeDecl, false, pc, "type reference");
            break;
        case op_stvar:
            later(&tmp->d, ops[pc].arg, Declaration::VarDecl, false, pc, "variable reference");
            break;
        case op_stfld:
            later(&tmp->d, ops[pc].arg, Declaration::Field, true, pc, "field reference");
            break;
        case op_exit:
        case op_free:
        case op_pop:
        case op_raise:
        case op_ret:
        case op_transfer:
            break;
        default:
            error(QString("unexpected operation: %1").arg(s_opName[op]), pc);
            return res;
        }
        pc++;
    }
    return res;
}

Expression* AstRenderer::translateExpr(const QList<ProcData::Op>& ops, int& pc)
{
    Expression* res = 0;
    while( pc < ops.size() && ( isExprOp((Op)ops[pc].op) || ops[pc].op == op_LINE ) )
    {
        const Op op = (Op)ops[pc].op;

        if( op == op_LINE )
        {
            const quint32 packed = ops[pc].arg.toUInt();
            curPos.setRowCol(RowCol::unpackRow2(packed), RowCol::unpackCol2(packed));
            pc++;
            continue;
        }

        Expression* tmp = new Expression();
        tmp->kind = op;
        tmp->pos = curPos;
        if( res )
            res->append(tmp);
        else
            res = tmp;

        switch( op )
        {
        case op_iif: {
                // the nested expressions are chained to the IF node, so that next of the
                // IIF node points to the instruction following the conditional
                pc++;
                Expression* if_ = new Expression();
                if_->kind = op_IF;
                if_->pos = tmp->pos;
                if_->e = translateExpr(ops, pc);
                tmp->e = if_;
                if( !expect(ops, pc, op_THEN) )
                    return res;
                pc++;
                Expression* then_ = new Expression();
                then_->kind = op_THEN;
                then_->pos = curPos;
                then_->e = translateExpr(ops, pc);
                if_->next = then_;
                if( !expect(ops, pc, op_ELSE) )
                    return res;
                pc++;
                Expression* else_ = new Expression();
                else_->kind = op_ELSE;
                else_->pos = curPos;
                else_->e = translateExpr(ops, pc);
                then_->next = else_;
                if( !expect(ops, pc, op_END) )
                    return res;
            } break;
        case op_call:
        case op_pcall:
        case op_ldproc:
            later(&tmp->d, ops[pc].arg, Declaration::Procedure, false, pc, "procedure reference");
            break;
        case op_calli:
        case op_callmi:
        case op_pcalli:
        case op_castobj:
        case op_isinst:
        case op_ldelem:
        case op_refelem:
        case op_ldind:
        case op_newarr:
        case op_newarr_um:
        case op_newobj:
        case op_newobj_um:
        case op_sizeof:
            later(&tmp->d, ops[pc].arg, Declaration::TypeDecl, false, pc, "type reference");
            break;
        case op_ldvar:
        case op_refvar:
            later(&tmp->d, ops[pc].arg, Declaration::VarDecl, false, pc, "variable reference");
            break;
        case op_ldfld:
        case op_reffld:
            later(&tmp->d, ops[pc].arg, Declaration::Field, true, pc, "field reference");
            break;
        case op_callinst:
        case op_callvirt:
        case op_ldmeth:
        case op_ldvirt:
            later(&tmp->d, ops[pc].arg, Declaration::Procedure, true, pc, "bound procedure reference");
            break;
        case op_ldarg:
        case op_ldloc:
        case op_refarg:
        case op_refloc:
            tmp->id = ops[pc].arg.toUInt();
            break;
        case op_ldc_i4:
        case op_ldc_i8:
            tmp->i = ops[pc].arg.toLongLong();
            break;
        case op_ldc_r4:
        case op_ldc_r8:
            tmp->f = ops[pc].arg.toDouble();
            break;
        case op_ldstr:
            tmp->c = Constant::fromVariant(ops[pc].arg);
            break;
        }
        pc++;
    }
    return res;
}
