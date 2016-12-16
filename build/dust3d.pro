TARGET = dust3d
TEMPLATE = app

QT += widgets opengl

VPATH += ../src
INCLUDEPATH += ../src

SOURCES += main.cpp \
	mainwindow.cpp \
	glwidget.cpp \
  drawcommon.c \
  drawsphere.c

HEADERS += mainwindow.h \
	glwidget.h \
  drawcommon.h \
  drawsphere.h