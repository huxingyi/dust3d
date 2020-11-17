QT += core widgets opengl network
CONFIG += release
DEFINES += NDEBUG
DEFINES += QT_MESSAGELOGCONTEXT
RESOURCES += resources.qrc

LANGUAGES = zh_CN \
            es_AR \
            it_IT

OBJECTS_DIR=obj
MOC_DIR=moc

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
	CONFIG += force_debug_info
}

win32 {
	RC_FILE = $${SOURCE_ROOT}dust3d.rc
}

macx {
	ICON = $${SOURCE_ROOT}dust3d.icns

	RESOURCE_FILES.files = $$ICON
	RESOURCE_FILES.path = Contents/Resources
	QMAKE_BUNDLE_DATA += RESOURCE_FILES
}

isEmpty(HUMAN_VERSION) {
	HUMAN_VERSION = "1.0.0-rc.7"
}
isEmpty(VERSION) {
	VERSION = 1.0.0.37
}

HOMEPAGE_URL = "https://dust3d.org/"
REPOSITORY_URL = "https://github.com/huxingyi/dust3d"
ISSUES_URL = "https://github.com/huxingyi/dust3d/issues"
REFERENCE_GUIDE_URL = "https://docs.dust3d.org"
UPDATES_CHECKER_URL = "https://dust3d.org/dust3d-updateinfo.xml"

PLATFORM = "Unknown"
macx {
	PLATFORM = "MacOS"
}
win32 {
	PLATFORM = "Win32"
}
unix:!macx {
	PLATFORM = "Linux"
}

QMAKE_TARGET_COMPANY = Dust3D
QMAKE_TARGET_PRODUCT = Dust3D
QMAKE_TARGET_DESCRIPTION = "Dust3D is a cross-platform open-source 3D modeling software"
QMAKE_TARGET_COPYRIGHT = "Copyright (C) 2018-2020 Dust3D Project. All Rights Reserved."

DEFINES += "PROJECT_DEFINED_APP_COMPANY=\"\\\"$$QMAKE_TARGET_COMPANY\\\"\""
DEFINES += "PROJECT_DEFINED_APP_NAME=\"\\\"$$QMAKE_TARGET_PRODUCT\\\"\""
DEFINES += "PROJECT_DEFINED_APP_VER=\"\\\"$$VERSION\\\"\""
DEFINES += "PROJECT_DEFINED_APP_HUMAN_VER=\"\\\"$$HUMAN_VERSION\\\"\""
DEFINES += "PROJECT_DEFINED_APP_HOMEPAGE_URL=\"\\\"$$HOMEPAGE_URL\\\"\""
DEFINES += "PROJECT_DEFINED_APP_REPOSITORY_URL=\"\\\"$$REPOSITORY_URL\\\"\""
DEFINES += "PROJECT_DEFINED_APP_ISSUES_URL=\"\\\"$$ISSUES_URL\\\"\""
DEFINES += "PROJECT_DEFINED_APP_REFERENCE_GUIDE_URL=\"\\\"$$REFERENCE_GUIDE_URL\\\"\""
DEFINES += "PROJECT_DEFINED_APP_UPDATES_CHECKER_URL=\"\\\"$$UPDATES_CHECKER_URL\\\"\""
DEFINES += "PROJECT_DEFINED_APP_PLATFORM=\"\\\"$$PLATFORM\\\"\""

CONFIG += c++14

macx {
	QMAKE_CXXFLAGS_RELEASE -= -O
	QMAKE_CXXFLAGS_RELEASE -= -O1
	QMAKE_CXXFLAGS_RELEASE -= -O2

	QMAKE_CXXFLAGS_RELEASE += -O3
}

unix:!macx {
	QMAKE_CXXFLAGS_RELEASE -= -O
	QMAKE_CXXFLAGS_RELEASE -= -O1
	QMAKE_CXXFLAGS_RELEASE -= -O2

	QMAKE_CXXFLAGS_RELEASE += -O3
}

