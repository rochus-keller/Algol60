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

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = AlgLjEditor
TEMPLATE = app

INCLUDEPATH += .. ../LjTools/luajit-2.0

SOURCES += AlgLjEditor.cpp \
    ../GuiTools/CodeEditor.cpp \
    ../LjTools/LuaJitBytecode.cpp \
    ../LjTools/Engine2.cpp \
    ../LjTools/Terminal2.cpp \
    ../LjTools/ExpressionParser.cpp \
    ../LjTools/BcViewer.cpp \
    ../LjTools/LuaJitEngine.cpp \
    ../LjTools/LuaJitComposer.cpp \
    AlgHighlighter.cpp \
    ../LjTools/BcViewer2.cpp \
    ../LjTools/LjDisasm.cpp

HEADERS  += AlgLjEditor.h \
    ../GuiTools/CodeEditor.h \
    ../LjTools/LuaJitBytecode.h \
    ../LjTools/Engine2.h \
    ../LjTools/Terminal2.h \
    ../LjTools/ExpressionParser.h \
    ../LjTools/BcViewer.h \
    ../LjTools/LuaJitEngine.h \
    ../LjTools/LuaJitComposer.h \
    AlgHighlighter.h \
    ../LjTools/BcViewer2.h \
    ../LjTools/LjDisasm.h

include( ../LuaJIT/src/LuaJit.pri ){
    LIBS += -ldl
} else {
    LIBS += -lluajit
}

include( Algol.pri )
include( ../GuiTools/Menu.pri )

CONFIG(debug, debug|release) {
        DEFINES += _DEBUG
}

!win32 {
    QMAKE_CXXFLAGS += -Wno-reorder -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable
}

