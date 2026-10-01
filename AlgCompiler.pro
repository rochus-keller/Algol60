QT       += core
QT       -= gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

QT       += core
QT       -= gui

TARGET = algc
TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle

INCLUDEPATH += ..

SOURCES += AlgCompiler.cpp \
    AlgAirGen.cpp

HEADERS += \
    AlgAirGen.h

include( Algol.pri )
include( AIR.pri )
include( AirLjGen.pri )

CONFIG(debug, debug|release) {
        DEFINES += _DEBUG
}

QMAKE_CXXFLAGS += -Wno-reorder -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable

