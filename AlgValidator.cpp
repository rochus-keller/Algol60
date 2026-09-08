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

#include <Algol60/AlgValidator.h>
#include <Algol60/AlgLexer.h>
#include <QtDebug>
using namespace Alg;

enum Use { Use_none = 0, Use_subscript = 1, Use_call = 2, Use_assign = 4, Use_goto = 8,
           Use_arith = 16, Use_bool = 32, Use_actual = 64 };

Validator::Validator(AstModel* mdl):mdl(mdl),module(0)
{
}

bool Validator::validate(Declaration* module)
{
    errors.clear();
    if( module == 0 )
        return false;
    this->module = module;
    if( module->path )
        sourcePath = *module->path;

    Declaration* prog = module->link;
    while( prog && prog->kind != Declaration::Program )
        prog = prog->next;
    if( prog == 0 )
         return error(RowCol(), "no program in this module"), false;

    procActuals.clear();
    formalCalls.clear();
    procActualSites.clear();
    Signatures(prog);
    ProgramDecl(prog);
    ResolveProcActuals();

    module->validated = true;
    module->hasErrors = !errors.isEmpty();
    return errors.isEmpty();
}

DeclList Validator::params(Declaration* proc)
{
    DeclList res;
    Declaration* d = proc ? proc->link : 0;
    while( d ) {
        if( d->isParam )
            res << d;
        d = d->next;
    }
    return res;
}

Declaration* Validator::declProc(Declaration* d)
{
    // the procedure or program activation a declaration belongs to
    Declaration* s = d ? d->outer : 0;
    while( s && s->kind != Declaration::Procedure && s->kind != Declaration::Program )
        s = s->outer;
    return s;
}

void Validator::Signatures(Declaration* scope)
{
    Declaration* d = scope->link;
    while( d ) {
        switch( d->kind ) {
        case Declaration::Procedure:
            ProcSignature(d);
            Signatures(d);
            break;
        case Declaration::Block:
            Signatures(d);
            break;
        default:
            break;
        }
        d = d->next;
    }
}

void Validator::ProcSignature(Declaration* proc)
{
    const DeclList p = params(proc);
    for( int i = 0; i < p.size(); i++ ) {
        Declaration* param = p[i];
        if( param->mode != Declaration::ModeValue )
            param->mode = Declaration::ModeName; // the Algol 60 default
        if( !param->isSpec )
            InferParam(proc, param);
        if( param->getType() == 0 )
            param->setType(mdl->getType(Type::Real));
    }
}

void Validator::InferParam(Declaration* proc, Declaration* param)
{
    // the specification part is (unfortunately) optional in Algol 60; the kind and the type of the
    // formal are inferred from its uses in the procedure body
    int uses = Use_none;
    ScanUses(proc->body, param, uses);

    if( uses & Use_goto ) {
        param->setType(mdl->getType(Type::Label));
        param->mode = Declaration::ModeName;

    }else if( uses & Use_call ) {
        Type* t = new Type(Type::Procedure);
        t->setType(mdl->getType( (uses & (Use_arith|Use_bool)) ? Type::Real : Type::NoType ));
        param->setType(t);
        param->mode = Declaration::ModeName;
    }else if( uses & Use_subscript ) {
        Type* t = new Type(Type::Array);
        t->setType(mdl->getType( (uses & Use_bool) ? Type::Boolean : Type::Real ));
        param->setType(t);
        param->kind = Declaration::Array;
    }else if( uses & Use_bool )
        param->setType(mdl->getType(Type::Boolean));
    else
        param->setType(mdl->getType(Type::Real));

    if( uses == Use_none || uses == Use_actual )
        // the formal is not used, or only passed on, so nothing can be inferred from this body
        param->setType(mdl->getType(Type::Real));
}

void Validator::ScanUses(Statement* s, Declaration* param, int& uses)
{
    while( s )
    {
        switch( s->kind ) {
        case Statement::Compound:
        case Statement::Block:
            ScanUses(s->body, param, uses);
            break;
        case Statement::Assign:
            {
                Expression* lhs = s->lhs;
                while( lhs ) {
                    if( lhs->kind == Expression::Identifier && lhs->a == param->sym )
                        uses |= Use_assign;
                    ScanUses(lhs, param, uses);
                    lhs = lhs->next;
                }
                ScanUses(s->rhs, param, uses);
            }
            break;
        case Statement::Call:
            if( s->lhs && s->lhs->kind == Expression::Identifier && s->lhs->a == param->sym )
                uses |= Use_call;
            ScanUses(s->rhs, param, uses);
            break;
        case Statement::If:
            ScanUses(s->cond, param, uses);
            ScanUses(s->body, param, uses);
            ScanUses(s->elseStmt, param, uses);
            break;
        case Statement::For:
            ScanUses(s->var, param, uses);
            ScanUses(s->list, param, uses);
            ScanUses(s->body, param, uses);
            break;
        case Statement::Goto:
            if( s->lhs && s->lhs->kind == Expression::Identifier && s->lhs->a == param->sym )
                uses |= Use_goto;
            ScanUses(s->lhs, param, uses);
            break;
        default:
            break;
        }
        s = s->next;
    }
}