win32 {
    QMAKE_CXXFLAGS += /MP
	QMAKE_CXXFLAGS += /O2
	QMAKE_CXXFLAGS += /bigobj
}

DEFINES += NOMINMAX

include(thirdparty/QtAwesome/QtAwesome/QtAwesome.pri)
include(thirdparty/qtsingleapplication/src/qtsingleapplication.pri)
include(thirdparty/Qt-Color-Widgets/color_widgets.pri)

INCLUDEPATH += src

SOURCES += src/fixholes.cpp
HEADERS += src/fixholes.h

SOURCES += src/toonline.cpp
HEADERS += src/toonline.h

SOURCES += src/meshstroketifier.cpp
HEADERS += src/meshstroketifier.h

SOURCES += src/autosaver.cpp
HEADERS += src/autosaver.h

SOURCES += src/documentsaver.cpp
HEADERS += src/documentsaver.h

SOURCES += src/normalanddepthmapsgenerator.cpp
HEADERS += src/normalanddepthmapsgenerator.h

SOURCES += src/modeloffscreenrender.cpp
HEADERS += src/modeloffscreenrender.h

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

SOURCES += src/model.cpp
HEADERS += src/model.h

SOURCES += src/texturegenerator.cpp
HEADERS += src/texturegenerator.h

SOURCES += src/object.cpp
HEADERS += src/object.h

SOURCES += src/meshresultpostprocessor.cpp
HEADERS += src/meshresultpostprocessor.h

SOURCES += src/logbrowser.cpp
HEADERS += src/logbrowser.h

SOURCES += src/logbrowserdialog.cpp
HEADERS += src/logbrowserdialog.h

SOURCES += src/floatnumberwidget.cpp
HEADERS += src/floatnumberwidget.h

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

SOURCES += src/preferenceswidget.cpp
HEADERS += src/preferenceswidget.h

SOURCES += src/motionmanagewidget.cpp
HEADERS += src/motionmanagewidget.h

SOURCES += src/motionlistwidget.cpp
HEADERS += src/motionlistwidget.h

SOURCES += src/motionwidget.cpp
HEADERS += src/motionwidget.h

SOURCES += src/motionsgenerator.cpp
HEADERS += src/motionsgenerator.h

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

SOURCES += src/skeletondocument.cpp
HEADERS += src/skeletondocument.h

SOURCES += src/combinemode.cpp
HEADERS += src/combinemode.h

SOURCES += src/polycount.cpp
HEADERS += src/polycount.h

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

SOURCES += src/preferences.cpp
HEADERS += src/preferences.h

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

SOURCES += src/updateschecker.cpp
HEADERS += src/updateschecker.h

SOURCES += src/updatescheckwidget.cpp
HEADERS += src/updatescheckwidget.h

SOURCES += src/intnumberwidget.cpp
HEADERS += src/intnumberwidget.h

SOURCES += src/imagepreviewwidget.cpp
HEADERS += src/imagepreviewwidget.h

SOURCES += src/texturepainter.cpp
HEADERS += src/texturepainter.h

SOURCES += src/paintmode.cpp
HEADERS += src/paintmode.h

SOURCES += src/proceduralanimation.cpp
HEADERS += src/proceduralanimation.h

SOURCES += src/boundingboxmesh.cpp
HEADERS += src/boundingboxmesh.h

SOURCES += src/regionfiller.cpp
HEADERS += src/regionfiller.h

SOURCES += src/cyclefinder.cpp
HEADERS += src/cyclefinder.h

SOURCES += src/shortestpath.cpp
HEADERS += src/shortestpath.h

SOURCES += src/meshwrapper.cpp
HEADERS += src/meshwrapper.h

SOURCES += src/meshstitcher.cpp
HEADERS += src/meshstitcher.h

SOURCES += src/strokemeshbuilder.cpp
HEADERS += src/strokemeshbuilder.h

