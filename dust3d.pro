QT += core widgets opengl
CONFIG += debug
RESOURCES += resources.qrc

INCLUDEPATH += src

SOURCES += src/mainwindow.cpp
HEADERS += src/mainwindow.h

SOURCES += src/modelwidget.cpp
HEADERS += src/modelwidget.h

SOURCES += src/skeletoneditgraphicsview.cpp
HEADERS += src/skeletoneditgraphicsview.h

SOURCES += src/skeletoneditnodeitem.cpp
HEADERS += src/skeletoneditnodeitem.h

SOURCES += src/skeletoneditedgeitem.cpp
HEADERS += src/skeletoneditedgeitem.h

SOURCES += src/skeletontomesh.cpp
HEADERS += src/skeletontomesh.h

SOURCES += src/turnaroundloader.cpp
HEADERS += src/turnaroundloader.h

SOURCES += src/skeletonwidget.cpp
HEADERS += src/skeletonwidget.h

SOURCES += src/ds3file.cpp
HEADERS += src/ds3file.h

SOURCES += src/theme.cpp
HEADERS += src/theme.h

SOURCES += src/mesh.cpp
HEADERS += src/mesh.h

SOURCES += src/main.cpp

INCLUDEPATH += ../meshlite/include
LIBS += -L../meshlite/target/debug -lmeshlite

INCLUDEPATH += thirdparty/carve-1.4.0/include
INCLUDEPATH += thirdparty/carve-1.4.0/build/include
LIBS += -Lthirdparty/carve-1.4.0/build/lib -lcarve

INCLUDEPATH += /usr/local/opt/boost/include

INCLUDEPATH += /usr/local/opt/gmp/include
LIBS += -L/usr/local/opt/gmp/lib -lgmp

INCLUDEPATH += /usr/local/opt/mpfr/include
LIBS += -L/usr/local/opt/mpfr/lib -lmpfr

INCLUDEPATH += /usr/local/opt/cgal/include
LIBS += -L/usr/local/opt/cgal/lib -lCGAL

target.path = ./
INSTALLS += target