void Validator::ScanUses(Expression* e, Declaration* param, int& uses)
{
    while( e )
    {
        switch( e->kind ) {
        case Expression::Subscript:
            if( e->lhs && e->lhs->kind == Expression::Identifier && e->lhs->a == param->sym )
                uses |= Use_subscript;
            break;
        case Expression::Call:
            if( e->lhs && e->lhs->kind == Expression::Identifier && e->lhs->a == param->sym )
                uses |= Use_call;
            else
                uses |= Use_actual;
            break;

        case Expression::Add:
        case Expression::Sub:
        case Expression::Mul:
        case Expression::Div:
        case Expression::IntDiv:
        case Expression::Mod:
        case Expression::Power:
        case Expression::Neg:
        case Expression::Lt:
        case Expression::Leq:
        case Expression::Gt:
        case Expression::Geq:
            if( ( e->lhs && e->lhs->kind == Expression::Identifier && e->lhs->a == param->sym ) ||
                ( e->rhs && e->rhs->kind == Expression::Identifier && e->rhs->a == param->sym ) )
                uses |= Use_arith;
            break;

        case Expression::And:
        case Expression::Or:
        case Expression::Not:
        case Expression::Imp:
        case Expression::Eqv:
            if( ( e->lhs && e->lhs->kind == Expression::Identifier && e->lhs->a == param->sym ) ||
                ( e->rhs && e->rhs->kind == Expression::Identifier && e->rhs->a == param->sym ) )
                uses |= Use_bool;
            break;
        default:
            break;
        }
        ScanUses(e->lhs, param, uses);
        ScanUses(e->rhs, param, uses);
        ScanUses(e->condition, param, uses);
        e = e->next;
    }
}

void Validator::ProgramDecl(Declaration* prog)
{
    scopeStack.push_back(prog);
    procStack.push_back(prog);
    DeclSeq(prog->link);
    StatSeq(prog->body);
    procStack.pop_back();
    scopeStack.pop_back();
    prog->validated = true;
}

void Validator::DeclSeq(Declaration* d)
{
    while( d ) {
        switch( d->kind ) {
        case Declaration::Procedure:
            ProcDecl(d);
            break;
        case Declaration::Array:
            ArrayDecl(d);
            break;
        case Declaration::Switch:
            SwitchDecl(d);
            break;
        default:
            break;
        }
        d = d->next;
    }
}

void Validator::ProcDecl(Declaration* d)
{
    if( visited.contains(d) )
        return;
    visited.insert(d);

    scopeStack.push_back(d);
    procStack.push_back(d);
    DeclSeq(d->link);
    StatSeq(d->body);
    procStack.pop_back();
    scopeStack.pop_back();
    d->validated = true;
}

void Validator::ArrayDecl(Declaration* d)
{
    Type* t = d->getType();
    if( t == 0 || t->kind != Type::Array )
        return error(d->pos, "invalid array declaration");
    if( d->isParam )
        return; // a formal array has no bounds

    int n = 0;
    Expression* e = t->getExpr();
    while( e ) {
        if( Expr(e) && e->getType() && !e->getType()->isArithmetic() )
            error(e->pos, "array bound must be of arithmetic type");
        n++;
        e = e->next;
    }
    if( n == 0 || (n & 1) )
        error(d->pos, "invalid array bounds");
    d->validated = true;
}

void Validator::SwitchDecl(Declaration* d)
{
    if( d->validated )
        return; // a switch list may be reached again through one of its own elements
    d->validated = true;
    Expression* e = d->list;
    while( e ) {
        if( Designator(e) )
            markLabel(e, true); // switch elements are reached by a dynamic goto
        e = e->next;
    }
}

void Validator::StatSeq(Statement* s)
{
    while( s ) {
        Stat(s);
        s = s->next;
    }
}

void Validator::Stat(Statement* s)
{
    if( s == 0 )
        return;
    switch( s->kind ) {
    case Statement::Compound:
    case Statement::Block:
        BlockStat(s);
        break;
    case Statement::Assign:
        AssignStat(s);
        break;
    case Statement::Call:
        CallStat(s);
        break;
    case Statement::If:
        IfStat(s);
        break;
    case Statement::For:
        ForStat(s);
        break;
    case Statement::Goto:
        GotoStat(s);
        break;
    case Statement::Label:
    case Statement::Dummy:
        break;
    default:
        error(s->pos, "invalid statement");
        break;
    }
    s->validated = true;
}

void Validator::BlockStat(Statement* s)
{
    Declaration* scope = s->getScope();
    if( scope == 0 )
        return error(s->pos, "block without scope");
    scopeStack.push_back(scope);
    DeclSeq(scope->link);
    StatSeq(s->body);
    scopeStack.pop_back();
}

