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

#include "AlgHighlighter.h"
#include "AlgLexer.h"
#include <QBuffer>
using namespace Alg;

static const char* s_reserved[] = {

    // https://public.support.unisys.com/aseries/docs/ClearPath-MCP-18.0/86000098-516/section-000026216.html
    "ABS", "ACCEPT", "ARCCOS", "ARCSIN", "ARCTAN", "ARCTAN2", "ARRAYSEARCH", "ATANH",
    "AVAILABLE", "BOOLEAN", "CABS", "CCOS", "CEXP", "CHANGEFILE", "CHECKPOINT",
    "CHECKSUM", "CLN", "CLOSE", "COMPILETIME", "COMPLEX", "CONJUGATE", "COS", "COSH",
    "COTAN", "CSIN", "CSQRT", "DABS", "DALPHA", "DAND", "DARCCOS", "DARCSIN", "DARCTAN",
    "DARCTAN2", "DCOS", "DCOSH", "DECIMAL", "DELINKLIBRARY", "DELTA", "DEQV", "DERF",
    "DERFC", "DEXP", "DGAMMA", "DIMP", "DINTEGER", "DINTEGERT", "DLGAMMA", "DLN", "DLOG",
    "DMAX", "DMIN", "DNABS", "DNORMALIZE", "DNOT", "DOR", "DOUBLE", "DREAL", "DROP",
    "DSCALELEFT", "DSCALERIGHT", "DSCALERIGHTT", "DSIN", "DSINH", "DSQRT", "DTAN",
    "DTANH", "ENTIER", "ERF", "ERFC", "EXP", "FIRST", "FIRSTONE", "FIRSTWORD", "FIX",
    "FREE", "GAMMA", "HAPPENED", "HEAD", "IMAG", "INTEGER", "INTEGERT", "ISVALID",
    "LENGTH", "LINENUMBER", "LINKLIBRARY", "LISTLOOKUP", "LN", "LNGAMMA", "LOCK Interlock",
    "LOCKSTATUS", "LOG", "MASKSEARCH", "MAX", "MESSAGESEARCHER", "MIN", "MLSACCEPT", "MLSDISPLAY",
    "MLSTRANSLATE", "NABS", "NORMALIZE", "OFFSET", "ONES", "OPEN", "POINTER", "POT", "PROCESSID",
    "RANDOM", "READ", "READLOCK", "READYCL", "REAL", "REMAININGCHARS", "REMOVEFILE", "REPEAT",
    "SCALELEFT", "SCALERIGHT", "SCALERIGHTF", "SCALERIGHTT", "SECONDWORD", "SEEK", "SETACTUALNAME",
    "SIGN", "SIN", "SINGLE", "SINH", "SIZE", "SPACE", "SQRT", "STRING", "TAIL", "TAKE", "TAN", "TANH",
    "THIS", "THISCL", "TIME", "TRANSLATE", "UNLOCK", "UNREADYCL", "USERDATA", "USERDATALOCATOR",
    "USERDATAREBUILD", "VALUE", "WAIT", "WAITANDRESET", "WRITE",

    // Report on I/O Procedures for Algol 60
    "INSYMBOL", "OUTSYMBOL", "LENGHT", "INREAL", "OUTREAL", "INARRAY", "OUTARRAY",

    // empirical, from examples
    "OUTSTRING", "OUTCHAR", "INPUT", "OUTPUT", "OUTINTEGER", "NEWLINE",
    "printsln", "prints", "printn", "outframe",
    0

};
Highlighter::Highlighter(QTextDocument* parent) :
    QSyntaxHighlighter(parent),d_enableExt(false)
{
    for( int i = 0; i < C_Max; i++ )
    {
        d_format[i].setFontWeight(QFont::Normal);
        d_format[i].setForeground(Qt::black);
        d_format[i].setBackground(Qt::transparent);
    }
    d_format[C_Num].setForeground(QColor(0, 153, 153));
    d_format[C_Str].setForeground(QColor(208, 16, 64));
    d_format[C_Cmt].setForeground(QColor(153, 153, 136));
    d_format[C_Kw].setForeground(QColor(68, 85, 136));
    d_format[C_Kw].setFontWeight(QFont::Bold);
    d_format[C_Op].setForeground(QColor(153, 0, 0));
    d_format[C_Op].setFontWeight(QFont::Bold);
    d_format[C_Type].setForeground(QColor(153, 0, 115));
    d_format[C_Type].setFontWeight(QFont::Bold);
    d_format[C_Pp].setForeground(QColor(0, 134, 179));
    d_format[C_Pp].setFontWeight(QFont::Bold);

    d_format[C_Section].setForeground(QColor(0, 128, 0));
    d_format[C_Section].setBackground(QColor(230, 255, 230));

    d_builtins = createBuiltins(true);
}

