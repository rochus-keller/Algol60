/*
* Copyright 2026 Rochus Keller <mailto:me@rochus-keller.ch>
*
* This file is part of the Algol60 compiler application.
*
* The following is the license that applies to this copy of the
* application. For a license to use the application under conditions
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

#include <QCoreApplication>
#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QtDebug>
#include <Algol60/AlgLexer.h>
#include <Algol60/AlgParser2.h>
#include <Algol60/AlgAst.h>
#include <Algol60/AlgValidator.h>
#include <Algol60/AlgAirGen.h>
#include <Algol60/AirRenderer.h>
#include <Algol60/AirAstRenderer.h>
#include <Algol60/AirAstSerializer.h>
#include <Algol60/AirValidator.h>
#include <Algol60/AirLjGen.h>
#include <LjTools/Engine2.h>

struct Options
{
    enum Action { Run, Compile, Check };
    Action action;
    QStringList sources;
    QString outFile;
    QString outDir;
    QString runtimeDir;
    QStringList luaPath;
    QSet<QByteArray> dumps; // air, lua, ljbc
    bool verbose;
    bool quiet;
    bool debug;
    Options():action(Run),verbose(false),quiet(false),debug(true) {}
};

class Lex : public Alg::Scanner
{
public:
    Alg::Lexer lex;
    Alg::Token next() { return lex.nextToken(); }
    Alg::Token peek(int offset) { return lex.peekToken(offset); }
    QString source() const { return lex.sourcePath(); }
};

static void usage(QTextStream& out)
{
    out << "usage: algc [options] sources" << endl;
    out << "  compiles Algol 60 sources to LuaJIT bytecode and runs them" << endl;
    out << "options:" << endl;
    out << "  -c, --compile    compile only, don't run" << endl;
    out << "      --check      parse and validate only" << endl;
    out << "      --run        compile and run (default)" << endl;
    out << "  -o, --output f   write the generated file to f" << endl;
    out << "      --out-dir d  write generated files to d (default: next to the source)" << endl;
    out << "      --dump k     also write a k=air|lua|ljbc file of each module" << endl;
    out << "  -g               generate debug information (default)" << endl;
    out << "      --strip      don't generate debug information" << endl;
    out << "      --runtime d  directory of AirRt.lua and Algol60Rt.lua" << endl;
    out << "  -L d             add d to the Lua module search path" << endl;
    out << "      --verbose    report each processing step" << endl;
    out << "  -q, --quiet      report errors only" << endl;
    out << "  -h, --help       display this information" << endl;
}

static QString outPathOf(const Options& o, const QString& source, const QString& suffix)
{
    if( !o.outFile.isEmpty() )
        return o.outFile;
    const QFileInfo info(source);
    const QString dir = o.outDir.isEmpty() ? info.absolutePath() : o.outDir;
    return QDir(dir).absoluteFilePath(info.fileName() + suffix);
}

static bool writeFile(const QString& path, const QByteArray& data)
{
    QFile f(path);
    if( !f.open(QIODevice::WriteOnly) )
    {
        qCritical() << "cannot open for writing:" << path;
        return false;
    }
    f.write(data);
    return true;
}

// runs the front end and returns the AIR of the module, or zero on error
static bool frontend(const Options& o, const QString& source, Air::AstModel& airMdl,
                     Air::AstRenderer& ar, QByteArray& airText)
{
    Lex lex;
    lex.lex.setStream(source);
    lex.lex.setIgnoreComments(true);
    lex.lex.setPackComments(true);

    Alg::AstModel mdl;
    Alg::Parser2 p(&lex, &mdl);
    Alg::Declaration* module = p.RunParser();
    if( !p.errors.isEmpty() )
    {
        foreach( const Alg::Parser2::Error& e, p.errors )
            qCritical() << e.path << e.pos.d_row << e.pos.d_col << e.msg;
        return false;
    }

    Alg::Validator v(&mdl);
    if( !v.validate(module) )
    {
        foreach( const Alg::Validator::Error& e, v.errors )
            qCritical() << "validator:" << e.path << e.pos.d_row << e.pos.d_col << e.msg;
        return false;
    }

    QBuffer buf(&airText);
    buf.open(QIODevice::WriteOnly);
    Air::AsmRenderer text(&buf);
    Alg::AirGen::generateRuntime(&ar); // so that the imports resolve
    QList<Air::AbstractRenderer*> rr;
    rr << &text << &ar;
    Air::RenderSplitter s(rr);
    Alg::AirGen g(&mdl);
    const bool res = g.generate(module, &s);
    buf.close();
    if( !res )
    {
        foreach( const Alg::AirGen::Error& e, g.errors )
            qCritical() << "airgen:" << e.path << e.pos.d_row << e.pos.d_col << e.msg;
        return false;
    }
    if( !ar.errors.isEmpty() )
    {
        foreach( const Air::AbstractRenderer::Error& e, ar.errors )
            qCritical() << "air:" << e.where << e.pc << e.msg;
        return false;
    }

    Air::Validator av(&airMdl);
    if( !av.validate(ar.getModule()) )
    {
        foreach( const Air::Validator::Error& e, av.errors )
            qCritical() << "air validator:" << e.where << e.pc << e.msg;
        return false;
    }
    return true;
}

static bool run(const Options& o, const QByteArray& code, const QString& source,
                const QStringList& path)
{
    Lua::Engine2 lua;
    Lua::Engine2::setInst(&lua);
    lua.addStdLibs();
    lua.addLibrary(Lua::Engine2::PACKAGE);
    lua.addLibrary(Lua::Engine2::IO);
    lua.addLibrary(Lua::Engine2::OS);
    lua.addLibrary(Lua::Engine2::BIT);
    lua.addLibrary(Lua::Engine2::JIT);
    lua.addLibrary(Lua::Engine2::DBG); // the runtime uses debug.getinfo for fault tracebacks
    lua.setPrintToStdout(true);

    QByteArray prelude = "package.path = '";
    foreach( const QString& p, path )
        prelude += QDir(p).absolutePath().toUtf8() + "/?.lua;";
    prelude += "' .. package.path";
    if( !lua.executeCmd(prelude,"prelude") )
    {
        qCritical() << "cannot set the module search path:" << lua.getLastError();
        return false;
    }

    if( !lua.executeCmd(code, source.toUtf8()) )
    {
        qCritical() << lua.getLastError().constData();
        return false;
    }
    return true;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    a.setApplicationName("algc");
    a.setApplicationVersion("2026-10-08");

    QTextStream out(stdout);

    Options o;
    const QStringList args = QCoreApplication::arguments();
    for( int i = 1; i < args.size(); i++ )
    {
        const QString arg = args[i];
        if( arg == "-h" || arg == "--help" )
        {
            usage(out);
            return 0;
        }else if( arg == "-c" || arg == "--compile" )
            o.action = Options::Compile;
        else if( arg == "--check" )
            o.action = Options::Check;
        else if( arg == "--run" )
            o.action = Options::Run;
        else if( arg == "-g" )
            o.debug = true;
        else if( arg == "--strip" )
            o.debug = false;
        else if( arg == "--verbose" )
            o.verbose = true;
        else if( arg == "-q" || arg == "--quiet" )
            o.quiet = true;
        else if( ( arg == "-o" || arg == "--output" ) && i+1 < args.size() )
            o.outFile = args[++i];
        else if( arg == "--out-dir" && i+1 < args.size() )
            o.outDir = args[++i];
        else if( arg == "--runtime" && i+1 < args.size() )
            o.runtimeDir = args[++i];
        else if( arg == "-L" && i+1 < args.size() )
            o.luaPath << args[++i];
        else if( arg == "--dump" && i+1 < args.size() )
            o.dumps.insert(args[++i].toUtf8());
        else if( !arg.startsWith('-') )
            o.sources << arg;
        else
        {
            qCritical() << "invalid command line option:" << arg;
            return -1;
        }
    }

    if( o.sources.isEmpty() )
    {
        usage(out);
        return -1;
    }
    if( o.runtimeDir.isEmpty() )
        o.runtimeDir = QCoreApplication::applicationDirPath();
    o.luaPath << o.runtimeDir;

    int errors = 0;
    foreach( const QString& source, o.sources )
    {
        if( o.verbose )
            out << "processing " << source << endl;

        Air::AstModel airMdl;
        Air::AstRenderer ar(&airMdl);
        QByteArray airText;
        if( !frontend(o, source, airMdl, ar, airText) )
        {
            errors++;
            continue;
        }
        if( o.dumps.contains("air") )
            writeFile(outPathOf(o, source, ".air"), airText);
        if( o.action == Options::Check )
            continue;

        Air::Declaration* module = ar.getModule();
        QStringList path = o.luaPath;
        path << QFileInfo(source).absolutePath();

        QByteArray code;
        QBuffer buf(&code);
        buf.open(QIODevice::WriteOnly);
        Air::LjGen g(&airMdl);
        const bool res = g.generate(module, &buf, !o.debug);
        buf.close();
        foreach( const Air::LjGen::Error& e, g.getErrors() )
            qCritical() << "ljgen:" << e.where << e.pc << e.msg;
        if( !res )
        {
            errors++;
            continue;
        }
        if( o.action == Options::Compile || o.dumps.contains("ljbc") )
            writeFile(outPathOf(o, source, ".ljbc"), code);
        if( o.action == Options::Run )
        {
            if( !run(o, code, source, path) )
                errors++;
        }
    }

    if( !o.quiet && o.sources.size() > 1 )
        out << o.sources.size() - errors << " of " << o.sources.size()
            << " files processed without error" << endl;
    return errors ? -1 : 0;
}
