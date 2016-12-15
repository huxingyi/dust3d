TARGET = dust3d
TEMPLATE = app

QT += widgets opengl

VPATH += ../src
INCLUDEPATH += ../src

SOURCES += main.cpp \
	mainwindow.cpp \
	glwidget.cpp

HEADERS += mainwindow.h \
	glwidget.h