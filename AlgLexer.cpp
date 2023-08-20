/*
* Copyright 2020 Rochus Keller <mailto:me@rochus-keller.ch>
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

#include "AlgLexer.h"
#include "AlgErrors.h"
#include "AlgFileCache.h"
#include <QBuffer>
#include <QFile>
#include <QIODevice>
#include <ctype.h>
#include <QtDebug>
using namespace Alg;

QHash<QByteArray,QByteArray> Lexer::d_symbols;

Lexer::Lexer(QObject *parent) : QObject(parent),
    d_lastToken(Tok_Invalid),d_lineNr(0),d_colNr(0),d_in(0),d_err(0),d_fcache(0),
    d_ignoreComments(true), d_packComments(true),d_quotedKeywords(false)
{

}

void Lexer::setStream(QIODevice* in, const QString& sourcePath)
{
    if( in == 0 )
        setStream( sourcePath );
    else
    {
        if( d_in != 0 && d_in->parent() == this )
            d_in->deleteLater();
        d_in = in;
        d_lineNr = 0;
        d_colNr = 0;
        d_sourcePath = sourcePath;
        d_lastToken = Tok_Invalid;
    }
}

bool Lexer::setStream(const QString& sourcePath)
{
    QIODevice* in = 0;

    if( d_fcache )
    {
        bool found;
        QByteArray content = d_fcache->getFile(sourcePath, &found );
        if( found )
        {
            QBuffer* buf = new QBuffer(this);
            buf->setData( content );
            buf->open(QIODevice::ReadOnly);
            in = buf;
        }
    }

    if( in == 0 )
    {
        QFile* file = new QFile(sourcePath, this);
        if( !file->open(QIODevice::ReadOnly) )
        {
            if( d_err )
            {
                d_err->error(Errors::Lexer, sourcePath, 0, 0,
                                 tr("cannot open file from path %1").arg(sourcePath) );
            }
            delete file;
            return false;
        }
        in = file;
    }
    // else
    setStream( in, sourcePath );
    return true;
}

Token Lexer::nextToken()
{
    Token t;
    if( !d_buffer.isEmpty() )
    {
        t = d_buffer.first();
        d_buffer.pop_front();
    }else
        t = nextTokenImp();
    if( t.d_type == Tok_Comment && d_ignoreComments )
        t = nextToken();
    return t;
}

Token Lexer::peekToken(quint8 lookAhead)
{
    Q_ASSERT( lookAhead > 0 );
    while( d_buffer.size() < lookAhead )
    {
        Token t = nextTokenImp();
        while( t.d_type == Tok_Comment && d_ignoreComments )
            t = nextTokenImp();
        d_buffer.push_back( t );
    }
    return d_buffer[ lookAhead - 1 ];
}

QList<Token> Lexer::tokens(const QString& code)
{
    return tokens( code.toLatin1() );
}

QList<Token> Lexer::tokens(const QByteArray& code, const QString& path)
{
    QBuffer in;
    in.setData( code );
    in.open(QIODevice::ReadOnly);
    setStream( &in, path );

    QList<Token> res;
    Token t = nextToken();
    while( t.isValid() )
    {
        res << t;
        t = nextToken();
    }
    return res;
}

QByteArray Lexer::getSymbol(const QByteArray& str)
{
    if( str.isEmpty() )
        return str;
    QByteArray& sym = d_symbols[str];
    if( sym.isEmpty() )
        sym = str;
    return sym;
}

Token Lexer::nextTokenImp()
{
    if( d_in == 0 )
        return token(Tok_Eof);
    skipWhiteSpace();

    while( d_colNr >= d_line.size() )
    {
        if( d_in->atEnd() )
        {
            Token t = token( Tok_Eof, 0 );
            if( d_in->parent() == this )
                d_in->deleteLater();
            return t;
        }
        nextLine();
        skipWhiteSpace();
    }
    Q_ASSERT( d_colNr < d_line.size() );
    while( d_colNr < d_line.size() )
    {
        const QChar ch = d_line[d_colNr];

        if( ch == L'¬' )
            return token( Tok_Unot, 1, d_line.mid(d_colNr,1).toUtf8() );
        if( ch == L'×' )
            return token( Tok_Umul, 1, d_line.mid(d_colNr,1).toUtf8() );
        if( ch == L'÷' )
            return token( Tok_Udiv, 1, d_line.mid(d_colNr,1).toUtf8() );
        if( ch == L'↑' )
            return token( Tok_Uexp, 1, d_line.mid(d_colNr,1).toUtf8() );
        if( ch == L'∧' )
            return token( Tok_Uand, 1, d_line.mid(d_colNr,1).toUtf8() );
        if( ch == L'∨' )
            return token( Tok_Uor, 1, d_line.mid(d_colNr,1).toUtf8() );
        if( ch == L'≠' )
            return token( Tok_Uneq, 1, d_line.mid(d_colNr,1).toUtf8() );
        if( ch == L'≡' )
            return token( Tok_Ueq, 1, d_line.mid(d_colNr,1).toUtf8() );
        if( ch == L'≤' )
            return token( Tok_Uleq, 1, d_line.mid(d_colNr,1).toUtf8() );
        if( ch == L'≥' )
            return token( Tok_Ugeq, 1, d_line.mid(d_colNr,1).toUtf8() );
        if( ch == L'⊃' )
            return token( Tok_Uimpl, 1, d_line.mid(d_colNr,1).toUtf8() );
        if( ch == '"' || ch == L'‘' || ch == L'`' )
            return string();
        if( ch.isLetter() || ( ch == '\'' && lookAhead().isLetter() ) )
            // some source code embedds the keywords by ''
            return ident();
        if( ch.isDigit() || ch == '.' || ch == L'⏨' || ch == '#' ) // exponential_part starting with 'E' are not supported because ambiguity with ident
            return number();
        // else
        int pos = 0;
        const QString part = d_line.mid(d_colNr);
        TokenType tt = tokenTypeFromString(part.toUtf8(),&pos);

        /*if( tt == Tok_Latt )
            return comment();
        else if( d_enableExt && tt == Tok_2Slash )
        {
            const int len = d_line.size() - d_colNr;
            return token( Tok_Comment, len, d_line.mid(d_colNr,len) );
        }else */if( tt == Tok_Invalid || pos == 0 )
            return token( Tok_Invalid, 1, QString("unexpected character '%1' %2").arg(ch).arg(ch.unicode()).toUtf8() );
        else {
            return token( tt, pos, part.left(pos).toUtf8() );
        }
    }
    Q_ASSERT(false);
    return token(Tok_Invalid);
}

