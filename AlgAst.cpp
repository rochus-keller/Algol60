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

// adopted from Simula project

#include <Algol60/AlgAst.h>
#include <Algol60/AlgLexer.h>
#include <QTextStream>
#include <QtDebug>
using namespace Alg;

const char* Builtin::name[] = {
    "ABS", "SIGN", "SQRT", "SIN", "COS", "ARCTAN", "LN", "EXP", "ENTIER",
    "ININTEGER", "OUTINTEGER", "INREAL", "OUTREAL", "INSYMBOL", "OUTSYMBOL",
    "OUTSTRING", "LENGTH", "STOP", "FAULT", "MAXREAL", "MINREAL", "MAXINT", "EPSILON"
};

const char* Type::name[] = {
    "Undefined", "NoType",
    "INTEGER", "REAL", "BOOLEAN", "LABEL", "STRING",
    "",
    "Array", "Procedure", "Switch"
};

const char* Expression::name[] = {
    "Invalid",
    "Neg",
    "Add", "Sub", "Mul", "Div", "IntDiv", "Mod", "Power",
    "And", "Or", "Not", "Imp", "Eqv",
    "Eq", "Neq", "Lt", "Leq", "Gt", "Geq",
    "Identifier", "DeclRef", "Subscript", "Call",
    "StringConst", "UnsignedConst", "RealConst", "BoolConst",
    "IfExpr",
    "StepUntil", "WhileLoop"
};

const char* Statement::name[] = {
    "Invalid", "Compound", "Block", "Assign", "Call", "If", "For", "Goto", "Label", "Dummy"
};

Node::Node(Meta m) :
#ifndef _DEBUG
    kind(0),
#endif
    meta(m), ownstype(0), owned(0), validated(0), hasErrors(0),
    mode(0), isOwn(0), isSpec(0), escapes(0), nonlocal(0), id(0), ownsexpr(0), type(0)
{
}

Node::~Node()
{
    if( type && ownstype )
        delete type;
}

void Node::setType(Type* t)
{
    if( type == t )
        return;
    if( type && ownstype ) {
        delete type;
        ownstype = false;
    }
    type = t;
    if( t && !t->owned ) {
        ownstype = true;
        t->owned = true;
    }
}

Type::Type(Kind k) : Node(T), expr(0)
{
    kind = k;
}

Type::~Type()
{
    if( expr && ownsexpr )
        delete expr;
    foreach( Declaration* d, subs )
        Declaration::deleteAll(d);
}

void Type::setExpr(Expression* e)
{
    if( expr == e )
        return;
    if( expr && ownsexpr ) {
        delete expr;
        ownsexpr = false;
    }
    expr = e;
    if( e && !e->owned ) {
        ownsexpr = true;
        e->owned = true;
    }
}

int Type::getDims() const
{
    // an Array type has a lower and an upper bound expression per dimension
    int n = 0;
    Expression* e = expr;
    while( e ) {
        n++;
        e = e->next;
    }
    return n / 2;
}

Declaration* Type::findSub(Atom sym) const
{
    foreach( Declaration* d, subs )
        if( d->sym == sym )
            return d;
    return 0;
}

Declaration::Declaration(Kind k) : Node(D), sym(0), link(0), next(0), outer(0), body(0), list(0)
{
    kind = k;
}

Declaration::~Declaration()
{
    if( link )
        deleteAll(link);
    if( body )
        Statement::deleteAll(body);

    switch( kind ) {
    case Switch:
        if( list )
            delete list;
        break;
    case Module:
        if( path )
            delete path;
        break;
    default:
        break;
    }
}

Declaration* Declaration::find(const char* id, bool recursive) const
{
    Declaration* d = link;
    while( d )
    {
        if( d->sym == id )
            return d;
        d = d->next;
    }
    if( recursive && outer )
        return outer->find(id);
    return 0;
}

Declaration* Declaration::getModule()
{
    if( kind == Module )
        return this;
    else if( outer )
        return outer->getModule();
    else
        return 0;
}

