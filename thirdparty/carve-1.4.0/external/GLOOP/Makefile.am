noinst_LTLIBRARIES=libgloop-math.la libgloop-image.la libgloop-model.la

noinst_HEADERS= include/gloop/exceptions.hpp include/gloop/fbo.hpp	\
	include/gloop/gloop.hpp include/gloop/gloopgl.hpp		\
	include/gloop/gloopglu.hpp include/gloop/gloopglut.hpp		\
	include/gloop/matrix.hpp include/gloop/model/obj_format.hpp	\
	include/gloop/model/ply_format.hpp				\
	include/gloop/model/stream.hpp					\
	include/gloop/model/vtk_format.hpp				\
	include/gloop/quaternion.hpp include/gloop/radiance.hpp		\
	include/gloop/ref.hpp include/gloop/shader.hpp			\
	include/gloop/stringfuncs.hpp include/gloop/surface.hpp		\
	include/gloop/texparam.hpp include/gloop/vbo.hpp		\
	include/gloop/vector.hpp include/gloop/vertexformat.hpp

AM_CPPFLAGS=@CPPFLAGS@ -Iinclude -I@top_srcdir@/external/GLEW/include

libgloop_math_la_SOURCES=src/matrix.cpp src/quaternion.cpp

libgloop_image_la_SOURCES=src/radiance.cpp

libgloop_model_la_SOURCES=src/model/ply_format.cpp	\
	src/model/stream.cpp src/model/obj_format.cpp	\
	src/model/vtk_format.cpp

if with_GUI
  noinst_LTLIBRARIES+=libgloop.la

  libgloop_la_CXXFLAGS= -Iinclude -I@top_srcdir@/external/GLEW/include @GL_CFLAGS@
  libgloop_la_SOURCES=src/shader.cpp src/surface.cpp src/texparam.cpp src/vbo.cpp src/fbo.cpp
endif
