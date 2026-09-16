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

#include <Algol60/AirEmitter.h>
#include <Algol60/AirOps.h>
#include <QtDebug>
using namespace Air;
using Alg::RowCol;

Emitter::Emitter(AbstractRenderer* r, DbgInfo di):
    d_typeKind(0),d_stackDepth(0),d_maxStackDepth(0),ops(0),d_out(r),
    lastLine(0),firstLine(0),dbgInfo(di)
{
    Q_ASSERT( r );
}

void Emitter::beginModule(const QByteArray& moduleName, const QString& sourceFile, const RowCol& pos)
{
    Q_ASSERT( !moduleName.isEmpty() );
    Q_ASSERT( d_proc.isEmpty() && d_typeKind == 0 );
    lineout(pos);
    d_out->beginModule(moduleName, dbgInfo == None ? QString() : sourceFile);
}

void Emitter::endModule(const RowCol& pos)
{
    Q_ASSERT( d_proc.isEmpty() && d_typeKind == 0 );
    lineout(pos);
    d_out->endModule();
}

void Emitter::addImport(const QByteArray& moduleName, const RowCol& pos)
{
    Q_ASSERT( d_proc.isEmpty() && d_typeKind == 0 );
    lineout(pos);
    d_out->addImport(moduleName);
}

void Emitter::addVariable(const Quali& typeRef, const QByteArray& name, const RowCol& pos, bool isPublic)
{
    Q_ASSERT( d_proc.isEmpty() && d_typeKind == 0 );
    Q_ASSERT( !typeRef.second.isEmpty() );
    lineout(pos);
    d_out->addVariable(typeRef, name, isPublic);
}

void Emitter::addConst(const Quali& typeRef, const QByteArray& name, const RowCol& pos, const QVariant& val)
{
    Q_ASSERT( d_proc.isEmpty() && d_typeKind == 0 );
    lineout(pos);
    d_out->addConst(typeRef, name, val);
}

void Emitter::beginType(const QByteArray& name, const RowCol& pos, bool isPublic, quint8 typeKind, const Quali& base)
{
    Q_ASSERT( d_typeKind == 0 );
    Q_ASSERT( typeKind == EmiTypes::Struct || typeKind == EmiTypes::Record
              || typeKind == EmiTypes::CStruct || typeKind == EmiTypes::CUnion
              || typeKind == EmiTypes::ProcType || typeKind == EmiTypes::BoundProcType );
    d_typeKind = typeKind;
    if( typeKind == EmiTypes::ProcType || typeKind == EmiTypes::BoundProcType )
    {
        firstLine = lineset(pos);
        d_proc.append(ProcData());
        d_proc.back().name = name;
        d_proc.back().isPublic = isPublic;
        d_proc.back().kind = typeKind == EmiTypes::ProcType ? ProcData::ProcType : ProcData::BoundProcType;
    }else
    {
        lineout(pos);
        d_out->beginType(name, isPublic, typeKind, base);
    }
}

void Emitter::endType()
{
    Q_ASSERT( d_typeKind != 0 );
    if( d_typeKind == EmiTypes::ProcType || d_typeKind == EmiTypes::BoundProcType )
    {
        if( firstLine )
            d_out->line(RowCol(RowCol::unpackRow2(firstLine),RowCol::unpackCol2(firstLine)));
        d_out->addProcedure(d_proc.back());
        d_proc.pop_back();
    }else
        d_out->endType();
    d_typeKind = 0;
}

void Emitter::addType(const QByteArray& name, const RowCol& pos, bool isPublic, const Quali& baseType,
                      quint8 typeKind, quint32 len)
{
    Q_ASSERT( d_typeKind == 0 );
    Q_ASSERT( typeKind == EmiTypes::Alias || typeKind == EmiTypes::Pointer || typeKind == EmiTypes::Array
              || typeKind == EmiTypes::CArray || typeKind == EmiTypes::CPointer );
    lineout(pos);
    d_out->addType(name, isPublic, baseType, typeKind, len);
}

