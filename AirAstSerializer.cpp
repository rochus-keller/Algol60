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

#include <Algol60/AirAstSerializer.h>
#include <Algol60/AirRenderer.h>
#include <QtDebug>
using namespace Air;
using Alg::RowCol;

static void renderProc( const Declaration* p, AbstractRenderer* r, AstSerializer::DbgInfo );

static quint32 packed(const RowCol& pos, AstSerializer::DbgInfo dbi)
{
    if( dbi == AstSerializer::None || !pos.isValid() )
        return 0;
    if( dbi == AstSerializer::RowsOnly )
        return RowCol(pos.d_row,1).packed();
    return pos.packed();
}

static void lineout(AbstractRenderer* r, const RowCol& pos, AstSerializer::DbgInfo dbi)
{
    const quint32 cur = packed(pos, dbi);
    if( cur )
        r->line(RowCol(RowCol::unpackRow2(cur),RowCol::unpackCol2(cur)));
}

static quint32 varLine(const RowCol& pos, AstSerializer::DbgInfo dbi)
{
    if( dbi == AstSerializer::None || !pos.isValid() )
        return 0;
    return pos.d_row;
}

static Quali toQuali(Type* t)
{
    if( t == 0 )
        return Quali();
    return t->toQuali();
}

static Trident toTrident(Declaration* d)
{
    Trident res;
    if( d == 0 )
        return res;
    res.second = d->name;
    if( d->outer )
        res.first = d->outer->toQuali();
    return res;
}

static void renderPos(ProcData& proc, const RowCol& pos, quint32& line, AstSerializer::DbgInfo dbi)
{
    const quint32 cur = packed(pos, dbi);
    if( cur && cur != line )
    {
        line = cur;
        proc.body << ProcData::Op(op_LINE, cur);
    }
}

static void renderExprs(ProcData& proc, Expression* e, quint32& line, AstSerializer::DbgInfo dbi)
{
    while( e )
    {
        renderPos(proc, e->pos, line, dbi);
        switch( e->kind )
        {
        case op_call:
        case op_calli:
        case op_callmi:
        case op_castobj:
        case op_isinst:
        case op_ldelem:
        case op_ldind:
        case op_ldproc:
        case op_ldvar:
        case op_newarr:
        case op_newarr_um:
        case op_newobj:
        case op_newobj_um:
        case op_pcall:
        case op_pcalli:
        case op_refelem:
        case op_refvar:
        case op_sizeof:
            if( e->d )
                proc.body << ProcData::Op(e->kind, QVariant::fromValue(e->d->toQuali()));
            else
                qCritical() << "AstSerializer:" << proc.name << s_opName[e->kind] << "invalid declaration";
            break;
        case op_callinst:
        case op_callvirt:
        case op_ldfld:
        case op_ldmeth:
        case op_ldvirt:
        case op_reffld:
            if( e->d )
                proc.body << ProcData::Op(e->kind, QVariant::fromValue(toTrident(e->d)));
            else
                qCritical() << "AstSerializer:" << proc.name << s_opName[e->kind] << "invalid declaration";
            break;
        case op_ldarg:
        case op_ldloc:
        case op_refarg:
        case op_refloc:
            proc.body << ProcData::Op(e->kind, e->id);
            break;
        case op_ldc_i4:
        case op_ldc_i8:
            proc.body << ProcData::Op(e->kind, e->i);
            break;
        case op_ldc_r4:
        case op_ldc_r8:
            proc.body << ProcData::Op(e->kind, e->f);
            break;
        case op_ldstr:
            if( e->c )
                proc.body << ProcData::Op(e->kind, e->c->toVariant());
            else
                qCritical() << "AstSerializer:" << proc.name << s_opName[e->kind] << "invalid literal";
            break;
        case op_iif: {
                proc.body << ProcData::Op(e->kind);
                Expression* if_ = e->e;
                if( if_ == 0 || if_->next == 0 || if_->next->next == 0 )
                {
                    qCritical() << "AstSerializer:" << proc.name << "incomplete conditional expression";
                    return;
                }
                Expression* then_ = if_->next;
                Expression* else_ = then_->next;
                renderExprs(proc, if_->e, line, dbi);
                proc.body << ProcData::Op(op_THEN);
                renderExprs(proc, then_->e, line, dbi);
                proc.body << ProcData::Op(op_ELSE);
                renderExprs(proc, else_->e, line, dbi);
                proc.body << ProcData::Op(op_END);
            } break;
        default:
            proc.body << ProcData::Op(e->kind);
            break;
        }
        e = e->next;
    }
}

