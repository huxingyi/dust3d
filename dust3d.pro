QT += core widgets opengl
CONFIG += release
DEFINES += NDEBUG
RESOURCES += resources.qrc

isEmpty(HUMAN_VERSION) {
	HUMAN_VERSION = "0.0-unversioned"
}
isEmpty(VERSION) {
	VERSION = 0.0.0.1
}

REPOSITORY_URL = "https://github.com/huxingyi/dust3d"
ISSUES_URL = "https://github.com/huxingyi/dust3d/issues"
REFERENCE_GUIDE_URL = "http://docs.dust3d.org"

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

QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2

QMAKE_CXXFLAGS_RELEASE += -O3

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

SOURCES += src/skeletonparttreewidget.cpp
HEADERS += src/skeletonparttreewidget.h

SOURCES += src/skeletonpartwidget.cpp
HEADERS += src/skeletonpartwidget.h

SOURCES += src/aboutwidget.cpp
HEADERS += src/aboutwidget.h

SOURCES += src/meshgenerator.cpp
HEADERS += src/meshgenerator.h

SOURCES += src/dust3dutil.cpp
HEADERS += src/dust3dutil.h

SOURCES += src/turnaroundloader.cpp
HEADERS += src/turnaroundloader.h

SOURCES += src/skeletonsnapshot.cpp
HEADERS += src/skeletonsnapshot.h

SOURCES += src/skeletonxml.cpp
HEADERS += src/skeletonxml.h

SOURCES += src/ds3file.cpp
HEADERS += src/ds3file.h

SOURCES += src/gltffile.cpp
HEADERS += src/gltffile.h

SOURCES += src/theme.cpp
HEADERS += src/theme.h

SOURCES += src/meshloader.cpp
HEADERS += src/meshloader.h

SOURCES += src/meshutil.cpp
HEADERS += src/meshutil.h

SOURCES += src/texturegenerator.cpp
HEADERS += src/texturegenerator.h

SOURCES += src/meshresultcontext.cpp
HEADERS += src/meshresultcontext.h

SOURCES += src/meshresultpostprocessor.cpp
HEADERS += src/meshresultpostprocessor.h

SOURCES += src/positionmap.cpp
HEADERS += src/positionmap.h

SOURCES += src/logbrowser.cpp
HEADERS += src/logbrowser.h

SOURCES += src/logbrowserdialog.cpp
HEADERS += src/logbrowserdialog.h

SOURCES += src/floatnumberwidget.cpp
HEADERS += src/floatnumberwidget.h

SOURCES += src/exportpreviewwidget.cpp
HEADERS += src/exportpreviewwidget.h

SOURCES += src/ambientocclusionbaker.cpp
HEADERS += src/ambientocclusionbaker.h

SOURCES += src/ccdikresolver.cpp
HEADERS += src/ccdikresolver.h

SOURCES += src/skeletonikmover.cpp
HEADERS += src/skeletonikmover.h

HEADERS += src/qtlightmapper.h

SOURCES += src/spinnableawesomebutton.cpp
HEADERS += src/spinnableawesomebutton.h

SOURCES += src/infolabel.cpp
HEADERS += src/infolabel.h

SOURCES += src/graphicscontainerwidget.cpp
HEADERS += src/graphicscontainerwidget.h

SOURCES += src/main.cpp

HEADERS += src/version.h

INCLUDEPATH += thirdparty/QtWaitingSpinner

SOURCES += thirdparty/QtWaitingSpinner/waitingspinnerwidget.cpp
HEADERS += thirdparty/QtWaitingSpinner/waitingspinnerwidget.h

INCLUDEPATH += thirdparty/json

INCLUDEPATH += thirdparty/thekla_atlas/src
INCLUDEPATH += thirdparty/thekla_atlas/src/thekla
INCLUDEPATH += thirdparty/thekla_atlas/extern/poshlib
INCLUDEPATH += thirdparty/thekla_atlas/src/nvmesh
INCLUDEPATH += thirdparty/thekla_atlas/src/nvmesh/param
INCLUDEPATH += thirdparty/thekla_atlas/src/nvcore
INCLUDEPATH += thirdparty/thekla_atlas/src/nvimage
INCLUDEPATH += thirdparty/thekla_atlas/src/nvmath