void Emitter::beginProc(const QByteArray& procName, const RowCol& pos, bool isPublic,
                        quint8 kind, const QByteArray& binding)
{
    Q_ASSERT( d_typeKind == 0 && !procName.isEmpty() );
    Q_ASSERT( kind == ProcData::Normal || kind == ProcData::Init
              || kind == ProcData::Extern || kind == ProcData::Foreign );
    firstLine = lineset(pos);
    d_proc.append(ProcData());
    d_proc.back().name = procName;
    d_proc.back().isPublic = isPublic;
    d_proc.back().kind = kind;
    d_proc.back().binding = binding;
    d_stackDepth = 0;
    d_maxStackDepth = 0;
    ops = &d_proc.back().body;
}

void Emitter::endProc(const RowCol& pos)
{
    Q_ASSERT( !d_proc.isEmpty() && d_typeKind == 0 && ops != 0 );
    d_proc.back().endLine = pos.d_row;
    if( firstLine )
        d_out->line(RowCol(RowCol::unpackRow2(firstLine),RowCol::unpackCol2(firstLine)));
    d_out->addProcedure(d_proc.back());
    d_proc.pop_back();
    ops = d_proc.isEmpty() ? 0 : &d_proc.back().body;
}

void Emitter::discardProc()
{
    while( !d_proc.isEmpty() )
        d_proc.pop_back();
    ops = 0;
    d_stackDepth = 0;
    d_maxStackDepth = 0;
    firstLine = 0;
    lastLine = 0;
}

void Emitter::addField(const QByteArray& fieldName, const RowCol& pos, const Quali& typeRef, bool isPublic)
{
    Q_ASSERT( d_typeKind == EmiTypes::Struct || d_typeKind == EmiTypes::Record
              || d_typeKind == EmiTypes::CStruct || d_typeKind == EmiTypes::CUnion );
    Q_ASSERT( !typeRef.second.isEmpty() );
    lineout(pos);
    d_out->addField(fieldName, typeRef, isPublic);
}

quint32 Emitter::addLocal(const Quali& typeRef, const QByteArray& name, const RowCol& pos)
{
    Q_ASSERT( !d_proc.isEmpty() && d_typeKind == 0 );
    Q_ASSERT( !typeRef.second.isEmpty() && !name.isEmpty() );
    d_proc.back().locals.append(ProcData::Var(typeRef, name, pos.d_row));
    return d_proc.back().locals.size() - 1;
}

quint32 Emitter::addArgument(const Quali& typeRef, const QByteArray& name, bool isVarParam, const RowCol& pos)
{
    Q_ASSERT( !d_proc.isEmpty() );
    Q_ASSERT( !typeRef.second.isEmpty() && !name.isEmpty() );
    ProcData::Var p(typeRef, name, pos.d_row);
    p.isVarParam = isVarParam;
    d_proc.back().params.append(p);
    return d_proc.back().params.size() - 1;
}

void Emitter::setReturnType(const Quali& typeRef)
{
    Q_ASSERT( !d_proc.isEmpty() );
    Q_ASSERT( d_proc.back().retType.second.isEmpty() );
    d_proc.back().retType = typeRef;
}

QByteArray Emitter::basicType(EmiTypes::Basic t)
{
    return EmiTypes::basicName(t);
}

Quali Emitter::basicTypeRef(EmiTypes::Basic t)
{
    return Quali(QByteArray(), basicType(t));
}

QByteArray Emitter::toString(const Quali& q)
{
    return AsmRenderer::toString(q);
}

QByteArray Emitter::toString(const Trident& t)
{
    return AsmRenderer::toString(t);
}

void Emitter::delta(int d)
{
    int s = d_stackDepth;
    s += d;
    d_stackDepth = s >= 0 ? s : 0;
    if( d_stackDepth > d_maxStackDepth )
        d_maxStackDepth = d_stackDepth;
}

