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

#include <Algol60/AirValidator.h>
using namespace Air;
using Alg::RowCol;

static bool usesDeclOperand(quint8 op)
{
    switch( op )
    {
    case op_call:
    case op_calli:
    case op_callinst:
    case op_callmi:
    case op_callvirt:
    case op_pcall:
    case op_pcalli:
    case op_castobj:
    case op_isinst:
    case op_ldelem:
    case op_ldfld:
    case op_ldmeth:
    case op_ldproc:
    case op_ldvar:
    case op_ldvirt:
    case op_newarr:
    case op_newarr_um:
    case op_newobj:
    case op_newobj_um:
    case op_refelem:
    case op_reffld:
    case op_refvar:
    case op_ldind:
    case op_sizeof:
    case op_copy:
    case op_stelem:
    case op_stfld:
    case op_stvar:
    case op_stind:
        return true;
    }
    return false;
}

Validator::Validator(AstModel* mdl):mdl(mdl),module(0),curProc(0),suppress(false),unknownStack(false)
{
    Q_ASSERT( mdl );
}

bool Validator::validate(Declaration* m)
{
    Q_ASSERT( m && m->kind == Declaration::Module );
    errors.clear();
    module = m;
    if( m->name.isEmpty() )
        error("the module has no name");

    QSet<QByteArray> names;
    Declaration* sub = m->subs;
    while( sub )
    {
        if( sub->kind != Declaration::Placeholder )
        {
            if( sub->name.isEmpty() )
                error("a declaration of the module has no name", sub->pos);
            else if( names.contains(sub->name) )
                error(QString("duplicate declaration: %1").arg(sub->name.constData()), sub->pos);
            else
                names.insert(sub->name);
            validateDecl(sub);
        }
        sub = sub->next;
    }
    module = 0;
    return errors.isEmpty();
}

void Validator::validateDecl(Declaration* d)
{
    curPos = d->pos;
    switch( d->kind )
    {
    case Declaration::Import:
        if( d->imported == 0 )
            error(QString("unresolved import: %1").arg(d->name.constData()));
        break;
    case Declaration::ConstDecl:
        if( d->c == 0 || d->c->kind == Constant::Invalid )
            error(QString("invalid constant: %1").arg(d->name.constData()));
        break;
    case Declaration::VarDecl:
        if( d->getType() == 0 )
            error(QString("variable without type: %1").arg(d->name.constData()));
        else
            validateType(d->getType(), d);
        break;
    case Declaration::TypeDecl:
        if( d->getType() == 0 )
            error(QString("type declaration without type: %1").arg(d->name.constData()));
        else
        {
            validateType(d->getType(), d);
            Type* t = d->getType();
            if( t->isRecord() )
            {
                foreach( Declaration* p, t->subs )
                {
                    if( p->kind == Declaration::Procedure )
                        validateProc(p);
                }
            }
        }
        break;
    case Declaration::Procedure:
        validateProc(d);
        break;
    }
}

void Validator::validateType(Type* t, Declaration* owner)
{
    if( t == 0 )
        return;
    if( t->kind == Type::NameRef )
    {
        if( t->getType() == 0 )
            error(QString("unresolved type reference in '%1'").arg(owner->name.constData()));
        return;
    }
    if( t->deref() == 0 )
        error(QString("cyclic type reference in '%1'").arg(owner->name.constData()));

    switch( t->kind )
    {
    case Type::Pointer:
    case Type::CPointer:
        if( t->getType() == 0 && t->kind == Type::Pointer )
            error(QString("pointer without base type: %1").arg(owner->name.constData()));
        break;
    case Type::Array:
    case Type::CArray:
        if( t->getType() == 0 )
            error(QString("array without element type: %1").arg(owner->name.constData()));
        break;
    case Type::Record:
        if( t->getType() && !t->getType()->deref()->isRecord() )
            error(QString("the base type of a record must be a record: %1").arg(owner->name.constData()));
        // fall through
    case Type::Struct:
    case Type::CStruct:
    case Type::CUnion: {
            QSet<QByteArray> names;
            foreach( Declaration* f, t->subs )
            {
                if( f->kind != Declaration::Field )
                    continue;
                if( f->name.isEmpty() )
                    error(QString("a field of '%1' has no name").arg(owner->name.constData()));
                else if( names.contains(f->name) )
                    error(QString("duplicate field '%1' in '%2'").
                          arg(f->name.constData()).arg(owner->name.constData()));
                else
                    names.insert(f->name);
                if( f->getType() == 0 )
                    error(QString("field without type: %1").arg(f->name.constData()));
            }
        } break;
    case Type::Proc:
        foreach( Declaration* p, t->subs )
        {
            if( p->name.isEmpty() )
                error(QString("a parameter of '%1' has no name").arg(owner->name.constData()));
            if( p->getType() == 0 )
                error(QString("parameter without type: %1").arg(p->name.constData()));
        }
        break;
    }
}

