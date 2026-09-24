#ifndef AIRASTRENDERER_H
#define AIRASTRENDERER_H

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

// Adapted from MilRenderer

#include <Algol60/AirRenderer.h>
#include <Algol60/AirAst.h>

namespace Air
{
    class AstRenderer : public AbstractRenderer
    {
    public:
        AstRenderer(AstModel*, bool runValidator = false); // TODO: set to true when Validator is available
        ~AstRenderer();

        void beginModule( const QByteArray& moduleName, const QString& sourceFile );
        void endModule();
        void addImport( const QByteArray& moduleName );
        void addVariable( const Quali& typeRef, const QByteArray& name, bool isPublic );
        void addConst( const Quali& typeRef, const QByteArray& name, const QVariant& val );
        void addProcedure( const ProcData& proc );
        void beginType( const QByteArray& name, bool isPublic, quint8 typeKind, const Quali& base );
        void endType();
        void addType( const QByteArray& name, bool isPublic, const Quali& baseType,
                      quint8 typeKind, quint32 len = 0 );
        void addField( const QByteArray& fieldName, const Quali& typeRef, bool isPublic = true );
        void line( const Alg::RowCol& );

        Declaration* getModule() const { return done; }

    protected:
        Type* derefType( const Quali& );
        Declaration* resolve( const Quali&, quint8 declKind = Declaration::NoMode );
        Declaration* resolve( const Trident&, quint8 declKind = Declaration::NoMode );
        Declaration* importOf( const QByteArray& moduleName );
        Declaration* stubOf( Declaration* module, const QByteArray& name, quint8 declKind );
        void resolveAll( bool reportErrors = false );
        void later( Declaration** slot, const QVariant& arg, quint8 declKind,
                    bool trident, int pc, const char* what );
        void resolveLater();
        Declaration* addDecl( quint8 declKind, const QByteArray& name, bool isPublic );
        Statement* translateStat( const QList<ProcData::Op>&, int& pc );
        Expression* translateExpr( const QList<ProcData::Op>&, int& pc );
        bool expect( const QList<ProcData::Op>&, int pc, quint8 op );
        void error( const QString&, int pc = -1 );
        Alg::RowCol setLine( quint32 packed );

    private:
        AstModel* mdl;
        Declaration* module;
        Declaration* done;
        Declaration* curProc;
        Type* curType;
        Alg::RowCol curPos;
        QList<Type*> unresolved;
        struct Pending // an instruction operand can refer to a declaration not yet seen
        {
            Declaration** slot;
            QVariant arg;
            Declaration* proc;
            const char* what;
            quint8 declKind;
            bool trident;
            int pc;
            Pending():slot(0),proc(0),what(0),declKind(0),trident(false),pc(0) {}
        };
        QList<Pending> pending;
        bool runValidator;
        bool toDelete;
    };
}

#endif // AIRASTRENDERER_H
