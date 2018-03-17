noinst_PROGRAMS = 
noinst_HEADERS = brick_texture.h carve_texture.h leaf_texture.h

CPPFLAGS += -I$(top_srcdir)/common -I$(top_srcdir)/include @GL_CFLAGS@ @GLUT_CFLAGS@
CPPFLAGS += -I$(top_srcdir)/external/GLOOP/include

if with_GUI
  CPPFLAGS += -I$(top_srcdir)/external/GLEW/include
  CPPFLAGS += -I$(top_srcdir)/external/GLUI/include
  noinst_PROGRAMS += custom_collector texture_example
endif

custom_collector_SOURCES=custom_collector.cpp
custom_collector_LDADD=../common/libcarve_fileformats.la ../common/libcarve_ui.la ../common/libcarve_misc.la ../lib/libintersect.la @GL_LIBS@ @GLUT_LIBS@

texture_example_SOURCES=texture_example.cpp
texture_example_LDADD=../common/libcarve_fileformats.la ../common/libcarve_ui.la ../common/libcarve_misc.la ../lib/libintersect.la @GL_LIBS@ @GLUT_LIBS@