static void renderStats(ProcData& proc, Statement* s, quint32& line, AstSerializer::DbgInfo dbi)
{
    while( s )
    {
        if( s->kind == Statement::ExprStat )
            renderExprs(proc, s->e, line, dbi);
        else
        {
            renderPos(proc, s->pos, line, dbi);
            switch( s->kind )
            {
            case op_WHILE:
                proc.body << ProcData::Op(op_WHILE);
                renderExprs(proc, s->e, line, dbi);
                proc.body << ProcData::Op(op_DO);
                renderStats(proc, s->body, line, dbi);
                proc.body << ProcData::Op(op_END);
                break;
            case op_REPEAT:
                proc.body << ProcData::Op(op_REPEAT);
                renderStats(proc, s->body, line, dbi);
                proc.body << ProcData::Op(op_UNTIL);
                renderExprs(proc, s->e, line, dbi);
                proc.body << ProcData::Op(op_END);
                break;
            case op_LOOP:
                proc.body << ProcData::Op(op_LOOP);
                renderStats(proc, s->body, line, dbi);
                proc.body << ProcData::Op(op_END);
                break;
            case op_IF:
                proc.body << ProcData::Op(op_IF);
                renderExprs(proc, s->e, line, dbi);
                proc.body << ProcData::Op(op_THEN);
                renderStats(proc, s->body, line, dbi);
                if( s->next && s->next->kind == op_ELSE )
                {
                    s = s->next;
                    proc.body << ProcData::Op(op_ELSE);
                    renderStats(proc, s->body, line, dbi);
                }
                proc.body << ProcData::Op(op_END);
                break;
            case op_SWITCH:
                proc.body << ProcData::Op(op_SWITCH);
                renderExprs(proc, s->e, line, dbi);
                while( s->next && s->next->kind == op_CASE )
                {
                    s = s->next;
                    if( s->labels )
                        proc.body << ProcData::Op(op_CASE, QVariant::fromValue(*s->labels));
                    else
                        qCritical() << "AstSerializer:" << proc.name << "case without labels";
                    renderStats(proc, s->body, line, dbi);
                }
                if( s->next && s->next->kind == op_ELSE )
                {
                    s = s->next;
                    proc.body << ProcData::Op(op_ELSE);
                    renderStats(proc, s->body, line, dbi);
                }
                proc.body << ProcData::Op(op_END);
                break;
            case op_goto:
            case op_label:
                proc.body << ProcData::Op(s->kind, s->name);
                break;
            case op_starg:
            case op_stloc:
                proc.body << ProcData::Op(s->kind, s->id);
                break;
            case op_copy:
            case op_stelem:
            case op_stind:
            case op_stvar:
                if( s->d )
                    proc.body << ProcData::Op(s->kind, QVariant::fromValue(s->d->toQuali()));
                else
                    qCritical() << "AstSerializer:" << proc.name << s_opName[s->kind] << "invalid declaration";
                break;
            case op_stfld:
                if( s->d )
                    proc.body << ProcData::Op(s->kind, QVariant::fromValue(toTrident(s->d)));
                else
                    qCritical() << "AstSerializer:" << proc.name << s_opName[s->kind] << "invalid declaration";
                break;
            default:
                proc.body << ProcData::Op(s->kind);
                break;
            }
        }
        s = s->next;
    }
}

static void renderProcType(const Declaration* d, AbstractRenderer* r, AstSerializer::DbgInfo dbi)
{
    Type* t = d->getType();
    ProcData proc;
    proc.name = d->name;
    proc.isPublic = d->public_;
    proc.kind = t->typebound ? ProcData::BoundProcType : ProcData::ProcType;
    proc.retType = toQuali(t->getType());
    foreach( Declaration* sub, t->subs )
    {
        ProcData::Var param;
        param.name = sub->name;
        param.type = toQuali(sub->getType());
        param.line = varLine(sub->pos, dbi);
        param.isVarParam = sub->varParam;
        proc.params.append(param);
    }
    lineout(r, d->pos, dbi);
    r->addProcedure(proc);
}