void Validator::AssignStat(Statement* s)
{
    Expr(s->rhs);

    Expression* lhs = s->lhs;
    while( lhs )
    {
        if( Expr(lhs) )
        {
            Declaration* d = 0;
            if( lhs->kind == Expression::DeclRef )
                d = lhs->d;
            else if( lhs->kind == Expression::Subscript && lhs->lhs && lhs->lhs->kind == Expression::DeclRef )
                d = lhs->lhs->d;
            if( d == 0 )
                error(lhs->pos, "invalid left part of an assignment");
            else
            {
                switch( d->kind ) {
                case Declaration::Variable:
                case Declaration::Parameter:
                case Declaration::Array:
                    d->assigned = true;
                    break;
                case Declaration::Procedure:
                    if( !procStack.contains(d) )
                        error(lhs->pos, "assignment to a procedure identifier outside its body");
                    else if( d->getType() == 0 || d->getType()->kind == Type::NoType )
                        error(lhs->pos, "assignment to the identifier of a proper procedure");
                    else
                        d->assigned = true;
                    break;
                default:
                    error(lhs->pos, "invalid left part of an assignment");
                    break;
                }
            }
            if( s->rhs && lhs->getType() )
                assigCompat(lhs->getType(), s->rhs->getType(), s->pos);
        }
        lhs = lhs->next;
    }
}

void Validator::CallStat(Statement* s)
{
    if( s->lhs == 0 )
        return;

    // a procedure statement is represented as a Call statement, even without actual parameters
    if( !Expr(s->lhs) )
        return;
    if( s->lhs->kind != Expression::DeclRef )
        return error(s->pos, "invalid procedure statement");

    Declaration* d = s->lhs->d;
    Expression* args = s->rhs;

    switch( d->kind ) {
    case Declaration::Builtin:
        BuiltinCall(0, d, args, s->pos);
        break;
    case Declaration::Procedure:
        Args(d, args, s->pos);
        break;
    case Declaration::Parameter:
        if( d->getType() && d->getType()->kind == Type::Procedure )
        {
            Args(0, args, s->pos); // the formals of a formal procedure are not known here yet
            formalCalls << FormalCall(d, args, s->pos);
        }else
            error(s->pos, "this identifier does not designate a procedure");
        break;
    default:
        error(s->pos, "this identifier does not designate a procedure");
        break;
    }
}

void Validator::IfStat(Statement* s)
{
    if( Expr(s->cond) && s->cond->getType() && s->cond->getType()->kind != Type::Boolean )
        error(s->cond->pos, "the condition must be of type Boolean");
    StatSeq(s->body);
    StatSeq(s->elseStmt);
}

void Validator::ForStat(Statement* s)
{
    if( Expr(s->var) ) {
        Type* t = s->var->getType();
        if( t && !t->isArithmetic() )
            error(s->var->pos, "the controlled variable must be of arithmetic type");
        Declaration* d = 0;
        if( s->var->kind == Expression::DeclRef )
            d = s->var->d;
        else if( s->var->kind == Expression::Subscript && s->var->lhs &&
                 s->var->lhs->kind == Expression::DeclRef )
            d = s->var->lhs->d;
        if( d )
            d->assigned = true;
    }

    Expression* e = s->list;
    while( e ) {
        switch( e->kind ) {
        case Expression::StepUntil:
            Expr(e->lhs);
            Expr(e->rhs);
            Expr(e->condition);
            break;
        case Expression::WhileLoop:
            Expr(e->lhs);
            if( Expr(e->condition) && e->condition->getType() &&
                    e->condition->getType()->kind != Type::Boolean )
                error(e->condition->pos, "the condition must be of type Boolean");
            break;
        default:
            Expr(e);
            break;
        }
        e = e->next;
    }

    StatSeq(s->body);
}

void Validator::GotoStat(Statement* s)
{
    Designator(s->lhs);
}

bool Validator::Expr(Expression* e)
{
    if( e == 0 )
        return false;
    if( e->validated )
        return e->getType() != 0;
    e->validated = true;

    bool ok = false;
    switch( e->kind )
    {
    case Expression::Add:
    case Expression::Sub:
    case Expression::Mul:
    case Expression::Div:
    case Expression::IntDiv:
    case Expression::Mod:
    case Expression::Power:
    case Expression::And:
    case Expression::Or:
    case Expression::Imp:
    case Expression::Eqv:
    case Expression::Eq:
    case Expression::Neq:
    case Expression::Lt:
    case Expression::Leq:
    case Expression::Gt:
    case Expression::Geq:
        ok = BinaryOp(e);
        break;

    case Expression::Neg:
    case Expression::Not:
        ok = UnaryOp(e);
        break;
    case Expression::Identifier:
        ok = Identifier(e);
        break;
    case Expression::DeclRef:
        ok = e->getType() != 0;
        break;
    case Expression::Subscript:
        ok = SubscriptExpr(e);
        break;
    case Expression::Call:
        ok = CallExpr(e);
        break;
    case Expression::IfExpr:
        ok = IfExpr(e);
        break;

    case Expression::StringConst:
    case Expression::UnsignedConst:
    case Expression::RealConst:
    case Expression::BoolConst:
        ok = e->getType() != 0;
        break;

    default:
        error(e->pos, "invalid expression");
        break;
    }
    if( !ok )
        e->hasErrors = true;
    return ok;
}

