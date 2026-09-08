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

// generated with EbnfStudio and then AST generation manually added

#include <Algol60/AlgParser2.h>
#include <Algol60/AlgLexer.h>
#include <QFileInfo>
using namespace Alg;

static inline bool FIRST_program(int tt) {
	switch(tt){
	case Tok_ARRAY:
	case Tok_BEGIN:
	case Tok_BOOLEAN:
	case Tok_FOR:
	case Tok_GO:
	case Tok_GOTO:
	case Tok_IF:
	case Tok_INTEGER:
	case Tok_OWN:
	case Tok_PROCEDURE:
	case Tok_REAL:
	case Tok_SWITCH:
	case Tok_Semi:
	case Tok_identifier:
	case Tok_unsigned_integer:
		return true;
	default: return false;
	}
}

static inline bool FIRST_declarations_(int tt) {
	switch(tt){
	case Tok_ARRAY:
	case Tok_BOOLEAN:
	case Tok_INTEGER:
	case Tok_OWN:
	case Tok_PROCEDURE:
	case Tok_REAL:
	case Tok_SWITCH:
		return true;
	default: return false;
	}
}

static inline bool FIRST_compoundBlock_(int tt) {
	return tt == Tok_BEGIN;
}

static inline bool FIRST_statementList_(int tt) {
	switch(tt){
	case Tok_BEGIN:
	case Tok_FOR:
	case Tok_GO:
	case Tok_GOTO:
	case Tok_IF:
	case Tok_Semi:
	case Tok_identifier:
	case Tok_unsigned_integer:
		return true;
	default: return false;
	}
}

static inline bool FIRST_compound_tail(int tt) {
	switch(tt){
	case Tok_BEGIN:
	case Tok_END:
	case Tok_FOR:
	case Tok_GO:
	case Tok_GOTO:
	case Tok_IF:
	case Tok_Semi:
	case Tok_identifier:
	case Tok_unsigned_integer:
		return true;
	default: return false;
	}
}

static inline bool FIRST_declaration(int tt) {
	switch(tt){
	case Tok_ARRAY:
	case Tok_BOOLEAN:
	case Tok_INTEGER:
	case Tok_OWN:
	case Tok_PROCEDURE:
	case Tok_REAL:
	case Tok_SWITCH:
		return true;
	default: return false;
	}
}

static inline bool FIRST_type_declaration(int tt) {
	return tt == Tok_BOOLEAN || tt == Tok_INTEGER || tt == Tok_OWN || tt == Tok_REAL;
}

static inline bool FIRST_local_or_own_type(int tt) {
	return tt == Tok_BOOLEAN || tt == Tok_INTEGER || tt == Tok_OWN || tt == Tok_REAL;
}

static inline bool FIRST_type(int tt) {
	return tt == Tok_BOOLEAN || tt == Tok_INTEGER || tt == Tok_REAL;
}

static inline bool FIRST_type_list(int tt) {
	return tt == Tok_identifier;
}

static inline bool FIRST_array_declaration(int tt) {
	return tt == Tok_ARRAY || tt == Tok_BOOLEAN || tt == Tok_INTEGER || tt == Tok_OWN || tt == Tok_REAL;
}

static inline bool FIRST_array_list(int tt) {
	return tt == Tok_identifier;
}

static inline bool FIRST_array_segment(int tt) {
	return tt == Tok_identifier;
}

static inline bool FIRST_bound_pair_list(int tt) {
	switch(tt){
	case Tok_IF:
	case Tok_Lpar:
	case Tok_Minus:
	case Tok_Plus:
	case Tok_decimal_number:
	case Tok_identifier:
	case Tok_unsigned_integer:
		return true;
	default: return false;
	}
}

static inline bool FIRST_bound_pair(int tt) {
	switch(tt){
	case Tok_IF:
	case Tok_Lpar:
	case Tok_Minus:
	case Tok_Plus:
	case Tok_decimal_number:
	case Tok_identifier:
	case Tok_unsigned_integer:
		return true;
	default: return false;
	}
}

static inline bool FIRST_upper_bound(int tt) {
	switch(tt){
	case Tok_IF:
	case Tok_Lpar:
	case Tok_Minus:
	case Tok_Plus:
	case Tok_decimal_number:
	case Tok_identifier:
	case Tok_unsigned_integer:
		return true;
	default: return false;
	}
}

static inline bool FIRST_lower_bound(int tt) {
	switch(tt){
	case Tok_IF:
	case Tok_Lpar:
	case Tok_Minus:
	case Tok_Plus:
	case Tok_decimal_number:
	case Tok_identifier:
	case Tok_unsigned_integer:
		return true;
	default: return false;
	}
}

static inline bool FIRST_switch_declaration(int tt) {
	return tt == Tok_SWITCH;
}

static inline bool FIRST_switch_identifier(int tt) {
	return tt == Tok_identifier;
}

static inline bool FIRST_switch_list(int tt) {
	return tt == Tok_IF || tt == Tok_Lpar || tt == Tok_decimal_number || tt == Tok_identifier || tt == Tok_unsigned_integer;
}

static inline bool FIRST_procedure_declaration(int tt) {
	return tt == Tok_BOOLEAN || tt == Tok_INTEGER || tt == Tok_PROCEDURE || tt == Tok_REAL;
}

static inline bool FIRST_procedure_heading(int tt) {
	return tt == Tok_identifier;
}

static inline bool FIRST_procedure_identifier(int tt) {
	return tt == Tok_identifier;
}

static inline bool FIRST_formal_parameter_part(int tt) {
	return tt == Tok_Lpar;
}

static inline bool FIRST_formal_parameter_list(int tt) {
	return tt == Tok_identifier;
}

static inline bool FIRST_formal_parameter(int tt) {
	return tt == Tok_identifier;
}

static inline bool FIRST_value_part(int tt) {
	return tt == Tok_VALUE;
}

static inline bool FIRST_specification_part(int tt) {
	switch(tt){
	case Tok_ARRAY:
	case Tok_BOOLEAN:
	case Tok_INTEGER:
	case Tok_LABEL:
	case Tok_PROCEDURE:
	case Tok_REAL:
	case Tok_STRING:
	case Tok_SWITCH:
		return true;
	default: return false;
	}
}

static inline bool FIRST_specifier(int tt) {
	switch(tt){
	case Tok_ARRAY:
	case Tok_BOOLEAN:
	case Tok_INTEGER:
	case Tok_LABEL:
	case Tok_PROCEDURE:
	case Tok_REAL:
	case Tok_STRING:
	case Tok_SWITCH:
		return true;
	default: return false;
	}
}

static inline bool FIRST_identifier_list(int tt) {
	return tt == Tok_identifier;
}

static inline bool FIRST_procedure_body(int tt) {
	switch(tt){
	case Tok_BEGIN:
	case Tok_FOR:
	case Tok_GO:
	case Tok_GOTO:
	case Tok_IF:
	case Tok_identifier:
	case Tok_unsigned_integer:
		return true;
	default: return false;
	}
}

static inline bool FIRST_statement(int tt) {
	switch(tt){
	case Tok_BEGIN:
	case Tok_FOR:
	case Tok_GO:
	case Tok_GOTO:
	case Tok_IF:
	case Tok_identifier:
	case Tok_unsigned_integer:
		return true;
	default: return false;
	}
}

static inline bool FIRST_unconditional_statement(int tt) {
	return tt == Tok_BEGIN || tt == Tok_GO || tt == Tok_GOTO || tt == Tok_identifier;
}

static inline bool FIRST_basic_statement(int tt) {
	return tt == Tok_GO || tt == Tok_GOTO || tt == Tok_identifier;
}

static inline bool FIRST_label(int tt) {
	return tt == Tok_identifier || tt == Tok_unsigned_integer;
}

static inline bool FIRST_unlabelled_basic_statement(int tt) {
	return tt == Tok_GO || tt == Tok_GOTO || tt == Tok_identifier;
}

static inline bool FIRST_procedureOrAssignmentStmt_(int tt) {
	return tt == Tok_identifier;
}

static inline bool FIRST_go_to_statement(int tt) {
	return tt == Tok_GO || tt == Tok_GOTO;
}

