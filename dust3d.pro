QT += core widgets opengl
CONFIG += debug

INCLUDEPATH += src

SOURCES += src/mainwindow.cpp
HEADERS += src/mainwindow.h

SOURCES += src/modelingwidget.cpp
HEADERS += src/modelingwidget.h

SOURCES += src/skeletoneditwidget.cpp
HEADERS += src/skeletoneditwidget.h

SOURCES += src/skeletoneditgraphicsview.cpp
HEADERS += src/skeletoneditgraphicsview.h

SOURCES += src/skeletoneditnodeitem.cpp
HEADERS += src/skeletoneditnodeitem.h

SOURCES += src/skeletoneditedgeitem.cpp
HEADERS += src/skeletoneditedgeitem.h

SOURCES += src/skeletontomesh.cpp
HEADERS += src/skeletontomesh.h

SOURCES += src/theme.cpp
HEADERS += src/theme.h

SOURCES += src/mesh.cpp
HEADERS += src/mesh.h

SOURCES += src/main.cpp

INCLUDEPATH += ../meshlite/include
LIBS += -L../meshlite/target/debug -lmeshlite

target.path = ./
INSTALLS += target