bool Validator::BinaryOp(Expression* e)
{
    if( e->lhs == 0 || e->rhs == 0 )
        return error(e->pos, "invalid binary operation"), false;
    const bool lok = Expr(e->lhs);
    const bool rok = Expr(e->rhs);
    if( !lok || !rok )
        return false;

    Type* t = resultType(e->kind, e->lhs->getType(), e->rhs->getType(), e);
    if( t == 0 )
        return error(e->pos, QString("operator %1 is not applicable to these operand types").
                     arg(Expression::name[e->kind])), false;
    e->setType(t);
    return true;
}

bool Validator::UnaryOp(Expression* e)
{
    if( e->rhs == 0 )
        return error(e->pos, "invalid unary operation"), false;
    if( !Expr(e->rhs) )
        return false;
    Type* t = e->rhs->getType();
    if( e->kind == Expression::Not )
    {
        if( t->kind != Type::Boolean )
            return error(e->pos, "the operand must be of type Boolean"), false;
        e->setType(mdl->getType(Type::Boolean));
    }else
    {
        if( !t->isArithmetic() )
            return error(e->pos, "the operand must be of arithmetic type"), false;
        e->setType(t);
    }
    return true;
}

bool Validator::Identifier(Expression* e)
{
    Declaration* d = resolve(e->a);
    if( d == 0 )
        return error(e->pos, QString("declaration for '%1' not found").arg(e->a)), false;

    e->kind = Expression::DeclRef;
    e->d = d;

    if( d->kind != Declaration::Builtin && d->kind != Declaration::Procedure ) {
        Declaration* home = declProc(d);
        if( home && !procStack.isEmpty() && home != procStack.back() )
            markEscape(d); // referenced from an inner procedure, thus the frame must be lifted
    }

    switch( d->kind )
    {
    case Declaration::Variable:
    case Declaration::Array:
    case Declaration::Switch:
    case Declaration::LabelDecl:
        e->setType(d->getType());
        break;
    case Declaration::Parameter:
        if( d->getType() && d->getType()->kind == Type::Procedure )
            e->setType(d->getType()->getType()); // a call of the formal procedure without actuals
        else
            e->setType(d->getType());
        break;
    case Declaration::Procedure:
        // either the result variable of the enclosing function, or a call without actuals
        e->setType(d->getType());
        break;
    case Declaration::Builtin:
        switch( d->id )
        {
        case Builtin::MAXREAL:
        case Builtin::MINREAL:
        case Builtin::EPSILON:
            e->setType(mdl->getType(Type::Real));
            break;
        case Builtin::MAXINT:
            e->setType(mdl->getType(Type::Integer));
            break;
        default:
            BuiltinCall(e, d, 0, e->pos, false);
            break;
        }
        break;
    default:
        return error(e->pos, QString("cannot use '%1' in an expression").arg(d->name.constData())), false;
    }
    return e->getType() != 0;
}

bool Validator::SubscriptExpr(Expression* e)
{
    if( e->lhs == 0 || !Expr(e->lhs) )
        return false;
    if( e->lhs->kind != Expression::DeclRef )
        return error(e->pos, "only an array or switch identifier can be subscripted"), false;

    Declaration* d = e->lhs->d;
    Type* t = d->getType();
    int n = 0;
    Expression* sub = e->rhs;
    while( sub ) {
        if( Expr(sub) && sub->getType() && !sub->getType()->isArithmetic() )
            error(sub->pos, "a subscript must be of arithmetic type");
        n++;
        sub = sub->next;
    }

    if( t == 0 )
        return false;
    if( t->kind == Type::Array )
    {
        if( !d->isParam && t->getDims() != n )
            error(e->pos, "number of subscripts differs from the number of dimensions");
        e->setType(t->getType());
    }else if( t->kind == Type::Switch )
    {
        if( n != 1 )
            error(e->pos, "a switch designator has exactly one subscript");
        e->setType(mdl->getType(Type::Label));
    }else
        return error(e->pos, "only an array or switch identifier can be subscripted"), false;
    return true;
}

bool Validator::CallExpr(Expression* e)
{
    if( e->lhs == 0 || !Expr(e->lhs) )
        return false;
    if( e->lhs->kind != Expression::DeclRef )
        return error(e->pos, "invalid function designator"), false;

    Declaration* d = e->lhs->d;
    switch( d->kind )
    {
    case Declaration::Builtin:
        BuiltinCall(e, d, e->rhs, e->pos);
        break;
    case Declaration::Procedure:
        Args(d, e->rhs, e->pos);
        if( d->getType() == 0 || d->getType()->kind == Type::NoType )
            return error(e->pos, "a proper procedure cannot be used in an expression"), false;
        e->setType(d->getType());
        break;
    case Declaration::Parameter:
        if( d->getType() == 0 || d->getType()->kind != Type::Procedure )
            return error(e->pos, "this identifier does not designate a procedure"), false;
        Args(0, e->rhs, e->pos);
        formalCalls << FormalCall(d, e->rhs, e->pos);
        e->setType(d->getType()->getType());
        break;
    default:
        return error(e->pos, "invalid function designator"), false;
    }
    return e->getType() != 0;
}