int Lexer::skipWhiteSpace()
{
    const int colNr = d_colNr;
    while( d_colNr < d_line.size() && d_line[d_colNr].isSpace() )
        d_colNr++;
    if( d_colNr == d_line.size() - 1 && d_line[d_colNr] == '\'' )
        d_colNr++; // some Algol compilers use ' to end lines
    return d_colNr - colNr;
}

void Lexer::nextLine()
{
    d_colNr = 0;
    d_lineNr++;
    d_line = d_in->readLine();

    if( d_line.endsWith("\r\n") )
        d_line.chop(2);
    else if( d_line.endsWith('\n') || d_line.endsWith('\r') || d_line.endsWith('\025') )
        d_line.chop(1);
}

QChar Lexer::lookAhead(int off) const
{
    if( int( d_colNr + off ) < d_line.size() )
    {
        return d_line[ d_colNr + off ];
    }else
        return QChar();
}

static bool pseudoKeyword(int t)
{
    switch(t)
    {
    case Tok_DIV:
    case Tok_MOD:
    case Tok_POWER:
    case Tok_EQUIV:
    case Tok_IMPL:
    case Tok_OR:
    case Tok_AND:
    case Tok_NOT:
    case Tok_LESS:
    case Tok_NOTGREATER:
    case Tok_EQUAL:
    case Tok_NOTLESS:
    case Tok_GREATER:
    case Tok_NOTEQUAL:
        return true;
    default:
        return false;
    }
}