void Validator::validateProc(Declaration* p)
{
    curProc = p;
    curPos = p->pos;
    stack.clear();
    labels.clear();
    gotos.clear();
    unknownStack = false;

    QSet<QByteArray> names;
    Declaration* sub = p->subs;
    while( sub )
    {
        if( sub->name.isEmpty() )
            error("a parameter or local variable has no name");
        else if( names.contains(sub->name) )
            error(QString("duplicate parameter or local variable: %1").arg(sub->name.constData()));
        else
            names.insert(sub->name);
        if( sub->getType() == 0 )
            error(QString("parameter or local variable without type: %1").arg(sub->name.constData()));
        else if( sub->varParam && sub->getType()->deref()->isStructured() &&
                 sub->getType()->deref()->kind == Type::Struct )
            ; // a VAR parameter of structured type is passed by reference anyway
        sub = sub->next;
    }

    if( p->typebound )
    {
        DeclList params = p->getParams();
        if( params.isEmpty() )
            error("a bound procedure requires a receiver parameter");
        else
        {
            Type* rt = params.first()->getType();
            if( rt )
                rt = rt->deref();
            if( rt == 0 || rt->kind != Type::Pointer || rt->getType() == 0 ||
                    !rt->getType()->deref()->isRecord() )
                error("the receiver of a bound procedure must be a POINTER TO the record type");
        }
    }

    if( p->extern_ || p->foreign_ )
    {
        if( p->body )
            error("an EXTERN or FOREIGN procedure has no body");
        curProc = 0;
        return;
    }

    if( p->init && p->getType() )
        error("a module initializer has no result type");

    validateBody(p->body, 0);

    if( !stack.isEmpty() && !unknownStack )
        error("the evaluation stack is not empty at the end of the procedure");

    foreach( QByteArray l, gotos )
    {
        if( !labels.contains(l) )
            error(QString("goto to an undeclared label: %1").arg(l.constData()));
    }

    curProc = 0;
}

void Validator::validateBody(Statement* s, int loopLevel)
{
    const int base = stack.size();
    while( s )
    {
        validateStat(s, loopLevel);
        s = s->next;
    }
    if( stack.size() != base )
    {
        if( !unknownStack )
            error("the evaluation stack is not balanced at the end of the statement sequence");
        while( stack.size() > base )
            stack.pop_back();
    }
}

