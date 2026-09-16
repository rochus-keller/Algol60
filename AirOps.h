#ifndef AIROPS_H
#define AIROPS_H

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

namespace Air
{

enum Op
{
    op_invalid,

    // expression instructions
    op_EXPRESSIONS,
    op_abs, op_add, op_and,
    op_call, op_calli, op_callinst, op_callmi, op_callvirt,
    op_castobj,
    op_ceq, op_cgt, op_cgt_un, op_clt, op_clt_un,
    op_conv_i1, op_conv_i2, op_conv_i4, op_conv_i8,
    op_conv_r4, op_conv_r8,
    op_conv_u1, op_conv_u2, op_conv_u4, op_conv_u8,
    op_curtask, op_div, op_div_un, op_dup,
    op_iif, op_isinst,
    op_ldarg, op_ldc_i4, op_ldc_i8, op_ldc_r4, op_ldc_r8,
    op_ldelem, op_ldfld, op_ldloc, op_ldmeth, op_ldnull,
    op_ldproc, op_ldstr, op_ldvar, op_ldvirt, op_len,
    op_mul, op_neg,
    op_newarr, op_newarr_um, op_newobj, op_newobj_um, op_newtask,
    op_not, op_or, op_pcall, op_pcalli,
    op_refarg, op_refelem, op_reffld, op_refloc, op_refvar,
    op_rem, op_rem_un, op_shl, op_shr, op_shr_un, op_sub,
    op_taskdone, op_xor,
    op_caddr, op_ldind, op_sizeof, // foreign profile

    // statements
    op_STATEMENTS,
    op_copy, op_exit, op_free, op_goto, op_label, op_pop, op_raise, op_ret,
    op_starg, op_stelem, op_stfld, op_stloc, op_stvar, op_transfer,
    op_stind, // foreign profile

    // structural, both in expressions and statements
    op_STRUCTURAL,
    op_IF, op_THEN, op_ELSE, op_END,
    op_LOOP, op_REPEAT, op_UNTIL, op_SWITCH, op_CASE, op_WHILE, op_DO,
    op_LINE,

    op_MAX
}; // update s_opName when this list changes

extern const char* s_opName[];

static inline bool isExprOp(Op op)
{
    return op > op_EXPRESSIONS && op < op_STATEMENTS;
}

static inline bool isStatOp(Op op)
{
    return op > op_STATEMENTS && op < op_STRUCTURAL;
}

static inline bool isStructuralOp(Op op)
{
    return op > op_STRUCTURAL && op < op_MAX;
}

}

#endif // AIROPS_H