bool Validator::IfExpr(Expression* e)
{
    if( Expr(e->condition) && e->condition->getType() &&
            e->condition->getType()->kind != Type::Boolean )
        error(e->condition->pos, "the condition must be of type Boolean");
    const bool lok = Expr(e->lhs);
    const bool rok = Expr(e->rhs);
    if( !lok || !rok )
        return false;
    Type* lt = e->lhs->getType();
    Type* rt = e->rhs->getType();
    if( lt->isArithmetic() && rt->isArithmetic() )
        e->setType( lt->kind == rt->kind ? lt : mdl->getType(Type::Real) );
    else if( lt->kind == rt->kind )
        e->setType(lt);
    else
        return error(e->pos, "the alternatives have incompatible types"), false;
    return true;
}

bool Validator::Designator(Expression* e)
{
    if( e == 0 )
        return false;

    if( e->kind == Expression::UnsignedConst )
    {
        // an integer label of the Revised Report
        e->kind = Expression::Identifier;
        e->a = Lexer::toId(QByteArray::number(e->u));
        e->setType(0);
        e->validated = false;
    }

    switch( e->kind )
    {
    case Expression::IfExpr:
        if( Expr(e->condition) && e->condition->getType() &&
                e->condition->getType()->kind != Type::Boolean )
            error(e->condition->pos, "the condition must be of type Boolean");
        return Designator(e->lhs) && Designator(e->rhs);

    case Expression::Subscript:
        {
            // a switch designator
            if( e->lhs == 0 || ( e->lhs->kind != Expression::Identifier &&
                                 e->lhs->kind != Expression::DeclRef ) )
                return error(e->pos, "invalid switch designator"), false;
            Declaration* d = e->lhs->kind == Expression::DeclRef ?
                        e->lhs->d : resolve(e->lhs->a);
            if( d == 0 )
                return error(e->pos, QString("declaration for '%1' not found").arg(e->lhs->a)), false;
            e->lhs->kind = Expression::DeclRef;
            e->lhs->d = d;
            e->lhs->setType(d->getType());
            e->lhs->validated = true;
            if( d->getType() == 0 || d->getType()->kind != Type::Switch )
                return error(e->pos, "only a switch identifier can be subscripted here"), false;
            if( d->kind == Declaration::Switch )
                SwitchDecl(d);
            Expression* sub = e->rhs;
            int n = 0;
            while( sub ) {
                if( Expr(sub) && sub->getType() && !sub->getType()->isArithmetic() )
                    error(sub->pos, "a subscript must be of arithmetic type");
                n++;
                sub = sub->next;
            }
            if( n != 1 )
                error(e->pos, "a switch designator has exactly one subscript");
            e->setType(mdl->getType(Type::Label));
            e->validated = true;
            return true;
        }

    case Expression::Identifier:
        {
            Declaration* d = resolveLabel(e->a);
            if( d == 0 )
                return error(e->pos, QString("label '%1' not found").arg(e->a)), false;
            e->kind = Expression::DeclRef;
            e->d = d;
            e->setType(mdl->getType(Type::Label));
            e->validated = true;
            if( d->kind == Declaration::LabelDecl ) {
                Declaration* home = declProc(d);
                if( home && !procStack.isEmpty() && home != procStack.back() )
                    d->nonlocal = true; // reached by a goto out of an inner procedure
            }else if( d->kind != Declaration::Parameter )
                return error(e->pos, QString("'%1' does not designate a label").arg(e->a)), false;
            return true;
        }

    case Expression::DeclRef:
        return true;

    default:
        return error(e->pos, "invalid designational expression"), false;
    }
}

