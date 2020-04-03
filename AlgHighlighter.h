#ifndef ALGHIGHLIGHTER_H
#define ALGHIGHLIGHTER_H

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

#include <QSyntaxHighlighter>
#include <QSet>

namespace Alg
{
    class Highlighter : public QSyntaxHighlighter
    {
    public:
        enum { TokenProp = QTextFormat::UserProperty };
        explicit Highlighter(QTextDocument *parent = 0);
        void setEnableExt( bool b );

    protected:
        QTextCharFormat formatForCategory(int) const;
        static QSet<QByteArray> createBuiltins(bool withLowercase = false);

        // overrides
        void highlightBlock(const QString &text);

    private:
        enum Category { C_Num, C_Str, C_Kw, C_Type, C_Ident, C_Op, C_Pp, C_Cmt, C_Section, C_Brack, C_Max };
        QTextCharFormat d_format[C_Max];
        QSet<QByteArray> d_builtins;
        bool d_enableExt; // Allow for both uppercase and lowercase keywords and for idents with underscores as in C
    };
}

#endif // ALGHIGHLIGHTER_H