const char* Declaration::getKindName() const
{
    switch( kind ) {
    case Declaration::Module:
        return "Module";
    case Declaration::Program:
        return "Program";
    case Declaration::Procedure:
        return "Procedure";
    case Declaration::Block:
        return "BlockDecls"; // never printed
    case Declaration::Variable:
        return "Variable";
    case Declaration::Array:
        return "Array";
    case Declaration::Switch:
        return "Switch";
    case Declaration::Parameter:
        return "Parameter";
    case Declaration::LabelDecl:
        return "Label";
    case Declaration::Builtin:
        return "Builtin";
    default:
        return "Invalid";
    }
}

void Declaration::appendMember(Declaration* d)
{
    if( !link )
        link = d;
    else {
        Declaration* cur = link;
        while( cur->next )
            cur = cur->next;
        cur->next = d;
    }
}

void Declaration::deleteAll(Declaration* d)
{
    while( d ) {
        Declaration* next = d->next;
        delete d;
        d = next;
    }
}

Expression::Expression(Kind k, const RowCol& rc) : Node(E), lhs(0), rhs(0), next(0), condition(0), u(0)
{
    kind = k;
    pos = rc;
}

Expression::~Expression()
{
    if( lhs )
        delete lhs;
    if( rhs )
        delete rhs;
    if( next )
        delete next;
    if( condition )
        delete condition;
}

void Expression::append(Expression* list, Expression* elem)
{
    while( list->next )
        list = list->next;
    list->next = elem;
}

Expression* Expression::toList(const QList<Expression*>& l)
{
    Expression* first = 0;
    Expression* last = 0;
    for( int i = 0; i < l.size(); i++ )
    {
        if( l[i] == 0 )
            continue;
        if( last )
            last->next = l[i];
        else
            first = l[i];
        last = l[i];
    }
    return first;
}

Statement::Statement(Kind k, const RowCol& p) : Node(S), next(0), body(0), lhs(0), rhs(0)
{
    kind = k;
    pos = p;
}

Statement::~Statement()
{
    if( body )
        deleteAll(body);

    // note: 'next' is not deleted here to avoid deep recursion on long blocks,
    // this is done by deleteAll

    switch( kind ) {
    case Compound:
    case Block:
        // no: if( scope ) delete scope; the scope is owned by the enclosing declaration
        break;
    case If:
        if( cond )
            delete cond;
        if( elseStmt )
            deleteAll(elseStmt);
        break;
    case For:
        if( var )
            delete var;
        if( list )
            delete list;
        break;
    case Assign:
    case Call:
    case Goto:
        if( lhs )
            delete lhs;
        if( rhs )
            delete rhs;
        break;
    default:
        break;
    }
}

Declaration* Statement::getScope() const
{
    if( kind == Compound || kind == Block )
        return scope;
    else
        return 0;
}

void Statement::append(Statement* s)
{
    Statement* last = this;
    while( last->next )
        last = last->next;
    last->next = s;
}

void Statement::deleteAll(Statement* s)
{
    while( s ) {
        Statement* next = s->next;
        s->next = 0;
        delete s;
        s = next;
    }
}

AstModel::AstModel(AlgolVersion v) : version(v), globalScope(0)
{
    initGlobals();
}

AstModel::~AstModel()
{
    clearGlobals();
}

void AstModel::openScope(Declaration* scope)
{
    scopes.push_back(scope);
}

Declaration* AstModel::closeScope()
{
    if( scopes.isEmpty() )
        return 0;
    return scopes.takeLast();
}

Declaration* AstModel::addDecl(const char* id, const QByteArray& name, Declaration::Kind k)
{
    Declaration* d = new Declaration(k);
    d->name = name;
    d->sym = id;
    if( !scopes.isEmpty() ) {
        d->outer = scopes.last();
        scopes.last()->appendMember(d);
    }
    return d;
}

Type* AstModel::getType(Type::Kind k) const
{
    if( k < Type::MaxBasicType )
        return basicTypes[k];
    return 0;
}

void AstModel::clear()
{
    clearGlobals();
    initGlobals();
}

Declaration* AstModel::findInScope(Declaration* scope, const char* sym)
{
    if( !scope )
        return 0;

    Declaration* d = scope->link;
    while( d ) {
        if( d->sym == sym )
            return d;
        d = d->next;
    }
    return 0;
}

Type* AstModel::newType(Type::Kind k)
{
    Type* t = new Type(k);
    t->owned = true;
    return t;
}

