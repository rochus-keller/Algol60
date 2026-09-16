/*
* Copyright 2026 Rochus Keller <mailto:me@rochus-keller.ch>
*
* This file is part of the Algol60 parser library.
*
* The following is the license that applies to this copy of the
* library. For a license to use the library under conditions
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

#include <Algol60/AlgAirGen.h>
#include <Algol60/AlgValidator.h>
#include <QtDebug>
using namespace Alg;
using namespace Air;

static const char* s_transfer = "Transfer";

static inline Alg::RowCol air(const Alg::RowCol& r)
{
    return Alg::RowCol(r.d_row, r.d_col);
}

static bool isNameScalar(Declaration* d)
{
    if( d == 0 || !d->isParam || d->mode == Declaration::ModeValue )
        return false;
    Type* t = d->getType();
    return t && ( t->kind == Type::Integer || t->kind == Type::Real || t->kind == Type::Boolean );
}

static bool isNameLabel(Declaration* d)
{
    if( d == 0 || !d->isParam || d->mode == Declaration::ModeValue )
        return false;
    Type* t = d->getType();
    return t && t->kind == Type::Label;
}

static bool isFormalProc(Declaration* d)
{
    Type* t = d ? d->getType() : 0;
    return d && d->isParam && t && t->kind == Type::Procedure;
}

static bool isArray(Declaration* d)
{
    Type* t = d ? d->getType() : 0;
    return t && t->kind == Type::Array;
}

static bool isFunc(Declaration* proc)
{
    Type* t = proc ? proc->getType() : 0;
    return t && t->kind != Type::NoType && t->kind != Type::Undefined;
}

AirGen::AirGen(AstModel* mdl):mdl(mdl),e(0),module(0),curFrame(0),
    frameBase(0),frameIsArg(false),maxDims(1),tmpCount(0)
{
}

AirGen::~AirGen()
{
    for( int i = 0; i < frames.size(); i++ )
        delete frames[i];
    delete e;
}

Quali AirGen::q(const QByteArray& name)
{
    return Quali(QByteArray(), name);
}

Quali AirGen::rt(const QByteArray& name)
{
    return Quali("Algol60Rt", name);
}

bool AirGen::generate(Declaration* module, AbstractRenderer* out)
{
    errors.clear();
    if( module == 0 || out == 0 )
        return false;
    this->module = module;
    moduleName = sanitize(module->name);
    if( module->path )
        sourcePath = *module->path;

    Declaration* prog = module->link;
    while( prog && prog->kind != Declaration::Program )
        prog = prog->next;
    if( prog == 0 )
    {
        error(Alg::RowCol(), "no program in this module");
        return false;
    }

    delete e;
    e = new Emitter(out, Emitter::RowsOnly);

    names.clear();
    names << moduleName;
    collect(prog, 0);

    e->beginModule(moduleName, sourcePath, air(module->pos));
    e->addImport("Algol60Rt", air(module->pos));
    auxTypes();
    procTypes();
    frameTypes();
    ownVariables();

    for( int i = 0; i < frames.size(); i++ )
        emitProc(frames[i]);

    e->endModule(prog->body ? air(prog->body->pos) : air(module->pos));

    return errors.isEmpty();
}

AirGen::Frame* AirGen::collect(Declaration* proc, Frame* outer)
{
    Frame* f = new Frame();
    f->decl = proc;
    f->outer = outer;
    f->proc = member( outer, proc->kind == Declaration::Program ?
                          moduleName + "$init" : sanitize(proc->name) );
    f->rec = f->proc + "$frame";
    f->ref = f->proc + "$frameref";
    frames << f;
    frameOfProc.insert(proc, f);

    collectVars(f, proc);

    // the labels which need a dispatch entry in the protected body
    for( int i = 0; i < f->labels.size(); i++ )
        f->labels[i]->id = i + 1;
    f->dispatch = !f->labels.isEmpty();

    collectProcs(f, proc);
    return f;
}

void AirGen::collectVars(Frame* f, Declaration* scope)
{
    Declaration* d = scope->link;
    while( d )
    {
        switch( d->kind )
        {
        case Declaration::Variable:
        case Declaration::Parameter:
        case Declaration::Array:
            if( d->isOwn ) {
                QByteArray n = unique("own$" + sanitize(d->name));
                fields.insert(d, n);
                ownVars << d;
            }else {
                const QByteArray base = sanitize(d->name);
                QByteArray n = base;
                int i = 1;
                while( f->members.contains(n) )
                    n = base + "$" + QByteArray::number(i++);
                f->members << n;
                fields.insert(d, n);
                f->vars << d;
            }
            if( d->isParam && isArray(d) )
                dimsOfFormal.insert(d, formalDims(d));
            if( isArray(d) ) {
                const int dims = d->isParam ? dimsOfFormal.value(d,1) : d->getType()->getDims();
                if( dims > maxDims )
                    maxDims = dims;
            }
            break;
        case Declaration::LabelDecl:
            if( d->nonlocal )
                f->labels << d;
            labelName.insert(d, "L$" + QByteArray::number(labelName.size()+1));
            break;
        case Declaration::Switch:
            switchProc.insert(d, member(f, sanitize(d->name) + "$switch"));
            break;
        case Declaration::Block:
            collectVars(f, d);
            break;
        default:
            break;
        }
        d = d->next;
    }
}

void AirGen::collectProcs(Frame* f, Declaration* scope)
{
    Declaration* d = scope->link;
    while( d )
    {
        if( d->kind == Declaration::Procedure )
            collect(d, f);
        else if( d->kind == Declaration::Block )
            collectProcs(f, d);
        d = d->next;
    }
}

int AirGen::formalDims(Declaration* param)
{
    QSet<int> counts;
    Declaration* proc = Validator::declProc(param);
    if( proc )
        scanDims(proc->body, param, counts);
    if( counts.size() > 1 )
        error(air(param->pos),
              "this formal array is subscripted with a different number of subscripts at different places");
    int dims = 1;
    foreach( int n, counts )
    {
        if( n > dims )
            dims = n;
    }
    return dims;
}

void AirGen::scanDims(Statement* s, Declaration* param, QSet<int>& dims)
{
    while( s )
    {
        switch( s->kind )
        {
        case Statement::Compound:
        case Statement::Block:
            scanDims(s->body, param, dims);
            break;
        case Statement::Assign:
        case Statement::Call:
        case Statement::Goto:
            scanDims(s->lhs, param, dims);
            scanDims(s->rhs, param, dims);
            break;
        case Statement::If:
            scanDims(s->cond, param, dims);
            scanDims(s->body, param, dims);
            scanDims(s->elseStmt, param, dims);
            break;
        case Statement::For:
            scanDims(s->var, param, dims);
            scanDims(s->list, param, dims);
            scanDims(s->body, param, dims);
            break;
        default:
            break;
        }
        s = s->next;
    }
}

void AirGen::scanDims(Expression* e, Declaration* param, QSet<int>& dims)
{
    while( e )
    {
        if( e->kind == Expression::Subscript && e->lhs && e->lhs->kind == Expression::DeclRef && e->lhs->d == param )
        {
            int n = 0;
            Expression* sub = e->rhs;
            while( sub ) {
                n++;
                sub = sub->next;
            }
            dims << n;
        }
        scanDims(e->lhs, param, dims);
        scanDims(e->rhs, param, dims);
        scanDims(e->condition, param, dims);
        e = e->next;
    }
}

bool AirGen::isDesignator(Expression* ex)
{
    if( ex == 0 )
        return false;
    if( ex->kind == Expression::DeclRef )
        return ex->d && ( ex->d->kind == Declaration::Variable ||
                          ex->d->kind == Declaration::Parameter ||
                          ex->d->kind == Declaration::Procedure );
    return ex->kind == Expression::Subscript;
}

QByteArray AirGen::basicName(Type* t)
{
    if( t == 0 )
        return "float64";
    switch( t->kind )
    {
    case Type::Integer:
        return "int32";
    case Type::Real:
        return "float64";
    case Type::Boolean:
        return "bool";
    case Type::String:
        return "str";
    case Type::Label:
        return "xfer";
    default:
        return QByteArray();
    }
}

EmiTypes::Basic AirGen::basicType(Type* t)
{
    if( t == 0 )
        return EmiTypes::FLOAT64;
    switch( t->kind )
    {
    case Type::Integer:
        return EmiTypes::INT32;
    case Type::Real:
        return EmiTypes::FLOAT64;
    case Type::Boolean:
        return EmiTypes::BOOL;
    default:
        return EmiTypes::Undefined;
    }
}

QByteArray AirGen::arrName(Type* elem)
{
    return "arr$" + basicName(elem);
}

QByteArray AirGen::descName(Type* elem, int dims)
{
    return "desc$" + basicName(elem) + "$" + QByteArray::number(dims);
}

QByteArray AirGen::typeName(Type* t, Declaration* d)
{
    if( t == 0 )
        return "float64";
    if( isNameLabel(d) )
        // the designational expression is evaluated when the goto is executed,
        // thus the actual is passed as a thunk delivering the transfer value
        return "get$xfer";
    if( t->kind == Type::Array ) {
        const int dims = ( d && d->isParam ) ? dimsOfFormal.value(d,1) : t->getDims();
        return descName(t->getType(), dims ? dims : 1) + "ref";
    }
    if( t->kind == Type::Procedure ) {
        Declaration* callee = calleeOf(d);
        if( callee && procType.contains(callee) )
            return procType.value(callee);
        return "get$" + ( t->getType() && t->getType()->kind != Type::NoType ?
                              basicName(t->getType()) : QByteArray("void") );
    }
    if( t->kind == Type::Switch )
        return "get$switch";
    return basicName(t);
}

Quali AirGen::typeRef(Type* t, Declaration* d)
{
    return q(typeName(t,d));
}

void AirGen::auxTypes()
{
    const Alg::RowCol pos = air(module->pos);
    e->addType("chars", pos, false, q("char"), EmiTypes::Array);
    e->addType("str", pos, false, q("chars"), EmiTypes::Pointer);
    e->addType("xfer", pos, false, rt(s_transfer), EmiTypes::Pointer);
    e->addType("anyref", pos, false, q("anyrec"), EmiTypes::Pointer);

    static const char* basics[] = { "int32", "float64", "bool", "str", "xfer", 0 };
    for( int i = 0; basics[i]; i++ )
    {
        const QByteArray t = basics[i];
        e->addType("arr$" + t, pos, false, q(t), EmiTypes::Array);
        e->addType("arr$" + t + "ref", pos, false, q("arr$" + t), EmiTypes::Pointer);

        e->beginType("get$" + t, pos, false, EmiTypes::BoundProcType);
        e->setReturnType(q(t));
        e->endType();

        e->beginType("set$" + t, pos, false, EmiTypes::BoundProcType);
        e->addArgument(q(t), "v");
        e->endType();

        for( int dims = 1; dims <= maxDims; dims++ ) {
            const QByteArray dn = "desc$" + t + "$" + QByteArray::number(dims);
            e->beginType(dn, pos, false, EmiTypes::Struct);
            for( int k = 1; k <= dims; k++ ) {
                e->addField("lb" + QByteArray::number(k), pos, q("int32"), false);
                e->addField("ub" + QByteArray::number(k), pos, q("int32"), false);
            }
            e->addField("data", pos, q("arr$" + t + "ref"), false);
            e->endType();
            e->addType(dn + "ref", pos, false, q(dn), EmiTypes::Pointer);
        }
    }

    e->beginType("get$void", pos, false, EmiTypes::BoundProcType);
    e->endType();

    // a switch is a procedure delivering the transfer object of its n-th element
    e->beginType("get$switch", pos, false, EmiTypes::BoundProcType);
    e->addArgument(q("int32"), "idx");
    e->setReturnType(q("xfer"));
    e->endType();
}

Declaration* AirGen::calleeOf(Declaration* proc)
{
    if( proc == 0 )
        return 0;
    if( proc->kind == Declaration::Procedure )
        return proc;
    if( isFormalProc(proc) )
        return proc->actual;
    return 0;
}

void AirGen::procTypes()
{
    // a formal procedure with formals of its own is called through a methref whose
    // bound procedure type is that of the actual procedure the validator resolved
    DeclList used;
    for( int i = 0; i < frames.size(); i++ )
    {
        const DeclList params = Validator::params(frames[i]->decl);
        for( int k = 0; k < params.size(); k++ ) {
            Declaration* callee = calleeOf(params[k]);
            if( callee && !Validator::params(callee).isEmpty() &&
                    frameOfProc.contains(callee) && !used.contains(callee) )
                used << callee;
        }
    }

    for( int i = 0; i < frames.size(); i++ )
        if( used.contains(frames[i]->decl) )
            procType.insert(frames[i]->decl, unique("fp$" + frames[i]->proc));

    for( int i = 0; i < frames.size(); i++ )
    {
        Frame* f = frames[i];
        if( !procType.contains(f->decl) )
            continue;
        const Alg::RowCol pos = air(f->decl->pos);
        e->beginType(procType.value(f->decl), pos, false, EmiTypes::BoundProcType);
        const DeclList params = Validator::params(f->decl);
        for( int k = 0; k < params.size(); k++ ) {
            Declaration* p = params[k];
            const QByteArray n = fields.value(p);
            if( isNameScalar(p) ) {
                e->addArgument(q("get$" + basicName(p->getType())), n + "$get");
                if( p->assigned )
                    e->addArgument(q("set$" + basicName(p->getType())), n + "$set");
            }else
                e->addArgument(typeRef(p->getType(), p), n);
        }
        if( isFunc(f->decl) )
            e->setReturnType(typeRef(f->decl->getType()));
        e->endType();
    }
}

void AirGen::frameTypes()
{
    for( int i = 0; i < frames.size(); i++ )
    {
        Frame* f = frames[i];
        const Alg::RowCol pos = air(f->decl->pos);
        e->beginType(f->rec, pos, false, EmiTypes::Record);
        if( f->outer )
            e->addField("$up", pos, q(f->outer->ref), false);
        for( int k = 0; k < f->vars.size(); k++ ) {
            Declaration* d = f->vars[k];
            const QByteArray n = fields.value(d);
            if( isNameScalar(d) ) {
                e->addField(n + "$get", air(d->pos), q("get$" + basicName(d->getType())), false);
                if( d->assigned )
                    e->addField(n + "$set", air(d->pos), q("set$" + basicName(d->getType())), false);
            }else
                e->addField(n, air(d->pos), typeRef(d->getType(), d), false);
        }
        if( isFunc(f->decl) )
            e->addField("$res", pos, typeRef(f->decl->getType()), false);
        e->endType();
        e->addType(f->ref, pos, false, q(f->rec), EmiTypes::Pointer);
    }
}

void AirGen::ownVariables()
{
    for( int i = 0; i < ownVars.size(); i++ ) {
        Declaration* d = ownVars[i];
        e->addVariable(typeRef(d->getType(), d), fields.value(d), air(d->pos), false);
    }
}

void AirGen::emitProc(Frame* f)
{
    curFrame = f;
    tmpCount = 0;
    thunks.clear();
    thunkOfActual.clear();
    collectThunks(f, f->decl);
    collectThunks(f, f->decl->body);
    emitThunks(f);
    emitSwitches(f);

    Declaration* proc = f->decl;
    const bool func = isFunc(proc);
    const Alg::RowCol pos = air(proc->pos);

    if( f->dispatch )
        emitBody(f); // the statements run in a separate procedure called protected

    const bool isProg = proc->kind == Declaration::Program;
    e->beginProc(f->proc, pos, false, isProg ? ProcData::Init : ProcData::Normal,
                 isProg ? QByteArray() : f->outer->rec);
    if( !isProg )
        e->addArgument(q(f->outer->ref), "$this");

    const DeclList params = Validator::params(proc);
    QHash<Declaration*,int> argOf, argOf2;
    for( int i = 0; i < params.size(); i++ )
    {
        Declaration* p = params[i];
        const QByteArray n = fields.value(p);
        if( isNameScalar(p) ) {
            argOf.insert(p, e->addArgument(q("get$" + basicName(p->getType())), n + "$get", false, air(p->pos)));
            if( p->assigned )
                argOf2.insert(p, e->addArgument(q("set$" + basicName(p->getType())),
                                                n + "$set", false, air(p->pos)));
        }else
            argOf.insert(p, e->addArgument(typeRef(p->getType(), p), n, false, air(p->pos)));
    }
    if( func )
        e->setReturnType(typeRef(proc->getType()));

    const int fr = e->addLocal(q(f->ref), "$fr", pos);
    frameBase = fr;
    frameIsArg = false;

    e->line_(pos);
    e->newobj_(q(f->rec));
    e->stloc_(fr);
    if( !isProg ) {
        e->ldloc_(fr);
        e->ldarg_(0);
        e->stfld_(Trident(q(f->rec), "$up"));
    }
    for( int i = 0; i < params.size(); i++ )
    {
        Declaration* p = params[i];
        const QByteArray n = fields.value(p);
        e->ldloc_(fr);
        e->ldarg_(argOf.value(p));
        e->stfld_(Trident(q(f->rec), isNameScalar(p) ? n + "$get" : n));
        if( isNameScalar(p) && p->assigned ) {
            e->ldloc_(fr);
            e->ldarg_(argOf2.value(p));
            e->stfld_(Trident(q(f->rec), n + "$set"));
        }
        if( p->mode == Declaration::ModeValue && isArray(p) )
        {
            // the value array is copied on entry, as decided in the specification
            const int dims = dimsOfFormal.value(p,1);
            Type* elem = p->getType()->getType();
            const QByteArray dn = descName(elem, dims);
            const int tmp = e->addLocal(q(dn + "ref"), tmpName(), air(p->pos));
            e->newobj_(q(dn));
            e->stloc_(tmp);
            for( int k = 1; k <= dims; k++ )
            {
                const QByteArray lb = "lb" + QByteArray::number(k);
                const QByteArray ub = "ub" + QByteArray::number(k);
                e->ldloc_(tmp);
                e->ldloc_(fr);
                e->ldfld_(Trident(q(f->rec), n));
                e->ldfld_(Trident(q(dn), lb));
                e->stfld_(Trident(q(dn), lb));
                e->ldloc_(tmp);
                e->ldloc_(fr);
                e->ldfld_(Trident(q(f->rec), n));
                e->ldfld_(Trident(q(dn), ub));
                e->stfld_(Trident(q(dn), ub));
            }
            e->ldloc_(tmp);
            e->ldloc_(fr);
            e->ldfld_(Trident(q(f->rec), n));
            e->ldfld_(Trident(q(dn), "data"));
            e->len_();
            e->newarr_(q(arrName(elem)));
            e->stfld_(Trident(q(dn), "data"));
            e->ldloc_(tmp);
            e->ldfld_(Trident(q(dn), "data"));
            e->ldloc_(fr);
            e->ldfld_(Trident(q(f->rec), n));
            e->ldfld_(Trident(q(dn), "data"));
            e->copy_(q(arrName(elem)));
            e->ldloc_(fr);
            e->ldloc_(tmp);
            e->stfld_(Trident(q(f->rec), n));
        }
    }

    if( f->dispatch )
    {
        const int entry = e->addLocal(q("int32"), "$entry", pos);
        const int exc = e->addLocal(q("anyref"), "$exc", pos);
        const int xf = e->addLocal(q("xfer"), "$xfer", pos);
        e->ldc_i4(0);
        e->stloc_(entry);
        e->loop_();
        e->ldloc_(fr);
        e->ldloc_(entry);
        e->pcall_(q(f->proc + "$body"), 2);
        e->stloc_(exc);
        e->if_();
        e->ldloc_(exc);
        e->ldnull_();
        e->ceq_();
        e->then_();
        e->exit_();
        e->end_();
        e->if_();
        e->ldloc_(exc);
        e->isinst_(rt(s_transfer));
        e->then_();
        e->ldloc_(exc);
        e->castobj_(rt(s_transfer));
        e->stloc_(xf);
        e->if_();
        e->ldloc_(xf);
        e->ldfld_(Trident(rt(s_transfer), "frame"));
        e->ldloc_(fr);
        e->ceq_();
        e->then_();
        e->ldloc_(xf);
        e->ldfld_(Trident(rt(s_transfer), "label"));
        e->stloc_(entry);
        e->else_();
        e->ldloc_(exc);
        e->raise_();
        e->end_();
        e->else_();
        e->ldloc_(exc);
        e->raise_();
        e->end_();
        e->end_();
    }else
    {
        blockEntry(proc);
        statSeq(proc->body);
    }

    if( func )
    {
        e->ldloc_(fr);
        e->ldfld_(Trident(q(f->rec), "$res"));
    }
    e->ret_(func);
    e->endProc(pos);
}

void AirGen::emitBody(Frame* f)
{
    const Alg::RowCol pos = air(f->decl->pos);
    e->beginProc(f->proc + "$body", pos, false);
    e->addArgument(q(f->ref), "$fr");
    e->addArgument(q("int32"), "$entry");
    frameBase = 0;
    frameIsArg = true;

    e->line_(pos);
    e->if_();
    e->ldarg_(1);
    e->ldc_i4(0);
    e->ceq_();
    e->not_();
    e->then_();
    e->switch_();
    e->ldarg_(1);
    for( int i = 0; i < f->labels.size(); i++ )
    {
        e->case_(CaseLabelList() << CaseLabel(f->labels[i]->id, f->labels[i]->id));
        e->goto_(labelName.value(f->labels[i]));
    }
    e->else_();
    e->ldstr_("invalid label index");
    e->call_(rt("faultmsg"), 1);
    e->end_();
    e->end_();

    blockEntry(f->decl);
    statSeq(f->decl->body);

    e->ret_(false);
    e->endProc(pos);

    frameBase = 0;
    frameIsArg = false;
}

void AirGen::collectThunks(Frame* f, Declaration* scope)
{
    // the bounds of an array and the elements of a switch belong to the enclosing activation
    Declaration* d = scope->link;
    while( d )
    {
        switch( d->kind )
        {
        case Declaration::Array:
            if( d->getType() )
                collectThunks(f, d->getType()->getExpr());
            break;
        case Declaration::Switch:
            collectThunks(f, d->list);
            break;
        case Declaration::Block:
            collectThunks(f, d);
            break;
        default:
            break;
        }
        d = d->next;
    }
}

void AirGen::collectThunks(Frame* f, Statement* s)
{
    while( s )
    {
        switch( s->kind )
        {
        case Statement::Compound:
        case Statement::Block:
            collectThunks(f, s->body);
            break;
        case Statement::Assign:
            collectThunks(f, s->lhs);
            collectThunks(f, s->rhs);
            break;
        case Statement::Call:
            if( s->lhs && s->lhs->kind == Expression::DeclRef && calleeOf(s->lhs->d) )
                thunksOfCall(f, calleeOf(s->lhs->d), s->rhs);
            collectThunks(f, s->rhs);
            break;
        case Statement::If:
            collectThunks(f, s->cond);
            collectThunks(f, s->body);
            collectThunks(f, s->elseStmt);
            break;
        case Statement::For:
            collectThunks(f, s->var);
            collectThunks(f, s->list);
            collectThunks(f, s->body);
            break;
        case Statement::Goto:
            collectThunks(f, s->lhs);
            break;
        default:
            break;
        }
        s = s->next;
    }
}

void AirGen::collectThunks(Frame* f, Expression* ex)
{
    while( ex )
    {
        if( ex->kind == Expression::Call && ex->lhs && ex->lhs->kind == Expression::DeclRef && calleeOf(ex->lhs->d) )
            thunksOfCall(f, calleeOf(ex->lhs->d), ex->rhs);
        collectThunks(f, ex->lhs);
        collectThunks(f, ex->rhs);
        collectThunks(f, ex->condition);
        ex = ex->next;
    }
}

void AirGen::thunksOfCall(Frame* f, Declaration* proc, Expression* args)
{
    const DeclList formals = Validator::params(proc);
    int i = 0;
    Expression* a = args;
    while( a )
    {
        Declaration* formal = i < formals.size() ? formals[i] : 0;
        if( formal && ( isNameScalar(formal) || isNameLabel(formal) ) && !thunkOfActual.contains(a) )
        {
            Thunk t;
            t.actual = a;
            t.frame = f;
            t.type = formal->getType();
            t.getter = unique("thunk$" + QByteArray::number(thunks.size()+1) + "$get");
            if( formal->assigned && isNameScalar(formal) )
                t.setter = unique("thunk$" + QByteArray::number(thunks.size()+1) + "$set");
            thunkOfActual.insert(a, thunks.size());
            thunks << t;
        }
        i++;
        a = a->next;
    }
}

void AirGen::emitThunks(Frame* f)
{
    frameBase = 0;
    frameIsArg = true;
    for( int i = 0; i < thunks.size(); i++ )
    {
        const Thunk& t = thunks[i];
        const Alg::RowCol pos = air(t.actual->pos);

        e->beginProc(t.getter, pos, false, ProcData::Normal, f->rec);
        e->addArgument(q(f->ref), "$this");
        e->setReturnType(typeRef(t.type));
        e->line_(pos);
        if( t.type && t.type->kind == Type::Label )
            transfer(t.actual);
        else
            expr(t.actual, t.type);
        e->ret_(true);
        e->endProc(pos);

        if( !t.setter.isEmpty() )
        {
            e->beginProc(t.setter, pos, false, ProcData::Normal, f->rec);
            e->addArgument(q(f->ref), "$this");
            const int v = e->addArgument(typeRef(t.type), "$v", false, pos);
            e->line_(pos);
            if( isDesignator(t.actual) )
                store(t.actual, 0, v, t.type, true);
            else {
                // the actual is an expression, so the assignment is an error of the program
                // which can only be detected when the callee actually assigns to the formal
                e->ldstr_("assignment to an actual parameter which is not a variable");
                e->call_(rt("faultmsg"), 1);
            }
            e->ret_(false);
            e->endProc(pos);
        }
    }
}

void AirGen::emitSwitches(Frame* f)
{
    frameBase = 0;
    frameIsArg = true;
    for( QHash<Declaration*,QByteArray>::const_iterator i = switchProc.begin(); i != switchProc.end(); ++i )
    {
        Declaration* sw = i.key();
        if( Validator::declProc(sw) != f->decl )
            continue;
        const Alg::RowCol pos = air(sw->pos);
        e->beginProc(i.value(), pos, false, ProcData::Normal, f->rec);
        e->addArgument(q(f->ref), "$this");
        e->addArgument(q("int32"), "$idx");
        e->setReturnType(q("xfer"));
        e->line_(pos);
        int n = 0;
        Expression* el = sw->list;
        e->switch_();
        e->ldarg_(1);
        while( el ) {
            n++;
            e->case_(CaseLabelList() << CaseLabel(n,n));
            transfer(el);
            e->ret_(true);
            el = el->next;
        }
        e->else_();
        e->ldstr_("switch designator undefined");
        e->call_(rt("faultmsg"), 1);
        e->end_();
        e->ldnull_();
        e->ret_(true);
        e->endProc(pos);
    }
}

void AirGen::framePtr(Frame* target)
{
    if( frameIsArg )
        e->ldarg_(frameBase);
    else
        e->ldloc_(frameBase);
    Frame* f = curFrame;
    while( f && f != target ) {
        e->ldfld_(Trident(q(f->rec), "$up"));
        f = f->outer;
    }
    if( f == 0 )
        error(Alg::RowCol(), "cannot reach the frame of an enclosing activation");
}

AirGen::Frame* AirGen::frameOfDecl(Declaration* d) const
{
    Declaration* proc = Validator::declProc(d);
    return proc ? frameOfProc.value(proc) : 0;
}

QByteArray AirGen::fieldOf(Declaration* d) const
{
    return fields.value(d);
}

Trident AirGen::field(Declaration* d) const
{
    Frame* f = frameOfDecl(d);
    return Trident(q(f ? f->rec : QByteArray()), fields.value(d));
}

void AirGen::blockEntry(Declaration* scope)
{
    // the arrays of a block are allocated when the block is entered
    Declaration* d = scope->link;
    while( d ) {
        if( d->kind == Declaration::Array && !d->isParam )
            allocArray(d);
        d = d->next;
    }
}

void AirGen::allocArray(Declaration* d)
{
    Type* t = d->getType();
    const int dims = t->getDims();
    Type* elem = t->getType();
    const QByteArray dn = descName(elem, dims);
    Frame* home = frameOfDecl(d);

    if( d->isOwn )
    {
        // an own array keeps its value between the activations of its block
        e->if_();
        e->ldvar_(q(fields.value(d)));
        e->ldnull_();
        e->ceq_();
        e->then_();
    }

    if( d->isOwn )
    {
        e->newobj_(q(dn));
        e->stvar_(q(fields.value(d)));
    }else
    {
        framePtr(home);
        e->newobj_(q(dn));
        e->stfld_(field(d));
    }

    Expression* b = t->getExpr();
    for( int k = 1; k <= dims && b && b->next; k++ )
    {
        descriptor(d);
        expr(b, mdl->getType(Type::Integer));
        e->stfld_(Trident(q(dn), "lb" + QByteArray::number(k)));
        descriptor(d);
        expr(b->next, mdl->getType(Type::Integer));
        e->stfld_(Trident(q(dn), "ub" + QByteArray::number(k)));
        b = b->next->next;
    }

    // the number of elements is the product of the extents
    descriptor(d);
    for( int k = 1; k <= dims; k++ )
    {
        descriptor(d);
        e->ldfld_(Trident(q(dn), "ub" + QByteArray::number(k)));
        descriptor(d);
        e->ldfld_(Trident(q(dn), "lb" + QByteArray::number(k)));
        e->sub_();
        e->ldc_i4(1);
        e->add_();
        if( k > 1 )
            e->mul_();
    }
    e->newarr_(q(arrName(elem)));
    e->stfld_(Trident(q(dn), "data"));

    if( d->isOwn )
        e->end_();
}

void AirGen::descriptor(Declaration* d)
{
    if( d->isOwn )
        e->ldvar_(q(fields.value(d)));
    else
    {
        framePtr(frameOfDecl(d));
        e->ldfld_(field(d));
    }
}

void AirGen::statSeq(Statement* s)
{
    while( s ) {
        stat(s);
        s = s->next;
    }
}

void AirGen::stat(Statement* s)
{
    if( s == 0 )
        return;
    switch( s->kind )
    {
    case Statement::Compound:
    case Statement::Block:
        blockStat(s);
        break;
    case Statement::Assign:
        e->line_(air(s->pos));
        assignStat(s);
        break;
    case Statement::Call:
        e->line_(air(s->pos));
        callStat(s);
        break;
    case Statement::If:
        e->line_(air(s->pos));
        ifStat(s);
        break;
    case Statement::For:
        e->line_(air(s->pos));
        forStat(s);
        break;
    case Statement::Goto:
        e->line_(air(s->pos));
        gotoStat(s);
        break;
    case Statement::Label:
        if( s->label ) {
            e->line_(air(s->pos));
            e->label_(labelName.value(s->label));
        }
        break;
    case Statement::Dummy:
        break;
    default:
        error(air(s->pos), "invalid statement");
        break;
    }
}

void AirGen::blockStat(Statement* s)
{
    if( s->getScope() )
        blockEntry(s->getScope());
    statSeq(s->body);
}

void AirGen::assignStat(Statement* s)
{
    Expression* lhs = s->lhs;
    if( lhs && lhs->next == 0 )
    {
        store(lhs, s->rhs);
        return;
    }

    // a left part list assigns the same value to each variable
    Type* t = lhs && lhs->getType() ? lhs->getType() : ( s->rhs ? s->rhs->getType() : 0 );
    const int tmp = e->addLocal(typeRef(t), tmpName(), air(s->pos));
    expr(s->rhs, t);
    e->stloc_(tmp);
    while( lhs )
    {
        store(lhs, 0, tmp, t);
        lhs = lhs->next;
    }
}

void AirGen::callStat(Statement* s)
{
    if( s->lhs == 0 || s->lhs->kind != Expression::DeclRef )
        return;
    Declaration* d = s->lhs->d;
    switch( d->kind )
    {
    case Declaration::Builtin:
        builtin(0, d, s->rhs);
        if( builtinIsFunc(d->id) )
            e->pop_(); // the result of a function called as a statement is discarded
        break;
    case Declaration::Procedure:
        call(0, d, s->rhs, 0);
        if( isFunc(d) )
            e->pop_(); // the result of a function called as a statement is discarded
        break;
    case Declaration::Parameter:
        {
            Declaration* callee = calleeOf(d);
            int argc = 0;
            if( s->rhs )
            {
                if( callee == 0 )
                    error(air(s->pos), "the actual procedure of this formal parameter "
                                       "is not known statically");
                else
                    argc = args(callee, s->rhs, air(s->pos));
            }
            const bool func = callee && isFunc(callee);
            framePtr(frameOfDecl(d));
            e->ldfld_(field(d));
            e->callmi_(typeRef(d->getType(), d), argc, func);
            if( func )
                e->pop_(); // the result of a function called as a statement is discarded
        }
        break;
    default:
        error(air(s->pos), "invalid procedure statement");
        break;
    }
}

void AirGen::ifStat(Statement* s)
{
    e->if_();
    expr(s->cond, mdl->getType(Type::Boolean));
    e->then_();
    statSeq(s->body);
    if( s->elseStmt )
    {
        e->else_();
        statSeq(s->elseStmt);
    }
    e->end_();
}

void AirGen::forStat(Statement* s)
{
    Type* vt = s->var ? s->var->getType() : mdl->getType(Type::Integer);
    int n = 0;
    for( Expression* el = s->list; el; el = el->next )
        n++;
    if( n == 0 )
        return;
    if( n == 1 )
        forElem(s, s->list, vt);
    else
        forList(s, vt);
}

void AirGen::forElem(Statement* s, Expression* el, Type* vt)
{
    switch( el->kind )
    {
    case Expression::StepUntil:
        {
            const int st = e->addLocal(typeRef(vt), tmpName(), air(el->pos));
            store(s->var, el->lhs);
            e->loop_();
            expr(el->rhs, vt);
            e->stloc_(st);
            e->if_();
            forDone(s, el, vt, st);
            e->then_();
            e->exit_();
            e->end_();
            statSeq(s->body);
            forStep(s, vt, st);
            e->end_();
        }
        break;
    case Expression::WhileLoop:
        e->loop_();
        store(s->var, el->lhs);
        e->if_();
        expr(el->condition, mdl->getType(Type::Boolean));
        e->not_();
        e->then_();
        e->exit_();
        e->end_();
        statSeq(s->body);
        e->end_();
        break;
    default:
        store(s->var, el);
        statSeq(s->body);
        break;
    }
}

void AirGen::forList(Statement* s, Type* vt)
{
    // the body is emitted once, so a label in it remains unique
    // an index selects the list element which provides the next value of the controlled variable
    const int idx = e->addLocal(q("int32"), tmpName(), air(s->pos));
    const int more = e->addLocal(q("bool"), tmpName(), air(s->pos));
    const int fresh = e->addLocal(q("bool"), tmpName(), air(s->pos));
    e->ldc_i4(0);
    e->stloc_(idx);
    e->ldc_i4(1);
    e->stloc_(fresh);
    e->loop_();
    e->ldc_i4(0);
    e->stloc_(more);
    e->loop_();
    e->switch_();
    e->ldloc_(idx);
    int n = 0;
    for( Expression* el = s->list; el; el = el->next, n++ )
    {
        e->case_(CaseLabelList() << CaseLabel(n,n));
        switch( el->kind )
        {
        case Expression::StepUntil:
            {
                const int st = e->addLocal(typeRef(vt), tmpName(), air(el->pos));
                e->if_();
                e->ldloc_(fresh);
                e->then_();
                store(s->var, el->lhs);
                e->ldc_i4(0);
                e->stloc_(fresh);
                expr(el->rhs, vt);
                e->stloc_(st);
                e->else_();
                expr(el->rhs, vt);
                e->stloc_(st);
                forStep(s, vt, st);
                e->end_();
                e->if_();
                forDone(s, el, vt, st);
                e->then_();
                forNext(n, idx, fresh);
                e->else_();
                e->ldc_i4(1);
                e->stloc_(more);
                e->end_();
            }
            break;
        case Expression::WhileLoop:
            store(s->var, el->lhs);
            e->if_();
            expr(el->condition, mdl->getType(Type::Boolean));
            e->then_();
            e->ldc_i4(1);
            e->stloc_(more);
            e->else_();
            forNext(n, idx, fresh);
            e->end_();
            break;
        default:
            store(s->var, el);
            e->ldc_i4(1);
            e->stloc_(more);
            forNext(n, idx, fresh);
            break;
        }
    }
    e->end_();
    e->if_();
    e->ldloc_(more);
    e->then_();
    e->exit_();
    e->end_();
    e->if_();
    e->ldloc_(idx);
    e->ldc_i4(n);
    e->clt_();
    e->not_();
    e->then_();
    e->exit_();
    e->end_();
    e->end_();
    e->if_();
    e->ldloc_(more);
    e->not_();
    e->then_();
    e->exit_();
    e->end_();
    statSeq(s->body);
    e->end_();
}

void AirGen::forNext(int n, int idx, int fresh)
{
    e->ldc_i4(n+1);
    e->stloc_(idx);
    e->ldc_i4(1);
    e->stloc_(fresh);
}

void AirGen::forStep(Statement* s, Type* vt, int st)
{
    const int t = e->addLocal(typeRef(vt), tmpName(), air(s->pos));
    expr(s->var, vt);
    e->ldloc_(st);
    e->add_();
    e->stloc_(t);
    store(s->var, 0, t, vt);
}

void AirGen::forDone(Statement* s, Expression* el, Type* vt, int st)
{
    // the element is exhausted when (v - until) * sign(step) > 0
    e->iif_();
    e->ldloc_(st);
    if( vt->kind == Type::Real )
        e->ldc_r8(0.0);
    else
        e->ldc_i4(0);
    e->clt_();
    e->not_();
    e->then_();
    expr(s->var, vt);
    expr(el->condition, vt);
    e->cgt_();
    e->else_();
    expr(s->var, vt);
    expr(el->condition, vt);
    e->clt_();
    e->end_();
}

void AirGen::gotoStat(Statement* s)
{
    designator(s->lhs);
}

void AirGen::designator(Expression* d)
{
    if( d == 0 )
        return;
    switch( d->kind )
    {
    case Expression::IfExpr:
        e->if_();
        expr(d->condition, mdl->getType(Type::Boolean));
        e->then_();
            designator(d->lhs);
        e->else_();
        designator(d->rhs);
            e->end_();
        break;
    case Expression::DeclRef:
        if( d->d->kind == Declaration::LabelDecl )
            jump(d->d);
        else
        {
            // a formal label, i.e. a transfer object or a thunk delivering it
            formalLabel(d->d);
            e->raise_();
        }
        break;
    case Expression::Subscript:
        transfer(d);
        e->raise_();
        break;
    default:
        error(air(d->pos), "invalid designational expression");
        break;
    }
}

void AirGen::jump(Declaration* label)
{
    Frame* home = frameOfDecl(label);
    if( home == curFrame && !label->nonlocal )
    {
        e->goto_(labelName.value(label));
        return;
    }
    // the goto leaves the current activation, or the label is reached by dispatch
    framePtr(home);
    e->ldc_i4(label->id);
    e->call_(rt("mkxfer"), 2, true);
    e->raise_();
}

void AirGen::formalLabel(Declaration* d)
{
    framePtr(frameOfDecl(d));
    e->ldfld_(field(d));
    if( isNameLabel(d) )
        // the designational expression of the actual is evaluated here, not at the call
        e->callmi_(q("get$xfer"), 0, true);
}

void AirGen::thunkRef(Expression* actual)
{
    const int idx = thunkOfActual.value(actual, -1);
    if( idx < 0 )
    {
        error(air(actual->pos), "cannot pass this actual parameter by name");
        e->ldnull_();
        return;
    }
    const Thunk& t = thunks[idx];
    framePtr(t.frame);
    e->ldmeth_(Trident(q(t.frame->rec), t.getter));
}

void AirGen::transfer(Expression* d)
{
    if( d == 0 )
        return;
    switch( d->kind )
    {
    case Expression::IfExpr:
        e->iif_();
        expr(d->condition, mdl->getType(Type::Boolean));
        e->then_();
        transfer(d->lhs);
        e->else_();
        transfer(d->rhs);
        e->end_();
        break;
    case Expression::DeclRef:
        if( d->d->kind == Declaration::LabelDecl ) {
            Frame* home = frameOfDecl(d->d);
            // a runtime procedure so that a transfer value can be built inside an IIF
            // where only expression instructions are allowed
            framePtr(home);
            e->ldc_i4(d->d->id);
            e->call_(rt("mkxfer"), 2, true);
        }else
            formalLabel(d->d);
        break;
    case Expression::Subscript:
        {
            // a switch designator delivers the transfer object of the designated label
            Declaration* sw = d->lhs && d->lhs->kind == Expression::DeclRef ? d->lhs->d : 0;
            if( sw && switchProc.contains(sw) ) {
                framePtr(frameOfDecl(sw));
                expr(d->rhs, mdl->getType(Type::Integer));
                e->callinst_(Trident(q(frameOfDecl(sw)->rec), switchProc.value(sw)), 1, true);
            }else if( sw && sw->kind == Declaration::Parameter &&
                      sw->getType() && sw->getType()->kind == Type::Switch ) {
                // a formal switch is reached through the methref of the actual switch procedure
                expr(d->rhs, mdl->getType(Type::Integer));
                framePtr(frameOfDecl(sw));
                e->ldfld_(field(sw));
                e->callmi_(q("get$switch"), 1, true);
            }else {
                error(air(d->pos), "invalid switch designator");
                e->ldnull_();
            }
        }
        break;
    default:
        error(air(d->pos), "invalid designational expression");
        e->ldnull_();
        break;
    }
}

void AirGen::conv(Type* from, Type* to)
{
    if( from == 0 || to == 0 || from->kind == to->kind )
        return;
    if( from->kind == Type::Integer && to->kind == Type::Real )
        e->conv_(EmiTypes::FLOAT64);
    else if( from->kind == Type::Real && to->kind == Type::Integer )
        e->call_(rt("round"), 1, true); // the Report rounds a real assigned to an integer
}

void AirGen::expr(Expression* ex, Type* target)
{
    if( ex == 0 )
        return;
    switch( ex->kind )
    {
    case Expression::UnsignedConst:
        if( target && target->kind == Type::Real )
            e->ldc_r8(double(ex->u));
        else
            e->ldc_i4(qint32(ex->u));
        return;
    case Expression::RealConst:
        e->ldc_r8(ex->r);
        if( target && target->kind == Type::Integer )
            conv(mdl->getType(Type::Real), target);
        return;
    case Expression::BoolConst:
        e->ldc_i4(ex->u ? 1 : 0);
        return;
    case Expression::StringConst:
        e->ldstr_(stringValue(QByteArray(ex->a)));
        return;
    case Expression::DeclRef:
        declRef(ex);
        break;
    case Expression::Subscript:
        subscript(ex, false);
        break;
    case Expression::Call:
        if( ex->lhs && ex->lhs->kind == Expression::DeclRef ) {
            Declaration* d = ex->lhs->d;
            if( d->kind == Declaration::Builtin )
                builtin(ex, d, ex->rhs);
            else
                call(ex, d, ex->rhs, target);
        }
        break;
    case Expression::IfExpr:
        e->iif_();
        expr(ex->condition, mdl->getType(Type::Boolean));
        e->then_();
        expr(ex->lhs, ex->getType());
        e->else_();
        expr(ex->rhs, ex->getType());
        e->end_();
        break;
    case Expression::Neg:
        expr(ex->rhs, ex->getType());
        e->neg_();
        break;
    case Expression::Not:
        expr(ex->rhs, mdl->getType(Type::Boolean));
        e->not_();
        break;
    default:
        binaryOp(ex);
        break;
    }
    conv(ex->getType(), target);
}

void AirGen::binaryOp(Expression* ex)
{
    Type* t = ex->getType();
    Type* lt = ex->lhs ? ex->lhs->getType() : 0;
    Type* rt_ = ex->rhs ? ex->rhs->getType() : 0;

    switch( ex->kind )
    {
    case Expression::Add:
    case Expression::Sub:
    case Expression::Mul:
        expr(ex->lhs, t);
        expr(ex->rhs, t);
        if( ex->kind == Expression::Add )
            e->add_();
        else if( ex->kind == Expression::Sub )
            e->sub_();
        else
            e->mul_();
        break;
    case Expression::Div:
        expr(ex->lhs, mdl->getType(Type::Real));
        expr(ex->rhs, mdl->getType(Type::Real));
        e->div_();
        break;
    case Expression::IntDiv:
        expr(ex->lhs, mdl->getType(Type::Integer));
        expr(ex->rhs, mdl->getType(Type::Integer));
        e->div_();
        break;
    case Expression::Mod:
        expr(ex->lhs, mdl->getType(Type::Integer));
        expr(ex->rhs, mdl->getType(Type::Integer));
        e->rem_();
        break;
    case Expression::Power:
        if( t->kind == Type::Integer ) {
            expr(ex->lhs, mdl->getType(Type::Integer));
            expr(ex->rhs, mdl->getType(Type::Integer));
            e->call_(rt("expi"), 2, true);
        }else if( rt_ && rt_->kind == Type::Integer ) {
            expr(ex->lhs, mdl->getType(Type::Real));
            expr(ex->rhs, mdl->getType(Type::Integer));
            e->call_(rt("expn"), 2, true);
        }else {
            expr(ex->lhs, mdl->getType(Type::Real));
            expr(ex->rhs, mdl->getType(Type::Real));
            e->call_(rt("expr"), 2, true);
        }
        break;
    case Expression::And:
        expr(ex->lhs, mdl->getType(Type::Boolean));
        expr(ex->rhs, mdl->getType(Type::Boolean));
        e->and_();
        break;
    case Expression::Or:
        expr(ex->lhs, mdl->getType(Type::Boolean));
        expr(ex->rhs, mdl->getType(Type::Boolean));
        e->or_();
        break;
    case Expression::Imp:
        expr(ex->lhs, mdl->getType(Type::Boolean));
        e->not_();
        expr(ex->rhs, mdl->getType(Type::Boolean));
        e->or_();
        break;
    case Expression::Eqv:
        expr(ex->lhs, mdl->getType(Type::Boolean));
        expr(ex->rhs, mdl->getType(Type::Boolean));
        e->ceq_();
        break;
    case Expression::Eq:
    case Expression::Neq:
    case Expression::Lt:
    case Expression::Leq:
    case Expression::Gt:
    case Expression::Geq:
        {
            Type* ct = ( lt && rt_ && lt->kind == Type::Integer && rt_->kind == Type::Integer ) ?
                        mdl->getType(Type::Integer) :
                        ( lt && lt->kind == Type::Boolean ? mdl->getType(Type::Boolean) :
                                                            mdl->getType(Type::Real) );
            expr(ex->lhs, ct);
            expr(ex->rhs, ct);
            switch( ex->kind )
            {
            case Expression::Eq:
                e->ceq_();
                break;
            case Expression::Neq:
                e->ceq_();
                e->not_();
                break;
            case Expression::Lt:
                e->clt_();
                break;
            case Expression::Geq:
                e->clt_();
                e->not_();
                break;
            case Expression::Gt:
                e->cgt_();
                break;
            case Expression::Leq:
                e->cgt_();
                e->not_();
                break;
            }
        }
        break;
    default:
        error(air(ex->pos), "invalid expression");
        break;
    }
}

void AirGen::declRef(Expression* ex)
{
    Declaration* d = ex->d;
    switch( d->kind )
    {
    case Declaration::Variable:
    case Declaration::Parameter:
        if( isFormalProc(d) ) {
            framePtr(frameOfDecl(d));
            e->ldfld_(field(d));
            e->callmi_(typeRef(d->getType(), d), 0, true);
        }else if( isNameScalar(d) ) {
            framePtr(frameOfDecl(d));
            e->ldfld_(Trident(q(frameOfDecl(d)->rec), fields.value(d) + "$get"));
            e->callmi_(q("get$" + basicName(d->getType())), 0, true);
        }else if( d->isOwn )
            e->ldvar_(q(fields.value(d)));
        else {
            framePtr(frameOfDecl(d));
            e->ldfld_(field(d));
        }
        break;
    case Declaration::Procedure:
        if( d == curFrame->decl ) {
            // the identifier of the enclosing function designates its result
            framePtr(curFrame);
            e->ldfld_(Trident(q(curFrame->rec), "$res"));
        }else
            call(ex, d, 0, 0);
        break;
    case Declaration::Builtin:
        builtin(ex, d, 0);
        break;
    case Declaration::Array:
        descriptor(d);
        break;
    default:
        error(air(ex->pos), "cannot use this identifier in an expression");
        break;
    }
}

void AirGen::arrayRef(Expression* ex)
{
    Declaration* d = ex->lhs->d;
    Type* t = d->getType();
    Type* elem = t->getType();
    const int dims = d->isParam ? dimsOfFormal.value(d,1) : t->getDims();
    const QByteArray dn = descName(elem, dims);

    descriptor(d);
    e->ldfld_(Trident(q(dn), "data"));

    // the linear index of a row major layout
    Expression* sub = ex->rhs;
    for( int k = 1; k <= dims; k++ )
    {
        if( sub == 0 )
        {
            e->ldc_i4(0);
            break;
        }
        if( k > 1 )
        {
            descriptor(d);
            e->ldfld_(Trident(q(dn), "ub" + QByteArray::number(k)));
            descriptor(d);
            e->ldfld_(Trident(q(dn), "lb" + QByteArray::number(k)));
            e->sub_();
            e->ldc_i4(1);
            e->add_();
            e->mul_();
        }
        expr(sub, mdl->getType(Type::Integer));
        descriptor(d);
        e->ldfld_(Trident(q(dn), "lb" + QByteArray::number(k)));
        e->sub_();
        if( k > 1 )
            e->add_();
        sub = sub->next;
    }
}

void AirGen::subscript(Expression* ex, bool store)
{
    if( ex->lhs == 0 || ex->lhs->kind != Expression::DeclRef )
    {
        error(air(ex->pos), "invalid subscripted variable");
        return;
    }
    Declaration* d = ex->lhs->d;
    if( !isArray(d) )
    {
        error(air(ex->pos), "invalid subscripted variable");
        return;
    }
    arrayRef(ex);
    if( !store )
        e->ldelem_(q(arrName(d->getType()->getType())));
}

void AirGen::loadTmp(int tmp, bool isArg)
{
    if( isArg )
        e->ldarg_(tmp);
    else
        e->ldloc_(tmp);
}

void AirGen::store(Expression* lhs, Expression* rhs, int tmp, Type* tmpType, bool tmpIsArg)
{
    if( lhs == 0 )
        return;

    Type* lt = lhs->getType();

    if( lhs->kind == Expression::Subscript )
    {
        if( lhs->lhs == 0 || lhs->lhs->kind != Expression::DeclRef || !isArray(lhs->lhs->d) ) {
            error(air(lhs->pos), "invalid subscripted variable");
            return;
        }
        Type* elem = lhs->lhs->d->getType()->getType();
        subscript(lhs, true);
        if( rhs )
            expr(rhs, elem);
        else {
            loadTmp(tmp, tmpIsArg);
            conv(tmpType, elem);
        }
        e->stelem_(q(arrName(elem)));
        return;
    }

    if( lhs->kind != Expression::DeclRef ) {
        error(air(lhs->pos), "invalid left part of an assignment");
        return;
    }

    Declaration* d = lhs->d;
    if( d->kind == Declaration::Procedure )
    {
        Frame* home = frameOfProc.value(d);
        framePtr(home);
        if( rhs )
            expr(rhs, d->getType());
        else {
            loadTmp(tmp, tmpIsArg);
            conv(tmpType, d->getType());
        }
        e->stfld_(Trident(q(home->rec), "$res"));
        return;
    }

    if( isNameScalar(d) )
    {
        if( !d->assigned ) {
            error(air(lhs->pos), "assignment to a name parameter without setter");
            return;
        }
        if( rhs )
            expr(rhs, d->getType());
        else {
            loadTmp(tmp, tmpIsArg);
            conv(tmpType, d->getType());
        }
        framePtr(frameOfDecl(d));
        e->ldfld_(Trident(q(frameOfDecl(d)->rec), fields.value(d) + "$set"));
        e->callmi_(q("set$" + basicName(d->getType())), 1, false);
        return;
    }

    if( d->isOwn )
    {
        if( rhs )
            expr(rhs, lt);
        else {
            loadTmp(tmp, tmpIsArg);
            conv(tmpType, lt);
        }
        e->stvar_(q(fields.value(d)));
        return;
    }

    framePtr(frameOfDecl(d));
    if( rhs )
        expr(rhs, lt);
    else {
        loadTmp(tmp, tmpIsArg);
        conv(tmpType, lt);
    }
    e->stfld_(field(d));
}

void AirGen::call(Expression* ex, Declaration* proc, Expression* actuals, Type* target)
{
    if( proc->kind == Declaration::Parameter )
    {
        const Alg::RowCol pos = ex ? air(ex->pos) : air(proc->pos);
        int argc = 0;
        Declaration* callee = calleeOf(proc);
        if( actuals ) {
            if( callee == 0 )
                error(pos, "the actual procedure of this formal parameter is not known statically");
            else
                argc = args(callee, actuals, pos); // the methref is pushed after the arguments
        }
        framePtr(frameOfDecl(proc));
        e->ldfld_(field(proc));
        e->callmi_(typeRef(proc->getType(), proc), argc, true);
        return;
    }

    Frame* callee = frameOfProc.value(proc);
    if( callee == 0 )
    {
        error(ex ? air(ex->pos) : Alg::RowCol(), "unknown procedure");
        return;
    }

    framePtr(callee->outer);
    const int argc = args(proc, actuals, ex ? air(ex->pos) : air(proc->pos));
    e->callinst_(Trident(q(callee->outer->rec), callee->proc), argc, isFunc(proc));
}

int AirGen::args(Declaration* proc, Expression* actuals, const Alg::RowCol& pos)
{
    const DeclList formals = Validator::params(proc);
    int argc = 0, i = 0;
    Expression* a = actuals;
    while( a && i < formals.size() )
    {
        Declaration* formal = formals[i];
        Type* ft = formal->getType();

        if( ft && ft->kind == Type::Label ) {
            if( isNameLabel(formal) )
                thunkRef(a);
            else
                transfer(a);
            argc++;
        }else if( isArray(formal) ) {
            if( a->kind == Expression::DeclRef && isArray(a->d) )
                descriptor(a->d);
            else {
                error(air(a->pos), "an array identifier is required here");
                e->ldnull_();
            }
            argc++;
        }else if( ft && ft->kind == Type::Procedure ) {
            if( a->kind == Expression::DeclRef && a->d->kind == Declaration::Procedure ) {
                Frame* callee = frameOfProc.value(a->d);
                if( callee == 0 )
                    error(air(a->pos), "unknown procedure"), e->ldnull_();
                else if( Validator::params(a->d).isEmpty() ||
                         ( calleeOf(formal) && Validator::sigCompat(calleeOf(formal), a->d) ) ) {
                    framePtr(callee->outer);
                    e->ldmeth_(Trident(q(callee->outer->rec), callee->proc));
                }else {
                    error(air(a->pos), "different actual procedures with parameters "
                                       "are passed to this formal procedure");
                    e->ldnull_();
                }
            }else if( a->kind == Expression::DeclRef && isFormalProc(a->d) ) {
                framePtr(frameOfDecl(a->d));
                e->ldfld_(field(a->d));
            }else {
                error(air(a->pos), "a procedure identifier is required here");
                e->ldnull_();
            }
            argc++;
        }else if( ft && ft->kind == Type::Switch ) {
            Declaration* sw = a->kind == Expression::DeclRef ? a->d : 0;
            if( sw && switchProc.contains(sw) ) {
                Frame* owner = frameOfDecl(sw);
                framePtr(owner);
                e->ldmeth_(Trident(q(owner->rec), switchProc.value(sw)));
            }else if( sw && sw->kind == Declaration::Parameter &&
                      sw->getType() && sw->getType()->kind == Type::Switch ) {
                framePtr(frameOfDecl(sw));
                e->ldfld_(field(sw));
            }else {
                error(air(a->pos), "a switch identifier is required here");
                e->ldnull_();
            }
            argc++;
        }else if( isNameScalar(formal) ) {
            const int idx = thunkOfActual.value(a, -1);
            if( idx < 0 ) {
                error(air(a->pos), "cannot pass this actual parameter by name");
                e->ldnull_();
                argc++;
            }else {
                const Thunk& t = thunks[idx];
                thunkRef(a);
                argc++;
                if( formal->assigned ) {
                    if( t.setter.isEmpty() ) {
                        error(air(a->pos), "this actual parameter cannot be assigned to");
                        e->ldnull_();
                    }else {
                        framePtr(t.frame);
                        e->ldmeth_(Trident(q(t.frame->rec), t.setter));
                    }
                    argc++;
                }
            }
        }else {
            expr(a, ft);
            argc++;
        }
        i++;
        a = a->next;
    }
    if( a || i < formals.size() )
        error(pos, "the number of actual parameters differs from the number of formals");
    return argc;
}

bool AirGen::builtinIsFunc(int id)
{
    switch( id )
    {
    case Builtin::ABS:
    case Builtin::IABS:
    case Builtin::SIGN:
    case Builtin::SQRT:
    case Builtin::SIN:
    case Builtin::COS:
    case Builtin::ARCTAN:
    case Builtin::LN:
    case Builtin::EXP:
    case Builtin::ENTIER:
    case Builtin::LENGTH:
    case Builtin::MAXREAL:
    case Builtin::MINREAL:
    case Builtin::MAXINT:
    case Builtin::EPSILON:
        return true;
    }
    return false;
}

void AirGen::builtin(Expression* ex, Declaration* b, Expression* actuals)
{
    Type* real = mdl->getType(Type::Real);
    Type* integer = mdl->getType(Type::Integer);

    switch( b->id )
    {
    case Builtin::ABS:
    case Builtin::IABS:
        expr(actuals, b->id == Builtin::IABS ? integer :
                      ( actuals ? actuals->getType() : real ));
        e->abs_();
        return;
    case Builtin::SIGN:
        expr(actuals, real);
        e->call_(rt("sign"), 1, true);
        return;
    case Builtin::ENTIER:
        expr(actuals, real);
        e->call_(rt("entier"), 1, true);
        return;
    case Builtin::SQRT:
    case Builtin::SIN:
    case Builtin::COS:
    case Builtin::ARCTAN:
    case Builtin::LN:
    case Builtin::EXP:
        expr(actuals, real);
        e->call_(rt(QByteArray(Builtin::name[b->id]).toLower()), 1, true);
        return;
    case Builtin::LENGTH:
        expr(actuals, 0);
        e->call_(rt("length"), 1, true);
        return;
    case Builtin::STOP:
        e->call_(rt("stop"), 0);
        return;
    case Builtin::MAXREAL:
    case Builtin::MINREAL:
    case Builtin::EPSILON:
        e->call_(rt(QByteArray(Builtin::name[b->id]).toLower()), 0, true);
        return;
    case Builtin::MAXINT:
        e->call_(rt("maxint"), 0, true);
        return;
    case Builtin::FAULT:
        expr(actuals, 0);
        expr(actuals ? actuals->next : 0, real);
        e->call_(rt("fault"), 2);
        return;
    case Builtin::OUTTERMINATOR:
        channel(actuals, 0);
        e->call_(rt("outterminator"), 1);
        return;
    case Builtin::OUTINTEGER:
    case Builtin::OUTREAL:
    case Builtin::OUTSTRING:
    case Builtin::OUTCHAR:
    case Builtin::OUTSYMBOL:
        {
            const int vals = ( b->id == Builtin::OUTCHAR ||
                               b->id == Builtin::OUTSYMBOL ) ? 2 : 1;
            Expression* a = channel(actuals, vals);
            for( int i = 0; i < vals && a; i++, a = a->next ) {
                Type* t = 0;
                if( b->id == Builtin::OUTINTEGER || ( i == 1 ) )
                    t = integer;
                else if( b->id == Builtin::OUTREAL )
                    t = real;
                expr(a, t);
            }
            e->call_(rt(runtimeName(b)), vals + 1);
        }
        return;
    case Builtin::ININTEGER:
    case Builtin::INREAL:
    case Builtin::INCHAR:
    case Builtin::INSYMBOL:
        {
            // the value is read into a temporary and then assigned to the actual
            const bool isChar = b->id == Builtin::INCHAR || b->id == Builtin::INSYMBOL;
            Type* t = b->id == Builtin::INREAL ? real : integer;
            const int tmp = e->addLocal(typeRef(t), tmpName(),
                                        actuals ? air(actuals->pos) : Alg::RowCol());
            Expression* a = channel(actuals, isChar ? 2 : 1);
            if( isChar && a ) {
                expr(a, 0); // the string of characters to be recognized
                a = a->next;
            }
            Expression* var = a;
            e->refloc_(tmp);
            e->call_(rt(runtimeName(b)), isChar ? 3 : 2);
            if( var )
                store(var, 0, tmp, t);
            else
                error(ex ? air(ex->pos) : Alg::RowCol(),
                      "the variable receiving the input value is missing");
        }
        return;
    default:
        error(ex ? air(ex->pos) : Alg::RowCol(), "this standard procedure is not yet supported");
        return;
    }
}

Expression* AirGen::channel(Expression* actuals, int vals)
{
    // the channel is the first actual, or the standard channel if it was left out
    int n = 0;
    Expression* a = actuals;
    while( a ) {
        n++;
        a = a->next;
    }
    if( n > vals ) {
        expr(actuals, mdl->getType(Type::Integer));
        return actuals->next;
    }
    e->call_(rt("stdchannel"), 0, true);
    return actuals;
}

QByteArray AirGen::runtimeName(Declaration* b)
{
    switch( b->id )
    {
    case Builtin::INSYMBOL:
        return "inchar";
    case Builtin::OUTSYMBOL:
        return "outchar";
    default:
        return QByteArray(Builtin::name[b->id]).toLower();
    }
}

QByteArray AirGen::sanitize(const QByteArray& name)
{
    QByteArray res;
    for( int i = 0; i < name.size(); i++ ) {
        const char ch = name[i];
        if( ::isalnum(quint8(ch)) || ch == '_' )
            res += ch;
        else
            res += '_';
    }
    if( res.isEmpty() )
        res = "_";
    if( ::isdigit(quint8(res[0])) )
        res = "_" + res;
    return res;
}

QByteArray AirGen::stringValue(const QByteArray& literal)
{
    QByteArray res = literal;
    if( res.size() >= 2 )
    {
        // remove the string quotes, which may be "", `', or the Algol quotes
        const int last = res.size() - 1;
        if( res[0] == '"' && res[last] == '"' )
            res = res.mid(1, res.size()-2);
        else if( res[0] == '`' && res[last] == '\'' )
            res = res.mid(1, res.size()-2);
        else if( res.startsWith("\xe2\x80\x98") && res.endsWith("\xe2\x80\x99") )
            res = res.mid(3, res.size()-6);
    }
    QByteArray out;
    for( int i = 0; i < res.size(); i++ )
    {
        if( res[i] != '\\' || i + 1 >= res.size() )
        {
            out += res[i];
            continue;
        }
        const char ch = res[++i];
        switch( ch )
        {
        case 'n':
            out += '\n'; break;
        case 't':
            out += '\t'; break;
        case 'r':
            out += '\r'; break;
        case 'b':
            out += '\b'; break;
        case 'f':
            out += '\f'; break;
        case '\\':
            out += '\\'; break;
        case '"':
            out += '"'; break;
        case '\'':
            out += '\''; break;
        default:
            out += '\\'; out += ch; break;
        }
    }
    return out;
}

QByteArray AirGen::member(Frame* f, const QByteArray& name)
{
    // fields and bound procedures share the name space of the frame record
    QByteArray res = unique(name);
    int i = 1;
    while( f && f->members.contains(res) )
        res = unique(name + "$" + QByteArray::number(i++));
    if( f )
        f->members << res;
    return res;
}

QByteArray AirGen::unique(const QByteArray& name)
{
    QByteArray res = name;
    int i = 1;
    while( names.contains(res) )
        res = name + "$" + QByteArray::number(i++);
    names << res;
    return res;
}

int AirGen::localOf(Type* t)
{
    return e->addLocal(typeRef(t), tmpName());
}

QByteArray AirGen::tmpName()
{
    return "$t" + QByteArray::number(++tmpCount);
}

void AirGen::error(const Alg::RowCol& pos, const QString& msg)
{
    errors << Error(msg, pos, sourcePath);
}

void AirGen::generateRuntime(AbstractRenderer* out)
{
    // the interface of the Algol 60 runtime; the implementation is provided by the backend
    Emitter e(out, Emitter::None);
    const Alg::RowCol pos(1,1);
    e.beginModule("Algol60Rt", "Algol60Rt.air", pos);

    e.beginType(s_transfer, pos, true, EmiTypes::Record);
    e.addField("frame", pos, q("anyrec$ref"), true);
    e.addField("label", pos, q("int32"), true);
    e.endType();
    e.addType("anyrec$ref", pos, true, q("anyrec"), EmiTypes::Pointer);
    e.addType("chars", pos, true, q("char"), EmiTypes::Array);
    e.addType("str", pos, true, q("chars"), EmiTypes::Pointer);
    e.addType("xfer", pos, true, q(s_transfer), EmiTypes::Pointer);

    struct Fn { const char* name; const char* args; const char* ret; };
    static const Fn fn[] =
    {
        { "sqrt", "f", "f" },
        { "sin", "f", "f" },
        { "cos", "f", "f" },
        { "arctan", "f", "f" },
        { "ln", "f", "f" },
        { "exp", "f", "f" },
        { "entier", "f", "i" },
        { "sign", "f", "i" },
        { "round", "f", "i" },
        { "expr", "ff", "f" },
        { "expn", "fi", "f" },
        { "expi", "ii", "i" },
        { "maxreal", "", "f" },
        { "minreal", "", "f" },
        { "epsilon", "", "f" },
        { "maxint", "", "i" },
        { "length", "s", "i" },
        { "stdchannel", "", "i" },
        { "outinteger", "ii", "" },
        { "outreal", "if", "" },
        { "outstring", "is", "" },
        { "outchar", "isi", "" },
        { "outterminator", "i", "" },
        { "ininteger", "iI", "" },
        { "inreal", "iF", "" },
        { "inchar", "isI", "" },
        { "stop", "", "" },
        { "fault", "sf", "" },
        { "faultmsg", "s", "" },
        { "mkxfer", "ri", "x" },
        { 0, 0, 0 }
    };
    for( int i = 0; fn[i].name; i++ )
    {
        e.beginProc(fn[i].name, pos, true, ProcData::Extern, QByteArray("algol60_") + fn[i].name);
        const QByteArray a = fn[i].args;
        for( int k = 0; k < a.size(); k++ )
        {
            switch( a[k] ) {
            case 'i':
                e.addArgument(q("int32"), "a" + QByteArray::number(k));
                break;
            case 'f':
                e.addArgument(q("float64"), "a" + QByteArray::number(k));
                break;
            case 's':
                e.addArgument(q("str"), "a" + QByteArray::number(k));
                break;
            case 'I':
                e.addArgument(q("int32"), "a" + QByteArray::number(k), true);
                break;
            case 'F':
                e.addArgument(q("float64"), "a" + QByteArray::number(k), true);
                break;
            case 'r':
                e.addArgument(q("anyrec$ref"), "a" + QByteArray::number(k));
                break;
            }
        }
        const QByteArray r = fn[i].ret;
        if( r == "f" )
            e.setReturnType(q("float64"));
        else if( r == "i" )
            e.setReturnType(q("int32"));
        else if( r == "x" )
            e.setReturnType(q("xfer"));
        e.endProc(pos);
    }

    e.endModule(pos);
}