void Emitter::add(quint8 op, const QVariant& arg)
{
    Q_ASSERT( ops != 0 );
    ops->append(ProcData::Op(op, arg));
}

void Emitter::lineout(const RowCol& pos)
{
    if( !pos.isValid() )
        return;
    const quint32 cur = lineset(pos);
    if( cur )
        d_out->line(RowCol(RowCol::unpackRow2(cur),RowCol::unpackCol2(cur)));
}

quint32 Emitter::lineset(const RowCol& pos)
{
    if( dbgInfo == None || !pos.isValid() )
        return 0;
    const quint32 cur = dbgInfo == RowsOnly ? RowCol(pos.d_row,1).packed() : pos.packed();
    if( cur != lastLine )
    {
        lastLine = cur;
        return cur;
    }
    return 0;
}

void Emitter::abs_()
{
    add(op_abs);
    delta(-1+1);
}

void Emitter::add_()
{
    add(op_add);
    delta(-2+1);
}

void Emitter::and_()
{
    add(op_and);
    delta(-2+1);
}

void Emitter::caddr_()
{
    add(op_caddr);
    delta(-1+1);
}

void Emitter::call_(const Quali& procRef, int argCount, bool hasRet)
{
    add(op_call, QVariant::fromValue(procRef));
    delta(-argCount + (hasRet ? 1 : 0));
}

void Emitter::calli_(const Quali& procType, int argCount, bool hasRet)
{
    add(op_calli, QVariant::fromValue(procType));
    delta(-argCount - 1 + (hasRet ? 1 : 0)); // args + procref
}

void Emitter::callinst_(const Trident& procRef, int argCount, bool hasRet)
{
    add(op_callinst, QVariant::fromValue(procRef));
    delta(-argCount - 1 + (hasRet ? 1 : 0)); // args + receiver
}

void Emitter::callmi_(const Quali& procType, int argCount, bool hasRet)
{
    add(op_callmi, QVariant::fromValue(procType));
    delta(-argCount - 1 + (hasRet ? 1 : 0)); // args + methref
}

void Emitter::callvirt_(const Trident& procRef, int argCount, bool hasRet)
{
    add(op_callvirt, QVariant::fromValue(procRef));
    delta(-argCount - 1 + (hasRet ? 1 : 0)); // args + receiver
}

void Emitter::case_(const CaseLabelList& labels)
{
    add(op_CASE, QVariant::fromValue(labels));
}

void Emitter::castobj_(const Quali& typeRef)
{
    add(op_castobj, QVariant::fromValue(typeRef));
    delta(-1+1);
}

void Emitter::ceq_()
{
    add(op_ceq);
    delta(-2+1);
}

void Emitter::cgt_(bool withUnsigned)
{
    add(withUnsigned ? op_cgt_un : op_cgt);
    delta(-2+1);
}

void Emitter::clt_(bool withUnsigned)
{
    add(withUnsigned ? op_clt_un : op_clt);
    delta(-2+1);
}

void Emitter::conv_(EmiTypes::Basic t)
{
    switch( t )
    {
    case EmiTypes::INT8:
        add(op_conv_i1);
        break;
    case EmiTypes::INT16:
        add(op_conv_i2);
        break;
    case EmiTypes::INT32:
        add(op_conv_i4);
        break;
    case EmiTypes::INT64:
        add(op_conv_i8);
        break;
    case EmiTypes::FLOAT32:
        add(op_conv_r4);
        break;
    case EmiTypes::FLOAT64:
        add(op_conv_r8);
        break;
    case EmiTypes::BOOL:
    case EmiTypes::CHAR:
    case EmiTypes::UINT8:
        add(op_conv_u1);
        break;
    case EmiTypes::UINT16:
        add(op_conv_u2);
        break;
    case EmiTypes::UINT32:
        add(op_conv_u4);
        break;
    case EmiTypes::UINT64:
        add(op_conv_u8);
        break;
    default:
        Q_ASSERT(false);
        break;
    }
    delta(-1+1);
}

