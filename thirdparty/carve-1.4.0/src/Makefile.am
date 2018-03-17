noinst_HEADERS= glu_triangulator.hpp

CPPFLAGS += -I$(top_srcdir)/common -I$(top_srcdir)/include @GL_CFLAGS@ @GLUT_CFLAGS@
CPPFLAGS += -I$(top_srcdir)/external/GLOOP/include

bin_PROGRAMS = intersect triangulate convert
noinst_PROGRAMS = cutgraph



triangulate_SOURCES=triangulate.cpp
triangulate_LDADD=../common/libcarve_fileformats.la ../lib/libintersect.la @GL_LIBS@

cutgraph_SOURCES=cut.cpp
cutgraph_LDADD=../common/libcarve_misc.la ../lib/libintersect.la ../common/libcarve_fileformats.la

convert_SOURCES=convert.cpp
convert_LDADD=../common/libcarve_fileformats.la ../common/libcarve_misc.la ../lib/libintersect.la



if enable_GLU_tri
  intersect_SOURCES=intersect.cpp glu_triangulator.cpp
  intersect_CPPFLAGS=
  intersect_LDADD=../common/libcarve_fileformats.la ../common/libcarve_misc.la ../lib/libintersect.la @GL_LIBS@
else
  intersect_SOURCES=intersect.cpp
  intersect_CPPFLAGS=-DDISABLE_GLU_TRIANGULATOR
  intersect_LDADD=../common/libcarve_fileformats.la ../common/libcarve_misc.la ../lib/libintersect.la
endif



if with_GUI
  bin_PROGRAMS += view
  CPPFLAGS += -I$(top_srcdir)/external/GLEW/include
  CPPFLAGS += -I$(top_srcdir)/external/GLUI/include
endif



view_SOURCES=view.cpp
view_LDADD=../common/libcarve_fileformats.la ../common/libcarve_ui.la ../common/libcarve_misc.la ../lib/libintersect.la @GL_LIBS@ @GLUT_LIBS@
