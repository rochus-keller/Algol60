#ifndef AIRAST_H
#define AIRAST_H

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

// Adapted from MilAst

#include <QByteArray>
#include <QList>
#include <QPair>
#include <QString>
#include <QVariant>
#include <Algol60/AlgRowCol.h>
#include <Algol60/AirOps.h>

namespace Air
{
    class Type;
    class Declaration;
    class Expression;
    class Statement;

    typedef QList<Declaration*> DeclList;
    typedef QPair<QByteArray,QByteArray> Quali; // [module '!'] element

    typedef QPair<qint64,qint64> CaseLabel; // single label if both are equal, else a range
    typedef QList<CaseLabel> CaseLabelList;

    enum StackType { ST_void, ST_int32, ST_int64, ST_float32, ST_float64,
                     ST_ref, ST_procref, ST_methref, ST_task, ST_desig, ST_nil };
    const char* stackTypeName(quint8);

    class Node
    {
    public:
        enum Meta { Inval, T, D, E, S };

        Node(quint8 m):
#ifndef _DEBUG
            kind(0),
#endif
            meta(m),public_(0),typebound(0),varParam(0),anonymous(0),
            extern_(0),foreign_(0),init(0),ownstype(0),owned(0),stub(0),validated(0),hasErrors(0),
            type(0) {}
        virtual ~Node();

#ifndef _DEBUG
        uint kind : 8;
#endif
        uint meta : 3;

        // Declaration
        uint public_ : 1;
        uint varParam : 1; // VAR parameter
        uint extern_ : 1; // procedure without body, resolved by the linker
        uint foreign_ : 1; // procedure of the foreign profile
        uint init : 1; // module initializer
        uint stub : 1; // created for a reference into a module not present in the model
        uint hasErrors : 1; // module
        uint ownstype : 1;

        // Type
        uint anonymous : 1;
        uint owned : 1; // owned by the declaration it is attached to
        uint validated : 1;

        uint typebound : 1; // Type: bound procedure type; Declaration: bound procedure, receiver param

        Alg::RowCol pos;

        void setType(Type*);
        Type* getType() const { return type; }

    protected:
        Type* type;
    };

    class Type : public Node
    {
    public:
        // the basic kinds are numerically equal to EmiTypes::Basic
        enum Kind {
            Undefined,
            BOOL, CHAR,
            INT8, INT16, INT32, INT64,
            UINT8, UINT16, UINT32, UINT64,
            FLOAT32, FLOAT64,
            TASK, ANYREC,
            MaxBasicType,
            Pointer, Proc, Array, Struct, Record, // Proc with typebound set is a bound procedure type
            CStruct, CUnion, CArray, CPointer,
            NameRef
        };
#ifdef _DEBUG
        Kind kind;
#endif
        union {
            quint32 len; // Array, CArray: length, zero for open arrays
            Quali* quali; // NameRef
        };
        QList<Declaration*> subs; // fields of a struct/record, params of a proc type, bound procs of a record; owned
        Declaration* decl; // the type declaration this type belongs to

        Type():Node(T),quali(0),decl(0) {
#ifdef _DEBUG
            kind = Undefined;
#endif
        }
        ~Type();

        bool isBasic() const { return kind > Undefined && kind < MaxBasicType; }
        bool isNumber() const { return kind >= INT8 && kind <= FLOAT64; }
        bool isInteger() const { return kind >= INT8 && kind <= UINT64; }
        bool isUnsigned() const { return kind >= UINT8 && kind <= UINT64; }
        bool isFloat() const { return kind == FLOAT32 || kind == FLOAT64; }
        bool isInt64() const { return kind == INT64 || kind == UINT64; }
        bool isRecord() const { return kind == Record || kind == ANYREC; }
        bool isStructured() const { return kind == Array || kind == Struct || kind == Record; }
        bool isForeign() const { return kind >= CStruct && kind <= CPointer; }
        bool isOpenArray() const { return (kind == Array || kind == CArray) && len == 0; }
        bool isCallable() const { return kind == Proc; }
        quint8 stackType() const; // one of the stack types, see the Expressions chapter

