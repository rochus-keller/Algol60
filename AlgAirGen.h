#ifndef ALGAIRGEN_H
#define ALGAIRGEN_H

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

#include <Algol60/AlgAst.h>
#include <Algol60/AirEmitter.h>
#include <QHash>
#include <QSet>

namespace Alg
{
    // generates AIR for a validated Algol 60 module

    class AirGen
    {
    public:
        AirGen(AstModel* mdl);
        ~AirGen();

        bool generate(Declaration* module, Air::AbstractRenderer* out);

        static void generateRuntime(Air::AbstractRenderer* out); // the Algol60Rt interface

        struct Error {
            QString msg;
            Alg::RowCol pos;
            QString path;
            Error(const QString& m, const Alg::RowCol& rc, const QString& p)
                : msg(m), pos(rc), path(p) {}
        };
        QList<Error> errors;

    protected:
        struct Frame // the activation record of a program or procedure
        {
            Declaration* decl;
            Frame* outer;
            QByteArray proc; // name of the AIR procedure
            QByteArray rec; // name of the frame record
            QByteArray ref; // name of the pointer to the frame record
            DeclList vars; // the members of the frame record, in order
            DeclList labels; // the labels reachable by a dynamic or non-local goto
            bool dispatch; // the body runs in a protected call with label dispatch
            QSet<QByteArray> members; // field and bound procedure names of the frame record
            Frame():decl(0),outer(0),dispatch(false) {}
        };

        struct Thunk // the getter and setter of an actual parameter called by name
        {
            Expression* actual;
            Frame* frame;
            QByteArray getter, setter;
            Type* type;
            Thunk():actual(0),frame(0),type(0) {}
        };

        // collecting
        Frame* collect(Declaration* proc, Frame* outer);
        void collectVars(Frame*, Declaration* scope);
        void collectProcs(Frame*, Declaration* scope);
        int formalDims(Declaration* param);
        void scanDims(Statement*, Declaration* param, QSet<int>& dims);
        void scanDims(Expression*, Declaration* param, QSet<int>& dims);

        // declarations
        void auxTypes();
        void procTypes();
        void frameTypes();
        void ownVariables();
        void emitProc(Frame*);
        void emitBody(Frame*);
        void emitThunks(Frame*);
        void emitSwitches(Frame*);
        void collectThunks(Frame*, Declaration* scope);
        void collectThunks(Frame*, Statement*);
        void collectThunks(Frame*, Expression*);
        void thunksOfCall(Frame*, Declaration* proc, Expression* args);
        static Declaration* calleeOf(Declaration* proc); // the procedure a formal procedure stands for

        // statements
        void statSeq(Statement*);
        void stat(Statement*);
        void blockStat(Statement*);
        void assignStat(Statement*);
        void callStat(Statement*);
        void ifStat(Statement*);
        void forStat(Statement*);
        void forElem(Statement*, Expression* el, Type* vt);
        void forList(Statement*, Type* vt);
        void forNext(int n, int idx, int fresh);
        void forStep(Statement*, Type* vt, int st);
        void forDone(Statement*, Expression* el, Type* vt, int st);
        void gotoStat(Statement*);
        void blockEntry(Declaration* scope);
        void allocArray(Declaration* array);

        // expressions
        void expr(Expression*, Type* target = 0);
        void binaryOp(Expression*);
        void declRef(Expression*);
        void subscript(Expression*, bool store);
        void call(Expression* call, Declaration* proc, Expression* args, Type* target);
        void builtin(Expression* call, Declaration* b, Expression* args);
        static bool builtinIsFunc(int id);
        Expression* channel(Expression* args, int vals); // pushes the channel number
        static QByteArray runtimeName(Declaration* builtin);
        void store(Expression* lhs, Expression* rhs, int tmp = -1, Type* tmpType = 0,
                   bool tmpIsArg = false);
        void loadTmp(int tmp, bool isArg);
        int args(Declaration* proc, Expression* args, const Alg::RowCol&);
        void conv(Type* from, Type* to);
        void arrayRef(Expression* subscript); // pushes the data array and the linear index
        void descriptor(Declaration* array); // pushes the descriptor pointer

        // designational expressions
        void designator(Expression*);  // emits the goto
        void transfer(Expression*);  // pushes a Transfer instance
        void jump(Declaration* label);
        void formalLabel(Declaration*); // pushes the Transfer instance of a formal label
        void thunkRef(Expression* actual); // pushes the methref of the thunk of an actual

        // helpers
        void framePtr(Frame*);
        Frame* frameOfDecl(Declaration*) const;
        QByteArray fieldOf(Declaration*) const;
        Air::Trident field(Declaration*) const;
        QByteArray typeName(Type*, Declaration* d = 0);
        Air::Quali typeRef(Type*, Declaration* d = 0);
        QByteArray descName(Type* elem, int dims);
        QByteArray arrName(Type* elem);
        static bool isDesignator(Expression*); // a variable or subscripted variable
        static QByteArray basicName(Type*);
        static Air::EmiTypes::Basic basicType(Type*);
        QByteArray unique(const QByteArray& name);
        QByteArray member(Frame* outer, const QByteArray& name);
        static QByteArray sanitize(const QByteArray&);
        static QByteArray stringValue(const QByteArray& literal);
        int localOf(Type*);
        QByteArray tmpName();
        void error(const Alg::RowCol& pos, const QString& msg);
        static Air::Quali q(const QByteArray& name);
        static Air::Quali rt(const QByteArray& name);

    private:
        AstModel* mdl;
        Air::Emitter* e;
        Declaration* module;
        QByteArray moduleName;
        QString sourcePath;
        QList<Frame*> frames;
        QHash<Declaration*,Frame*> frameOfProc;
        QHash<Declaration*,QByteArray> fields; // variable -> frame field or module variable
        QHash<Declaration*,int> dimsOfFormal;
        QHash<Declaration*,QByteArray> switchProc;
        QHash<Declaration*,QByteArray> procType; // procedure -> bound proc type of a methref to it
        QHash<Declaration*,QByteArray> labelName;
        QList<Thunk> thunks;
        QHash<Expression*,int> thunkOfActual;
        QSet<QByteArray> names;
        QSet<QByteArray> arrays; // the emitted array types
        QSet<QByteArray> descs; // the emitted descriptor records
        DeclList ownVars;
        Frame* curFrame;
        int frameBase; // local or argument holding the frame pointer
        bool frameIsArg;
        int maxDims;
        int tmpCount;
    };
}

#endif // ALGAIRGEN_H
