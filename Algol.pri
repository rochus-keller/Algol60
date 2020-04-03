#/*
#* Copyright 2020 Rochus Keller <mailto:me@rochus-keller.ch>
#*
#* This file is part of the Algol60 parser library.
#*
#* The following is the license that applies to this copy of the
#* library. For a license to use the library under conditions
#* other than those described here, please email to me@rochus-keller.ch.
#*
#* GNU General Public License Usage
#* This file may be used under the terms of the GNU General Public
#* License (GPL) versions 2.0 or 3.0 as published by the Free Software
#* Foundation and appearing in the file LICENSE.GPL included in
#* the packaging of this file. Please review the following information
#* to ensure GNU General Public Licensing requirements will be met:
#* http://www.fsf.org/licensing/licenses/info/GPLv2.html and
#* http://www.gnu.org/copyleft/gpl.html.
#*/

HEADERS += \
    $$PWD/AlgErrors.h \
    $$PWD/AlgFileCache.h \
    $$PWD/AlgLexer.h \
    $$PWD/AlgParser.h \
    $$PWD/AlgSynTree.h \
    $$PWD/AlgToken.h \
    $$PWD/AlgTokenType.h

SOURCES += \
    $$PWD/AlgErrors.cpp \
    $$PWD/AlgFileCache.cpp \
    $$PWD/AlgLexer.cpp \
    $$PWD/AlgParser.cpp \
    $$PWD/AlgSynTree.cpp \
    $$PWD/AlgToken.cpp \
    $$PWD/AlgTokenType.cpp