        Type* deref() const; // resolve NameRef and alias chains
        Declaration* findSubByName(const QByteArray&, bool recursive = true) const;
        DeclList getFieldList(bool recursive) const;
        DeclList getParams() const;
        bool extends(const Type* base) const;
        Quali toQuali() const;
    };

    struct ModuleData
    {
        QString source;
        Alg::RowCol end;
    };

    struct ProcedureData
    {
        Alg::RowCol end;
        QByteArray externalName; // EXTERN and FOREIGN: the name in the target, if given
    };

    struct Constant
    {
        enum Kind { Invalid, I, D, S, B }; // integer, double, string, hexstring
        quint8 kind;
        qint64 i;
        double d;
        QByteArray s; // S and B

        Constant():kind(Invalid),i(0),d(0) {}
        QVariant toVariant() const;
        static Constant* fromVariant(const QVariant&);
    };

    class Declaration : public Node
    {
    public:
        enum Kind { NoMode, Module, TypeDecl, ConstDecl, Import,
                    Field, VarDecl, LocalDecl, ParamDecl, Procedure, Placeholder, Max };
#ifdef _DEBUG
        Kind kind;
#endif
        Declaration* next; // next declaration in the enclosing scope, owned
        Declaration* subs; // params and locals of a procedure, owned
        Declaration* outer; // the owning declaration, to reconstruct the qualident
        Statement* body; // Procedure, owned
        QByteArray name;
        union {
            Constant* c; // ConstDecl, owned
            ProcedureData* pd; // Procedure, optionally, owned
            ModuleData* md; // Module, optionally, owned
            Declaration* imported;// Import, owned if it is a stub module; Placeholder: the bound procedure declared here, not owned
        };

        Declaration():Node(D),next(0),subs(0),outer(0),body(0),c(0) {
#ifdef _DEBUG
            kind = NoMode;
#endif
        }
        ~Declaration();

        void appendSub(Declaration*);
        Declaration* findSubByName(const QByteArray&) const;
        DeclList getParams() const;
        DeclList getLocals() const;
        int indexOf(Declaration*) const;
        Declaration* getModule() const;
        ProcedureData* getPd();
        ModuleData* getMd();
        QByteArray toPath() const;
        Quali toQuali() const;
    };

    class Expression : public Node
    {
    public:
        enum Kind { Invalid = op_invalid };
#ifdef _DEBUG
        Op kind;
#endif
        Expression* next; // the next instruction of this expression, owned
        Expression* e; // IIF, IF, THEN, ELSE: the nested expression, owned
        union {
            Declaration* d; // not owned
            quint32 id;
            qint64 i;
            double f;
            Constant* c; // owned
        };

        Expression():Node(E),next(0),e(0),d(0) {
#ifdef _DEBUG
            kind = op_invalid;
#endif
        }
        ~Expression();
        void append(Expression*);
    };

    class Statement : public Node
    {
    public:
        enum Kind { ExprStat = op_MAX };
#ifdef _DEBUG
        Op kind;
#endif
        Statement* next; // the next statement of this sequence, owned
        Statement* body; // IF, ELSE, LOOP, REPEAT, WHILE, SWITCH, CASE: the nested sequence, owned
        Expression* e; // the expression of this statement, owned
        union {
            Declaration* d; // not owned
            quint32 id;
            CaseLabelList* labels; // owned
        };
        QByteArray name; // goto, label

        Statement():Node(S),next(0),body(0),e(0),d(0) {
#ifdef _DEBUG
            kind = op_invalid;
#endif
        }
        ~Statement();
        void append(Statement*);
    };

    class AstModel
    {
    public:
        AstModel();
        ~AstModel();

        void clear();

        bool addModule(Declaration*);
        Declaration* findModuleByName(const QByteArray&) const;
        Declaration* getGlobals() const { return const_cast<Declaration*>(&globals); }
        const DeclList& getModules() const { return modules; }
        Type* getBasicType(quint8) const;
        Declaration* resolve(const Quali&) const;

        static const char* basicTypeName(quint8);
        static quint8 basicTypeKind(const QByteArray&);

    private:
        DeclList modules;
        Declaration globals;
        Type* basicTypes[Type::MaxBasicType];
    };
}

#endif // AIRAST_H
