#ifndef AIRRENDERER_H
#define AIRRENDERER_H

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

#include <QByteArray>
#include <QPair>
#include <QList>
#include <QVariant>
#include <QIODevice>
#include <QTextStream>
#include <Algol60/AlgRowCol.h>

// adopted from MilRenderer

namespace Air
{
    typedef QPair<QByteArray,QByteArray> Quali; // [module '!'] element
    typedef QPair<Quali,QByteArray> Trident;    // qualident '.' element

    struct EmiTypes // types used in the Emitter and Renderer interface
    {
        enum Basic { Undefined, BOOL, CHAR, INT8, INT16, INT32, INT64,
                     UINT8, UINT16, UINT32, UINT64, FLOAT32, FLOAT64, TASK, ANYREC, MaxBasic };
        enum TypeKind { Invalid, Alias, Pointer, Array, Struct, Record,
                        ProcType, BoundProcType, CStruct, CUnion, CArray, CPointer, MaxTypeKind };

        static const char* basicName(quint8);
    };

    struct ProcData
    {
        struct Op
        {
            quint8 op;
            QVariant arg;
            Op():op(0){}
            Op(quint8 airop, const QVariant& arg = QVariant()):op(airop),arg(arg){}
        };

        struct Var
        {
            QByteArray name;
            Quali type;
            quint32 line;
            uint isPublic : 1;
            uint isVarParam : 1;
            Var():line(0),isPublic(0),isVarParam(0) {}
            Var(const Quali& type, const QByteArray& name, quint32 line):
                name(name),type(type),line(line),isPublic(0),isVarParam(0) {}
        };

        enum Kind { Invalid, Normal, Init, Extern, Foreign, ProcType, BoundProcType };
        uint kind : 4;
        uint isPublic : 1;
        quint32 endLine;
        QByteArray name;
        QByteArray binding; // if Normal and not empty, the first param is the receiver;
                            // for Extern and Foreign the external name
        QList<Op> body;
        QList<Var> params;
        QList<Var> locals;
        Quali retType;
        ProcData():kind(Invalid),isPublic(0),endLine(0) {}
    };

    typedef QPair<qint64,qint64> CaseLabel; // single label if both are equal, else a range
    typedef QList<CaseLabel> CaseLabelList;

    class AbstractRenderer
    {
    public:
        struct Error
        {
            QString msg;
            QByteArray where;
            quint32 pc;
            Error():pc(0){}
        };
        QList<Error> errors;

        virtual ~AbstractRenderer() {}

        virtual void beginModule( const QByteArray& moduleName, const QString& sourceFile ) {}
        virtual void endModule() {}

        virtual void addImport( const QByteArray& moduleName ) {}

        virtual void addVariable( const Quali& typeRef, const QByteArray& name, bool isPublic ) {}
        virtual void addConst( const Quali& typeRef, const QByteArray& name, const QVariant& val ) {}
        virtual void addProcedure( const ProcData& proc ) {} // also ProcType and BoundProcType

        virtual void beginType( const QByteArray& name, bool isPublic, quint8 typeKind,
                                const Quali& base = Quali() ) {} // only Struct, Record, CStruct, CUnion
        virtual void endType() {}
        virtual void addType( const QByteArray& name, bool isPublic, const Quali& baseType,
                              quint8 typeKind, quint32 len = 0 ) {} // only Alias, Pointer, Array, CArray, CPointer

        virtual void addField( const QByteArray& fieldName, const Quali& typeRef, bool isPublic = true ) {}

        virtual void line( const Alg::RowCol& ) {}
    };

    class AsmRenderer : public AbstractRenderer
    {
    public:
        AsmRenderer(QIODevice*, bool renderLineInfo = true, bool renderColumns = false);

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

        static QByteArray formatNumber( const QVariant& );
        static QByteArray formatString( const QByteArray& );
        static QByteArray toString( const Quali& );
        static QByteArray toString( const Trident& );

    protected:
        enum Section { None, ConstSection, TypeSection, VarSection };
        void section(quint8);
        void render( const ProcData& );
        void renderBody( const ProcData& );
        void renderFields();
        QByteArray fieldList() const;
        QByteArray shortName( const Quali& ) const;
        QByteArray shortName( const Trident& ) const;
        QByteArray argName( const ProcData&, const QVariant&, bool local ) const;
        QByteArray formals( const ProcData& ) const;
        void emit_( const QByteArray& text, bool ownLine = false );
        void flush();
        QByteArray lineout();
        inline QByteArray ws() const { return QByteArray(level*3,' '); }

    private:
        enum State { Idle, Module, Struct, Proc };
        QTextStream out;
        QList<ProcData::Var> fields;
        QByteArray moduleName;
        QByteArray pending; // instructions collected for the current output line
        QByteArray typeName;
        Quali typeBase;
        quint8 state;
        quint8 curSection;
        quint8 typeKind;
        int level;
        Alg::RowCol curPos, lastPos;
        bool typeIsPublic;
        bool renderLineInfo, renderColumns;
    };

    class RenderSplitter : public AbstractRenderer
    {
    public:
        RenderSplitter(const QList<AbstractRenderer*>& r = QList<AbstractRenderer*>()):renderer(r) {}

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
    private:
        QList<AbstractRenderer*> renderer;
    };
}

Q_DECLARE_METATYPE(Air::Quali)
Q_DECLARE_METATYPE(Air::Trident)
Q_DECLARE_METATYPE(Air::CaseLabelList)

#endif // AIRRENDERER_H
