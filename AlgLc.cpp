#include <QCoreApplication>
#include <QFile>
#include <QtDebug>
#include <QFileInfo>
#include <QDir>
#include <QElapsedTimer>
#include <QThread>
#include "AlgErrors.h"
#include "AlgParser.h"

static QStringList collectFiles( const QDir& dir )
{
    QStringList res;
    QStringList files = dir.entryList( QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name );

    //foreach( const QString& f, files )
    //    res += collectFiles( QDir( dir.absoluteFilePath(f) ) );

    files = dir.entryList( QStringList() << QString("*.alg")
                                           << QString("*.al") << QString("*.a60"),
                                           QDir::Files, QDir::Name );
    foreach( const QString& f, files )
    {
        res.append( dir.absoluteFilePath(f) );
    }
    return res;
}

static void dumpTree( Alg::SynTree* node, int level = 0)
{
    QByteArray str;
    if( node->d_tok.d_type == Alg::Tok_Invalid )
        level--;
    else if( node->d_tok.d_type < Alg::SynTree::R_First )
    {
        if( Alg::tokenTypeIsKeyword( node->d_tok.d_type ) )
            str = Alg::tokenTypeString(node->d_tok.d_type);
        else if( node->d_tok.d_type > Alg::TT_Specials )
            str = QByteArray("\"") + node->d_tok.d_val + QByteArray("\"");
        else
            str = QByteArray("\"") + node->d_tok.getString() + QByteArray("\"");

    }else
        str = Alg::SynTree::rToStr( node->d_tok.d_type );
    if( !str.isEmpty() )
    {
        str += QByteArray("\t") /* + QFileInfo(node->d_tok.d_sourcePath).baseName().toUtf8() +
                ":" */ + QByteArray::number(node->d_tok.d_lineNr) +
                ":" + QByteArray::number(node->d_tok.d_colNr);
        QByteArray ws;
        for( int i = 0; i < level; i++ )
            ws += "|  ";
        str = ws + str;
        qDebug() << str.data();
    }
    foreach( Alg::SynTree* sub, node->d_children )
        dumpTree( sub, level + 1 );
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    a.setOrganizationName("me@rochus-keller.ch");
    a.setOrganizationDomain("https://github.com/rochus-keller/Algol");
    a.setApplicationName("AlgLc");
    a.setApplicationVersion("2019-10-22");

    QTextStream out(stdout);
    out << "AlgLc version: " << a.applicationVersion() <<
                 " author: me@rochus-keller.ch  license: GPL" << endl;

    QStringList dirOrFilePaths;
    QString outPath;
    bool dump = false;
    QString ns;
    QString mod;
    const QStringList args = QCoreApplication::arguments();
    for( int i = 1; i < args.size(); i++ ) // arg 0 enthaelt Anwendungspfad
    {
        if(  args[i] == "-h" || args.size() == 1 )
        {
            out << "usage: AlgLc [options] sources" << endl;
            out << "  reads Algol60 sources (files or directories) and translates them to corresponding Lua code." << endl;
            out << "options:" << endl;
            out << "  -dst      dump syntax trees to files" << endl;
            out << "  -o=path   path where to save generated files (default like first source)" << endl;
            out << "  -ns=name  namespace for the generated files (default empty)" << endl;
            out << "  -mod=name directory of the generated files (default empty)" << endl;
            out << "  -h        display this information" << endl;
            return 0;
        }else if( args[i] == "-dst" )
            dump = true;
        else if( args[i].startsWith("-o=") )
            outPath = args[i].mid(3);
        else if( args[i].startsWith("-ns=") )
            ns = args[i].mid(4);
        else if( args[i].startsWith("-mod=") )
            mod = args[i].mid(5);
        else if( !args[ i ].startsWith( '-' ) )
        {
            dirOrFilePaths += args[ i ];
        }else
        {
            qCritical() << "error: invalid command line option " << args[i] << endl;
            return -1;
        }
    }
    if( dirOrFilePaths.isEmpty() )
    {
        qWarning() << "no file or directory to process; quitting (use -h option for help)" << endl;
        return -1;
    }

    QStringList files;
    foreach( const QString& path, dirOrFilePaths )
    {
        QFileInfo info(path);
        if( outPath.isEmpty() )
            outPath = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
        if( info.isDir() )
            files += collectFiles( info.absoluteFilePath() );
        else
            files << path;
    }

    Alg::Errors err;
    err.setReportToConsole(true);

    foreach( const QString& path, files )
    {
        qDebug() << "processing" << path;

        Alg::Lexer lex;
        lex.setStream(path);
        lex.setErrors(&err);
        lex.setIgnoreComments(true);
        lex.setPackComments(true);
    #if 0
        Alg::Token t = lex.nextToken();
        while( t.isValid() )
        {
            qDebug() << t.getString() << QString::fromUtf8(t.d_val);
            t = lex.nextToken();
        }
    #else
        Alg::Parser p(&lex,&err);
        p.Parse();
        if( dump )
            dumpTree( &p.d_root );
    #endif

    }
    if( err.getErrCount() == 0 && err.getWrnCount() == 0 )
        qDebug() << "successfully completed";
    else
        qDebug() << "completed with" << err.getErrCount() << "errors and" <<
                    err.getWrnCount() << "warnings";
    return 0;
}