void Validator::validateStat(Statement*& s, int loopLevel)
{
    if( s->pos.isValid() )
        curPos = s->pos;

    if( s->kind == Statement::ExprStat )
    {
        validateExprs(s->e);
        return;
    }

    const bool oldSuppress = suppress;
    if( usesDeclOperand(s->kind) && s->d && s->d->stub )
        suppress = true;

    switch( s->kind )
    {
    case op_IF:
        validateExprs(s->e);
        expectInt32(pop("IF"), "IF");
        validateBody(s->body, loopLevel);
        if( s->next && s->next->kind == op_ELSE )
        {
            s = s->next;
            validateBody(s->body, loopLevel);
        }
        break;
    case op_WHILE:
        validateExprs(s->e);
        expectInt32(pop("WHILE"), "WHILE");
        validateBody(s->body, loopLevel + 1);
        break;
    case op_REPEAT:
        validateBody(s->body, loopLevel + 1);
        validateExprs(s->e);
        expectInt32(pop("UNTIL"), "UNTIL");
        break;
    case op_LOOP:
        validateBody(s->body, loopLevel + 1);
        break;
    case op_SWITCH: {
            validateExprs(s->e);
            const Val v = pop("SWITCH");
            if( v.st != ST_void && v.st != ST_int32 && v.st != ST_int64 )
                error("SWITCH requires an integer value");
            CaseLabelList seen;
            while( s->next && s->next->kind == op_CASE )
            {
                s = s->next;
                if( s->labels == 0 || s->labels->isEmpty() )
                    error("CASE without labels");
                else
                {
                    for( int i = 0; i < s->labels->size(); i++ )
                    {
                        const CaseLabel& l = s->labels->at(i);
                        if( l.first > l.second )
                            error("invalid case label range");
                        for( int j = 0; j < seen.size(); j++ )
                        {
                            if( l.first <= seen[j].second && seen[j].first <= l.second )
                                error("overlapping case labels");
                        }
                        seen << l;
                    }
                }
                validateBody(s->body, loopLevel);
            }
            if( s->next && s->next->kind == op_ELSE )
            {
                s = s->next;
                validateBody(s->body, loopLevel);
            }
        } break;
    case op_CASE:
    case op_ELSE:
        error("CASE or ELSE without the corresponding IF or SWITCH");
        break;
    case op_exit:
        if( loopLevel == 0 )
            error("exit outside of a loop");
        break;
    case op_label:
        if( s->name.isEmpty() )
            error("label without name");
        else if( labels.contains(s->name) )
            error(QString("duplicate label: %1").arg(s->name.constData()));
        else
            labels.insert(s->name);
        if( !stack.isEmpty() && !unknownStack )
            error("the evaluation stack must be empty at a label");
        break;
    case op_goto:
        if( s->name.isEmpty() )
            error("goto without label");
        else
            gotos.insert(s->name);
        if( !stack.isEmpty() && !unknownStack )
            error("the evaluation stack must be empty at a goto");
        break;
    case op_pop: {
            const Val v = pop("pop");
            if( v.st == ST_desig )
                error("pop is not applicable to a designator reference");
        } break;
    case op_ret:
        if( curProc && curProc->getType() )
        {
            const Val v = pop("ret");
            assignable(curProc->getType(), v, "ret");
        }
        if( !stack.isEmpty() && !unknownStack )
            error("the evaluation stack must be empty when ret is executed");
        break;
    case op_raise:
        expectRef(pop("raise"), "raise");
        break;
    case op_free:
        expectRef(pop("free"), "free");
        break;
    case op_transfer: {
            const Val v = pop("transfer");
            if( v.st != ST_void && v.st != ST_task && v.st != ST_nil )
                error("transfer requires a task");
        } break;
    case op_copy: {
            const Val src = pop("copy");
            const Val dst = pop("copy");
            Type* t = typeOf(s->d);
            if( t == 0 || !( t->isStructured() || t->isForeign() ) )
                error("copy requires a structured or foreign type operand");
            expectRef(src, "copy");
            expectRef(dst, "copy");
        } break;
    case op_starg:
    case op_stloc: {
            const bool local = s->kind == op_stloc;
            DeclList l = local ? curProc->getLocals() : curProc->getParams();
            if( (int)s->id >= l.size() )
            {
                error(QString("%1 index out of range: %2").
                      arg(local ? "stloc" : "starg").arg(s->id));
                pop(local ? "stloc" : "starg");
            }else
            {
                Type* t = l[s->id]->getType();
                const Val v = pop(local ? "stloc" : "starg");
                if( t && t->deref()->isStructured() )
                    error("a structured value cannot be stored, use copy");
                else
                    assignable(t, v, local ? "stloc" : "starg");
            }
        } break;
    case op_stvar: {
            Type* t = typeOf(s->d);
            const Val v = pop("stvar");
            if( s->d == 0 || s->d->kind != Declaration::VarDecl )
                error("stvar requires a module variable");
            else if( t && t->deref()->isStructured() )
                error("a structured value cannot be stored, use copy");
            else
                assignable(t, v, "stvar");
        } break;
    case op_stfld: {
            const Val v = pop("stfld");
            expectRef(pop("stfld"), "stfld");
            if( s->d == 0 || s->d->kind != Declaration::Field )
                error("stfld requires a field");
            else
            {
                Type* t = s->d->getType();
                if( t && t->deref()->isStructured() )
                    error("a structured value cannot be stored, use copy");
                else
                    assignable(t, v, "stfld");
            }
        } break;
    case op_stelem: {
            const Val v = pop("stelem");
            expectInt32(pop("stelem"), "stelem");
            expectRef(pop("stelem"), "stelem");
            Type* t = typeOf(s->d);
            if( t == 0 || ( t->kind != Type::Array && t->kind != Type::CArray ) )
                error("stelem requires an array type operand");
            else
            {
                Type* et = t->getType();
                if( et && et->deref()->isStructured() )
                    error("a structured value cannot be stored, use copy");
                else
                    assignable(et, v, "stelem");
            }
        } break;
    case op_stind: {
            pop("stind");
            const Val p = pop("stind");
            Type* t = typeOf(s->d);
            if( t == 0 || !t->isBasic() )
                error("stind requires a basic type operand");
            expectRef(p, "stind");
        } break;
    default:
        error(QString("unexpected statement: %1").arg(s_opName[s->kind]));
        break;
    }
    suppress = oldSuppress;
}