static inline bool FIRST_actual_parameter_list(int tt) {
	switch(tt){
	case Tok_Bang:
	case Tok_FALSE:
	case Tok_IF:
	case Tok_Lpar:
	case Tok_Minus:
	case Tok_NOT:
	case Tok_Plus:
	case Tok_TRUE:
	case Tok_Unot:
	case Tok_decimal_number:
	case Tok_identifier:
	case Tok_string:
	case Tok_unsigned_integer:
		return true;
	default: return false;
	}
}

static inline bool FIRST_parameter_delimiter(int tt) {
	return tt == Tok_Comma || tt == Tok_Rpar;
}

static inline bool FIRST_actual_parameter(int tt) {
	switch(tt){
	case Tok_Bang:
	case Tok_FALSE:
	case Tok_IF:
	case Tok_Lpar:
	case Tok_Minus:
	case Tok_NOT:
	case Tok_Plus:
	case Tok_TRUE:
	case Tok_Unot:
	case Tok_decimal_number:
	case Tok_identifier:
	case Tok_string:
	case Tok_unsigned_integer:
		return true;
	default: return false;
	}
}

static inline bool FIRST_conditional_statement(int tt) {
	return tt == Tok_IF;
}

static inline bool FIRST_if_clause(int tt) {
	return tt == Tok_IF;
}

static inline bool FIRST_for_statement(int tt) {
	return tt == Tok_FOR;
}

static inline bool FIRST_for_clause(int tt) {
	return tt == Tok_FOR;
}

static inline bool FIRST_for_list(int tt) {
	switch(tt){
	case Tok_IF:
	case Tok_Lpar:
	case Tok_Minus:
	case Tok_Plus:
	case Tok_decimal_number:
	case Tok_identifier:
	case Tok_unsigned_integer:
		return true;
	default: return false;
	}
}

static inline bool FIRST_for_list_element(int tt) {
	switch(tt){
	case Tok_IF:
	case Tok_Lpar:
	case Tok_Minus:
	case Tok_Plus:
	case Tok_decimal_number:
	case Tok_identifier:
	case Tok_unsigned_integer:
		return true;
	default: return false;
	}
}

static inline bool FIRST_expression(int tt) {
	switch(tt){
	case Tok_Bang:
	case Tok_FALSE:
	case Tok_IF:
	case Tok_Lpar:
	case Tok_Minus:
	case Tok_NOT:
	case Tok_Plus:
	case Tok_TRUE:
	case Tok_Unot:
	case Tok_decimal_number:
	case Tok_identifier:
	case Tok_unsigned_integer:
		return true;
	default: return false;
	}
}

static inline bool FIRST_arithmetic_expression(int tt) {
	switch(tt){
	case Tok_IF:
	case Tok_Lpar:
	case Tok_Minus:
	case Tok_Plus:
	case Tok_decimal_number:
	case Tok_identifier:
	case Tok_unsigned_integer:
		return true;
	default: return false;
	}
}

static inline bool FIRST_simple_arithmetic_expression(int tt) {
	switch(tt){
	case Tok_Lpar:
	case Tok_Minus:
	case Tok_Plus:
	case Tok_decimal_number:
	case Tok_identifier:
	case Tok_unsigned_integer:
		return true;
	default: return false;
	}
}

static inline bool FIRST_adding_operator(int tt) {
	return tt == Tok_Minus || tt == Tok_Plus;
}

static inline bool FIRST_term(int tt) {
	return tt == Tok_Lpar || tt == Tok_decimal_number || tt == Tok_identifier || tt == Tok_unsigned_integer;
}

static inline bool FIRST_multiplying_operator(int tt) {
	switch(tt){
	case Tok_DIV:
	case Tok_MOD:
	case Tok_Percent:
	case Tok_Slash:
	case Tok_Star:
	case Tok_Udiv:
	case Tok_Umul:
		return true;
	default: return false;
	}
}

static inline bool FIRST_factor(int tt) {
	return tt == Tok_Lpar || tt == Tok_decimal_number || tt == Tok_identifier || tt == Tok_unsigned_integer;
}

static inline bool FIRST_power_sym_(int tt) {
	return tt == Tok_2Star || tt == Tok_Hat || tt == Tok_POWER || tt == Tok_Uexp;
}

static inline bool FIRST_primary(int tt) {
	return tt == Tok_Lpar || tt == Tok_decimal_number || tt == Tok_identifier || tt == Tok_unsigned_integer;
}

static inline bool FIRST_designational_expression(int tt) {
	return tt == Tok_IF || tt == Tok_Lpar || tt == Tok_decimal_number || tt == Tok_identifier || tt == Tok_unsigned_integer;
}

static inline bool FIRST_simple_designational_expression(int tt) {
	return tt == Tok_Lpar || tt == Tok_decimal_number || tt == Tok_identifier || tt == Tok_unsigned_integer;
}

static inline bool FIRST_Boolean_expression(int tt) {
	switch(tt){
	case Tok_Bang:
	case Tok_FALSE:
	case Tok_IF:
	case Tok_Lpar:
	case Tok_Minus:
	case Tok_NOT:
	case Tok_Plus:
	case Tok_TRUE:
	case Tok_Unot:
	case Tok_decimal_number:
	case Tok_identifier:
	case Tok_unsigned_integer:
		return true;
	default: return false;
	}
}

static inline bool FIRST_simple_Boolean(int tt) {
	switch(tt){
	case Tok_Bang:
	case Tok_FALSE:
	case Tok_Lpar:
	case Tok_Minus:
	case Tok_NOT:
	case Tok_Plus:
	case Tok_TRUE:
	case Tok_Unot:
	case Tok_decimal_number:
	case Tok_identifier:
	case Tok_unsigned_integer:
		return true;
	default: return false;
	}
}

static inline bool FIRST_equiv_sym_(int tt) {
	return tt == Tok_2Eq || tt == Tok_EQUIV || tt == Tok_Ueq;
}

static inline bool FIRST_implication(int tt) {
	switch(tt){
	case Tok_Bang:
	case Tok_FALSE:
	case Tok_Lpar:
	case Tok_Minus:
	case Tok_NOT:
	case Tok_Plus:
	case Tok_TRUE:
	case Tok_Unot:
	case Tok_decimal_number:
	case Tok_identifier:
	case Tok_unsigned_integer:
		return true;
	default: return false;
	}
}

static inline bool FIRST_impl_sym_(int tt) {
	return tt == Tok_IMPL || tt == Tok_MinusGt || tt == Tok_Uimpl;
}

static inline bool FIRST_Boolean_term(int tt) {
	switch(tt){
	case Tok_Bang:
	case Tok_FALSE:
	case Tok_Lpar:
	case Tok_Minus:
	case Tok_NOT:
	case Tok_Plus:
	case Tok_TRUE:
	case Tok_Unot:
	case Tok_decimal_number:
	case Tok_identifier:
	case Tok_unsigned_integer:
		return true;
	default: return false;
	}
}

static inline bool FIRST_or_sym_(int tt) {
	return tt == Tok_Bar || tt == Tok_OR || tt == Tok_Uor;
}

static inline bool FIRST_Boolean_factor(int tt) {
	switch(tt){
	case Tok_Bang:
	case Tok_FALSE:
	case Tok_Lpar:
	case Tok_Minus:
	case Tok_NOT:
	case Tok_Plus:
	case Tok_TRUE:
	case Tok_Unot:
	case Tok_decimal_number:
	case Tok_identifier:
	case Tok_unsigned_integer:
		return true;
	default: return false;
	}
}

static inline bool FIRST_and_sym_(int tt) {
	return tt == Tok_AND || tt == Tok_Amp || tt == Tok_Uand;
}

static inline bool FIRST_Boolean_secondary(int tt) {
	switch(tt){
	case Tok_Bang:
	case Tok_FALSE:
	case Tok_Lpar:
	case Tok_Minus:
	case Tok_NOT:
	case Tok_Plus:
	case Tok_TRUE:
	case Tok_Unot:
	case Tok_decimal_number:
	case Tok_identifier:
	case Tok_unsigned_integer:
		return true;
	default: return false;
	}
}

