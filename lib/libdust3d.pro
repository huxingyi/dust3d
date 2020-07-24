TEMPLATE = lib

VPATH += ../

SOURCE_ROOT = ../

CONFIG += skip_target_version_ext

include(../dust3d.pro)

DEFINES += DUST3D_EXPORTING

INCLUDEPATH += include

SOURCES += src/libdust3d.cpp
HEADERS += include/dust3d.h

for(path, INCLUDEPATH) {
    PREFIXED_INCLUDEPATH += "../$$path"
}

INCLUDEPATH += $$PREFIXED_INCLUDEPATH