void Validator::validateExprs(Expression* e)
{
    while( e )
    {
        validateExpr(e);
        e = e->next;
    }
}

void Validator::validateExpr(Expression* e)
{
    if( e->pos.isValid() )
        curPos = e->pos;

    const char* op = s_opName[e->kind];

    const bool oldSuppress = suppress;
    if( usesDeclOperand(e->kind) && e->d && e->d->stub )
    {
        // the declaration comes from a module which is not part of the model;
        // neither its type nor, for a call, its arity is known here
        suppress = true;
        switch( e->kind )
        {
        case op_call: case op_calli: case op_callinst: case op_callmi: case op_callvirt:
        case op_pcall: case op_pcalli:
            unknownStack = true;
            stack.clear();
            suppress = oldSuppress;
            return;
        }
    }

    switch( e->kind )
    {
    case op_abs:
    case op_neg: {
            const Val v = pop(op);
            expectNumeric(v, op);
            push(v);
        } break;
    case op_not: {
            const Val v = pop(op);
            expectInteger(v, op);
            push(v);
        } break;
    case op_add:
    case op_sub:
    case op_mul:
    case op_div:
    case op_rem: {
            const Val b = pop(op);
            const Val a = pop(op);
            sameStackType(a, b, op);
            expectNumeric(a, op);
            push(a);
        } break;
    case op_div_un:
    case op_rem_un:
    case op_and:
    case op_or:
    case op_xor: {
            const Val b = pop(op);
            const Val a = pop(op);
            sameStackType(a, b, op);
            expectInteger(a, op);
            push(a);
        } break;
    case op_shl:
    case op_shr:
    case op_shr_un: {
            expectInt32(pop(op), op);
            const Val v = pop(op);
            expectInteger(v, op);
            push(v);
        } break;
    case op_ceq:
    case op_cgt:
    case op_cgt_un:
    case op_clt:
    case op_clt_un: {
            const Val b = pop(op);
            const Val a = pop(op);
            sameStackType(a, b, op);
            if( a.st == ST_desig || b.st == ST_desig )
                error(QString("%1 is not applicable to designator references").arg(op));
            if( e->kind != op_ceq )
                expectNumeric(a, op);
            push(mdl->getBasicType(Type::INT32));
        } break;
    case op_conv_i1:
    case op_conv_i2:
    case op_conv_i4:
    case op_conv_u1:
    case op_conv_u2:
    case op_conv_u4:
        expectNumeric(pop(op), op);
        push(mdl->getBasicType(Type::INT32));
        break;
    case op_conv_i8:
    case op_conv_u8:
        expectNumeric(pop(op), op);
        push(mdl->getBasicType(Type::INT64));
        break;
    case op_conv_r4:
        expectNumeric(pop(op), op);
        push(mdl->getBasicType(Type::FLOAT32));
        break;
    case op_conv_r8:
        expectNumeric(pop(op), op);
        push(mdl->getBasicType(Type::FLOAT64));
        break;
    case op_dup: {
            const Val v = top();
            if( v.st == ST_desig )
                error("dup is not applicable to a designator reference");
            push(v);
        } break;
    case op_iif: {
            Expression* if_ = e->e;
            if( if_ == 0 || if_->next == 0 || if_->next->next == 0 )
            {
                error("incomplete conditional expression");
                break;
            }
            validateExprs(if_->e);
            expectInt32(pop("IIF"), "IIF");
            validateExprs(if_->next->e);
            const Val a = pop("IIF");
            validateExprs(if_->next->next->e);
            const Val b = pop("IIF");
            sameStackType(a, b, "IIF");
            push(a);
        } break;
    case op_ldc_i4:
        push(mdl->getBasicType(Type::INT32));
        break;
    case op_ldc_i8:
        push(mdl->getBasicType(Type::INT64));
        break;
    case op_ldc_r4:
        push(mdl->getBasicType(Type::FLOAT32));
        break;
    case op_ldc_r8:
        push(mdl->getBasicType(Type::FLOAT64));
        break;
    case op_ldnull:
        push(Val(0, ST_nil));
        break;
    case op_ldstr:
        if( e->c == 0 )
            error("ldstr without literal");
        push(Val(0, ST_ref));
        break;
    case op_curtask:
        push(mdl->getBasicType(Type::TASK));
        break;
    case op_newtask: {
            const Val v = pop(op);
            if( v.st != ST_void && v.st != ST_methref )
                error("newtask requires a bound procedure value");
            push(mdl->getBasicType(Type::TASK));
        } break;
    case op_taskdone: {
            const Val v = pop(op);
            if( v.st != ST_void && v.st != ST_task )
                error("taskdone requires a task");
            push(mdl->getBasicType(Type::INT32));
        } break;
    case op_len:
        expectRef(pop(op), op);
        push(mdl->getBasicType(Type::INT32));
        break;
    case op_ldarg:
    case op_ldloc:
    case op_refarg:
    case op_refloc: {
            const bool local = e->kind == op_ldloc || e->kind == op_refloc;
            DeclList l = local ? curProc->getLocals() : curProc->getParams();
            if( (int)e->id >= l.size() )
            {
                error(QString("%1 index out of range: %2").arg(op).arg(e->id));
                push(Val(0, ST_void));
            }else if( e->kind == op_ldarg || e->kind == op_ldloc )
                push(l[e->id]->getType());
            else
                push(Val(l[e->id]->getType(), ST_desig));
        } break;
    case op_ldvar:
        if( e->d == 0 || e->d->kind != Declaration::VarDecl )
            error("ldvar requires a module variable");
        push(typeOf(e->d));
        break;
    case op_refvar:
        if( e->d == 0 || e->d->kind != Declaration::VarDecl )
            error("refvar requires a module variable");
        push(Val(typeOf(e->d), ST_desig));
        break;
    case op_ldfld:
    case op_reffld:
        expectRef(pop(op), op);
        if( e->d == 0 || e->d->kind != Declaration::Field )
            error(QString("%1 requires a field").arg(op));
        if( e->kind == op_ldfld )
            push(typeOf(e->d));
        else
            push(Val(typeOf(e->d), ST_desig));
        break;
    case op_ldelem:
    case op_refelem: {
            expectInt32(pop(op), op);
            expectRef(pop(op), op);
            Type* t = typeOf(e->d);
            if( t == 0 || ( t->kind != Type::Array && t->kind != Type::CArray ) )
            {
                error(QString("%1 requires an array type operand").arg(op));
                push(Val(0, e->kind == op_ldelem ? ST_void : ST_desig));
            }else if( e->kind == op_ldelem )
                push(t->getType());
            else
                push(Val(t->getType(), ST_desig));
        } break;
    case op_ldproc:
        if( e->d == 0 || e->d->kind != Declaration::Procedure )
            error("ldproc requires a procedure");
        else if( e->d->typebound )
            error("ldproc is not applicable to a bound procedure");
        push(Val(0, ST_procref));
        break;
    case op_ldmeth:
    case op_ldvirt:
        expectRef(pop(op), op);
        if( e->d == 0 || e->d->kind != Declaration::Procedure || !e->d->typebound )
            error(QString("%1 requires a bound procedure").arg(op));
        push(Val(0, ST_methref));
        break;
    case op_castobj:
    case op_isinst: {
            expectRef(pop(op), op);
            Type* t = typeOf(e->d);
            if( t == 0 || !t->isRecord() )
                error(QString("%1 requires a record type operand").arg(op));
            if( e->kind == op_isinst )
                push(mdl->getBasicType(Type::INT32));
            else
                push(Val(t, ST_ref));
        } break;
    case op_newobj:
    case op_newobj_um: {
            Type* t = typeOf(e->d);
            if( t == 0 || !( t->kind == Type::Struct || t->kind == Type::Record ||
                             t->kind == Type::CStruct || t->kind == Type::CUnion ) )
                error(QString("%1 requires a struct or record type operand").arg(op));
            push(Val(t, ST_ref));
        } break;
    case op_newarr:
    case op_newarr_um: {
            expectInt32(pop(op), op);
            if( typeOf(e->d) == 0 )
                error(QString("%1 requires an element type operand").arg(op));
            push(Val(0, ST_ref));
        } break;
    case op_sizeof:
        if( typeOf(e->d) == 0 )
            error("sizeof requires a type operand");
        push(mdl->getBasicType(Type::INT32));
        break;
    case op_caddr: {
            const Val v = pop(op);
            if( v.st != ST_void && v.st != ST_desig )
                error("caddr requires a designator reference");
            push(Val(0, ST_ref));
        } break;
    case op_ldind: {
            expectRef(pop(op), op);
            Type* t = typeOf(e->d);
            if( t == 0 || !t->isBasic() )
                error("ldind requires a basic type operand");
            push(t);
        } break;
    case op_call:
    case op_pcall: {
            if( e->d == 0 || e->d->kind != Declaration::Procedure )
            {
                error(QString("%1 requires a procedure").arg(op));
                break;
            }
            if( e->d->typebound )
                error(QString("%1 is not applicable to a bound procedure").arg(op));
            popArgs(e->d->getParams(), op);
            if( e->kind == op_pcall )
            {
                if( e->d->getType() )
                    error("pcall requires a procedure without result");
                push(Val(0, ST_ref)); // the exception, POINTER TO ANYREC
            }else if( e->d->getType() )
                push(e->d->getType());
        } break;
    case op_calli:
    case op_pcalli:
    case op_callmi: {
            Type* t = typeOf(e->d);
            if( t )
                t = t->deref();
            if( t == 0 || !t->isCallable() )
            {
                error(QString("%1 requires a procedure type").arg(op));
                break;
            }
            const Val v = pop(op);
            const quint8 expected = e->kind == op_callmi ? ST_methref : ST_procref;
            if( v.st != ST_void && v.st != expected && v.st != ST_nil )
                error(QString("%1 requires a %2 on the stack").
                      arg(op).arg(stackTypeName(expected)));
            if( t->typebound != (e->kind == op_callmi) )
                error(QString("%1 with an inappropriate procedure type").arg(op));
            popArgs(t->getParams(), op);
            if( e->kind == op_pcalli )
            {
                if( t->getType() )
                    error("pcalli requires a procedure type without result");
                push(Val(0, ST_ref));
            }else if( t->getType() )
                push(t->getType());
        } break;
    case op_callinst:
    case op_callvirt: {
            if( e->d == 0 || e->d->kind != Declaration::Procedure || !e->d->typebound )
            {
                error(QString("%1 requires a bound procedure").arg(op));
                break;
            }
            DeclList params = e->d->getParams();
            if( params.isEmpty() )
                error(QString("%1: the bound procedure has no receiver").arg(op));
            else
            {
                DeclList args = params;
                args.removeFirst();
                popArgs(args, op);
                expectRef(pop(op), op); // the receiver
            }
            if( e->d->getType() )
                push(e->d->getType());
        } break;
    default:
        error(QString("unexpected expression instruction: %1").arg(op));
        break;
    }
    suppress = oldSuppress;
}