static inline bool FIRST_not_sym_(int tt) {
	return tt == Tok_Bang || tt == Tok_NOT || tt == Tok_Unot;
}

static inline bool FIRST_Boolean_primary(int tt) {
	switch(tt){
	case Tok_FALSE:
	case Tok_Lpar:
	case Tok_Minus:
	case Tok_Plus:
	case Tok_TRUE:
	case Tok_decimal_number:
	case Tok_identifier:
	case Tok_unsigned_integer:
		return true;
	default: return false;
	}
}

static inline bool FIRST_relation(int tt) {
	switch(tt){
	case Tok_Lpar:
	case Tok_Minus:
	case Tok_Plus:
	case Tok_decimal_number:
	case Tok_identifier:
	case Tok_unsigned_integer:
		return true;
	default: return false;
	}
}

static inline bool FIRST_relational_operator(int tt) {
	switch(tt){
	case Tok_BangEq:
	case Tok_EQUAL:
	case Tok_Eq:
	case Tok_GREATER:
	case Tok_Geq:
	case Tok_Gt:
	case Tok_HatEq:
	case Tok_LESS:
	case Tok_Leq:
	case Tok_Lt:
	case Tok_LtGt:
	case Tok_NOTEQUAL:
	case Tok_NOTGREATER:
	case Tok_NOTLESS:
	case Tok_Ugeq:
	case Tok_Uleq:
	case Tok_Uneq:
		return true;
	default: return false;
	}
}

static inline bool FIRST_variableOrFunction_(int tt) {
	return tt == Tok_identifier;
}

static inline bool FIRST_variable(int tt) {
	return tt == Tok_identifier;
}

static inline bool FIRST_simple_variable(int tt) {
	return tt == Tok_identifier;
}

static inline bool FIRST_variable_identifier(int tt) {
	return tt == Tok_identifier;
}

static inline bool FIRST_subscript_list(int tt) {
	switch(tt){
	case Tok_IF:
	case Tok_Lpar:
	case Tok_Minus:
	case Tok_Plus:
	case Tok_decimal_number:
	case Tok_identifier:
	case Tok_unsigned_integer:
		return true;
	default: return false;
	}
}

static inline bool FIRST_subscript_expression(int tt) {
	switch(tt){
	case Tok_IF:
	case Tok_Lpar:
	case Tok_Minus:
	case Tok_Plus:
	case Tok_decimal_number:
	case Tok_identifier:
	case Tok_unsigned_integer:
		return true;
	default: return false;
	}
}

static inline bool FIRST_unsigned_number(int tt) {
	return tt == Tok_decimal_number || tt == Tok_unsigned_integer;
}

static inline bool FIRST_letter_string(int tt) {
	return tt == Tok_identifier;
}

static inline bool FIRST_logical_value(int tt) {
	return tt == Tok_FALSE || tt == Tok_TRUE;
}

Parser2::~Parser2()
{
    if( thisMod )
        Declaration::deleteAll(thisMod);
}

Declaration* Parser2::RunParser() {
    errors.clear();
    next();
    return module();
}

Declaration* Parser2::takeResult() {
    Declaration* res = thisMod;
    thisMod = 0;
    return res;
}

void Parser2::next() {
    cur = la;
    la = scanner->next();
    while( la.d_type == Tok_Invalid ) {
        errors << Error(la.d_val, toRowCol(la), la.d_sourcePath);
        la = scanner->next();
    }
}

Token Parser2::peek(int off) {
    if( off == 1 )
        return la;
    else if( off == 0 )
        return cur;
    else
        return scanner->peek(off-1);
}

void Parser2::invalid(const char* what) {
    errors << Error(QString("invalid %1").arg(what), toRowCol(la), la.d_sourcePath);
}

bool Parser2::expect(int tt, bool pkw, const char* where) {
    if( la.d_type == tt || la.d_code == tt) { next(); return true; }
    else { errors << Error(QString("'%1' expected in %2").arg(tokenTypeString(tt)).arg(where),
                           toRowCol(la), la.d_sourcePath); return false; }
}

void Parser2::error(const Token& t, const QString& msg) {
    errors << Error(msg, toRowCol(t), t.d_sourcePath);
}

RowCol Parser2::toRowCol(const Token& t) const {
    return RowCol(t.d_lineNr, t.d_colNr);
}

Declaration* Parser2::addLabel(const Token& t) {
    // a label is either an identifier or an unsigned integer
    const char* sym = t.d_id ? t.d_id : Lexer::toId(t.d_val);
    Declaration* d = mdl->addDecl(sym, t.d_val, Declaration::LabelDecl);
    d->pos = toRowCol(t);
    d->setType(mdl->getType(Type::Label));
    return d;
}

Declaration* Parser2::findParam(Declaration* procDecl, const Token& id) {
    Declaration* param = procDecl->link;
    while( param ) {
        if( param->kind == Declaration::Parameter && param->sym == id.d_id )
            return param;
        param = param->next;
    }
    return 0;
}

Declaration* Parser2::declInScope(Declaration* scope, const Token& id, Type* t, bool isArray, bool isProcedure) {
    Declaration* d = new Declaration(isArray ? Declaration::Array : Declaration::Variable);
    d->name = id.d_val;
    d->sym = id.d_id;
    d->pos = toRowCol(id);
    if( scope ) {
        d->outer = scope;
        scope->appendMember(d);
    }
    if( isArray ) {
        Type* arrType = new Type(Type::Array);
        arrType->setType(t ? t : mdl->getType(Type::Real));
        d->setType(arrType);
    } else if( isProcedure ) {
        Type* procType = new Type(Type::Procedure);
        procType->setType(t ? t : mdl->getType(Type::NoType));
        d->setType(procType);
        d->kind = Declaration::Procedure;
    } else
        d->setType(t);
    return d;
}

static inline void appendStmt(Statement*& first, Statement*& last, Statement* s) {
    if( s == 0 )
        return;
    if( last )
        last->append(s);
    else
        first = s;
    last = s;
    while( last->next )
        last = last->next;
}

static inline bool isLabelAhead(const Token& t1, const Token& t2) {
    return ( t1.d_type == Tok_identifier || t1.d_type == Tok_unsigned_integer ) && t2.d_type == Tok_Colon;
}

Declaration* Parser2::module() {
    Declaration* mod = new Declaration(Declaration::Module);
    if( thisMod )
        Declaration::deleteAll(thisMod);
    thisMod = mod;

    mod->path = new QString(scanner->source());
    mod->name = QFileInfo(*mod->path).baseName().toUtf8();
    mod->sym = Lexer::toId(mod->name);
    mod->pos = toRowCol(la);

    mdl->openScope(mod);

    Declaration* prog = mdl->addDecl(mod->sym, mod->name, Declaration::Program);
    prog->pos = toRowCol(la);
    prog->setType(mdl->getType(Type::NoType));

    mdl->openScope(prog);
    prog->body = program();
    mdl->closeScope();

    mdl->closeScope();

    if( la.d_type != Tok_Eof )
        error(la, "unexpected token after end of program");

    return mod;
}

Statement* Parser2::program() {
    Statement* res = 0;
    Statement* last = 0;

    while( isLabelAhead(peek(1),peek(2)) ) {
        const Token lbl = label();
        expect(Tok_Colon, false, "program");
        Statement* s = new Statement(Statement::Label, toRowCol(lbl));
        s->label = addLabel(lbl);
        appendStmt(res, last, s);
    }
    if( FIRST_declarations_(la.d_type) || FIRST_declarations_(la.d_code) ) {
        declarations_();
    }
    appendStmt(res, last, statementList_());
    return res;
}

void Parser2::declarations_() {
    declaration();
    while( ( ( peek(1).d_type == Tok_Semi && peek(2).d_type == Tok_ARRAY ) || ( peek(1).d_type == Tok_Semi && peek(2).d_code == Tok_BOOLEAN ) || ( peek(1).d_type == Tok_Semi && peek(2).d_code == Tok_INTEGER ) || ( peek(1).d_type == Tok_Semi && peek(2).d_type == Tok_OWN ) || ( peek(1).d_type == Tok_Semi && peek(2).d_type == Tok_PROCEDURE ) || ( peek(1).d_type == Tok_Semi && peek(2).d_code == Tok_REAL ) || ( peek(1).d_type == Tok_Semi && peek(2).d_code == Tok_SWITCH ) )  ) {
        expect(Tok_Semi, false, "declarations_");
        declaration();
    }
    expect(Tok_Semi, false, "declarations_");
}