Token Lexer::token(TokenType tt, int len, const QByteArray& val)
{
    QByteArray v = val;
    if( tt != Tok_Comment && tt != Tok_Invalid )
        v = getSymbol(v);
    Token t( tt, d_lineNr, d_colNr + 1, len, v );
    if( tt == Tok_Invalid )
    {
        if( len == 0 )
            len = 1;
        if( d_err != 0 )
            d_err->error(Errors::Syntax, t.d_sourcePath, t.d_lineNr, t.d_colNr, t.d_val );
    }else if( pseudoKeyword(tt) )
    {
        t.d_type = Tok_identifier;
        t.d_code = tt;
    }
    d_lastToken = t;
    d_colNr += len;
    t.d_sourcePath = d_sourcePath;
    return t;
}

static inline bool isAllLowerCase( const QString& str )
{
    for( int i = 0; i < str.size(); i++ )
    {
        if( !str[i].isLower() )
                return false;
    }
    return true;
}

Token Lexer::ident()
{
    const bool quotedKeyword = lookAhead(0) == '\'';
    int off = 1;
    while( true )
    {
        const QChar c = lookAhead(off);
        if( !c.isLetterOrNumber() &&
                c.unicode() != '_' // extension by RK not in the standard
                )
            break;
        else
            off++;
    }
    if( quotedKeyword && lookAhead(off++) != '\'' )
        return token( Tok_Invalid, off, "non-terminated quoted keyword" );

    const QString str = d_line.mid(d_colNr, off );
    Q_ASSERT( !str.isEmpty() );
    if( quotedKeyword )
        d_quotedKeywords = true;

    int pos = 0;
    QString keyword = str.toUpper(); // ignore case in keywords (unspecified)
    if(quotedKeyword)
    {
        keyword = keyword.mid(1,keyword.size()-2);
        if( keyword.isEmpty() )
            return token( Tok_Invalid, off, "empty quoted keyword" );
    }
    TokenType t = tokenTypeFromString( keyword.toUtf8(), &pos );
    if( t != Tok_Invalid && pos != keyword.size() )
        t = Tok_Invalid;
    if( quotedKeyword && t == Tok_Invalid )
        return token( Tok_Invalid, off, "invalid quoted keyword" );
    if( d_quotedKeywords && !quotedKeyword )
        t = Tok_Invalid; // we found an unquoted ident which looks like a keyword but in a file with quoted keywords.
    if( t == Tok_COMMENT )
        return comment();
    if( t == Tok_END )
    {
        const Token res = token(t,off);
        const Token cmt = comment2();
        if( cmt.isValid() && !d_ignoreComments )
            d_buffer.push_back( cmt );
        return res;
    }
    if( t != Tok_Invalid )
        return token( t, off );
    else
        return token( Tok_identifier, off, str.toUtf8() );
}

Token Lexer::number()
{
    int off = 0;
    while( true )
    {
        const QChar c = lookAhead(off);
        if( !c.isDigit() )
            break;
        else
            off++;
    }
    bool isReal = false;

    const int decflen = decimal_fraction(off);
    if( decflen > 0 )
    {
        isReal = true;
        off += decflen;
    }else if( decflen < 0 )
        return token( Tok_Invalid, 1, "invalid decimal_number" );

    const int explen = exponential_part(off);
    if( explen > 0 )
    {
        isReal = true;
        off += explen;
    }else if( explen < 0 )
        return token( Tok_Invalid, off, "invalid decimal_number" );

    const QString str = d_line.mid(d_colNr, off );
    Q_ASSERT( !str.isEmpty() );

    if( isReal)
        return token( Tok_decimal_number, off, str.toUtf8() );
    else
        return token( Tok_unsigned_integer, off, str.toUtf8() );
}

