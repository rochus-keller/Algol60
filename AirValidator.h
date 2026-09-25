#ifndef AIRVALIDATOR_H
#define AIRVALIDATOR_H

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

#include <QSet>
#include <Algol60/AirAst.h>
#include <Algol60/AirRenderer.h>

namespace Air
{
    class Validator
    {
    public:
        typedef AbstractRenderer::Error Error;

        Validator(AstModel*);

        bool validate(Declaration* module);

        QList<Error> errors;

    protected:
        struct Val
        {
            Type* t; // the static type, zero if unknown
            quint8 st; // the stack type
            Val():t(0),st(ST_void) {}
            Val(Type* t, quint8 st):t(t),st(st) {}
        };

        void validateDecl(Declaration*);
        void validateType(Type*, Declaration* owner);
        void validateProc(Declaration*);
        void validateBody(Statement*, int loopLevel);
        void validateStat(Statement*&, int loopLevel);
        void validateExprs(Expression*);
        void validateExpr(Expression*);

        Val pop(const char* op);
        void push(Type*);
        void push(const Val&);
        Val top(int i = 0) const;
        void clear();

        bool sameStackType(const Val& a, const Val& b, const char* op);
        bool expectNumeric(const Val&, const char* op);
        bool expectInteger(const Val&, const char* op);
        bool expectRef(const Val&, const char* op);
        bool expectInt32(const Val&, const char* op);
        bool assignable(Type* target, const Val&, const char* op);
        void popArgs(const DeclList& params, const char* op);

        Type* typeOf(Declaration*) const;
        Val valOf(Type*) const;
        void error(const QString&);
        void error(const QString&, const Alg::RowCol&);

    private:
        AstModel* mdl;
        Declaration* module;
        Declaration* curProc;
        QList<Val> stack;
        QSet<QByteArray> labels, gotos;
        bool suppress; // the current instruction refers to a declaration of a stub module
        bool unknownStack; // a call of unknown arity was seen, stack checks are off
        Alg::RowCol curPos;
    };
}

#endif // AIRVALIDATOR_H