Statement* Parser2::compoundBlock_() {
    expect(Tok_BEGIN, false, "compoundBlock_");
    const RowCol pos = toRowCol(cur);

    Declaration* blockScope = mdl->addDecl("", "", Declaration::Block);
    blockScope->pos = pos;
    mdl->openScope(blockScope);

    bool hasDecls = false;
    if( FIRST_declarations_(la.d_type) || FIRST_declarations_(la.d_code) ) {
        hasDecls = true;
        declarations_();
    }
    Statement* body = compound_tail();

    mdl->closeScope();

    Statement* blk = new Statement(hasDecls ? Statement::Block : Statement::Compound, pos);
    blk->body = body;
    blk->scope = blockScope;
    return blk;
}

Statement* Parser2::statementList_() {
    Statement* res = 0;
    Statement* last = 0;

    appendStmt(res, last, statement());
    while( la.d_type == Tok_Semi ) {
        expect(Tok_Semi, false, "statementList_");
        appendStmt(res, last, statement());
    }
    return res;
}

Statement* Parser2::compound_tail() {
    Statement* res = statementList_();
    expect(Tok_END, false, "compound_tail");
    return res;
}

void Parser2::declaration() {
    if( FIRST_switch_declaration(la.d_type) || FIRST_switch_declaration(la.d_code) ) {
        switch_declaration();
    } else if( ( ( peek(1).d_type == Tok_PROCEDURE || peek(2).d_type == Tok_PROCEDURE ) )  ) {
        procedure_declaration();
    } else if( ( ( peek(1).d_type == Tok_ARRAY || peek(2).d_type == Tok_ARRAY || peek(3).d_type == Tok_ARRAY ) )  ) {
        array_declaration();
    } else if( FIRST_type_declaration(la.d_type) || FIRST_type_declaration(la.d_code) ) {
        type_declaration();
    } else
        invalid("declaration");
}

void Parser2::type_declaration() {
    bool isOwn = false;
    Type* t = local_or_own_type(isOwn);
    type_list(t, isOwn);
}

Type* Parser2::local_or_own_type(bool& isOwn) {
    isOwn = false;
    if( la.d_type == Tok_OWN ) {
        expect(Tok_OWN, false, "local_or_own_type");
        isOwn = true;
    }
    return type();
}

Type* Parser2::type() {
    if( la.d_code == Tok_REAL ) {
        expect(Tok_REAL, true, "type");
        return mdl->getType(Type::Real);
    } else if( la.d_code == Tok_INTEGER ) {
        expect(Tok_INTEGER, true, "type");
        return mdl->getType(Type::Integer);
    } else if( la.d_code == Tok_BOOLEAN ) {
        expect(Tok_BOOLEAN, true, "type");
        return mdl->getType(Type::Boolean);
    } else {
        invalid("type");
        return 0;
    }
}

void Parser2::type_list(Type* t, bool isOwn) {
    simple_variable(t, isOwn);
    while( la.d_type == Tok_Comma ) {
        expect(Tok_Comma, false, "type_list");
        simple_variable(t, isOwn);
    }
}

void Parser2::array_declaration() {
    bool isOwn = false;
    Type* elemType = 0;
    if( FIRST_local_or_own_type(la.d_type) || FIRST_local_or_own_type(la.d_code) ) {
        elemType = local_or_own_type(isOwn);
    } else
        elemType = mdl->getType(Type::Real); // the default element type is real
    expect(Tok_ARRAY, false, "array_declaration");
    array_list(elemType, isOwn);
}

void Parser2::array_list(Type* elemType, bool isOwn) {
    array_segment(elemType, isOwn);
    while( la.d_type == Tok_Comma ) {
        expect(Tok_Comma, false, "array_list");
        array_segment(elemType, isOwn);
    }
}

void Parser2::array_segment(Type* elemType, bool isOwn) {
    TokenList names;

    expect(Tok_identifier, false, "array_segment");
    names.append(cur);
    while( la.d_type == Tok_Comma ) {
        expect(Tok_Comma, false, "array_segment");
        expect(Tok_identifier, false, "array_segment");
        names.append(cur);
    }
    expect(Tok_Lbrack, false, "array_segment");
    QList<Expression*> bounds;
    bound_pair_list(bounds);
    expect(Tok_Rbrack, false, "array_segment");

    // the bound expressions are evaluated once per segment, thus all arrays of
    // the segment share the same bounds; the first array type owns them
    Expression* boundList = Expression::toList(bounds);

    for( int i = 0; i < names.size(); i++ ) {
        Declaration* arrDecl = mdl->addDecl(names[i].d_id, names[i].d_val, Declaration::Array);
        arrDecl->pos = toRowCol(names[i]);
        arrDecl->isOwn = isOwn;

        Type* arrType = new Type(Type::Array);
        arrType->setType(elemType);
        arrType->setExpr(boundList);
        arrDecl->setType(arrType);
    }
}

void Parser2::bound_pair_list(QList<Expression*>& bounds) {
    bound_pair(bounds);
    while( la.d_type == Tok_Comma ) {
        expect(Tok_Comma, false, "bound_pair_list");
        bound_pair(bounds);
    }
}

void Parser2::bound_pair(QList<Expression*>& bounds) {
    // lower and upper bound of each dimension alternate in the list
    bounds.append(lower_bound());
    expect(Tok_Colon, false, "bound_pair");
    bounds.append(upper_bound());
}

Expression* Parser2::upper_bound() {
    return arithmetic_expression();
}

Expression* Parser2::lower_bound() {
    return arithmetic_expression();
}

Declaration* Parser2::switch_declaration() {
    expect(Tok_SWITCH, true, "switch_declaration");
    const Token name = switch_identifier();
    expect(Tok_ColonEq, false, "switch_declaration");

    Declaration* switchDecl = mdl->addDecl(name.d_id, name.d_val, Declaration::Switch);
    switchDecl->pos = toRowCol(name);

    // a switch is an array of designational expressions, i.e. of labels
    Type* t = new Type(Type::Switch);
    t->setType(mdl->getType(Type::Label));
    switchDecl->setType(t);

    switch_list(switchDecl);

    return switchDecl;
}

Token Parser2::switch_identifier() {
    expect(Tok_identifier, false, "switch_identifier");
    return cur;
}

void Parser2::switch_list(Declaration* switchDecl) {
    QList<Expression*> list;
    list.append(designational_expression());
    while( la.d_type == Tok_Comma ) {
        expect(Tok_Comma, false, "switch_list");
        list.append(designational_expression());
    }
    switchDecl->list = Expression::toList(list);
}

void Parser2::procedure_declaration() {
    Type* retType = 0;
    if( FIRST_type(la.d_type) || FIRST_type(la.d_code) ) {
        retType = type();
    }
    expect(Tok_PROCEDURE, false, "procedure_declaration");
    Declaration* procDecl = procedure_heading(retType);

    mdl->openScope(procDecl);
    procDecl->body = procedure_body();
    mdl->closeScope();
}

Declaration* Parser2::procedure_heading(Type* retType) {
    const Token name = procedure_identifier();

    Declaration* procDecl = mdl->addDecl(name.d_id, name.d_val, Declaration::Procedure);
    procDecl->pos = toRowCol(name);
    procDecl->setType(retType ? retType : mdl->getType(Type::NoType));

    mdl->openScope(procDecl);
    if( FIRST_formal_parameter_part(la.d_type) ) {
        formal_parameter_part(procDecl);
    }
    expect(Tok_Semi, false, "procedure_heading");
    if( FIRST_value_part(la.d_type) ) {
        value_part(procDecl);
    }
    if( FIRST_specification_part(la.d_type) || FIRST_specification_part(la.d_code) ) {
        specification_part(procDecl);
    }
    mdl->closeScope();

    return procDecl;
}

