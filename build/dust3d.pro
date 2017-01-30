TARGET = dust3d
TEMPLATE = app

QT += widgets opengl
CONFIG += debug

VPATH += ../src
INCLUDEPATH += ../src

SOURCES += main.cpp \
	mainwindow.cpp \
	render.cpp \
  vector3d.c \
  draw.cpp \
  array.c \
  bmesh.c \
  matrix.c \
  convexhull.c \
  dict.c \
  osutil.cpp \
  subdivide.c \
  skinnedmesh.c \
  dmemory.c

HEADERS += mainwindow.h \
	render.h \
  vector3d.h \
  draw.h \
  array.h \
  bmesh.h \
  matrix.h \
  convexhull.h \
  dict.h \
  3dstruct.h \
  osutil.h \
  subdivide.h \
  skinnedmesh.h \
  dmemory.h