SOURCES += thirdparty/thekla_atlas/extern/poshlib/posh.c
HEADERS += thirdparty/thekla_atlas/extern/poshlib/posh.h

SOURCES += thirdparty/thekla_atlas/src/thekla/thekla_atlas.cpp
HEADERS += thirdparty/thekla_atlas/src/thekla/thekla_atlas.h

HEADERS += thirdparty/thekla_atlas/src/nvcore/Stream.h

SOURCES += thirdparty/thekla_atlas/src/nvcore/Debug.cpp
HEADERS += thirdparty/thekla_atlas/src/nvcore/Debug.h

HEADERS += thirdparty/thekla_atlas/src/nvcore/StdStream.h

SOURCES += thirdparty/thekla_atlas/src/nvcore/StrLib.cpp
HEADERS += thirdparty/thekla_atlas/src/nvcore/StrLib.h

HEADERS += thirdparty/thekla_atlas/src/nvcore/Utils.h

SOURCES += thirdparty/thekla_atlas/src/nvcore/Array.inl
HEADERS += thirdparty/thekla_atlas/src/nvcore/Array.h

SOURCES += thirdparty/thekla_atlas/src/nvcore/Memory.cpp
HEADERS += thirdparty/thekla_atlas/src/nvcore/Memory.h

SOURCES += thirdparty/thekla_atlas/src/nvcore/RadixSort.cpp
HEADERS += thirdparty/thekla_atlas/src/nvcore/RadixSort.h

SOURCES += thirdparty/thekla_atlas/src/nvcore/HashMap.inl
HEADERS += thirdparty/thekla_atlas/src/nvcore/HashMap.h

HEADERS += thirdparty/thekla_atlas/src/nvcore/Hash.h

HEADERS += thirdparty/thekla_atlas/src/nvcore/ForEach.h

SOURCES += thirdparty/thekla_atlas/src/nvmesh/nvmesh.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmesh/nvmesh.h

SOURCES += thirdparty/thekla_atlas/src/nvmesh/BaseMesh.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmesh/BaseMesh.h

SOURCES += thirdparty/thekla_atlas/src/nvmesh/MeshBuilder.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmesh/MeshBuilder.h

SOURCES += thirdparty/thekla_atlas/src/nvmesh/TriMesh.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmesh/TriMesh.h

SOURCES += thirdparty/thekla_atlas/src/nvmesh/QuadTriMesh.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmesh/QuadTriMesh.h

SOURCES += thirdparty/thekla_atlas/src/nvmesh/MeshTopology.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmesh/MeshTopology.h

SOURCES += thirdparty/thekla_atlas/src/nvmesh/halfedge/Edge.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmesh/halfedge/Edge.h

SOURCES += thirdparty/thekla_atlas/src/nvmesh/halfedge/Mesh.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmesh/halfedge/Mesh.h

SOURCES += thirdparty/thekla_atlas/src/nvmesh/halfedge/Face.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmesh/halfedge/Face.h

SOURCES += thirdparty/thekla_atlas/src/nvmesh/halfedge/Vertex.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmesh/halfedge/Vertex.h

SOURCES += thirdparty/thekla_atlas/src/nvmesh/geometry/Bounds.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmesh/geometry/Bounds.h

SOURCES += thirdparty/thekla_atlas/src/nvmesh/geometry/Measurements.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmesh/geometry/Measurements.h

SOURCES += thirdparty/thekla_atlas/src/nvmesh/raster/Raster.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmesh/raster/raster.h

HEADERS += thirdparty/thekla_atlas/src/nvmesh/raster/ClippedTriangle.h

SOURCES += thirdparty/thekla_atlas/src/nvmesh/param/Atlas.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmesh/param/Atlas.h

SOURCES += thirdparty/thekla_atlas/src/nvmesh/param/AtlasBuilder.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmesh/param/AtlasBuilder.h