Token Parser2::procedure_identifier() {
    expect(Tok_identifier, false, "procedure_identifier");
    return cur;
}

void Parser2::formal_parameter_part(Declaration* procDecl) {
    expect(Tok_Lpar, false, "formal_parameter_part");
    formal_parameter_list(procDecl);
    expect(Tok_Rpar, false, "formal_parameter_part");
}

void Parser2::formal_parameter_list(Declaration* procDecl) {
    formal_parameter(procDecl);
    while( ( ( peek(1).d_type == Tok_Rpar && peek(2).d_type == Tok_identifier ) || ( peek(1).d_type == Tok_Comma && peek(2).d_type == Tok_identifier ) )  ) {
        parameter_delimiter();
        formal_parameter(procDecl);
    }
}

void Parser2::formal_parameter(Declaration* procDecl) {
    expect(Tok_identifier, false, "formal_parameter");

    Declaration* param = mdl->addDecl(cur.d_id, cur.d_val, Declaration::Parameter);
    param->pos = toRowCol(cur);
    param->outer = procDecl;
    param->isParam = true;
    // the specification part is optional in Algol 60, so the type and the mode
    // of the parameter are not necessarily known here
}

void Parser2::value_part(Declaration* procDecl) {
    expect(Tok_VALUE, false, "value_part");
    const TokenList ids = identifier_list();
    expect(Tok_Semi, false, "value_part");

    for( int i = 0; i < ids.size(); i++ ) {
        Declaration* param = findParam(procDecl, ids[i]);
        if( param )
            param->mode = Declaration::ModeValue;
        else
            error(ids[i], "not a formal parameter of this procedure");
    }
}

void Parser2::specification_part(Declaration* procDecl) {
    while( FIRST_specifier(la.d_type) || FIRST_specifier(la.d_code) ) {
        bool isArray = false;
        bool isProcedure = false;
        Type* t = specifier(isArray, isProcedure);
        const TokenList ids = identifier_list();
        expect(Tok_Semi, false, "specification_part");

        for( int i = 0; i < ids.size(); i++ ) {
            Declaration* param = findParam(procDecl, ids[i]);
            if( param == 0 ) {
                // some implementations accept declarations of the enclosing block
                // interspersed with the specification part; the declaration belongs
                // to the block, not to the procedure
                declInScope(procDecl->outer, ids[i], t, isArray, isProcedure);
                continue;
            }
            if( isArray ) {
                Type* arrType = new Type(Type::Array);
                arrType->setType(t ? t : mdl->getType(Type::Real));
                param->setType(arrType);
                param->kind = Declaration::Array;
            } else if( isProcedure ) {
                Type* procType = new Type(Type::Procedure);
                procType->setType(t ? t : mdl->getType(Type::NoType));
                param->setType(procType);
            } else
                param->setType(t);
            param->isSpec = true;
        }
        if( t && !t->owned )
            delete t;
    }
}

Type* Parser2::specifier(bool& isArray, bool& isProcedure) {
    isArray = false;
    isProcedure = false;

    if( la.d_code == Tok_STRING ) {
        expect(Tok_STRING, true, "specifier");
        return mdl->getType(Type::String);
    } else if( la.d_code == Tok_LABEL ) {
        expect(Tok_LABEL, true, "specifier");
        return mdl->getType(Type::Label);
    } else if( la.d_code == Tok_SWITCH ) {
        expect(Tok_SWITCH, true, "specifier");
        Type* t = new Type(Type::Switch);
        t->setType(mdl->getType(Type::Label));
        return t;
    } else if( ( ( peek(1).d_type == Tok_PROCEDURE || peek(2).d_type == Tok_PROCEDURE || peek(1).d_type == Tok_ARRAY || peek(2).d_type == Tok_ARRAY ) )  ) {
        Type* t = 0;
        if( FIRST_type(la.d_type) || FIRST_type(la.d_code) ) {
            t = type();
        }
        if( la.d_type == Tok_ARRAY ) {
            expect(Tok_ARRAY, false, "specifier");
            isArray = true;
        } else if( la.d_type == Tok_PROCEDURE ) {
            expect(Tok_PROCEDURE, false, "specifier");
            isProcedure = true;
        } else
            invalid("specifier");
        return t;
    } else if( FIRST_type(la.d_type) || FIRST_type(la.d_code) ) {
        return type();
    } else {
        invalid("specifier");
        return 0;
    }
}

TokenList Parser2::identifier_list() {
    TokenList ids;
    expect(Tok_identifier, false, "identifier_list");
    ids.append(cur);
    while( la.d_type == Tok_Comma ) {
        expect(Tok_Comma, false, "identifier_list");
        expect(Tok_identifier, false, "identifier_list");
        ids.append(cur);
    }
    return ids;
}

Statement* Parser2::procedure_body() {
    return statement();
}

Statement* Parser2::statement() {
    Statement* res = 0;
    Statement* last = 0;

    while( isLabelAhead(peek(1),peek(2)) ) {
        const Token lbl = label();
        expect(Tok_Colon, false, "statement");
        Statement* s = new Statement(Statement::Label, toRowCol(lbl));
        s->label = addLabel(lbl);
        appendStmt(res, last, s);
    }
    if( FIRST_unconditional_statement(la.d_type) || FIRST_conditional_statement(la.d_type) || FIRST_for_statement(la.d_type) ) {
        if( FIRST_unconditional_statement(la.d_type) ) {
            appendStmt(res, last, unconditional_statement());
        } else if( FIRST_conditional_statement(la.d_type) ) {
            appendStmt(res, last, conditional_statement());
        } else if( FIRST_for_statement(la.d_type) ) {
            appendStmt(res, last, for_statement());
        } else
            invalid("statement");
    }
    if( res )
        return res;
    else
        return new Statement(Statement::Dummy, toRowCol(la)); // the empty statement
}

Statement* Parser2::unconditional_statement() {
    if( FIRST_basic_statement(la.d_type) ) {
        return basic_statement();
    } else if( FIRST_compoundBlock_(la.d_type) ) {
        return compoundBlock_();
    } else {
        invalid("unconditional_statement");
        return 0;
    }
}

Statement* Parser2::basic_statement() {
    return unlabelled_basic_statement();
}

Token Parser2::label() {
    if( la.d_type == Tok_identifier ) {
        expect(Tok_identifier, false, "label");
    } else if( la.d_type == Tok_unsigned_integer ) {
        expect(Tok_unsigned_integer, false, "label");
    } else
        invalid("label");
    return cur;
}

Statement* Parser2::unlabelled_basic_statement() {
    if( FIRST_procedureOrAssignmentStmt_(la.d_type) ) {
        return procedureOrAssignmentStmt_();
    } else if( FIRST_go_to_statement(la.d_type) ) {
        return go_to_statement();
    } else {
        invalid("unlabelled_basic_statement");
        return 0;
    }
}

Statement* Parser2::procedureOrAssignmentStmt_() {
    expect(Tok_identifier, false, "procedureOrAssignmentStmt_");
    const Token name = cur;
    const RowCol pos = toRowCol(name);

    Expression* des = new Expression(Expression::Identifier, pos);
    des->a = name.d_id;

    if( la.d_type == Tok_Lbrack || la.d_type == Tok_ColonEq || la.d_type == Tok_Lpar ) {
        if( la.d_type == Tok_Lbrack || la.d_type == Tok_ColonEq ) {
            if( la.d_type == Tok_Lbrack ) {
                expect(Tok_Lbrack, false, "procedureOrAssignmentStmt_");
                QList<Expression*> subs;
                subscript_list(subs);
                expect(Tok_Rbrack, false, "procedureOrAssignmentStmt_");
                Expression* sub = new Expression(Expression::Subscript, pos);
                sub->lhs = des;
                sub->rhs = Expression::toList(subs);
                des = sub;
            }
            expect(Tok_ColonEq, false, "procedureOrAssignmentStmt_");

            // the left part list of an assignment is a sequence of variables
            // separated by ':=', the last expression is the value
            QList<Expression*> parts;
            parts.append(des);
            parts.append(expression());
            while( la.d_type == Tok_ColonEq ) {
                expect(Tok_ColonEq, false, "procedureOrAssignmentStmt_");
                parts.append(expression());
            }
            Expression* value = parts.takeLast();

            Statement* stmt = new Statement(Statement::Assign, pos);
            stmt->lhs = Expression::toList(parts);
            stmt->rhs = value;
            return stmt;
        } else if( la.d_type == Tok_Lpar ) {
            expect(Tok_Lpar, false, "procedureOrAssignmentStmt_");
            QList<Expression*> args;
            actual_parameter_list(args);
            expect(Tok_Rpar, false, "procedureOrAssignmentStmt_");

            Statement* stmt = new Statement(Statement::Call, pos);
            stmt->lhs = des;
            stmt->rhs = Expression::toList(args);
            return stmt;
        } else
            invalid("procedureOrAssignmentStmt_");
    }

    Statement* stmt = new Statement(Statement::Call, pos);
    stmt->lhs = des;
    return stmt;
}

