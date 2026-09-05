#ifndef __ALG_PARSER2__
#define __ALG_PARSER2__

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

#include <Algol60/AlgToken.h>
#include <Algol60/AlgAst.h>
#include <QList>

namespace Alg {

    typedef QList<Token> TokenList;

    class Scanner {
    public:
        virtual Token next() = 0;
        virtual Token peek(int offset) = 0;
        virtual QString source() const = 0;
    };

    class Parser2 {
    public:
        Parser2(Scanner* s, AstModel* m):scanner(s),mdl(m),thisMod(0) {}
        ~Parser2();

        Declaration* RunParser(); // the returned module is owned by the parser
        Declaration* takeResult(); // take ownership of the module

        struct Error {
            QString msg;
            RowCol pos;
            QString path;
            Error( const QString& m, const RowCol& rc, const QString& p):msg(m),pos(rc),path(p){}
        };
        QList<Error> errors;
    protected:
        Declaration* module();
        Statement* program();
        void declarations_();
        Statement* compoundBlock_();
        Statement* statementList_();
        Statement* compound_tail();
        void declaration();
        void type_declaration();
        Type* local_or_own_type(bool& isOwn);
        Type* type();
        void type_list(Type* t, bool isOwn);
        void array_declaration();
        void array_list(Type* elemType, bool isOwn);
        void array_segment(Type* elemType, bool isOwn);
        void bound_pair_list(QList<Expression*>& bounds);
        void bound_pair(QList<Expression*>& bounds);
        Expression* upper_bound();
        Expression* lower_bound();
        Declaration* switch_declaration();
        Token switch_identifier();
        void switch_list(Declaration* switchDecl);
        void procedure_declaration();
        Declaration* procedure_heading(Type* retType);
        Token procedure_identifier();
        void formal_parameter_part(Declaration* procDecl);
        void formal_parameter_list(Declaration* procDecl);
        void formal_parameter(Declaration* procDecl);
        void value_part(Declaration* procDecl);
        void specification_part(Declaration* procDecl);
        Type* specifier(bool& isArray, bool& isProcedure);
        TokenList identifier_list();
        Statement* procedure_body();
        Statement* statement();
        Statement* unconditional_statement();
        Statement* basic_statement();
        Token label();
        Statement* unlabelled_basic_statement();
        Statement* procedureOrAssignmentStmt_();
        Statement* go_to_statement();
        void actual_parameter_list(QList<Expression*>& args);
        void parameter_delimiter();
        Expression* actual_parameter();
        Statement* conditional_statement();
        Expression* if_clause();
        Statement* for_statement();
        void for_clause(Expression*& var, QList<Expression*>& list);
        void for_list(QList<Expression*>& list);
        Expression* for_list_element();
        Expression* expression();
        Expression* arithmetic_expression();
        Expression* simple_arithmetic_expression();
        Expression::Kind adding_operator();
        Expression* term();
        Expression::Kind multiplying_operator();
        Expression* factor();
        Expression::Kind power_sym_();
        Expression* primary();
        Expression* designational_expression();
        Expression* simple_designational_expression();
        Expression* Boolean_expression();
        Expression* simple_Boolean();
        Expression::Kind equiv_sym_();
        Expression* implication();
        Expression::Kind impl_sym_();
        Expression* Boolean_term();
        Expression::Kind or_sym_();
        Expression* Boolean_factor();
        Expression::Kind and_sym_();
        Expression* Boolean_secondary();
        Expression::Kind not_sym_();
        Expression* Boolean_primary();
        Expression* relation();
        Expression::Kind relational_operator();
        Expression* variableOrFunction_();
        Expression* variable();
        Declaration* simple_variable(Type* t, bool isOwn);
        Token variable_identifier();
        void subscript_list(QList<Expression*>& subs);
        Expression* subscript_expression();
        Expression* unsigned_number();
        Token letter_string();
        Expression* logical_value();
    protected:
        Token cur;
        Token la;
        Scanner* scanner;
        AstModel* mdl;
        Declaration* thisMod;
        void next();
        Token peek(int off);
        void invalid(const char* what);
        bool expect(int tt, bool pkw, const char* where);
        void error(const Token&, const QString& msg);
        RowCol toRowCol(const Token&) const;
        Declaration* addLabel(const Token&);
        Declaration* findParam(Declaration* procDecl, const Token& id);
        Declaration* declInScope(Declaration* scope, const Token& id, Type* t, bool isArray, bool isProcedure);
    };
}
#endif // include
