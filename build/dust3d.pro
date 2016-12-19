TARGET = dust3d
TEMPLATE = app

QT += widgets opengl

VPATH += ../src
INCLUDEPATH += ../src

SOURCES += main.cpp \
	mainwindow.cpp \
	render.cpp \
  vector3d.c \
  draw.cpp \
  array.c \
  bmesh.c \
  matrix.c

HEADERS += mainwindow.h \
	render.h \
  vector3d.h \
  draw.h \
  array.h \
  bmesh.h \
  matrix.h