void Validator::Args(Declaration* proc, Expression* args, const RowCol& pos)
{
    DeclList formals;
    if( proc )
        formals = params(proc);

    int i = 0;
    Expression* a = args;
    while( a )
    {
        Declaration* formal = i < formals.size() ? formals[i] : 0;
        Type* ft = formal ? formal->getType() : 0;

        if( ft && ft->kind == Type::Label ) {
            if( Designator(a) )
                markLabel(a, true); // the label is reached by a goto through the formal
        }else if( ft && ft->kind == Type::Switch ) {
            Expr(a);
        }else if( Expr(a) ) {
            Type* at = a->getType();
            if( formal == 0 )
                markEscape(a); // the mode of the formal is unknown, thus assume by name
            else if( formal->kind == Declaration::Array || ( ft && ft->kind == Type::Array ) ) {
                if( a->kind != Expression::DeclRef || a->d == 0 ||
                        a->d->getType() == 0 || a->d->getType()->kind != Type::Array )
                    error(a->pos, "an array identifier is required here");
                else
                    markEscape(a);
            }else if( ft && ft->kind == Type::Procedure ) {
                if( a->kind != Expression::DeclRef ||
                        ( a->d && a->d->kind != Declaration::Procedure && a->d->kind != Declaration::Parameter ) )
                    error(a->pos, "a procedure identifier is required here");
                else
                    ProcActual(formal, a);
            }else if( formal->mode == Declaration::ModeName ) {
                markEscape(a); // the actual becomes a thunk bound to the frame of this activation
                if( at && ft && at->isBasic() && ft->isBasic() && at->kind != ft->kind &&
                        !( at->isArithmetic() && ft->isArithmetic() ) )
                    error(a->pos, "the actual parameter is not compatible with the formal");
            }else if( at && ft )
                assigCompat(ft, at, a->pos);
        }
        i++;
        a = a->next;
    }

    if( proc && i != formals.size() )
        error(pos, QString("expecting %1 actual parameters, got %2").arg(formals.size()).arg(i));
}

void Validator::ProcActual(Declaration* formal, Expression* actual)
{
    DeclList& actuals = procActuals[formal];
    if( !actuals.contains(actual->d) )
        actuals << actual->d;
    procActualSites << qMakePair(formal, actual);
}

void Validator::ResolveProcActuals()
{
    QHash<Declaration*,DeclList> resolved;
    bool changed = true;
    while( changed )
    {
        changed = false;

        bool grown = true;
        while( grown )
        {
            grown = false;
            QHash<Declaration*,DeclList>::const_iterator i;
            for( i = procActuals.begin(); i != procActuals.end(); ++i )
            {
                DeclList& to = resolved[i.key()];
                foreach( Declaration* a, i.value() )
                {
                    DeclList from;
                    if( a->kind == Declaration::Parameter )
                        from = resolved.value(a);
                    else
                        from << a;
                    foreach( Declaration* b, from )
                        if( b != i.key() && !to.contains(b) ) {
                            to << b;
                            grown = true;
                        }
                }
            }
        }

        for( int k = 0; k < formalCalls.size(); k++ )
        {
            Declaration* callee = uniqueProc(resolved.value(formalCalls[k].formal));
            if( callee == 0 )
                continue;
            const DeclList formals = params(callee);
            int n = 0;
            Expression* a = formalCalls[k].args;
            while( a && n < formals.size() )
            {
                Type* ft = formals[n]->getType();
                if( ft && ft->kind == Type::Procedure && a->kind == Expression::DeclRef &&
                        a->d && ( a->d->kind == Declaration::Procedure ||
                                  a->d->kind == Declaration::Parameter ) ) {
                    DeclList& list = procActuals[formals[n]];
                    if( !list.contains(a->d) ) {
                        list << a->d;
                        changed = true;
                    }
                }
                n++;
                a = a->next;
            }
        }
    }

    QHash<Declaration*,DeclList>::const_iterator i;
    for( i = resolved.begin(); i != resolved.end(); ++i )
    {
        i.key()->actual = uniqueProc(i.value());
        if( i.key()->actual == 0 )
            ProcActualsError(i.key(), i.value(), resolved);
    }

    FormalCallsError();
}

void Validator::FormalCallsError()
{
    for( int k = 0; k < formalCalls.size(); k++ )
    {
        Declaration* callee = formalCalls[k].formal->actual;
        if( callee == 0 )
            continue;
        int n = 0;
        Expression* a = formalCalls[k].args;
        while( a )
        {
            n++;
            a = a->next;
        }
        const DeclList formals = params(callee);
        if( n != formals.size() )
        {
            error(formalCalls[k].pos,
                  QString("the actual procedure of this formal expects %1 actual parameters, got %2")
                  .arg(formals.size()).arg(n));
        }
    }
}


void Validator::ProcActualsError(Declaration* formal, const DeclList& candidates, const QHash<Declaration*,DeclList>& resolved)
{
    Declaration* ref = 0;
    foreach( Declaration* d, candidates )
    {
        if( d->kind == Declaration::Procedure && !params(d).isEmpty() )
        {
            ref = d;
            break;
        }
    }

    if( ref == 0 )
        return; // a parameterless formal procedure needs no signature of its own

    for( int i = 0; i < procActualSites.size(); i++ ) {
        if( procActualSites[i].first != formal )
            continue;
        Expression* site = procActualSites[i].second;
        DeclList actuals;
        if( site->d->kind == Declaration::Parameter )
            actuals = resolved.value(site->d);
        else
            actuals << site->d;
        foreach( Declaration* d, actuals )
        {
            if( d->kind == Declaration::Procedure && !sigCompat(ref, d) ) {
                error(site->pos, "the actual procedures passed to this formal procedure "
                                 "have different signatures");
                break;
            }
        }
    }
}

