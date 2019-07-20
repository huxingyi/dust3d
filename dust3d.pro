QT += core widgets opengl network
CONFIG += release
DEFINES += NDEBUG
DEFINES += QT_MESSAGELOGCONTEXT
RESOURCES += resources.qrc

LANGUAGES = zh_CN

############## Generate .qm from .ts #######################

# parameters: var, prepend, append
defineReplace(prependAll) {
	for(a,$$1):result += $$2$${a}$$3
	return($$result)
}
TRANSLATIONS = $$prependAll(LANGUAGES, $$PWD/languages/dust3d_, .ts)
TRANSLATIONS_FILES =
qtPrepareTool(LRELEASE, lrelease)
for(tsfile, TRANSLATIONS) {
	qmfile = $$shadowed($$tsfile)
	qmfile ~= s,.ts$,.qm,
	qmdir = $$dirname(qmfile)
	!exists($$qmdir) {
		mkpath($$qmdir)|error("Aborting.")
	}
	command = $$LRELEASE -removeidentical $$tsfile -qm $$qmfile
	system($$command)|error("Failed to run: $$command")
	TRANSLATIONS_FILES += $$qmfile
}

########################################################

############## Generate .ts file #######################
macx {
	wd = $$replace(PWD, /, $$QMAKE_DIR_SEP)

	# Update the .ts file from source
	qtPrepareTool(LUPDATE, lupdate)
	LUPDATE += src/*.cpp src/*.h -locations none
	for(lang, LANGUAGES) {
		command = $$LUPDATE -ts languages/dust3d_$${lang}.ts
		system($$command)|error("Failed to run: $$command")
	}
}
##########################################################

win32 {
	RC_FILE = dust3d.rc
}

macx {
	ICON = dust3d.icns

	RESOURCE_FILES.files = $$ICON
	RESOURCE_FILES.path = Contents/Resources
	QMAKE_BUNDLE_DATA += RESOURCE_FILES
}

isEmpty(HUMAN_VERSION) {
	HUMAN_VERSION = "1.0.0-beta.22"
}
isEmpty(VERSION) {
	VERSION = 1.0.0.22
}

HOMEPAGE_URL = "https://dust3d.org/"
REPOSITORY_URL = "https://github.com/huxingyi/dust3d"
ISSUES_URL = "https://github.com/huxingyi/dust3d/issues"
REFERENCE_GUIDE_URL = "http://docs.dust3d.org"

QMAKE_TARGET_COMPANY = Dust3D
QMAKE_TARGET_PRODUCT = Dust3D
QMAKE_TARGET_DESCRIPTION = "Dust3D is a cross-platform open-source 3D modeling software"
QMAKE_TARGET_COPYRIGHT = "Copyright (C) 2018-2019 Dust3D Project. All Rights Reserved."

DEFINES += "PROJECT_DEFINED_APP_COMPANY=\"\\\"$$QMAKE_TARGET_COMPANY\\\"\""
DEFINES += "PROJECT_DEFINED_APP_NAME=\"\\\"$$QMAKE_TARGET_PRODUCT\\\"\""
DEFINES += "PROJECT_DEFINED_APP_VER=\"\\\"$$VERSION\\\"\""
DEFINES += "PROJECT_DEFINED_APP_HUMAN_VER=\"\\\"$$HUMAN_VERSION\\\"\""
DEFINES += "PROJECT_DEFINED_APP_HOMEPAGE_URL=\"\\\"$$HOMEPAGE_URL\\\"\""
DEFINES += "PROJECT_DEFINED_APP_REPOSITORY_URL=\"\\\"$$REPOSITORY_URL\\\"\""
DEFINES += "PROJECT_DEFINED_APP_ISSUES_URL=\"\\\"$$ISSUES_URL\\\"\""
DEFINES += "PROJECT_DEFINED_APP_REFERENCE_GUIDE_URL=\"\\\"$$REFERENCE_GUIDE_URL\\\"\""

QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2

QMAKE_CXXFLAGS_RELEASE += -O3

QMAKE_CXXFLAGS += -std=c++11

include(thirdparty/QtAwesome/QtAwesome/QtAwesome.pri)

INCLUDEPATH += src

SOURCES += src/modelshaderprogram.cpp
HEADERS += src/modelshaderprogram.h

SOURCES += src/modelmeshbinder.cpp
HEADERS += src/modelmeshbinder.h

SOURCES += src/modelwidget.cpp
HEADERS += src/modelwidget.h

SOURCES += src/document.cpp
HEADERS += src/document.h

SOURCES += src/documentwindow.cpp
HEADERS += src/documentwindow.h

SOURCES += src/skeletongraphicswidget.cpp
HEADERS += src/skeletongraphicswidget.h

SOURCES += src/parttreewidget.cpp
HEADERS += src/parttreewidget.h

SOURCES += src/partwidget.cpp
HEADERS += src/partwidget.h

SOURCES += src/aboutwidget.cpp
HEADERS += src/aboutwidget.h

SOURCES += src/meshgenerator.cpp
HEADERS += src/meshgenerator.h

SOURCES += src/util.cpp
HEADERS += src/util.h

SOURCES += src/turnaroundloader.cpp
HEADERS += src/turnaroundloader.h

SOURCES += src/snapshot.cpp
HEADERS += src/snapshot.h

SOURCES += src/snapshotxml.cpp
HEADERS += src/snapshotxml.h

SOURCES += src/ds3file.cpp
HEADERS += src/ds3file.h

SOURCES += src/glbfile.cpp
HEADERS += src/glbfile.h

SOURCES += src/theme.cpp
HEADERS += src/theme.h

SOURCES += src/meshloader.cpp
HEADERS += src/meshloader.h

SOURCES += src/texturegenerator.cpp
HEADERS += src/texturegenerator.h

SOURCES += src/outcome.cpp
HEADERS += src/outcome.h

SOURCES += src/meshresultpostprocessor.cpp
HEADERS += src/meshresultpostprocessor.h

SOURCES += src/logbrowser.cpp
HEADERS += src/logbrowser.h

SOURCES += src/logbrowserdialog.cpp
HEADERS += src/logbrowserdialog.h

SOURCES += src/floatnumberwidget.cpp
HEADERS += src/floatnumberwidget.h

SOURCES += src/exportpreviewwidget.cpp
HEADERS += src/exportpreviewwidget.h

SOURCES += src/ccdikresolver.cpp
HEADERS += src/ccdikresolver.h

SOURCES += src/skeletonikmover.cpp
HEADERS += src/skeletonikmover.h

SOURCES += src/spinnableawesomebutton.cpp
HEADERS += src/spinnableawesomebutton.h

SOURCES += src/infolabel.cpp
HEADERS += src/infolabel.h

SOURCES += src/graphicscontainerwidget.cpp
HEADERS += src/graphicscontainerwidget.h

SOURCES += src/rigwidget.cpp
HEADERS += src/rigwidget.h

SOURCES += src/markiconcreator.cpp
HEADERS += src/markiconcreator.h

SOURCES += src/bonemark.cpp
HEADERS += src/bonemark.h

SOURCES += src/skeletonside.cpp
HEADERS += src/skeletonside.h

SOURCES += src/meshsplitter.cpp
HEADERS += src/meshsplitter.h

SOURCES += src/rigger.cpp
HEADERS += src/rigger.h

SOURCES += src/rigtype.cpp
HEADERS += src/rigtype.h

SOURCES += src/riggenerator.cpp
HEADERS += src/riggenerator.h

SOURCES += src/skinnedmeshcreator.cpp
HEADERS += src/skinnedmeshcreator.h

SOURCES += src/jointnodetree.cpp
HEADERS += src/jointnodetree.h

SOURCES += src/poser.cpp
HEADERS += src/poser.h

SOURCES += src/posemeshcreator.cpp
HEADERS += src/posemeshcreator.h

SOURCES += src/posepreviewmanager.cpp
HEADERS += src/posepreviewmanager.h

SOURCES += src/poseeditwidget.cpp
HEADERS += src/poseeditwidget.h

SOURCES += src/poselistwidget.cpp
HEADERS += src/poselistwidget.h

SOURCES += src/posemanagewidget.cpp
HEADERS += src/posemanagewidget.h

SOURCES += src/posepreviewsgenerator.cpp
HEADERS += src/posepreviewsgenerator.h

SOURCES += src/posewidget.cpp
HEADERS += src/posewidget.h

SOURCES += src/preferenceswidget.cpp
HEADERS += src/preferenceswidget.h

SOURCES += src/motioneditwidget.cpp
HEADERS += src/motioneditwidget.h

SOURCES += src/motionmanagewidget.cpp
HEADERS += src/motionmanagewidget.h

SOURCES += src/motionlistwidget.cpp
HEADERS += src/motionlistwidget.h

SOURCES += src/motionwidget.cpp
HEADERS += src/motionwidget.h

SOURCES += src/motionsgenerator.cpp
HEADERS += src/motionsgenerator.h

SOURCES += src/animationclipplayer.cpp
HEADERS += src/animationclipplayer.h

SOURCES += src/texturetype.cpp
HEADERS += src/texturetype.h

SOURCES += src/imageforever.cpp
HEADERS += src/imageforever.h

SOURCES += src/materialeditwidget.cpp
HEADERS += src/materialeditwidget.h

SOURCES += src/materiallistwidget.cpp
HEADERS += src/materiallistwidget.h

SOURCES += src/materialmanagewidget.cpp
HEADERS += src/materialmanagewidget.h

SOURCES += src/materialpreviewsgenerator.cpp
HEADERS += src/materialpreviewsgenerator.h

SOURCES += src/materialwidget.cpp
HEADERS += src/materialwidget.h

SOURCES += src/material.cpp
HEADERS += src/material.h

SOURCES += src/fbxfile.cpp
HEADERS += src/fbxfile.h

SOURCES += src/motiontimelinewidget.cpp
HEADERS += src/motiontimelinewidget.h

SOURCES += src/interpolationtype.cpp
HEADERS += src/interpolationtype.h

SOURCES += src/motionclipwidget.cpp
HEADERS += src/motionclipwidget.h

SOURCES += src/tabwidget.cpp
HEADERS += src/tabwidget.h

SOURCES += src/flowlayout.cpp
HEADERS += src/flowlayout.h

SOURCES += src/shortcuts.cpp
HEADERS += src/shortcuts.h

SOURCES += src/trianglesourcenoderesolve.cpp
HEADERS += src/trianglesourcenoderesolve.h

SOURCES += src/uvunwrap.cpp
HEADERS += src/uvunwrap.h

SOURCES += src/triangletangentresolve.cpp
HEADERS += src/triangletangentresolve.h

SOURCES += src/animalrigger.cpp
HEADERS += src/animalrigger.h

SOURCES += src/animalposer.cpp
HEADERS += src/animalposer.h

SOURCES += src/riggerconstruct.cpp
HEADERS += src/riggerconstruct.h

SOURCES += src/poserconstruct.cpp
HEADERS += src/poserconstruct.h

SOURCES += src/skeletondocument.cpp
HEADERS += src/skeletondocument.h

SOURCES += src/posedocument.cpp
HEADERS += src/posedocument.h

SOURCES += src/combinemode.cpp
HEADERS += src/combinemode.h

SOURCES += src/cutdocument.cpp
HEADERS += src/cutdocument.h

SOURCES += src/cutface.cpp
HEADERS += src/cutface.h

SOURCES += src/parttarget.cpp
HEADERS += src/parttarget.h

SOURCES += src/partbase.cpp
HEADERS += src/partbase.h

SOURCES += src/cutfacewidget.cpp
HEADERS += src/cutfacewidget.h

SOURCES += src/cutfacelistwidget.cpp
HEADERS += src/cutfacelistwidget.h

SOURCES += src/remoteioserver.cpp
HEADERS += src/remoteioserver.h

SOURCES += src/remoteioconnection.cpp
HEADERS += src/remoteioconnection.h

SOURCES += src/preferences.cpp
HEADERS += src/preferences.h

SOURCES += src/bonenameconverter.cpp
HEADERS += src/bonenameconverter.h

SOURCES += src/toolmesh.cpp
HEADERS += src/toolmesh.h

HEADERS += src/shadervertex.h

SOURCES += src/scripteditwidget.cpp
HEADERS += src/scripteditwidget.h

SOURCES += src/scriptvariableswidget.cpp
HEADERS += src/scriptvariableswidget.h

SOURCES += src/scriptwidget.cpp
HEADERS += src/scriptwidget.h

SOURCES += src/scriptrunner.cpp
HEADERS += src/scriptrunner.h

SOURCES += src/variablesxml.cpp
HEADERS += src/variablesxml.h

SOURCES += src/main.cpp

HEADERS += src/version.h

INCLUDEPATH += thirdparty/quickjs/quickjs-2019-07-09

DEFINES += "CONFIG_VERSION=\"\\\"2019-07-09\\\"\""

SOURCES += thirdparty/quickjs/quickjs-2019-07-09/quickjs.c
HEADERS += thirdparty/quickjs/quickjs-2019-07-09/quickjs.h

SOURCES += thirdparty/quickjs/quickjs-2019-07-09/cutils.c
HEADERS += thirdparty/quickjs/quickjs-2019-07-09/cutils.h

SOURCES += thirdparty/quickjs/quickjs-2019-07-09/libunicode.c
HEADERS += thirdparty/quickjs/quickjs-2019-07-09/libunicode.h

SOURCES += thirdparty/quickjs/quickjs-2019-07-09/libregexp.c
HEADERS += thirdparty/quickjs/quickjs-2019-07-09/libregexp.h

INCLUDEPATH += thirdparty/nodemesh

SOURCES += thirdparty/nodemesh/nodemesh/wrapper.cpp
HEADERS += thirdparty/nodemesh/nodemesh/wrapper.h

SOURCES += thirdparty/nodemesh/nodemesh/stitcher.cpp
HEADERS += thirdparty/nodemesh/nodemesh/stitcher.h

SOURCES += thirdparty/nodemesh/nodemesh/builder.cpp
HEADERS += thirdparty/nodemesh/nodemesh/builder.h

SOURCES += thirdparty/nodemesh/nodemesh/combiner.cpp
HEADERS += thirdparty/nodemesh/nodemesh/combiner.h

SOURCES += thirdparty/nodemesh/nodemesh/misc.cpp
HEADERS += thirdparty/nodemesh/nodemesh/misc.h

SOURCES += thirdparty/nodemesh/nodemesh/positionkey.cpp
HEADERS += thirdparty/nodemesh/nodemesh/positionkey.h

SOURCES += thirdparty/nodemesh/nodemesh/modifier.cpp
HEADERS += thirdparty/nodemesh/nodemesh/modifier.h

SOURCES += thirdparty/nodemesh/nodemesh/box.cpp
HEADERS += thirdparty/nodemesh/nodemesh/box.h

SOURCES += thirdparty/nodemesh/nodemesh/recombiner.cpp
HEADERS += thirdparty/nodemesh/nodemesh/recombiner.h

HEADERS += thirdparty/nodemesh/nodemesh/cgalmesh.h

INCLUDEPATH += thirdparty/crc64

SOURCES += thirdparty/crc64/crc64.c
HEADERS += thirdparty/crc64/crc64.h

INCLUDEPATH += thirdparty/miniz

SOURCES += thirdparty/miniz/miniz.c
HEADERS += thirdparty/miniz/miniz.h

INCLUDEPATH += thirdparty/fbx/src

SOURCES += thirdparty/fbx/src/fbxdocument.cpp
HEADERS += thirdparty/fbx/src/fbxdocument.h

SOURCES += thirdparty/fbx/src/fbxnode.cpp
HEADERS += thirdparty/fbx/src/fbxnode.h

SOURCES += thirdparty/fbx/src/fbxproperty.cpp
HEADERS += thirdparty/fbx/src/fbxproperty.h

SOURCES += thirdparty/fbx/src/fbxutil.cpp
HEADERS += thirdparty/fbx/src/fbxutil.h

INCLUDEPATH += thirdparty/simpleuv
INCLUDEPATH += thirdparty/simpleuv/thirdparty/libigl/include
INCLUDEPATH += thirdparty/simpleuv/thirdparty/eigen
INCLUDEPATH += thirdparty/simpleuv/thirdparty/squeezer

SOURCES += thirdparty/simpleuv/simpleuv/uvunwrapper.cpp
HEADERS += thirdparty/simpleuv/simpleuv/uvunwrapper.h

SOURCES += thirdparty/simpleuv/simpleuv/parametrize.cpp
HEADERS += thirdparty/simpleuv/simpleuv/parametrize.h

SOURCES += thirdparty/simpleuv/simpleuv/chartpacker.cpp
HEADERS += thirdparty/simpleuv/simpleuv/chartpacker.h

SOURCES += thirdparty/simpleuv/simpleuv/triangulate.cpp
HEADERS += thirdparty/simpleuv/simpleuv/triangulate.h

HEADERS += thirdparty/simpleuv/simpleuv/meshdatatype.h

SOURCES += thirdparty/simpleuv/thirdparty/squeezer/maxrects.c
HEADERS += thirdparty/simpleuv/thirdparty/squeezer/maxrects.h

INCLUDEPATH += thirdparty/QtWaitingSpinner

SOURCES += thirdparty/QtWaitingSpinner/waitingspinnerwidget.cpp
HEADERS += thirdparty/QtWaitingSpinner/waitingspinnerwidget.h

INCLUDEPATH += thirdparty/json

win32 {
    LIBS += -luser32
	LIBS += -lopengl32

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

	GMP_LIBNAME = libgmp-10
	MPFR_LIBNAME = libmpfr-4
	CGAL_LIBNAME = CGAL-vc140-mt-4.13
	CGAL_INCLUDEDIR = $$CGAL_DIR\include
	CGAL_BUILDINCLUDEDIR = $$CGAL_DIR\build\include
	CGAL_LIBDIR = $$CGAL_DIR\build\lib\Release
	GMP_INCLUDEDIR = $$CGAL_DIR\auxiliary\gmp\include
	GMP_LIBDIR = $$CGAL_DIR\auxiliary\gmp\lib
	MPFR_INCLUDEDIR = $$GMP_INCLUDEDIR
	MPFR_LIBDIR = $$GMP_LIBDIR
}

macx {
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
