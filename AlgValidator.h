#ifndef ALGVALIDATOR_H
#define ALGVALIDATOR_H

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
#include <QList>
#include <QSet>
#include <QHash>

namespace Alg
{
    class Validator
    {
    public:
        Validator(AstModel* mdl);

        bool validate(Declaration* module);

        struct Error {
            QString msg;
            RowCol pos;
            QString path;
            Error(const QString& m, const RowCol& rc, const QString& p)
                : msg(m), pos(rc), path(p) {}
        };
        QList<Error> errors;

        static DeclList params(Declaration* proc);
        static Declaration* declProc(Declaration* d);
        static bool sigCompat(Declaration* lhs, Declaration* rhs); // procedures with matching formals

    protected:
        void Signatures(Declaration* scope);
        void ProcSignature(Declaration* proc);
        void InferParam(Declaration* proc, Declaration* param);
        void ScanUses(Statement* s, Declaration* param, int& uses);
        void ScanUses(Expression* e, Declaration* param, int& uses);

        void ProgramDecl(Declaration* prog);
        void DeclSeq(Declaration* d);
        void ProcDecl(Declaration* d);
        void ArrayDecl(Declaration* d);
        void SwitchDecl(Declaration* d);

        void StatSeq(Statement* s);
        void Stat(Statement* s);
        void BlockStat(Statement* s);
        void AssignStat(Statement* s);
        void CallStat(Statement* s);
        void IfStat(Statement* s);
        void ForStat(Statement* s);
        void GotoStat(Statement* s);

        bool Expr(Expression* e);
        bool BinaryOp(Expression* e);
        bool UnaryOp(Expression* e);
        bool Identifier(Expression* e);
        bool SubscriptExpr(Expression* e);
        bool CallExpr(Expression* e);
        bool IfExpr(Expression* e);
        bool Designator(Expression* e); // designational expression

        void Args(Declaration* proc, Expression* args, const RowCol& pos);
        void ProcActual(Declaration* formal, Expression* actual);
        void ResolveProcActuals();
        void ProcActualsError(Declaration* formal, const DeclList& candidates,
                              const QHash<Declaration*,DeclList>& resolved);
        void FormalCallsError();
        static Declaration* uniqueProc(const DeclList& candidates);

        void BuiltinCall(Expression* call, Declaration* builtin, Expression* args,
                         const RowCol& pos, bool checkArity = true);
        Type* resultType(int op, Type* lhs, Type* rhs, Expression* e);
        bool assigCompat(Type* lhs, Type* rhs, const RowCol& pos);

        Declaration* resolve(Atom sym) const;
        Declaration* resolveLabel(Atom sym) const;
        static Declaration* findLabel(Declaration* scope, Atom sym, bool transparentOnly);
        Declaration* curProc() const;
        void markEscape(Expression* e);
        void markEscape(Declaration* d);
        void markLabel(Expression* e, bool dynamic);
        void error(const RowCol& pos, const QString& msg);

    private:
        AstModel* mdl;
        Declaration* module;
        QString sourcePath;
        QList<Declaration*> scopeStack; // Program, Procedure and Block scopes
        QList<Declaration*> procStack; // Program and Procedure scopes only
        QSet<Declaration*> visited;
        QHash<Declaration*,DeclList> procActuals; // formal procedure -> the actuals passed to it
        struct FormalCall {
            Declaration* formal;
            Expression* args;
            RowCol pos;
            FormalCall(Declaration* f = 0, Expression* a = 0, const RowCol& p = RowCol())
                : formal(f), args(a), pos(p) {}
        };
        QList<FormalCall> formalCalls; // calls of a formal procedure
        QList< QPair<Declaration*,Expression*> > procActualSites; // where an actual was passed
    };
}

#endif // ALGVALIDATOR_H
