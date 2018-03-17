noinst_LTLIBRARIES = libcarve_fileformats.la libcarve_misc.la

noinst_HEADERS = geom_draw.hpp geometry.hpp opts.hpp read_ply.hpp rgb.hpp scene.hpp stringfuncs.hpp write_ply.hpp

CPPFLAGS += -I$(top_srcdir)/include @GL_CFLAGS@ @GLUT_CFLAGS@
CPPFLAGS += -I$(top_srcdir)/external/GLOOP/include



libcarve_fileformats_la_SOURCES = read_ply.cpp write_ply.cpp
libcarve_fileformats_la_LIBADD = $(top_srcdir)/external/GLOOP/libgloop-model.la
libcarve_fileformats_la_CPPFLAGS = -I$(top_srcdir)/external/GLOOP/include



libcarve_misc_la_SOURCES=geometry.cpp



if with_GUI
  CPPFLAGS += -I$(top_srcdir)/external/GLEW/include
  CPPFLAGS += -I$(top_srcdir)/external/GLUI/include
  noinst_LTLIBRARIES += libcarve_ui.la
endif



libcarve_ui_la_SOURCES = geom_draw.cpp scene.cpp
libcarve_ui_la_LIBADD = $(top_srcdir)/external/GLUI/libglui.la
