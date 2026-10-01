#ifndef AIRLJGEN_H
#define AIRLJGEN_H

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

#include <Algol60/AirAst.h>
#include <Algol60/AirRenderer.h>
#include <LjTools/LuaJitBytecode2.h>

class QIODevice;

namespace Air
{
    class LjGen
    {
    public:
        typedef AbstractRenderer::Error Error;

        LjGen(AstModel*);
        ~LjGen();

        bool generate(Declaration* module, QIODevice* out, bool stripped = false);
        static QByteArray luaName(const QByteArray& airName);

        const QList<Error>& getErrors() const;

    private:
        class Imp;
        Imp* imp;
    };
}

#endif // AIRLJGEN_H