void Emitter::copy_(const Quali& typeRef)
{
    add(op_copy, QVariant::fromValue(typeRef));
    delta(-2);
}

void Emitter::curtask_()
{
    add(op_curtask);
    delta(+1);
}

void Emitter::div_(bool withUnsigned)
{
    add(withUnsigned ? op_div_un : op_div);
    delta(-2+1);
}

void Emitter::do_()
{
    add(op_DO);
}

void Emitter::dup_()
{
    add(op_dup);
    delta(+1);
}

void Emitter::else_()
{
    add(op_ELSE);
}

void Emitter::end_()
{
    add(op_END);
}

void Emitter::exit_()
{
    add(op_exit);
}

void Emitter::free_()
{
    add(op_free);
    delta(-1);
}

void Emitter::goto_(const QByteArray& label)
{
    add(op_goto, label);
}

void Emitter::if_()
{
    add(op_IF);
}

void Emitter::iif_()
{
    add(op_iif);
}

void Emitter::isinst_(const Quali& typeRef)
{
    add(op_isinst, QVariant::fromValue(typeRef));
    delta(-1+1);
}

void Emitter::label_(const QByteArray& name)
{
    add(op_label, name);
}

void Emitter::ldarg_(quint16 arg)
{
    add(op_ldarg, arg);
    delta(+1);
}

void Emitter::ldc_i4(qint32 v)
{
    add(op_ldc_i4, v);
    delta(+1);
}

void Emitter::ldc_i8(qint64 v)
{
    add(op_ldc_i8, v);
    delta(+1);
}

void Emitter::ldc_r4(double v)
{
    add(op_ldc_r4, v);
    delta(+1);
}

void Emitter::ldc_r8(double v)
{
    add(op_ldc_r8, v);
    delta(+1);
}

void Emitter::ldelem_(const Quali& typeRef)
{
    add(op_ldelem, QVariant::fromValue(typeRef));
    delta(-2+1);
}

void Emitter::ldfld_(const Trident& fieldRef)
{
    add(op_ldfld, QVariant::fromValue(fieldRef));
    delta(-1+1);
}

void Emitter::ldind_(const Quali& typeRef)
{
    add(op_ldind, QVariant::fromValue(typeRef));
    delta(-1+1);
}

void Emitter::ldloc_(quint16 loc)
{
    add(op_ldloc, loc);
    delta(+1);
}

void Emitter::ldmeth_(const Trident& procRef)
{
    add(op_ldmeth, QVariant::fromValue(procRef));
    delta(-1+1);
}

void Emitter::ldnull_()
{
    add(op_ldnull);
    delta(+1);
}

void Emitter::ldproc_(const Quali& procRef)
{
    add(op_ldproc, QVariant::fromValue(procRef));
    delta(+1);
}

void Emitter::ldstr_(const QByteArray& str)
{
    add(op_ldstr, str);
    delta(+1);
}

void Emitter::ldvar_(const Quali& varRef)
{
    add(op_ldvar, QVariant::fromValue(varRef));
    delta(+1);
}

void Emitter::ldvirt_(const Trident& procRef)
{
    add(op_ldvirt, QVariant::fromValue(procRef));
    delta(-1+1);
}

void Emitter::len_()
{
    add(op_len);
    delta(-1+1);
}

void Emitter::line_(const RowCol& pos)
{
    const quint32 cur = lineset(pos);
    if( cur == 0 )
        return;
    if( ops )
        add(op_LINE, cur);
    else
        d_out->line(RowCol(RowCol::unpackRow2(cur),RowCol::unpackCol2(cur)));
}

void Emitter::loop_()
{
    add(op_LOOP);
}

void Emitter::mul_()
{
    add(op_mul);
    delta(-2+1);
}

void Emitter::neg_()
{
    add(op_neg);
    delta(-1+1);
}