void Validator::popArgs(const DeclList& params, const char* op)
{
    QList<Val> args;
    for( int i = 0; i < params.size(); i++ )
        args.prepend(pop(op));
    for( int i = 0; i < params.size(); i++ )
    {
        Type* t = params[i]->getType();
        if( params[i]->varParam )
        {
            if( args[i].st != ST_void && args[i].st != ST_desig )
                error(QString("%1: the argument for the VAR parameter '%2' must be a designator reference").
                      arg(op).arg(params[i]->name.constData()));
        }else if( t && t->deref()->isStructured() )
        {
            if( args[i].st != ST_void && args[i].st != ST_ref && args[i].st != ST_nil )
                error(QString("%1: the argument for the parameter '%2' must be a reference").
                      arg(op).arg(params[i]->name.constData()));
        }else if( args[i].st == ST_desig )
            error(QString("%1: a designator reference cannot be passed to the value parameter '%2'").
                  arg(op).arg(params[i]->name.constData()));
        else
            assignable(t, args[i], op);
    }
}

Validator::Val Validator::pop(const char* op)
{
    if( stack.isEmpty() )
    {
        if( !unknownStack )
            error(QString("%1: the evaluation stack is empty").arg(op));
        return Val();
    }
    const Val res = stack.back();
    stack.pop_back();
    return res;
}

