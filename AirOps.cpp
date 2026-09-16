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

#include <Algol60/AirOps.h>

const char* Air::s_opName[] = {
    "?invalid?",

    "?expressions?",
    "abs", "add", "and",
    "call", "calli", "callinst", "callmi", "callvirt",
    "castobj",
    "ceq", "cgt", "cgt_un", "clt", "clt_un",
    "conv_i1", "conv_i2", "conv_i4", "conv_i8",
    "conv_r4", "conv_r8",
    "conv_u1", "conv_u2", "conv_u4", "conv_u8",
    "curtask", "div", "div_un", "dup",
    "IIF", "isinst",
    "ldarg", "ldc_i4", "ldc_i8", "ldc_r4", "ldc_r8",
    "ldelem", "ldfld", "ldloc", "ldmeth", "ldnull",
    "ldproc", "ldstr", "ldvar", "ldvirt", "len",
    "mul", "neg",
    "newarr", "newarr_um", "newobj", "newobj_um", "newtask",
    "not", "or", "pcall", "pcalli",
    "refarg", "refelem", "reffld", "refloc", "refvar",
    "rem", "rem_un", "shl", "shr", "shr_un", "sub",
    "taskdone", "xor",
    "caddr", "ldind", "sizeof",

    "?statements?",
    "copy", "exit", "free", "goto", "label", "pop", "raise", "ret",
    "starg", "stelem", "stfld", "stloc", "stvar", "transfer",
    "stind",

    "?structural?",
    "IF", "THEN", "ELSE", "END",
    "LOOP", "REPEAT", "UNTIL", "SWITCH", "CASE", "WHILE", "DO",
    "LINE",

    "?max?"
};
