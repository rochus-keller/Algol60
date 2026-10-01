INCLUDEPATH += .. ../LuaJIT-2.1/src

DEFINES += _LJTOOLS_DONT_CREATE_TAIL_CALLS

SOURCES += \
    ../LjTools/LuaJitBytecode2.cpp \
    ../LjTools/Engine2.cpp \
    ../LjTools/LuaJitComposer2.cpp \
    ../LjTools/LuaJitHelper.cpp \
    $$PWD/AirLjGen.cpp

HEADERS  += \
    ../LjTools/LuaJitBytecode2.h \
    ../LjTools/Engine2.h \
    ../LjTools/LuaJitComposer2.h \
    ../LjTools/LuaJitHelper.h \
    $$PWD/AirLjGen.h

win32 {
    LIBS += -L../LuaJIT-2.1/src -llua51
}
linux | macx {
    LIBS += $$absolute_path(../LuaJIT-2.1/src/libluajit.a, $$_PRO_FILE_PWD_)
    QMAKE_LFLAGS += -rdynamic -ldl
    #rdynamic is required so that the LjLibFfi functions are visible to LuaJIT FFI
}