void Validator::push(Type* t)
{
    stack.push_back(valOf(t));
}

void Validator::push(const Val& v)
{
    stack.push_back(v);
}

Validator::Val Validator::top(int i) const
{
    if( i >= stack.size() )
        return Val();
    return stack[stack.size() - 1 - i];
}

void Validator::clear()
{
    stack.clear();
}

Validator::Val Validator::valOf(Type* t) const
{
    if( t == 0 )
        return Val();
    return Val(t, t->stackType());
}

Type* Validator::typeOf(Declaration* d) const
{
    if( d == 0 || d->getType() == 0 )
        return 0;
    return d->getType()->deref();
}

bool Validator::sameStackType(const Val& a, const Val& b, const char* op)
{
    if( a.st == ST_void || b.st == ST_void )
        return true; // already reported
    if( a.st == b.st )
        return true;
    if( a.st == ST_nil || b.st == ST_nil )
        return true;
    error(QString("%1: incompatible operands %2 and %3").
          arg(op).arg(stackTypeName(a.st)).arg(stackTypeName(b.st)));
    return false;
}

bool Validator::expectNumeric(const Val& v, const char* op)
{
    if( v.st == ST_void || ( v.st >= ST_int32 && v.st <= ST_float64 ) )
        return true;
    error(QString("%1 requires a numeric operand instead of %2").arg(op).arg(stackTypeName(v.st)));
    return false;
}