Statement* Parser2::go_to_statement() {
    if( la.d_type == Tok_GOTO ) {
        expect(Tok_GOTO, false, "go_to_statement");
    } else if( la.d_type == Tok_GO ) {
        expect(Tok_GO, false, "go_to_statement");
        expect(Tok_TO, false, "go_to_statement");
    } else
        invalid("go_to_statement");

    Statement* stmt = new Statement(Statement::Goto, toRowCol(cur));
    stmt->lhs = designational_expression();
    return stmt;
}

void Parser2::actual_parameter_list(QList<Expression*>& args) {
    args.append(actual_parameter());
    while( ( ( peek(1).d_type == Tok_Comma || ( peek(1).d_type == Tok_Rpar && peek(2).d_type == Tok_identifier && peek(3).d_type == Tok_Colon ) ) )  ) {
        parameter_delimiter();
        args.append(actual_parameter());
    }
}

void Parser2::parameter_delimiter() {
    if( la.d_type == Tok_Comma ) {
        expect(Tok_Comma, false, "parameter_delimiter");
    } else if( la.d_type == Tok_Rpar ) {
        // ')' letter_string ':' '(' is just a comment version of ','
        expect(Tok_Rpar, false, "parameter_delimiter");
        letter_string();
        expect(Tok_Colon, false, "parameter_delimiter");
        expect(Tok_Lpar, false, "parameter_delimiter");
    } else
        invalid("parameter_delimiter");
}

Expression* Parser2::actual_parameter() {
    if( la.d_type == Tok_string ) {
        expect(Tok_string, false, "actual_parameter");
        Expression* res = new Expression(Expression::StringConst, toRowCol(cur));
        res->a = Lexer::toStr(cur.d_val);
        res->setType(mdl->getType(Type::String));
        return res;
    } else if( FIRST_expression(la.d_type) || FIRST_expression(la.d_code) ) {
        return expression();
    } else {
        invalid("actual_parameter");
        return 0;
    }
}

Statement* Parser2::conditional_statement() {
    Expression* cond = if_clause();

    Statement* ifStmt = new Statement(Statement::If, cond ? cond->pos : toRowCol(cur));
    ifStmt->cond = cond;

    Statement* then = 0;
    Statement* last = 0;

    while( isLabelAhead(peek(1),peek(2)) ) {
        const Token lbl = label();
        expect(Tok_Colon, false, "conditional_statement");
        Statement* s = new Statement(Statement::Label, toRowCol(lbl));
        s->label = addLabel(lbl);
        appendStmt(then, last, s);
    }
    if( FIRST_unconditional_statement(la.d_type) || la.d_type == Tok_ELSE || la.d_type == Tok_END || la.d_type == Tok_Semi ) {
        if( FIRST_unconditional_statement(la.d_type) ) {
            appendStmt(then, last, unconditional_statement());
        }
        if( la.d_type == Tok_ELSE ) {
            expect(Tok_ELSE, false, "conditional_statement");
            ifStmt->elseStmt = statement();
        }
    } else if( FIRST_for_statement(la.d_type) ) {
        appendStmt(then, last, for_statement());
    } else
        invalid("conditional_statement");

    ifStmt->body = then;
    return ifStmt;
}

Expression* Parser2::if_clause() {
    expect(Tok_IF, false, "if_clause");
    Expression* res = Boolean_expression();
    expect(Tok_THEN, false, "if_clause");
    return res;
}

Statement* Parser2::for_statement() {
    Expression* var = 0;
    QList<Expression*> list;

    for_clause(var, list);

    Statement* stmt = new Statement(Statement::For, var ? var->pos : toRowCol(cur));
    stmt->var = var;
    stmt->list = Expression::toList(list);
    stmt->body = statement();
    return stmt;
}

void Parser2::for_clause(Expression*& var, QList<Expression*>& list) {
    expect(Tok_FOR, false, "for_clause");
    var = variable();
    expect(Tok_ColonEq, false, "for_clause");
    for_list(list);
    expect(Tok_DO, false, "for_clause");
}

void Parser2::for_list(QList<Expression*>& list) {
    list.append(for_list_element());
    while( la.d_type == Tok_Comma ) {
        expect(Tok_Comma, false, "for_list");
        list.append(for_list_element());
    }
}

Expression* Parser2::for_list_element() {
    Expression* res = arithmetic_expression();
    if( la.d_type == Tok_STEP || la.d_type == Tok_WHILE ) {
        if( la.d_type == Tok_STEP ) {
            expect(Tok_STEP, false, "for_list_element");
            const RowCol pos = toRowCol(cur);
            Expression* step = arithmetic_expression();
            expect(Tok_UNTIL, false, "for_list_element");
            Expression* until = arithmetic_expression();
            Expression* e = new Expression(Expression::StepUntil, pos);
            e->lhs = res;
            e->rhs = step;
            e->condition = until;
            res = e;
        } else if( la.d_type == Tok_WHILE ) {
            expect(Tok_WHILE, false, "for_list_element");
            const RowCol pos = toRowCol(cur);
            Expression* e = new Expression(Expression::WhileLoop, pos);
            e->lhs = res;
            e->condition = Boolean_expression();
            res = e;
        } else
            invalid("for_list_element");
    }
    return res;
}

Expression* Parser2::expression() {
    return Boolean_expression();
}

Expression* Parser2::arithmetic_expression() {
    if( FIRST_simple_arithmetic_expression(la.d_type) ) {
        return simple_arithmetic_expression();
    } else if( FIRST_if_clause(la.d_type) ) {
        Expression* cond = if_clause();
        Expression* res = new Expression(Expression::IfExpr, cond ? cond->pos : toRowCol(cur));
        res->condition = cond;
        res->lhs = simple_arithmetic_expression();
        expect(Tok_ELSE, false, "arithmetic_expression");
        res->rhs = arithmetic_expression();
        return res;
    } else {
        invalid("arithmetic_expression");
        return 0;
    }
}

Expression* Parser2::simple_arithmetic_expression() {
    Expression::Kind sign = Expression::Invalid;
    const RowCol pos = toRowCol(la);

    if( FIRST_adding_operator(la.d_type) ) {
        sign = adding_operator();
    }
    Expression* res = term();
    if( sign == Expression::Sub && res ) {
        Expression* neg = new Expression(Expression::Neg, pos);
        neg->rhs = res;
        res = neg;
    }
    while( FIRST_adding_operator(la.d_type) ) {
        const Expression::Kind k = adding_operator();
        const RowCol p = toRowCol(cur);
        Expression* rhs = term();
        Expression* op = new Expression(k, p);
        op->lhs = res;
        op->rhs = rhs;
        res = op;
    }
    return res;
}

Expression::Kind Parser2::adding_operator() {
    if( la.d_type == Tok_Plus ) {
        expect(Tok_Plus, false, "adding_operator");
        return Expression::Add;
    } else if( la.d_type == Tok_Minus ) {
        expect(Tok_Minus, false, "adding_operator");
        return Expression::Sub;
    } else {
        invalid("adding_operator");
        return Expression::Invalid;
    }
}

Expression* Parser2::term() {
    Expression* res = factor();
    while( FIRST_multiplying_operator(la.d_type) || FIRST_multiplying_operator(la.d_code) ) {
        const Expression::Kind k = multiplying_operator();
        const RowCol pos = toRowCol(cur);
        Expression* rhs = factor();
        Expression* op = new Expression(k, pos);
        op->lhs = res;
        op->rhs = rhs;
        res = op;
    }
    return res;
}

Expression::Kind Parser2::multiplying_operator() {
    if( la.d_type == Tok_Star ) {
        expect(Tok_Star, false, "multiplying_operator");
        return Expression::Mul;
    } else if( la.d_type == Tok_Slash ) {
        expect(Tok_Slash, false, "multiplying_operator");
        return Expression::Div;
    } else if( la.d_type == Tok_Percent ) {
        expect(Tok_Percent, false, "multiplying_operator");
        return Expression::IntDiv;
    } else if( la.d_type == Tok_Udiv ) {
        expect(Tok_Udiv, false, "multiplying_operator");
        return Expression::IntDiv;
    } else if( la.d_type == Tok_Umul ) {
        expect(Tok_Umul, false, "multiplying_operator");
        return Expression::Mul;
    } else if( la.d_code == Tok_DIV ) {
        expect(Tok_DIV, true, "multiplying_operator");
        return Expression::IntDiv;
    } else if( la.d_code == Tok_MOD ) {
        expect(Tok_MOD, true, "multiplying_operator");
        return Expression::Mod;
    } else {
        invalid("multiplying_operator");
        return Expression::Invalid;
    }
}

Expression* Parser2::factor() {
    Expression* res = primary();
    while( FIRST_power_sym_(la.d_type) || FIRST_power_sym_(la.d_code) ) {
        power_sym_();
        const RowCol pos = toRowCol(cur);
        Expression* rhs = primary();
        Expression* op = new Expression(Expression::Power, pos);
        op->lhs = res;
        op->rhs = rhs;
        res = op;
    }
    return res;
}

Expression::Kind Parser2::power_sym_() {
    if( la.d_code == Tok_POWER ) {
        expect(Tok_POWER, true, "power_sym_");
    } else if( la.d_type == Tok_Uexp ) {
        expect(Tok_Uexp, false, "power_sym_");
    } else if( la.d_type == Tok_Hat ) {
        expect(Tok_Hat, false, "power_sym_");
    } else if( la.d_type == Tok_2Star ) {
        expect(Tok_2Star, false, "power_sym_");
    } else
        invalid("power_sym_");
    return Expression::Power;
}

Expression* Parser2::primary() {
    if( FIRST_unsigned_number(la.d_type) ) {
        return unsigned_number();
    } else if( FIRST_variableOrFunction_(la.d_type) ) {
        return variableOrFunction_();
    } else if( la.d_type == Tok_Lpar ) {
        expect(Tok_Lpar, false, "primary");
        Expression* res = Boolean_expression();
        expect(Tok_Rpar, false, "primary");
        return res;
    } else {
        invalid("primary");
        return 0;
    }
}

Expression* Parser2::designational_expression() {
    if( FIRST_simple_designational_expression(la.d_type) ) {
        return simple_designational_expression();
    } else if( FIRST_if_clause(la.d_type) ) {
        Expression* cond = if_clause();
        Expression* res = new Expression(Expression::IfExpr, cond ? cond->pos : toRowCol(cur));
        res->condition = cond;
        res->lhs = simple_designational_expression();
        expect(Tok_ELSE, false, "designational_expression");
        res->rhs = designational_expression();
        return res;
    } else {
        invalid("designational_expression");
        return 0;
    }
}

Expression* Parser2::simple_designational_expression() {
    return primary();
}

Expression* Parser2::Boolean_expression() {
    if( FIRST_simple_Boolean(la.d_type) || FIRST_simple_Boolean(la.d_code) ) {
        return simple_Boolean();
    } else if( FIRST_if_clause(la.d_type) ) {
        Expression* cond = if_clause();
        Expression* res = new Expression(Expression::IfExpr, cond ? cond->pos : toRowCol(cur));
        res->condition = cond;
        res->lhs = simple_Boolean();
        expect(Tok_ELSE, false, "Boolean_expression");
        res->rhs = Boolean_expression();
        return res;
    } else {
        invalid("Boolean_expression");
        return 0;
    }
}

Expression* Parser2::simple_Boolean() {
    Expression* res = implication();
    while( FIRST_equiv_sym_(la.d_type) || FIRST_equiv_sym_(la.d_code) ) {
        const Expression::Kind k = equiv_sym_();
        const RowCol pos = toRowCol(cur);
        Expression* rhs = implication();
        Expression* op = new Expression(k, pos);
        op->lhs = res;
        op->rhs = rhs;
        res = op;
    }
    return res;
}

Expression::Kind Parser2::equiv_sym_() {
    if( la.d_code == Tok_EQUIV ) {
        expect(Tok_EQUIV, true, "equiv_sym_");
    } else if( la.d_type == Tok_Ueq ) {
        expect(Tok_Ueq, false, "equiv_sym_");
    } else if( la.d_type == Tok_2Eq ) {
        expect(Tok_2Eq, false, "equiv_sym_");
    } else
        invalid("equiv_sym_");
    return Expression::Eqv;
}

Expression* Parser2::implication() {
    Expression* res = Boolean_term();
    while( FIRST_impl_sym_(la.d_type) || FIRST_impl_sym_(la.d_code) ) {
        const Expression::Kind k = impl_sym_();
        const RowCol pos = toRowCol(cur);
        Expression* rhs = Boolean_term();
        Expression* op = new Expression(k, pos);
        op->lhs = res;
        op->rhs = rhs;
        res = op;
    }
    return res;
}

Expression::Kind Parser2::impl_sym_() {
    if( la.d_code == Tok_IMPL ) {
        expect(Tok_IMPL, true, "impl_sym_");
    } else if( la.d_type == Tok_Uimpl ) {
        expect(Tok_Uimpl, false, "impl_sym_");
    } else if( la.d_type == Tok_MinusGt ) {
        expect(Tok_MinusGt, false, "impl_sym_");
    } else
        invalid("impl_sym_");
    return Expression::Imp;
}

Expression* Parser2::Boolean_term() {
    Expression* res = Boolean_factor();
    while( FIRST_or_sym_(la.d_type) || FIRST_or_sym_(la.d_code) ) {
        const Expression::Kind k = or_sym_();
        const RowCol pos = toRowCol(cur);
        Expression* rhs = Boolean_factor();
        Expression* op = new Expression(k, pos);
        op->lhs = res;
        op->rhs = rhs;
        res = op;
    }
    return res;
}

Expression::Kind Parser2::or_sym_() {
    if( la.d_code == Tok_OR ) {
        expect(Tok_OR, true, "or_sym_");
    } else if( la.d_type == Tok_Uor ) {
        expect(Tok_Uor, false, "or_sym_");
    } else if( la.d_type == Tok_Bar ) {
        expect(Tok_Bar, false, "or_sym_");
    } else
        invalid("or_sym_");
    return Expression::Or;
}

Expression* Parser2::Boolean_factor() {
    Expression* res = Boolean_secondary();
    while( FIRST_and_sym_(la.d_type) || FIRST_and_sym_(la.d_code) ) {
        const Expression::Kind k = and_sym_();
        const RowCol pos = toRowCol(cur);
        Expression* rhs = Boolean_secondary();
        Expression* op = new Expression(k, pos);
        op->lhs = res;
        op->rhs = rhs;
        res = op;
    }
    return res;
}

Expression::Kind Parser2::and_sym_() {
    if( la.d_code == Tok_AND ) {
        expect(Tok_AND, true, "and_sym_");
    } else if( la.d_type == Tok_Uand ) {
        expect(Tok_Uand, false, "and_sym_");
    } else if( la.d_type == Tok_Amp ) {
        expect(Tok_Amp, false, "and_sym_");
    } else
        invalid("and_sym_");
    return Expression::And;
}

Expression* Parser2::Boolean_secondary() {
    if( FIRST_not_sym_(la.d_type) || FIRST_not_sym_(la.d_code) ) {
        not_sym_();
        const RowCol pos = toRowCol(cur);
        Expression* res = new Expression(Expression::Not, pos);
        res->rhs = Boolean_primary();
        return res;
    } else if( FIRST_Boolean_primary(la.d_type) ) {
        return Boolean_primary();
    } else {
        invalid("Boolean_secondary");
        return 0;
    }
}

