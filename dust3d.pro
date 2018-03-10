QT += core widgets opengl
CONFIG += debug

INCLUDEPATH += src

SOURCES += src/mainwindow.cpp
HEADERS += src/mainwindow.h

SOURCES += src/modelingwidget.cpp
HEADERS += src/modelingwidget.h

SOURCES += src/mesh.cpp
HEADERS += src/mesh.h

SOURCES += src/main.cpp

INCLUDEPATH += ../meshlite/include
LIBS += -L../meshlite/target/debug -lmeshlite

target.path = ./
INSTALLS += target