SOURCES += src/meshcombiner.cpp
HEADERS += src/meshcombiner.h

SOURCES += src/positionkey.cpp
HEADERS += src/positionkey.h

SOURCES += src/strokemodifier.cpp
HEADERS += src/strokemodifier.h

SOURCES += src/boxmesh.cpp
HEADERS += src/boxmesh.h

SOURCES += src/meshrecombiner.cpp
HEADERS += src/meshrecombiner.h

SOURCES += src/triangulatefaces.cpp
HEADERS += src/triangulatefaces.h

SOURCES += src/booleanmesh.cpp
HEADERS += src/booleanmesh.h

SOURCES += src/imageskeletonextractor.cpp
HEADERS += src/imageskeletonextractor.h

SOURCES += src/contourtopartconverter.cpp
HEADERS += src/contourtopartconverter.h

SOURCES += src/remesher.cpp
HEADERS += src/remesher.h

SOURCES += src/clothsimulator.cpp
HEADERS += src/clothsimulator.h

SOURCES += src/componentlayer.cpp
HEADERS += src/componentlayer.h

SOURCES += src/isotropicremesh.cpp
HEADERS += src/isotropicremesh.h

SOURCES += src/clothforce.cpp
HEADERS += src/clothforce.h

SOURCES += src/projectfacestonodes.cpp
HEADERS += src/projectfacestonodes.h

SOURCES += src/simulateclothmeshes.cpp
HEADERS += src/simulateclothmeshes.h

SOURCES += src/ddsfile.cpp
HEADERS += src/ddsfile.h

SOURCES += src/fileforever.cpp
HEADERS += src/fileforever.h

SOURCES += src/partpreviewimagesgenerator.cpp
HEADERS += src/partpreviewimagesgenerator.h

SOURCES += src/remeshhole.cpp
HEADERS += src/remeshhole.h

SOURCES += src/centripetalcatmullromspline.cpp
HEADERS += src/centripetalcatmullromspline.h

SOURCES += src/simpleshadermesh.cpp
HEADERS += src/simpleshadermesh.h

SOURCES += src/simpleshadermeshbinder.cpp
HEADERS += src/simpleshadermeshbinder.h

SOURCES += src/simpleshaderwidget.cpp
HEADERS += src/simpleshaderwidget.h

SOURCES += src/blockmesh.cpp
HEADERS += src/blockmesh.h

SOURCES += src/planemesh.cpp
HEADERS += src/planemesh.h

SOURCES += src/hermitecurveinterpolation.cpp
HEADERS += src/hermitecurveinterpolation.h

SOURCES += src/genericspineandpseudophysics.cpp
HEADERS += src/genericspineandpseudophysics.h

SOURCES += src/chainsimulator.cpp
HEADERS += src/chainsimulator.h

SOURCES += src/vertebratamotion.cpp
HEADERS += src/vertebratamotion.h

SOURCES += src/simplerendermeshgenerator.cpp
HEADERS += src/simplerendermeshgenerator.h

SOURCES += src/motioneditwidget.cpp
HEADERS += src/motioneditwidget.h

SOURCES += src/vertebratamotionparameterswidget.cpp
HEADERS += src/vertebratamotionparameterswidget.h

SOURCES += src/objectxml.cpp
HEADERS += src/objectxml.h

SOURCES += src/main.cpp

HEADERS += src/version.h

INCLUDEPATH += thirdparty/FastMassSpring/ClothApp
SOURCES += thirdparty/FastMassSpring/ClothApp/MassSpringSolver.cpp
HEADERS += thirdparty/FastMassSpring/ClothApp/MassSpringSolver.h