void Emitter::newarr_(const Quali& typeRef, bool unmanaged)
{
    add(unmanaged ? op_newarr_um : op_newarr, QVariant::fromValue(typeRef));
    delta(-1+1);
}

void Emitter::newobj_(const Quali& typeRef, bool unmanaged)
{
    add(unmanaged ? op_newobj_um : op_newobj, QVariant::fromValue(typeRef));
    delta(+1);
}

void Emitter::newtask_()
{
    add(op_newtask);
    delta(-1+1);
}

void Emitter::not_()
{
    add(op_not);
    delta(-1+1);
}

void Emitter::or_()
{
    add(op_or);
    delta(-2+1);
}

void Emitter::pcall_(const Quali& procRef, int argCount)
{
    add(op_pcall, QVariant::fromValue(procRef));
    delta(-argCount + 1); // exc
}

void Emitter::pcalli_(const Quali& procType, int argCount)
{
    add(op_pcalli, QVariant::fromValue(procType));
    delta(-argCount - 1 + 1); // args + procref, exc
}

void Emitter::pop_()
{
    add(op_pop);
    delta(-1);
}

void Emitter::raise_()
{
    add(op_raise);
    delta(-1);
}

void Emitter::refarg_(quint16 arg)
{
    add(op_refarg, arg);
    delta(+1);
}

void Emitter::refelem_(const Quali& typeRef)
{
    add(op_refelem, QVariant::fromValue(typeRef));
    delta(-2+1);
}

void Emitter::reffld_(const Trident& fieldRef)
{
    add(op_reffld, QVariant::fromValue(fieldRef));
    delta(-1+1);
}

void Emitter::refloc_(quint16 loc)
{
    add(op_refloc, loc);
    delta(+1);
}

void Emitter::refvar_(const Quali& varRef)
{
    add(op_refvar, QVariant::fromValue(varRef));
    delta(+1);
}

void Emitter::rem_(bool withUnsigned)
{
    add(withUnsigned ? op_rem_un : op_rem);
    delta(-2+1);
}

void Emitter::repeat_()
{
    add(op_REPEAT);
}

void Emitter::ret_(bool hasRet)
{
    add(op_ret);
    delta(hasRet ? -1 : 0);
}

void Emitter::shl_()
{
    add(op_shl);
    delta(-2+1);
}

void Emitter::shr_(bool withUnsigned)
{
    add(withUnsigned ? op_shr_un : op_shr);
    delta(-2+1);
}

void Emitter::sizeof_(const Quali& typeRef)
{
    add(op_sizeof, QVariant::fromValue(typeRef));
    delta(+1);
}

void Emitter::starg_(quint16 arg)
{
    add(op_starg, arg);
    delta(-1);
}

void Emitter::stelem_(const Quali& typeRef)
{
    add(op_stelem, QVariant::fromValue(typeRef));
    delta(-3);
}

void Emitter::stfld_(const Trident& fieldRef)
{
    add(op_stfld, QVariant::fromValue(fieldRef));
    delta(-2);
}

void Emitter::stind_(const Quali& typeRef)
{
    add(op_stind, QVariant::fromValue(typeRef));
    delta(-2);
}

void Emitter::stloc_(quint16 loc)
{
    add(op_stloc, loc);
    delta(-1);
}

void Emitter::stvar_(const Quali& varRef)
{
    add(op_stvar, QVariant::fromValue(varRef));
    delta(-1);
}

void Emitter::sub_()
{
    add(op_sub);
    delta(-2+1);
}

void Emitter::switch_()
{
    add(op_SWITCH);
    delta(-1);
}

void Emitter::taskdone_()
{
    add(op_taskdone);
    delta(-1+1);
}

void Emitter::then_()
{
    add(op_THEN);
}

void Emitter::transfer_()
{
    add(op_transfer);
    delta(-1);
}

void Emitter::until_()
{
    add(op_UNTIL);
}

void Emitter::while_()
{
    add(op_WHILE);
}

void Emitter::xor_()
{
    add(op_xor);
    delta(-2+1);
}
