QT += core widgets opengl
CONFIG += debug
RESOURCES += resources.qrc

include(thirdparty/QtAwesome/QtAwesome/QtAwesome.pri)

INCLUDEPATH += src

SOURCES += src/modelshaderprogram.cpp
HEADERS += src/modelshaderprogram.h

SOURCES += src/modelmeshbinder.cpp
HEADERS += src/modelmeshbinder.h

SOURCES += src/modelofflinerender.cpp
HEADERS += src/modelofflinerender.h

SOURCES += src/modelwidget.cpp
HEADERS += src/modelwidget.h

SOURCES += src/skeletondocument.cpp
HEADERS += src/skeletondocument.h

SOURCES += src/skeletondocumentwindow.cpp
HEADERS += src/skeletondocumentwindow.h

SOURCES += src/skeletongraphicswidget.cpp
HEADERS += src/skeletongraphicswidget.h

SOURCES += src/skeletonpartlistwidget.cpp
HEADERS += src/skeletonpartlistwidget.h

SOURCES += src/meshgenerator.cpp
HEADERS += src/meshgenerator.h

SOURCES += src/util.cpp
HEADERS += src/util.h

SOURCES += src/turnaroundloader.cpp
HEADERS += src/turnaroundloader.h

SOURCES += src/skeletonsnapshot.cpp
HEADERS += src/skeletonsnapshot.h

SOURCES += src/skeletonxml.cpp
HEADERS += src/skeletonxml.h

SOURCES += src/ds3file.cpp
HEADERS += src/ds3file.h

SOURCES += src/theme.cpp
HEADERS += src/theme.h

SOURCES += src/mesh.cpp
HEADERS += src/mesh.h

SOURCES += src/meshutil.cpp
HEADERS += src/meshutil.h

SOURCES += src/logbrowser.cpp
HEADERS += src/logbrowser.h

SOURCES += src/logbrowserdialog.cpp
HEADERS += src/logbrowserdialog.h

SOURCES += src/main.cpp

INCLUDEPATH += ../meshlite/include
LIBS += -L../meshlite/target/debug -lmeshlite

INCLUDEPATH += /usr/local/opt/boost/include

INCLUDEPATH += /usr/local/opt/gmp/include
LIBS += -L/usr/local/opt/gmp/lib -lgmp

INCLUDEPATH += /usr/local/opt/mpfr/include
LIBS += -L/usr/local/opt/mpfr/lib -lmpfr

INCLUDEPATH += /usr/local/opt/cgal/include
LIBS += -L/usr/local/opt/cgal/lib -lCGAL

target.path = ./
INSTALLS += target