INCLUDEPATH += thirdparty/instant-meshes
INCLUDEPATH += thirdparty/instant-meshes/instant-meshes-dust3d/src
INCLUDEPATH += thirdparty/instant-meshes/instant-meshes-dust3d/ext/tbb/include
INCLUDEPATH += thirdparty/instant-meshes/instant-meshes-dust3d/ext/dset
INCLUDEPATH += thirdparty/instant-meshes/instant-meshes-dust3d/ext/pss
INCLUDEPATH += thirdparty/instant-meshes/instant-meshes-dust3d/ext/pcg32
INCLUDEPATH += thirdparty/instant-meshes/instant-meshes-dust3d/ext/rply
INCLUDEPATH += thirdparty/instant-meshes/instant-meshes-dust3d/ext/half
unix {
	SOURCES += thirdparty/instant-meshes/instant-meshes-api.cpp
	LIBS += -L$${SOURCE_ROOT}thirdparty/instant-meshes/build -linstant-meshes
	LIBS += -L$${SOURCE_ROOT}thirdparty/instant-meshes/build/ext_build/tbb -ltbb_static
	unix:!macx {
		LIBS += -ldl
	}
}
win32 {
	DEFINES += _USE_MATH_DEFINES
	LIBS += -L$${SOURCE_ROOT}thirdparty/instant-meshes/build/RelWithDebInfo -linstant-meshes
	LIBS += -L$${SOURCE_ROOT}thirdparty/instant-meshes/build/ext_build/tbb/RelWithDebInfo -ltbb
}

INCLUDEPATH += thirdparty/quickjs/quickjs-2019-07-09-dust3d

DEFINES += "CONFIG_VERSION=\"\\\"2019-07-09\\\"\""

SOURCES += thirdparty/quickjs/quickjs-2019-07-09-dust3d/quickjs.c
HEADERS += thirdparty/quickjs/quickjs-2019-07-09-dust3d/quickjs.h

SOURCES += thirdparty/quickjs/quickjs-2019-07-09-dust3d/cutils.c
HEADERS += thirdparty/quickjs/quickjs-2019-07-09-dust3d/cutils.h

SOURCES += thirdparty/quickjs/quickjs-2019-07-09-dust3d/libunicode.c
HEADERS += thirdparty/quickjs/quickjs-2019-07-09-dust3d/libunicode.h

SOURCES += thirdparty/quickjs/quickjs-2019-07-09-dust3d/libregexp.c
HEADERS += thirdparty/quickjs/quickjs-2019-07-09-dust3d/libregexp.h

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
SOURCES += thirdparty/simpleuv/simpleuv/meshdatatype.cpp

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
		error("No BOOST_INCLUDEDIR define found in environment variables")
	}

	isEmpty(CGAL_DIR) {
		error("No CGAL_DIR define found in environment variables")
	}

	GMP_LIBNAME = libgmp-10
	MPFR_LIBNAME = libmpfr-4
	CGAL_INCLUDEDIR = $$CGAL_DIR\include
	GMP_INCLUDEDIR = $$CGAL_DIR\auxiliary\gmp\include
	GMP_LIBDIR = $$CGAL_DIR\auxiliary\gmp\lib
	MPFR_INCLUDEDIR = $$GMP_INCLUDEDIR
	MPFR_LIBDIR = $$GMP_LIBDIR
}

macx {
	GMP_LIBNAME = gmp
	MPFR_LIBNAME = mpfr
	BOOST_INCLUDEDIR = /usr/local/opt/boost/include
	CGAL_INCLUDEDIR = /usr/local/opt/cgal/include
	GMP_INCLUDEDIR = /usr/local/opt/gmp/include
	GMP_LIBDIR = /usr/local/opt/gmp/lib
	MPFR_INCLUDEDIR = /usr/local/opt/mpfr/include
	MPFR_LIBDIR = /usr/local/opt/mpfr/lib
}

unix:!macx {
	GMP_LIBNAME = gmp
	MPFR_LIBNAME = mpfr
	BOOST_INCLUDEDIR = /usr/local/include
	CGAL_INCLUDEDIR = /usr/local/include
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

target.path = ./
INSTALLS += target