SOURCES += thirdparty/thekla_atlas/src/nvmesh/param/AtlasPacker.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmesh/param/AtlasPacker.h

SOURCES += thirdparty/thekla_atlas/src/nvmesh/param/ConformalMap.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmesh/param/ConformalMap.h

SOURCES += thirdparty/thekla_atlas/src/nvmesh/param/LeastSquaresConformalMap.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmesh/param/LeastSquaresConformalMap.h

SOURCES += thirdparty/thekla_atlas/src/nvmesh/param/OrthogonalProjectionMap.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmesh/param/OrthogonalProjectionMap.h

SOURCES += thirdparty/thekla_atlas/src/nvmesh/param/ParameterizationQuality.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmesh/param/ParameterizationQuality.h

SOURCES += thirdparty/thekla_atlas/src/nvmesh/param/SingleFaceMap.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmesh/param/SingleFaceMap.h

SOURCES += thirdparty/thekla_atlas/src/nvmesh/param/Util.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmesh/param/Util.h

SOURCES += thirdparty/thekla_atlas/src/nvmesh/weld/VertexWeld.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmesh/weld/VertexWeld.h

HEADERS += thirdparty/thekla_atlas/src/nvmesh/weld/Weld.h

SOURCES += thirdparty/thekla_atlas/src/nvmesh/weld/Snap.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmesh/weld/Snap.h

SOURCES += thirdparty/thekla_atlas/src/nvimage/Image.cpp
HEADERS += thirdparty/thekla_atlas/src/nvimage/Image.h

SOURCES += thirdparty/thekla_atlas/src/nvimage/BitMap.cpp
HEADERS += thirdparty/thekla_atlas/src/nvimage/BitMap.h

HEADERS += thirdparty/thekla_atlas/src/nvimage/nvimage.h

SOURCES += thirdparty/thekla_atlas/src/nvmath/Basis.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmath/Basis.h

SOURCES += thirdparty/thekla_atlas/src/nvmath/Box.inl
SOURCES += thirdparty/thekla_atlas/src/nvmath/Box.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmath/Box.h

SOURCES += thirdparty/thekla_atlas/src/nvmath/Color.h

SOURCES += thirdparty/thekla_atlas/src/nvmath/ConvexHull.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmath/ConvexHull.h

SOURCES += thirdparty/thekla_atlas/src/nvmath/Fitting.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmath/Fitting.h

SOURCES += thirdparty/thekla_atlas/src/nvmath/KahanSum.h

SOURCES += thirdparty/thekla_atlas/src/nvmath/Matrix.h

SOURCES += thirdparty/thekla_atlas/src/nvmath/Plane.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmath/Plane.h

SOURCES += thirdparty/thekla_atlas/src/nvmath/ProximityGrid.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmath/ProximityGrid.h

HEADERS += thirdparty/thekla_atlas/src/nvmath/Quaternion.h

SOURCES += thirdparty/thekla_atlas/src/nvmath/Random.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmath/Random.h

SOURCES += thirdparty/thekla_atlas/src/nvmath/Solver.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmath/Solver.h

SOURCES += thirdparty/thekla_atlas/src/nvmath/Sparse.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmath/Sparse.h

SOURCES += thirdparty/thekla_atlas/src/nvmath/TypeSerialization.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmath/TypeSerialization.h

SOURCES += thirdparty/thekla_atlas/src/nvmath/Vector.inl
SOURCES += thirdparty/thekla_atlas/src/nvmath/Vector.cpp
HEADERS += thirdparty/thekla_atlas/src/nvmath/Vector.h

HEADERS += thirdparty/thekla_atlas/src/nvmath/ftoi.h

QMAKE_CXXFLAGS += -std=c++11

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

	exists(/usr/local/opt/opencv) {
		INCLUDEPATH += /usr/local/opt/opencv/include
		LIBS += -L/usr/local/opt/opencv/lib -lopencv_core -lopencv_videoio -lopencv_imgproc
	
		DEFINES += "USE_OPENCV=1"
	}
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
