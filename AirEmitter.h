#ifndef AIREMITTER_H
#define AIREMITTER_H

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

// Adapted from MilEmitter

#include <Algol60/AirRenderer.h>

namespace Air
{
    // this is a simplified backend API for frontend implementation so that the frontend
    // doesn't have to care about the AIR AST

    class Emitter
    {
    public:
        enum DbgInfo { None, RowsOnly, RowsAndCols };
        Emitter(AbstractRenderer*, DbgInfo = RowsOnly);

        void beginModule( const QByteArray& moduleName, const QString& sourceFile, const Alg::RowCol& );
        void endModule( const Alg::RowCol& );

        void addImport( const QByteArray& moduleName, const Alg::RowCol& );

        void addVariable( const Quali& typeRef, const QByteArray& name, const Alg::RowCol&, bool isPublic = true );
        void addConst( const Quali& typeRef, const QByteArray& name, const Alg::RowCol&, const QVariant& val );

        void beginType( const QByteArray& name, const Alg::RowCol&, bool isPublic = true,
                        quint8 typeKind = EmiTypes::Struct, const Quali& base = Quali() );
            // use for Struct, Record, ProcType, BoundProcType, CStruct, CUnion
            // supports addField, addArgument, setReturnType
        void endType();

        void addType( const QByteArray& name, const Alg::RowCol&, bool isPublic, const Quali& baseType,
                      quint8 typeKind = EmiTypes::Alias, quint32 len = 0 );
            // use for Alias, Pointer, Array, CArray, CPointer

        void beginProc( const QByteArray& procName, const Alg::RowCol&, bool isPublic = true,
                        quint8 kind = ProcData::Normal, const QByteArray& binding = QByteArray() );
            // binding is the record type for bound procedures, the external name for Extern and Foreign
        void endProc( const Alg::RowCol& );
        void discardProc();

        void addField( const QByteArray& fieldName, const Alg::RowCol&, const Quali& typeRef, bool isPublic = true );
        quint32 addLocal( const Quali& typeRef, const QByteArray& name, const Alg::RowCol& = Alg::RowCol() );
        quint32 addArgument( const Quali& typeRef, const QByteArray& name,
                             bool isVarParam = false, const Alg::RowCol& = Alg::RowCol() ); // the receiver is explicit
        void setReturnType( const Quali& typeRef );

        quint16 maxStackDepth() const { return d_maxStackDepth; }

        static QByteArray basicType(EmiTypes::Basic);
        static Quali basicTypeRef(EmiTypes::Basic);
        static QByteArray toString(const Quali&);
        static QByteArray toString(const Trident&);

        void abs_();
        void add_();
        void and_();
        void caddr_();
        void call_( const Quali& procRef, int argCount = 0, bool hasRet = false );
        void calli_( const Quali& procType, int argCount = 0, bool hasRet = false );
        void callinst_( const Trident& procRef, int argCount = 0, bool hasRet = false );
        void callmi_( const Quali& procType, int argCount = 0, bool hasRet = false );
        void callvirt_( const Trident& procRef, int argCount = 0, bool hasRet = false );
        void case_( const CaseLabelList& );
        void castobj_( const Quali& typeRef );
        void ceq_();
        void cgt_( bool withUnsigned = false );
        void clt_( bool withUnsigned = false );
        void conv_( EmiTypes::Basic );
        void copy_( const Quali& typeRef );
        void curtask_();
        void div_( bool withUnsigned = false );
        void do_();
        void dup_();
        void else_();
        void end_();
        void exit_();
        void free_();
        void goto_( const QByteArray& label );
        void if_();
        void iif_();
        void isinst_( const Quali& typeRef );
        void label_( const QByteArray& name );
        void ldarg_( quint16 arg );
        void ldc_i4( qint32 );
        void ldc_i8( qint64 );
        void ldc_r4( double );
        void ldc_r8( double );
        void ldelem_( const Quali& typeRef );
        void ldfld_( const Trident& fieldRef );
        void ldind_( const Quali& typeRef );
        void ldloc_( quint16 );
        void ldmeth_( const Trident& procRef );
        void ldnull_();
        void ldproc_( const Quali& procRef );
        void ldstr_( const QByteArray& str );
        void ldvar_( const Quali& varRef );
        void ldvirt_( const Trident& procRef );
        void len_();
        void line_( const Alg::RowCol& );
        void loop_();
        void mul_();
        void neg_();
        void newarr_( const Quali& typeRef, bool unmanaged = false );
        void newobj_( const Quali& typeRef, bool unmanaged = false );
        void newtask_();
        void not_();
        void or_();
        void pcall_( const Quali& procRef, int argCount = 0 );
        void pcalli_( const Quali& procType, int argCount = 0 );
        void pop_();
        void raise_();
        void refarg_( quint16 arg );
        void refelem_( const Quali& typeRef );
        void reffld_( const Trident& fieldRef );
        void refloc_( quint16 );
        void refvar_( const Quali& varRef );
        void rem_( bool withUnsigned = false );
        void repeat_();
        void ret_( bool hasRet );
        void shl_();
        void shr_( bool withUnsigned = false );
        void sizeof_( const Quali& typeRef );
        void starg_( quint16 arg );
        void stelem_( const Quali& typeRef );
        void stfld_( const Trident& fieldRef );
        void stind_( const Quali& typeRef );
        void stloc_( quint16 );
        void stvar_( const Quali& varRef );
        void sub_();
        void switch_();
        void taskdone_();
        void then_();
        void transfer_();
        void until_();
        void while_();
        void xor_();

    protected:
        void delta(int d);
        void add(quint8 op, const QVariant& arg = QVariant());
        void lineout(const Alg::RowCol&);
        quint32 lineset(const Alg::RowCol&);

    private:
        quint8 d_typeKind;
        quint16 d_stackDepth;
        quint16 d_maxStackDepth;
        QList<ProcData> d_proc; // proc stack
        QList<ProcData::Op>* ops;
        AbstractRenderer* d_out;
        quint32 lastLine, firstLine;
        quint8 dbgInfo;
    };
}

#endif // AIREMITTER_H
