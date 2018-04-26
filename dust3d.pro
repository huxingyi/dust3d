QT += core widgets opengl
CONFIG += release
RESOURCES += resources.qrc

isEmpty(HUMAN_VERSION) {
	HUMAN_VERSION = "0.0-unversioned"
}
isEmpty(VERSION) {
	VERSION = 0.0.0.1
}

REPOSITORY_URL = "https://github.com/huxingyi/dust3d"
ISSUES_URL = "https://github.com/huxingyi/dust3d/issues"
REFERENCE_GUIDE_URL = "https://dust3d.readthedocs.io"

QMAKE_TARGET_COMPANY = Dust3D
QMAKE_TARGET_PRODUCT = Dust3D
QMAKE_TARGET_DESCRIPTION = "Aim to be a quick modeling tool for game development"
QMAKE_TARGET_COPYRIGHT = "Copyright (C) 2018 Dust3D Project. All Rights Reserved."

DEFINES += "PROJECT_DEFINED_APP_NAME=\"\\\"$$QMAKE_TARGET_PRODUCT\\\"\""
DEFINES += "PROJECT_DEFINED_APP_VER=\"\\\"$$VERSION\\\"\""
DEFINES += "PROJECT_DEFINED_APP_HUMAN_VER=\"\\\"$$HUMAN_VERSION\\\"\""
DEFINES += "PROJECT_DEFINED_APP_REPOSITORY_URL=\"\\\"$$REPOSITORY_URL\\\"\""
DEFINES += "PROJECT_DEFINED_APP_ISSUES_URL=\"\\\"$$ISSUES_URL\\\"\""
DEFINES += "PROJECT_DEFINED_APP_REFERENCE_GUIDE_URL=\"\\\"$$REFERENCE_GUIDE_URL\\\"\""

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

SOURCES += src/aboutwidget.cpp
HEADERS += src/aboutwidget.h

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

SOURCES += src/uvunwrapper.cpp
HEADERS += src/uvunwrapper.h

SOURCES += src/vertexskingenerator.cpp
HEADERS += src/vertexskingenerator.h

SOURCES += src/meshresultcontext.cpp
HEADERS += src/meshresultcontext.h

SOURCES += src/positionmap.cpp
HEADERS += src/positionmap.h

SOURCES += src/logbrowser.cpp
HEADERS += src/logbrowser.h

SOURCES += src/logbrowserdialog.cpp
HEADERS += src/logbrowserdialog.h

SOURCES += src/floatnumberwidget.cpp
HEADERS += src/floatnumberwidget.h

SOURCES += src/boneexportwidget.cpp
HEADERS += src/boneexportwidget.h

SOURCES += src/main.cpp

HEADERS += src/version.h

QMAKE_CXXFLAGS += -std=c++11

win32 {
	isEmpty(BOOST_INCLUDEDIR) {
		BOOST_INCLUDEDIR = $$(BOOST_INCLUDEDIR)
	}
	isEmpty(CGAL_DIR) {
		CGAL_DIR = $$(CGAL_DIR)
	}
	
	isEmpty(BOOST_INCLUDEDIR) {
		error("No BOOST_INCLUDEDIR define found in enviroment variables")
	}

	isEmpty(CGAL_DIR) {
		error("No CGAL_DIR define found in enviroment variables")
	}

	contains(QMAKE_TARGET.arch, x86_64) {
		MESHLITE_DIR = thirdparty/meshlite/meshlite_unstable_vc14_x64
	} else {
		MESHLITE_DIR = thirdparty/meshlite/meshlite_unstable_vc14_x86
	}
	MESHLITE_LIBNAME = meshlite.dll
	GMP_LIBNAME = libgmp-10
	MPFR_LIBNAME = libmpfr-4
	CGAL_LIBNAME = CGAL-vc140-mt-4.11.1
	CGAL_INCLUDEDIR = $$CGAL_DIR\include
	CGAL_BUILDINCLUDEDIR = $$CGAL_DIR\build\include
	CGAL_LIBDIR = $$CGAL_DIR\build\lib\Release
	GMP_INCLUDEDIR = $$CGAL_DIR\auxiliary\gmp\include
	GMP_LIBDIR = $$CGAL_DIR\auxiliary\gmp\lib
	MPFR_INCLUDEDIR = $$GMP_INCLUDEDIR
	MPFR_LIBDIR = $$GMP_LIBDIR
}

macx {
	MESHLITE_DIR = thirdparty/meshlite
	MESHLITE_LIBNAME = meshlite
	GMP_LIBNAME = gmp
	MPFR_LIBNAME = mpfr
	CGAL_LIBNAME = cgal
	BOOST_INCLUDEDIR = /usr/local/opt/boost/include
	CGAL_INCLUDEDIR = /usr/local/opt/cgal/include
	CGAL_BUILDINCLUDEDIR = /usr/local/opt/cgal/include
	CGAL_LIBDIR = /usr/local/opt/cgal/lib
	GMP_INCLUDEDIR = /usr/local/opt/gmp/include
	GMP_LIBDIR = /usr/local/opt/gmp/lib
	MPFR_INCLUDEDIR = /usr/local/opt/mpfr/include
	MPFR_LIBDIR = /usr/local/opt/mpfr/lib
}

unix:!macx {
	MESHLITE_DIR = thirdparty/meshlite
	MESHLITE_LIBNAME = meshlite
	GMP_LIBNAME = gmp
	MPFR_LIBNAME = mpfr
	CGAL_LIBNAME = CGAL
	BOOST_INCLUDEDIR = /usr/local/include
	CGAL_INCLUDEDIR = /usr/local/include
	CGAL_BUILDINCLUDEDIR = /usr/local/include
	CGAL_LIBDIR = /usr/local/lib
	GMP_INCLUDEDIR = /usr/local/include
	GMP_LIBDIR = /usr/local/lib
	MPFR_INCLUDEDIR = /usr/local/include
	MPFR_LIBDIR = /usr/local/lib
}

INCLUDEPATH += $$MESHLITE_DIR
LIBS += -L$$MESHLITE_DIR -l$$MESHLITE_LIBNAME

INCLUDEPATH += $$BOOST_INCLUDEDIR

INCLUDEPATH += $$GMP_INCLUDEDIR
LIBS += -L$$GMP_LIBDIR -l$$GMP_LIBNAME

INCLUDEPATH += $$MPFR_INCLUDEDIR
LIBS += -L$$MPFR_LIBDIR -l$$MPFR_LIBNAME

INCLUDEPATH += $$CGAL_INCLUDEDIR
INCLUDEPATH += $$CGAL_BUILDINCLUDEDIR
LIBS += -L$$CGAL_LIBDIR -l$$CGAL_LIBNAME

target.path = ./
INSTALLS += target
