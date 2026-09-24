#ifndef AIRASTSERIALIZER_H
#define AIRASTSERIALIZER_H

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

// Adapted from MilAstSerializer

#include <Algol60/AirAst.h>

namespace Air
{
    class AbstractRenderer;

    class AstSerializer
    {
    public:
        enum DbgInfo { None, RowsOnly, RowsAndCols };

        static bool render( AbstractRenderer*, const Declaration* module, DbgInfo = RowsAndCols );
        static bool render( AbstractRenderer*, const AstModel*, DbgInfo = RowsAndCols );
    };
}

#endif // AIRASTSERIALIZER_H