Declaration* Validator::uniqueProc(const DeclList& candidates)
{
    // all candidates must be procedures with the same signature to give the formal a signature
    if( candidates.isEmpty() || candidates.first()->kind != Declaration::Procedure )
        return 0;
    for( int i = 1; i < candidates.size(); i++ )
        if( candidates[i]->kind != Declaration::Procedure ||
                !sigCompat(candidates.first(), candidates[i]) )
            return 0;
    return candidates.first();
}

bool Validator::sigCompat(Declaration* lhs, Declaration* rhs)
{
    Type* lt = lhs->getType();
    Type* rt = rhs->getType();
    if( ( lt ? lt->kind : Type::NoType ) != ( rt ? rt->kind : Type::NoType ) )
        return false;
    const DeclList l = params(lhs);
    const DeclList r = params(rhs);
    if( l.size() != r.size() )
        return false;
    for( int i = 0; i < l.size(); i++ ) {
        if( l[i]->kind != r[i]->kind || l[i]->mode != r[i]->mode ||
                l[i]->assigned != r[i]->assigned )
            return false;
        Type* a = l[i]->getType();
        Type* b = r[i]->getType();
        if( ( a ? a->kind : Type::NoType ) != ( b ? b->kind : Type::NoType ) )
            return false;
        if( a && a->kind == Type::Array &&
                ( a->getType() ? a->getType()->kind : Type::NoType ) !=
                ( b->getType() ? b->getType()->kind : Type::NoType ) )
            return false;
    }
    return true;
}

void Validator::BuiltinCall(Expression* call, Declaration* builtin, Expression* args,
                           const RowCol& pos, bool checkArity)
{
    int n = 0;
    Expression* a = args;
    while( a ) {
        if( a->kind == Expression::StringConst )
            a->validated = true;
        else
            Expr(a);
        n++;
        a = a->next;
    }

    Type* res = 0;
    int minArgs = -1, maxArgs = -1;
    switch( builtin->id ) {
    case Builtin::ABS:
    case Builtin::IABS:
    case Builtin::SIGN:
    case Builtin::ENTIER:
        minArgs = maxArgs = 1;
        res = mdl->getType( builtin->id == Builtin::ABS ? Type::Real : Type::Integer );
        if( builtin->id == Builtin::ABS && args && args->getType() )
            res = args->getType();
        break;
    case Builtin::SQRT:
    case Builtin::SIN:
    case Builtin::COS:
    case Builtin::ARCTAN:
    case Builtin::LN:
    case Builtin::EXP:
        minArgs = maxArgs = 1;
        res = mdl->getType(Type::Real);
        break;
    case Builtin::LENGTH:
        minArgs = maxArgs = 1;
        res = mdl->getType(Type::Integer);
        break;
    case Builtin::ININTEGER:
    case Builtin::INREAL:
    case Builtin::OUTINTEGER:
    case Builtin::OUTREAL:
    case Builtin::OUTSTRING:
        // the channel is the first actual in the Modified Report, but often left out
        minArgs = 1;
        maxArgs = 2;
        res = mdl->getType(Type::NoType);
        break;
    case Builtin::INCHAR:
    case Builtin::OUTCHAR:
    case Builtin::INSYMBOL:
    case Builtin::OUTSYMBOL:
        minArgs = 2;
        maxArgs = 3;
        res = mdl->getType(Type::NoType);
        break;
    case Builtin::OUTTERMINATOR:
        minArgs = 0;
        maxArgs = 1;
        res = mdl->getType(Type::NoType);
        break;
    case Builtin::STOP:
        minArgs = maxArgs = 0;
        res = mdl->getType(Type::NoType);
        break;
    case Builtin::FAULT:
        minArgs = 1;
        maxArgs = 2;
        res = mdl->getType(Type::NoType);
        break;
    case Builtin::MAXREAL:
    case Builtin::MINREAL:
    case Builtin::EPSILON:
        minArgs = maxArgs = 0;
        res = mdl->getType(Type::Real);
        break;
    case Builtin::MAXINT:
        minArgs = maxArgs = 0;
        res = mdl->getType(Type::Integer);
        break;
    default:
        break;
    }

    if( checkArity && minArgs >= 0 && ( n < minArgs || n > maxArgs ) )
        error(pos, QString("%1 expects %2 actual parameters, got %3").
              arg(builtin->name.constData()).arg(maxArgs).arg(n));

    // the input procedures deliver their result through a name parameter
    if( ( builtin->id == Builtin::ININTEGER || builtin->id == Builtin::INREAL ||
          builtin->id == Builtin::INSYMBOL || builtin->id == Builtin::INCHAR ) && args ) {
        Expression* v = args;
        while( v->next )
            v = v->next; // the last actual is the variable receiving the value
        Declaration* d = 0;
        if( v->kind == Expression::DeclRef )
            d = v->d;
        else if( v->kind == Expression::Subscript && v->lhs && v->lhs->kind == Expression::DeclRef )
            d = v->lhs->d;
        if( d == 0 )
            error(v->pos, "a variable is required here");
        else
            d->assigned = true;
    }

    if( call )
        call->setType(res);
}