static void renderType(const Declaration* d, AbstractRenderer* r, AstSerializer::DbgInfo dbi)
{
    Type* t = d->getType();
    Q_ASSERT( t );
    switch( t->kind )
    {
    case Type::Struct:
    case Type::Record:
    case Type::CStruct:
    case Type::CUnion: {
            quint8 kind = EmiTypes::Struct;
            if( t->kind == Type::Record )
                kind = EmiTypes::Record;
            else if( t->kind == Type::CStruct )
                kind = EmiTypes::CStruct;
            else if( t->kind == Type::CUnion )
                kind = EmiTypes::CUnion;
            lineout(r, d->pos, dbi);
            r->beginType(d->name, d->public_, kind, toQuali(t->getType()));
            foreach( Declaration* f, t->getFieldList(false) )
            {
                lineout(r, f->pos, dbi);
                r->addField(f->name, toQuali(f->getType()), f->public_);
            }
            r->endType();
            // the bound procedures of a record are rendered from the module level, see Placeholder
        } break;
    case Type::Proc:
        renderProcType(d, r, dbi);
        break;
    case Type::NameRef:
        lineout(r, d->pos, dbi);
        r->addType(d->name, d->public_, t->toQuali(), EmiTypes::Alias);
        break;
    case Type::Pointer:
    case Type::CPointer:
        lineout(r, d->pos, dbi);
        r->addType(d->name, d->public_, toQuali(t->getType()),
                   t->kind == Type::Pointer ? EmiTypes::Pointer : EmiTypes::CPointer);
        break;
    case Type::Array:
    case Type::CArray:
        lineout(r, d->pos, dbi);
        r->addType(d->name, d->public_, toQuali(t->getType()),
                   t->kind == Type::Array ? EmiTypes::Array : EmiTypes::CArray, t->len);
        break;
    default:
        qCritical() << "AstSerializer: cannot render type" << d->name;
        break;
    }
}

static void renderProc(const Declaration* proc, AbstractRenderer* r, AstSerializer::DbgInfo dbi)
{
    ProcData pdata;
    pdata.name = proc->name;
    pdata.isPublic = proc->public_;
    if( proc->init )
        pdata.kind = ProcData::Init;
    else if( proc->extern_ )
        pdata.kind = ProcData::Extern;
    else if( proc->foreign_ )
        pdata.kind = ProcData::Foreign;
    else
        pdata.kind = ProcData::Normal;

    if( proc->typebound && proc->outer )
        pdata.binding = proc->outer->name;
    else if( ( proc->extern_ || proc->foreign_ ) && proc->pd )
        pdata.binding = proc->pd->externalName;

    if( proc->pd )
        pdata.endLine = varLine(proc->pd->end, dbi);
    pdata.retType = toQuali(proc->getType());

    Declaration* sub = proc->subs;
    while( sub )
    {
        ProcData::Var v;
        v.name = sub->name;
        v.type = toQuali(sub->getType());
        v.line = varLine(sub->pos, dbi);
        switch( sub->kind )
        {
        case Declaration::ParamDecl:
            v.isVarParam = sub->varParam;
            pdata.params.append(v);
            break;
        case Declaration::LocalDecl:
            pdata.locals.append(v);
            break;
        }
        sub = sub->next;
    }

    if( !proc->extern_ && !proc->foreign_ )
    {
        quint32 line = 0;
        renderStats(pdata, proc->body, line, dbi);
    }

    lineout(r, proc->pos, dbi);
    r->addProcedure(pdata);
}

bool AstSerializer::render(AbstractRenderer* r, const Declaration* module, DbgInfo dbi)
{
    Q_ASSERT( r && module && module->kind == Declaration::Module );

    QString source;
    if( module->md == 0 || module->md->source.isEmpty() )
        dbi = None;
    else
        source = module->md->source;

    lineout(r, module->pos, dbi);
    r->beginModule(module->name, source);

    Declaration* sub = module->subs;
    while( sub )
    {
        switch( sub->kind )
        {
        case Declaration::Import:
            lineout(r, sub->pos, dbi);
            r->addImport(sub->name);
            break;
        case Declaration::ConstDecl:
            lineout(r, sub->pos, dbi);
            r->addConst(Quali(), sub->name, sub->c ? sub->c->toVariant() : QVariant());
            break;
        case Declaration::VarDecl:
            lineout(r, sub->pos, dbi);
            r->addVariable(toQuali(sub->getType()), sub->name, sub->public_);
            break;
        case Declaration::TypeDecl:
            renderType(sub, r, dbi);
            break;
        case Declaration::Procedure:
            renderProc(sub, r, dbi);
            break;
        case Declaration::Placeholder:
            if( sub->imported )
                renderProc(sub->imported, r, dbi);
            break;
        }
        sub = sub->next;
    }

    if( module->md )
        lineout(r, module->md->end, dbi);
    r->endModule();
    return true;
}

bool AstSerializer::render(AbstractRenderer* r, const AstModel* mdl, DbgInfo dbi)
{
    Q_ASSERT( r && mdl );
    bool res = true;
    foreach( Declaration* m, mdl->getModules() )
        res = render(r, m, dbi) && res;
    return res;
}