bool Validator::expectInteger(const Val& v, const char* op)
{
    if( v.st == ST_void || v.st == ST_int32 || v.st == ST_int64 )
        return true;
    error(QString("%1 requires an integer operand instead of %2").arg(op).arg(stackTypeName(v.st)));
    return false;
}

bool Validator::expectInt32(const Val& v, const char* op)
{
    if( v.st == ST_void || v.st == ST_int32 )
        return true;
    error(QString("%1 requires an int32 operand instead of %2").arg(op).arg(stackTypeName(v.st)));
    return false;
}

bool Validator::expectRef(const Val& v, const char* op)
{
    if( v.st == ST_void || v.st == ST_ref || v.st == ST_nil )
        return true;
    error(QString("%1 requires a reference instead of %2").arg(op).arg(stackTypeName(v.st)));
    return false;
}

bool Validator::assignable(Type* target, const Val& v, const char* op)
{
    if( target == 0 || v.st == ST_void )
        return true; // already reported
    Type* t = target->deref();
    if( t == 0 )
        return true;
    const quint8 ts = t->stackType();
    if( v.st == ST_desig )
    {
        error(QString("%1: a designator reference cannot be assigned").arg(op));
        return false;
    }
    if( v.st == ST_nil )
    {
        if( ts == ST_ref || ts == ST_procref || ts == ST_methref || ts == ST_task )
            return true;
        error(QString("%1: nil cannot be assigned to %2").arg(op).arg(stackTypeName(ts)));
        return false;
    }
    if( ts == v.st )
        return true;
    if( ts == ST_int64 && v.st == ST_int32 )
        return true; // widening to the larger integer type
    if( ts == ST_float64 && v.st == ST_float32 )
        return true;
    error(QString("%1: %2 is not assignment compatible with %3").
          arg(op).arg(stackTypeName(v.st)).arg(stackTypeName(ts)));
    return false;
}

void Validator::error(const QString& msg)
{
    error(msg, curPos);
}

void Validator::error(const QString& msg, const RowCol& pos)
{
    if( suppress )
        return;
    Error e;
    e.msg = msg;
    e.where = curProc ? curProc->toPath() : ( module ? module->name : QByteArray() );
    e.pc = pos.d_row;
    errors << e;
}