void AstModel::initBuiltins()
{
    for( int i = 0; i < Builtin::Max; i++ ) {
        const QByteArray name = Builtin::name[i];
        Declaration* d = addDecl(Lexer::toId(name.toLower()), name, Declaration::Builtin);
        d->id = i;
        d->validated = true;
    }
}

void AstModel::initGlobals()
{
    globalScope = new Declaration(Declaration::Invalid);
    openScope(globalScope);

    for( int i = 0; i < Type::MaxBasicType; i++ )
        basicTypes[i] = 0;

    basicTypes[Type::NoType] = newType(Type::NoType);
    basicTypes[Type::Integer] = newType(Type::Integer);
    basicTypes[Type::Real] = newType(Type::Real);
    basicTypes[Type::Boolean] = newType(Type::Boolean);
    basicTypes[Type::Label] = newType(Type::Label);
    basicTypes[Type::String] = newType(Type::String);

    initBuiltins();
}

void AstModel::clearGlobals()
{
    scopes.clear();
    Declaration::deleteAll(globalScope);
    for( int i = 0; i < Type::MaxBasicType; i++ )
        if( basicTypes[i] )
            delete basicTypes[i];
    globalScope = 0;
}

class AstDumper
{
public:
    AstDumper(QTextStream& out) : out(out), indent(0) {}

    void dump(Declaration* d)
    {
        if( d )
            dumpDecl(d);
    }

private:
    QTextStream& out;
    int indent;

    void writeIndent()
    {
        for( int i = 0; i < indent; i++ )
        {
            if( i % 2 == 0 )
                out << "| ";
            else
                out << "  ";
        }
    }

    void title(const char* what)
    {
        writeIndent();
        out << what << ":\n";
    }

    static inline bool hasTrueDecls(Declaration* d)
    {
        while( d )
        {
            if( d->kind != Declaration::Block && d->kind != Declaration::LabelDecl )
                return true;
            d = d->next;
        }
        return false;
    }

    void dumpDecl(Declaration* d)
    {
        while( d ) {
            if( d->kind == Declaration::Block || d->kind == Declaration::LabelDecl )
            {
                // a decl block logically belongs to the block statement pointing to it,
                // a label decl to the label statement
                d = d->next;
                continue;
            }
            writeIndent();
            out << d->getKindName();
            if( !d->name.isEmpty() )
                out << " \"" << d->name << "\"";
            out << " [" << d->pos.d_row << ":" << d->pos.d_col << "]";

            if( d->isOwn )
                out << " own";
            if( d->mode == Declaration::ModeValue )
                out << " value";
            else if( d->kind == Declaration::Parameter )
                out << " name";
            if( d->kind == Declaration::Parameter && !d->isSpec )
                out << " unspecified";
            if( d->kind == Declaration::Module && d->path )
                out << " path=\"" << *d->path << "\"";

            out << "\n";

            if( d->getType() ) {
                indent++;
                dumpType(d->getType());
                indent--;
            }

            if( d->kind == Declaration::Switch && d->list ) {
                indent++;
                title("switch_list");
                indent++;
                dumpExprList(d->list);
                indent -= 2;
            }

            if( d->link && hasTrueDecls(d->link) ) {
                indent++;
                title("locals");
                indent++;
                dumpDecl(d->link);
                indent -= 2;
            }

            if( d->body ) {
                indent++;
                title("body");
                indent++;
                dumpStmt(d->body);
                indent -= 2;
            }

            d = d->next;
        }
    }

    void dumpType(Type* t)
    {
        if( !t )
            return;
        writeIndent();
        out << "type: " << Type::name[t->kind];
        if( t->kind == Type::Array )
            out << " dims=" << t->getDims();
        out << "\n";

        if( t->getType() ) {
            indent++;
            dumpType(t->getType());
            indent--;
        }

        if( t->getExpr() ) {
            indent++;
            title(t->kind == Type::Array ? "bounds" : "expr");
            indent++;
            dumpExprList(t->getExpr());
            indent -= 2;
        }

        if( !t->subs.isEmpty() ) {
            indent++;
            title("formals");
            indent++;
            foreach( Declaration* d, t->subs )
                dumpDecl(d);
            indent -= 2;
        }
    }