Expression::Kind Parser2::not_sym_() {
    if( la.d_code == Tok_NOT ) {
        expect(Tok_NOT, true, "not_sym_");
    } else if( la.d_type == Tok_Unot ) {
        expect(Tok_Unot, false, "not_sym_");
    } else if( la.d_type == Tok_Bang ) {
        expect(Tok_Bang, false, "not_sym_");
    } else
        invalid("not_sym_");
    return Expression::Not;
}

Expression* Parser2::Boolean_primary() {
    if( FIRST_logical_value(la.d_type) ) {
        return logical_value();
    } else if( FIRST_relation(la.d_type) ) {
        return relation();
    } else {
        invalid("Boolean_primary");
        return 0;
    }
}

Expression* Parser2::relation() {
    Expression* res = simple_arithmetic_expression();
    if( FIRST_relational_operator(la.d_type) || FIRST_relational_operator(la.d_code) ) {
        const Expression::Kind k = relational_operator();
        const RowCol pos = toRowCol(cur);
        Expression* rhs = simple_arithmetic_expression();
        Expression* op = new Expression(k, pos);
        op->lhs = res;
        op->rhs = rhs;
        res = op;
    }
    return res;
}

Expression::Kind Parser2::relational_operator() {
    if( la.d_type == Tok_Lt ) {
        expect(Tok_Lt, false, "relational_operator");
        return Expression::Lt;
    } else if( la.d_type == Tok_Leq ) {
        expect(Tok_Leq, false, "relational_operator");
        return Expression::Leq;
    } else if( la.d_type == Tok_Eq ) {
        expect(Tok_Eq, false, "relational_operator");
        return Expression::Eq;
    } else if( la.d_type == Tok_Geq ) {
        expect(Tok_Geq, false, "relational_operator");
        return Expression::Geq;
    } else if( la.d_type == Tok_Gt ) {
        expect(Tok_Gt, false, "relational_operator");
        return Expression::Gt;
    } else if( la.d_type == Tok_LtGt ) {
        expect(Tok_LtGt, false, "relational_operator");
        return Expression::Neq;
    } else if( la.d_type == Tok_Uleq ) {
        expect(Tok_Uleq, false, "relational_operator");
        return Expression::Leq;
    } else if( la.d_type == Tok_Ugeq ) {
        expect(Tok_Ugeq, false, "relational_operator");
        return Expression::Geq;
    } else if( la.d_type == Tok_Uneq ) {
        expect(Tok_Uneq, false, "relational_operator");
        return Expression::Neq;
    } else if( la.d_type == Tok_BangEq ) {
        expect(Tok_BangEq, false, "relational_operator");
        return Expression::Neq;
    } else if( la.d_type == Tok_HatEq ) {
        expect(Tok_HatEq, false, "relational_operator");
        return Expression::Neq;
    } else if( la.d_code == Tok_LESS ) {
        expect(Tok_LESS, true, "relational_operator");
        return Expression::Lt;
    } else if( la.d_code == Tok_NOTGREATER ) {
        expect(Tok_NOTGREATER, true, "relational_operator");
        return Expression::Leq;
    } else if( la.d_code == Tok_EQUAL ) {
        expect(Tok_EQUAL, true, "relational_operator");
        return Expression::Eq;
    } else if( la.d_code == Tok_NOTLESS ) {
        expect(Tok_NOTLESS, true, "relational_operator");
        return Expression::Geq;
    } else if( la.d_code == Tok_GREATER ) {
        expect(Tok_GREATER, true, "relational_operator");
        return Expression::Gt;
    } else if( la.d_code == Tok_NOTEQUAL ) {
        expect(Tok_NOTEQUAL, true, "relational_operator");
        return Expression::Neq;
    } else {
        invalid("relational_operator");
        return Expression::Invalid;
    }
}

Expression* Parser2::variableOrFunction_() {
    expect(Tok_identifier, false, "variableOrFunction_");
    const Token name = cur;
    const RowCol pos = toRowCol(name);

    Expression* res = new Expression(Expression::Identifier, pos);
    res->a = name.d_id;

    if( la.d_type == Tok_Lbrack || la.d_type == Tok_Lpar ) {
        if( la.d_type == Tok_Lbrack ) {
            expect(Tok_Lbrack, false, "variableOrFunction_");
            QList<Expression*> subs;
            subscript_list(subs);
            expect(Tok_Rbrack, false, "variableOrFunction_");
            Expression* sub = new Expression(Expression::Subscript, pos);
            sub->lhs = res;
            sub->rhs = Expression::toList(subs);
            res = sub;
        } else if( la.d_type == Tok_Lpar ) {
            expect(Tok_Lpar, false, "variableOrFunction_");
            QList<Expression*> args;
            actual_parameter_list(args);
            expect(Tok_Rpar, false, "variableOrFunction_");
            Expression* call = new Expression(Expression::Call, pos);
            call->lhs = res;
            call->rhs = Expression::toList(args);
            res = call;
        } else
            invalid("variableOrFunction_");
    }
    return res;
}

Expression* Parser2::variable() {
    expect(Tok_identifier, false, "variable");
    const Token name = cur;
    const RowCol pos = toRowCol(name);

    Expression* res = new Expression(Expression::Identifier, pos);
    res->a = name.d_id;

    if( la.d_type == Tok_Lbrack ) {
        expect(Tok_Lbrack, false, "variable");
        QList<Expression*> subs;
        subscript_list(subs);
        expect(Tok_Rbrack, false, "variable");
        Expression* sub = new Expression(Expression::Subscript, pos);
        sub->lhs = res;
        sub->rhs = Expression::toList(subs);
        res = sub;
    }
    return res;
}

Declaration* Parser2::simple_variable(Type* t, bool isOwn) {
    const Token name = variable_identifier();

    Declaration* varDecl = mdl->addDecl(name.d_id, name.d_val, Declaration::Variable);
    varDecl->pos = toRowCol(name);
    varDecl->setType(t);
    varDecl->isOwn = isOwn;
    return varDecl;
}

Token Parser2::variable_identifier() {
    expect(Tok_identifier, false, "variable_identifier");
    return cur;
}

void Parser2::subscript_list(QList<Expression*>& subs) {
    subs.append(subscript_expression());
    while( la.d_type == Tok_Comma ) {
        expect(Tok_Comma, false, "subscript_list");
        subs.append(subscript_expression());
    }
}

Expression* Parser2::subscript_expression() {
    return arithmetic_expression();
}

Expression* Parser2::unsigned_number() {
    if( la.d_type == Tok_unsigned_integer ) {
        expect(Tok_unsigned_integer, false, "unsigned_number");
        Expression* res = new Expression(Expression::UnsignedConst, toRowCol(cur));
        res->u = cur.d_val.toULongLong();
        res->setType(mdl->getType(Type::Integer));
        return res;
    } else if( la.d_type == Tok_decimal_number ) {
        expect(Tok_decimal_number, false, "unsigned_number");
        Expression* res = new Expression(Expression::RealConst, toRowCol(cur));
        res->r = cur.d_val.toDouble();
        res->setType(mdl->getType(Type::Real));
        return res;
    } else {
        invalid("unsigned_number");
        return 0;
    }
}

Token Parser2::letter_string() {
    expect(Tok_identifier, false, "letter_string");
    return cur;
}

Expression* Parser2::logical_value() {
    if( la.d_type == Tok_TRUE ) {
        expect(Tok_TRUE, false, "logical_value");
        Expression* res = new Expression(Expression::BoolConst, toRowCol(cur));
        res->u = true;
        res->setType(mdl->getType(Type::Boolean));
        return res;
    } else if( la.d_type == Tok_FALSE ) {
        expect(Tok_FALSE, false, "logical_value");
        Expression* res = new Expression(Expression::BoolConst, toRowCol(cur));
        res->u = false;
        res->setType(mdl->getType(Type::Boolean));
        return res;
    } else {
        invalid("logical_value");
        return 0;
    }
}