Type* Validator::resultType(int op, Type* lhs, Type* rhs, Expression* e)
{
    if( lhs == 0 || rhs == 0 )
        return 0;

    switch( op )
    {
    case Expression::Add:
    case Expression::Sub:
    case Expression::Mul:
        if( !lhs->isArithmetic() || !rhs->isArithmetic() )
            return 0;
        return ( lhs->kind == Type::Integer && rhs->kind == Type::Integer ) ?
                    mdl->getType(Type::Integer) : mdl->getType(Type::Real);

    case Expression::Div:
        if( !lhs->isArithmetic() || !rhs->isArithmetic() )
            return 0;
        return mdl->getType(Type::Real); // '/' always yields a real value

    case Expression::IntDiv:
    case Expression::Mod:
        if( lhs->kind != Type::Integer || rhs->kind != Type::Integer )
            return 0;
        return mdl->getType(Type::Integer);

    case Expression::Power:
        if( !lhs->isArithmetic() || !rhs->isArithmetic() )
            return 0;
        // integer only if the base is an integer and the exponent a positive integer literal
        if( lhs->kind == Type::Integer && e && e->rhs &&
                e->rhs->kind == Expression::UnsignedConst && e->rhs->u > 0 )
            return mdl->getType(Type::Integer);
        return mdl->getType(Type::Real);

    case Expression::And:
    case Expression::Or:
    case Expression::Imp:
    case Expression::Eqv:
        if( lhs->kind != Type::Boolean || rhs->kind != Type::Boolean )
            return 0;
        return mdl->getType(Type::Boolean);

    case Expression::Eq:
    case Expression::Neq:
        if( lhs->kind == Type::Boolean && rhs->kind == Type::Boolean )
            return mdl->getType(Type::Boolean);
        // fall through
    case Expression::Lt:
    case Expression::Leq:
    case Expression::Gt:
    case Expression::Geq:
        if( !lhs->isArithmetic() || !rhs->isArithmetic() )
            return 0;
        return mdl->getType(Type::Boolean);
    }
    return 0;
}

bool Validator::assigCompat(Type* lhs, Type* rhs, const RowCol& pos)
{
    if( lhs == 0 || rhs == 0 )
        return false;
    if( lhs->isArithmetic() && rhs->isArithmetic() )
        return true; // the value is converted, a real is rounded to an integer
    if( lhs->kind == rhs->kind )
        return true;
    error(pos, "the value is not assignment compatible with the variable");
    return false;
}

Declaration* Validator::resolve(Atom sym) const
{
    for( int i = scopeStack.size() - 1; i >= 0; i-- )
    {
        Declaration* d = AstModel::findInScope(scopeStack[i], sym);
        if( d )
            return d;
    }
    return AstModel::findInScope(mdl->getGlobals(), sym);
}

Declaration* Validator::findLabel(Declaration* scope, Atom sym, bool transparentOnly)
{
    Declaration* d = scope->link;
    bool transparent = true;
    while( d )
    {
        if( d->sym == sym && ( d->kind == Declaration::LabelDecl || d->kind == Declaration::Parameter ) )
            return d;
        if( d->kind != Declaration::Block && d->kind != Declaration::LabelDecl )
            transparent = false;
        d = d->next;
    }
    if( transparentOnly && !transparent )
        return 0;

    // a compound statement is not a block, so its labels belong to the smallest enclosing block
    d = scope->link;
    while( d )
    {
        if( d->kind == Declaration::Block ) {
            Declaration* res = findLabel(d, sym, true);
            if( res )
                return res;
        }
        d = d->next;
    }
    return 0;
}

Declaration* Validator::resolveLabel(Atom sym) const
{
    for( int i = scopeStack.size() - 1; i >= 0; i-- )
    {
        Declaration* d = findLabel(scopeStack[i], sym, false);
        if( d )
            return d;
    }
    return 0;
}

Declaration* Validator::curProc() const
{
    return procStack.isEmpty() ? 0 : procStack.back();
}

void Validator::markEscape(Expression* e)
{
    while( e )
    {
        if( e->kind == Expression::DeclRef )
            markEscape(e->d);
        markEscape(e->lhs);
        markEscape(e->rhs);
        markEscape(e->condition);
        e = e->next;
    }
}

void Validator::markEscape(Declaration* d)
{
    if( d == 0 )
        return;
    switch( d->kind ) {
    case Declaration::Variable:
    case Declaration::Parameter:
    case Declaration::Array:
        d->escapes = true;
        break;
    default:
        break;
    }
}


void Validator::markLabel(Expression* e, bool dynamic)
{
    while( e ) {
        if( e->kind == Expression::DeclRef && e->d && e->d->kind == Declaration::LabelDecl && dynamic )
            e->d->nonlocal = true;
        markLabel(e->lhs, dynamic);
        markLabel(e->rhs, dynamic);
        e = e->next;
    }
}

void Validator::error(const RowCol& pos, const QString& msg)
{
    errors << Error(msg, pos, sourcePath);
}