    void dumpExprList(Expression* e)
    {
        while( e ) {
            dumpExpr(e);
            e = e->next;
        }
    }

    void dumpExpr(Expression* e)
    {
        if( !e )
            return;

        writeIndent();
        out << Expression::name[e->kind];
        out << " [" << e->pos.d_row << ":" << e->pos.d_col << "]";

        switch( e->kind ) {
        case Expression::Identifier:
            if( e->a )
                out << " \"" << e->a << "\"";
            break;
        case Expression::StringConst:
            if( e->a )
                out << " '" << e->a << "'";
            break;
        case Expression::UnsignedConst:
            out << " " << e->u;
            break;
        case Expression::RealConst:
            out << " " << e->r;
            break;
        case Expression::BoolConst:
            out << " " << ( e->u ? "true" : "false" );
            break;
        case Expression::DeclRef:
            if( e->d )
                out << " -> " << e->d->name;
            break;
        default:
            break;
        }
        out << "\n";

        if( e->condition ) {
            indent++;
            title(e->kind == Expression::StepUntil ? "until" : "condition");
            indent++;
            dumpExpr(e->condition);
            indent -= 2;
        }

        if( e->lhs ) {
            indent++;
            title("lhs");
            indent++;
            dumpExpr(e->lhs);
            indent -= 2;
        }

        if( e->rhs ) {
            indent++;
            title(e->kind == Expression::Call ? "args" :
                                                ( e->kind == Expression::Subscript ? "subscripts" : "rhs" ));
            indent++;
            if( e->kind == Expression::Call || e->kind == Expression::Subscript )
                dumpExprList(e->rhs);
            else
                dumpExpr(e->rhs);
            indent -= 2;
        }
    }

    void dumpStmt(Statement* s)
    {
        while( s ) {
            writeIndent();
            out << Statement::name[s->kind];
            out << " [" << s->pos.d_row << ":" << s->pos.d_col << "]\n";

            switch( s->kind ) {
            case Statement::Compound:
            case Statement::Block:
                if( s->scope && s->scope->link ) {
                    indent++;
                    title("locals");
                    indent++;
                    dumpDecl(s->scope->link);
                    indent -= 2;
                }
                if( s->body ) {
                    indent++;
                    title("body");
                    indent++;
                    dumpStmt(s->body);
                    indent -= 2;
                }
                break;

            case Statement::If:
                if( s->cond ) {
                    indent++;
                    title("cond");
                    indent++;
                    dumpExpr(s->cond);
                    indent -= 2;
                }
                if( s->body ) {
                    indent++;
                    title("then");
                    indent++;
                    dumpStmt(s->body);
                    indent -= 2;
                }
                if( s->elseStmt ) {
                    indent++;
                    title("else");
                    indent++;
                    dumpStmt(s->elseStmt);
                    indent -= 2;
                }
                break;

            case Statement::For:
                if( s->var ) {
                    indent++;
                    title("var");
                    indent++;
                    dumpExpr(s->var);
                    indent -= 2;
                }
                if( s->list ) {
                    indent++;
                    title("for_list");
                    indent++;
                    dumpExprList(s->list);
                    indent -= 2;
                }
                if( s->body ) {
                    indent++;
                    title("do");
                    indent++;
                    dumpStmt(s->body);
                    indent -= 2;
                }
                break;

            case Statement::Assign:
            case Statement::Call:
            case Statement::Goto:
                if( s->lhs ) {
                    indent++;
                    title(s->kind == Statement::Assign ? "left_part_list" : "lhs");
                    indent++;
                    dumpExprList(s->lhs);
                    indent -= 2;
                }
                if( s->rhs ) {
                    indent++;
                    title(s->kind == Statement::Call ? "args" : "rhs");
                    indent++;
                    if( s->kind == Statement::Call )
                        dumpExprList(s->rhs);
                    else
                        dumpExpr(s->rhs);
                    indent -= 2;
                }
                break;

            case Statement::Label:
                indent++;
                writeIndent();
                out << ": " << ( s->label ? s->label->name : QByteArray("?") ) << "\n";
                indent--;
                break;

            default:
                break;
            }

            s = s->next;
        }
    }
};

void AstModel::dump(QTextStream& out, Declaration* d)
{
    AstDumper dumper(out);
    dumper.dump(d);
}