Token Lexer::comment()
{
    // COMMENT detected
    const int startLine = d_lineNr;
    const int startCol = d_colNr;
    static const int symLen = ::strlen("comment");

    if( !d_packComments )
        d_colNr += symLen;

    int semiPos = -1;
    QString str;
    while( semiPos == -1 )
    {
        semiPos = d_line.indexOf(";", d_colNr);
        if( semiPos != -1 )
        {
            semiPos += 1;
            if( !str.isEmpty() )
                str += '\n';
            str += d_line.mid(d_colNr,semiPos-d_colNr);
            break;
        }else
        {
            if( !str.isEmpty() )
                str += '\n';
            str += d_line.mid(d_colNr);
            if( d_in->atEnd() )
                break;
        }
        nextLine();
    }
    if( d_packComments && semiPos == -1 && d_in->atEnd() )
    {
        d_colNr = d_line.size();
        Token t( Tok_Invalid, startLine, startCol + 1, str.size(), tr("non-terminated comment").toLatin1() );
        if( d_err )
            d_err->error(Errors::Syntax, t.d_sourcePath, t.d_lineNr, t.d_colNr, t.d_val );
        return t;
    }
    // Col + 1 weil wir immer bei Spalte 1 beginnen, nicht bei Spalte 0
    Token t;
    if( d_packComments )
    {
        t = Token(Tok_Comment,startLine, startCol + 1, str.size(), str.toUtf8() );
        d_colNr += str.size();
        t.d_sourcePath = d_sourcePath;
        d_lastToken = t;
    }else
    {
        t = Token( Tok_COMMENT, startLine, startCol + 1, symLen );

        // also send Tok_Comment for empty strings because "comment" could be followed immediately by \n
        Token t2( Tok_Comment, startLine, startCol + 1 + symLen, str.size(), str.toUtf8() );
        t2.d_sourcePath = d_sourcePath;
        d_lastToken = t2;
        d_colNr += symLen + str.size();
        d_buffer.append( t2 );

        if( semiPos != -1 )
        {
            Token t(Tok_Semi,d_lineNr, semiPos - 1 + 1, 1 );
            t.d_sourcePath = d_sourcePath;
            d_lastToken = t;
            d_buffer.append( t );
            d_colNr = semiPos;
        }
    }
    return t;
}

Token Lexer::comment2()
{
    // passed END
    const int startLine = d_lineNr;
    const int startCol = d_colNr;

    QRegExp re("\\b(end|END|else|ELSE)\\b|;"); // any sequence not containing 'end' or ';' or 'else'

    int pos = -1;
    QString str;
    while( pos == -1 )
    {
        pos = d_line.indexOf(re, d_colNr);
        if( pos != -1 )
        {
            if( !str.isEmpty() )
                str += '\n';
            str += d_line.mid(d_colNr,pos-d_colNr);
            break;
        }else
        {
            if( !str.isEmpty() )
                str += '\n';
            str += d_line.mid(d_colNr);
            if( d_in->atEnd() )
                break;
        }
        nextLine();
    }
    if( pos == -1 && d_in->atEnd() )
        pos = d_line.size();

    // Col + 1 weil wir immer bei Spalte 1 beginnen, nicht bei Spalte 0
    Token t( ( str.isEmpty() ? Tok_Invalid : Tok_Comment ), startLine, startCol + 1, str.size(), str.toUtf8() );
    t.d_sourcePath = d_sourcePath;
    d_lastToken = t;
    d_colNr = pos;
    return t;
}

Token Lexer::string()
{
    const QChar other = lookAhead(0) == L'‘' ? L'’' : ( lookAhead(0) == L'`' ? L'\'' : L'"' );
    int off = 1;
    while( true )
    {
        const QChar c = lookAhead(off);
        off++;
        if( c == other )
            break;
        if( c == 0 )
            return token( Tok_Invalid, off, "non-terminated string" );
    }
    const QString str = d_line.mid(d_colNr, off );
    return token( Tok_string, off, str.toUtf8() );
}

int Lexer::exponential_part(int off)
{
    int origOff = off;
    QChar o1 = lookAhead(off);
    if( o1 == 'E' || o1 == 'e' || o1 == L'⏨' || o1 == '#' )
    {
        off++;
        QChar o = lookAhead(off);
        if( o == '+' || o == '-' )
        {
            off++;
            o = lookAhead(off);
        }
        if( !o.isDigit() )
            return -1; // token( Tok_Invalid, off, "invalid real" );
        while( true )
        {
            const QChar c = lookAhead(off);
            if( !c.isDigit() )
                break;
            else
                off++;
        }
    }
    return off - origOff;
}

int Lexer::decimal_fraction(int off)
{
    int origOff = off;
    QChar o1 = lookAhead(off);
    if( o1 == '.' )
    {
        off++;
        while( true )
        {
            const QChar c = lookAhead(off);
            if( !c.isDigit() )
                break;
            else
                off++;
        }
    }
    return off - origOff;
}

