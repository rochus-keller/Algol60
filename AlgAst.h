#ifndef ALGAST_H
#define ALGAST_H

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

#include <QByteArray>
#include <QList>
#include <QHash>
#include <Algol60/AlgRowCol.h>

class QTextStream;

namespace Alg
{
    enum AlgolVersion { Alg60, Alg60Mod }; // Revised Report 1963, Modified Report 1976

    class Declaration;
    class Type;
    class Statement;
    class Expression;

    typedef const char* Atom;
    typedef QList<Declaration*> DeclList;

    struct Builtin
    {
        enum Kind {
            // standard functions of the Revised Report 3.2.4
            ABS, SIGN, SQRT, SIN, COS, ARCTAN, LN, EXP, ENTIER,
            // environmental procedures of the Modified Report 5
            ININTEGER, OUTINTEGER, INREAL, OUTREAL, INSYMBOL, OUTSYMBOL,
            OUTSTRING, LENGTH, STOP, FAULT, MAXREAL, MINREAL, MAXINT, EPSILON,
            Max
        };
        static const char* name[];
    };

    class Node
    {
    public:
        enum Meta { T, D, E, S }; // Type, Declaration, Expression, Statement

#ifndef _DEBUG
        uint kind : 6; // the Kind enum of the subclass
#endif
        uint meta : 3;
        uint ownstype : 1;
        uint owned : 1;
        uint validated : 1;
        uint hasErrors : 1;

        // Declaration
        uint mode : 2; // ParamMode
        uint isOwn : 1; // own variable or array
        uint isSpec : 1; // formal parameter with explicit specification
        uint escapes : 1; // local used by a thunk or an inner procedure, thus frame lifting required
        uint nonlocal : 1; // label used as target of a goto from an inner block instance
        uint id : 16;  // Builtin::Kind

        // Type
        uint ownsexpr : 1;

        RowCol pos;

        Type* getType() const { return type; }
        void setType(Type*);

        Node(Meta m);
        virtual ~Node();
    protected:
        Type* type;
    };

    class Type : public Node
    {
    public:
        enum Kind {
            Undefined, NoType,
            Integer, Real, Boolean, // value types
            Label, // label specifier and designational expression
            String, // string specifier and string literal
            MaxBasicType,
            Array, Procedure, Switch
        };
        static const char* name[];
#ifdef _DEBUG
        Kind kind;
#endif

        DeclList subs; // Procedure: the formal parameters, owned

        bool isArithmetic() const { return kind == Integer || kind == Real; }
        bool isBasic() const { return kind > NoType && kind < MaxBasicType; }

        Type(Kind k = Undefined);
        ~Type();

        void setExpr(Expression* e); // Array bounds, lower and upper alternating, owned
        Expression* getExpr() const { return expr; }
        int getDims() const; // number of dimensions of an Array
        Declaration* findSub(Atom sym) const;
    private:
        Expression* expr;
    };

    class Declaration : public Node
    {
    public:
        enum Kind {
            Invalid, Module, Program, Procedure, Block, Variable, Array, Switch,
            Parameter, LabelDecl, Builtin
        };
        enum ParamMode { ModeDefault, ModeValue, ModeName }; // Algol 60 default is by name
#ifdef _DEBUG
        Kind kind;
#endif

        QByteArray name;
        Atom sym; // the internalized version of name

        Declaration* link; // Locals
        Declaration* next; // Next in scope
        Declaration* outer; // Parent scope
        Statement*   body; // Procedure, Program
        union {
            // Switch
            Expression* list; // chain of designational expressions

            // Module
            QString* path; // source path
        };

        Declaration(Kind k = Invalid);

        Declaration* find(const char* id, bool recursive = true) const;
        Declaration* getModule();
        const char* getKindName() const;

        void appendMember(Declaration* d);
        static void deleteAll(Declaration* d);
    private:
        ~Declaration();
    };

    class Expression : public Node
    {
    public:
        enum Kind {
            Invalid,
            Neg,
            Add, Sub, Mul, Div, IntDiv, Mod, Power,
            And, Or, Not, Imp, Eqv,
            Eq, Neq, Lt, Leq, Gt, Geq,
            Identifier, DeclRef, Subscript, Call,
            StringConst, UnsignedConst, RealConst, BoolConst,
            IfExpr, // condition, lhs=then, rhs=else
            // Helper
            StepUntil, // lhs=start, rhs=step, condition=until
            WhileLoop, // lhs=expression, condition=cond
            MAX
        };
        static const char* name[];
#ifdef _DEBUG
        Kind kind;
#endif

        union {
            quint64 u;
            double r;
            Atom a; // Identifier, StringConst
            Declaration* d; // DeclRef
        };
        Expression* lhs;
        Expression* rhs;
        Expression* next; // for lists: args, subscripts, switch list, for list, left part list
        Expression* condition; // IfExpr, StepUntil, WhileLoop

        Expression(Kind k = Invalid, const RowCol& rc = RowCol());
        ~Expression();

        static void append(Expression* list, Expression* elem);
        static Expression* toList(const QList<Expression*>&);
    };

    class Statement : public Node
    {
    public:
        enum Kind {
            Invalid, Compound, Block, Assign, Call, If, For, Goto, Label, Dummy
        };
        static const char* name[];
#ifdef _DEBUG
        Kind kind;
#endif

        Statement* next;
        Statement* body;  // Compound, Block, If then part, For do part

        union {
            // Compound / Block
            Declaration* scope;    // block locals, not owned

            // If
            struct {
                Expression* cond; // owned
                Statement* elseStmt; // owned
            };

            // For
            struct {
                Expression* var; // control variable, owned
                Expression* list; // chain of for list elements, owned
            };

            // Assign / Call / Goto
            struct {
                Expression* lhs; // left part list / callee / goto target, owned
                Expression* rhs; // value / arguments, owned
            };

            // Label
            Declaration* label; // not owned
        };

        Statement(Kind k = Invalid, const RowCol& p = RowCol());
        ~Statement();

        Declaration* getScope() const;

        void append(Statement* s);
        static void deleteAll(Statement* s);
    };

    class AstModel
    {
    public:
        AstModel(AlgolVersion v = Alg60Mod);
        ~AstModel();

        void openScope(Declaration* scope);
        Declaration* closeScope();
        Declaration* addDecl(const char* id, const QByteArray& name, Declaration::Kind k);
        Declaration* getGlobals() const { return globalScope; }
        Type* getType(Type::Kind k) const;
        AlgolVersion getVersion() const { return version; }
        void setVersion(AlgolVersion v) { version = v; }
        void clear();

        static Declaration* findInScope(Declaration* scope, const char* sym);

        static void dump(QTextStream&, Declaration*);
    private:
        AstModel& operator=(const AstModel& rhs);
        AstModel(const AstModel&);
        void initGlobals();
        void clearGlobals();
        Type* newType(Type::Kind k);
        void initBuiltins();
        AlgolVersion version;
        QList<Declaration*> scopes;
        Declaration* globalScope;
        Type* basicTypes[Type::MaxBasicType];
    };
}

#endif // ALGAST_H
