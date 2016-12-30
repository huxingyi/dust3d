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
  hashtable.c \
  osutil.cpp

HEADERS += mainwindow.h \
	render.h \
  vector3d.h \
  draw.h \
  array.h \
  bmesh.h \
  matrix.h \
  convexhull.h \
  hashtable.h \
  3dstruct.h \
  osutil.h