TEMPLATE = lib

CONFIG += release
DEFINES += NDEBUG

INCLUDEPATH += ./
INCLUDEPATH += thirdparty/libigl/include
INCLUDEPATH += thirdparty/eigen
INCLUDEPATH += thirdparty/squeezer

SOURCES += simpleuv/uvunwrapper.cpp
HEADERS += simpleuv/uvunwrapper.h

SOURCES += simpleuv/parametrize.cpp
HEADERS += simpleuv/parametrize.h

SOURCES += simpleuv/chartpacker.cpp
HEADERS += simpleuv/chartpacker.h

SOURCES += simpleuv/triangulate.cpp
HEADERS += simpleuv/triangulate.h

HEADERS += simpleuv/meshdatatype.h

SOURCES += thirdparty/squeezer/maxrects.c
HEADERS += thirdparty/squeezer/maxrects.h

QMAKE_CXXFLAGS += -std=c++11

target.path = ./
INSTALLS += target