void Highlighter::setEnableExt(bool b)
{
    const bool old = d_enableExt;
    d_enableExt = b;
    if( old != b )
    {
        d_builtins = createBuiltins(d_enableExt);
        rehighlight();
    }
}

QTextCharFormat Highlighter::formatForCategory(int c) const
{
    return d_format[c];
}

QSet<QByteArray> Highlighter::createBuiltins(bool withLowercase)
{
    QSet<QByteArray> res;
    int i = 0;
    while( s_reserved[i] )
    {
        res << s_reserved[i];
        if( withLowercase )
            res << QByteArray(s_reserved[i]).toLower();
        i++;
    }
    return res;
}

void Highlighter::highlightBlock(const QString& text)
{
    static const QRegExp commentEnd("\\b(end|END|else|ELSE)\\b|;"); // any sequence not containing 'end' or ';' or 'else'

    const int previousBlockState_ = previousBlockState();
    int lexerState = 0, initialBraceDepth = 0;
    if (previousBlockState_ != -1) {
        lexerState = previousBlockState_ & 0xff;
        initialBraceDepth = previousBlockState_ >> 8;
    }

    int braceDepth = initialBraceDepth;


    int start = 0;
    if( lexerState > 0 )
    {
        // wir sind in einem Multi Line Comment
        // suche das Ende
        QTextCharFormat f = formatForCategory(C_Cmt);
        f.setProperty( TokenProp, int(Tok_Comment) );
        int pos = lexerState == 1 ? text.indexOf(";") : text.indexOf(commentEnd);
        if( pos == -1 )
        {
            // the whole block ist part of the comment
            setFormat( start, text.size(), f );
            setCurrentBlockState( (braceDepth << 8) | lexerState);
            return;
        }else
        {
            // End of Comment found; semi is not part of comment
            setFormat( start, pos , f );
            lexerState = 0;
            braceDepth--;
            start = pos;
        }
    }


    Alg::Lexer lex;
    lex.setIgnoreComments(false);
    lex.setPackComments(false);

    QList<Token> tokens =  lex.tokens(text.mid(start));
    for( int i = 0; i < tokens.size(); ++i )
    {
        Token &t = tokens[i];
        t.d_colNr += start;

        QTextCharFormat f;
        if( t.d_type == Tok_Comment )
        {
            f = formatForCategory(C_Cmt);
            if( i == tokens.size() - 1 )
            {
                // enable multi line comment
                // if there is another token following the comment then it cannot be multi line
                braceDepth++;
                if( i > 0 && tokens[i-1].d_type == Tok_COMMENT )
                    lexerState = 1;
                else
                    lexerState = 2; // after end comment
            }
        }/*else if( t.d_type == Tok_Semi && lexerState > 0 )
        {
            // not needed
            braceDepth--;
            f = formatForCategory(C_Op);
            lexerState = 0;
        }*/else if( t.d_type == Tok_string )
            f = formatForCategory(C_Str);
        else if( t.d_type == Tok_decimal_number || t.d_type == Tok_unsigned_integer )
            f = formatForCategory(C_Num);
        else if( tokenTypeIsLiteral(t.d_type) )
        {
            f = formatForCategory(C_Op);
        }else if( tokenTypeIsKeyword(t.d_type) )
        {
            f = formatForCategory(C_Kw);
        }else if( t.d_type == Tok_identifier )
        {
            if( i < tokens.size() - 1 && tokens[i+1].d_type == Tok_Colon )
                f = formatForCategory(C_Section);
            else if( d_builtins.contains(t.d_val) )
                f = formatForCategory(C_Type);
            else
                f = formatForCategory(C_Ident);
        }

        if( f.isValid() )
        {
            setFormat( t.d_colNr-1, t.d_len, f );
        }
    }

    setCurrentBlockState((braceDepth << 8) | lexerState );
}



