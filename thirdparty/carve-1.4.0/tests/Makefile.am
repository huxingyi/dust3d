CPPFLAGS += -I$(top_srcdir)/common -I$(top_srcdir)/include @GL_CFLAGS@ @GLUT_CFLAGS@
CPPFLAGS += -I$(top_srcdir)/external/GLOOP/include

noinst_HEADERS = mersenne_twister.h

noinst_PROGRAMS = test_geom test_eigen test_spacetree test_aabb test_aabb_tri test_rescale tetrahedron



test_geom_SOURCES=test_geom.cpp
test_geom_LDADD=../lib/libintersect.la

test_eigen_SOURCES=test_eigen.cpp
test_eigen_LDADD=../lib/libintersect.la

test_spacetree_SOURCES=test_spacetree.cpp
test_spacetree_LDADD=../lib/libintersect.la

test_aabb_SOURCES=test_aabb.cpp
test_aabb_LDADD=../lib/libintersect.la

test_aabb_tri_SOURCES=test_aabb_tri.cpp
test_aabb_tri_LDADD=../lib/libintersect.la

test_rescale_SOURCES=test_rescale.cpp
test_rescale_LDADD=../lib/libintersect.la

tetrahedron_SOURCES=tetrahedron.cpp
tetrahedron_LDADD=../lib/libintersect.la



if with_GUI
  CPPFLAGS += -I$(top_srcdir)/external/GLEW/include
  CPPFLAGS += -I$(top_srcdir)/external/GLUI/include
  noinst_PROGRAMS += test_intersect test_interpolate test_csg_interpolate test_slice test_slice_classify test_triangulate test_hole_incorporate problem
endif



test_intersect_SOURCES=test_intersect.cpp
test_intersect_LDADD=../common/libcarve_fileformats.la ../common/libcarve_ui.la ../common/libcarve_misc.la ../lib/libintersect.la @GL_LIBS@ @GLUT_LIBS@

test_interpolate_SOURCES=test_interpolate.cpp
test_interpolate_LDADD=../common/libcarve_ui.la ../lib/libintersect.la @GL_LIBS@ @GLUT_LIBS@

test_csg_interpolate_SOURCES=test_csg_interpolate.cpp
test_csg_interpolate_LDADD=../common/libcarve_ui.la ../common/libcarve_misc.la ../lib/libintersect.la @GL_LIBS@ @GLUT_LIBS@

test_slice_SOURCES=test_slice.cpp
test_slice_LDADD=../common/libcarve_ui.la ../common/libcarve_misc.la ../lib/libintersect.la @GL_LIBS@ @GLUT_LIBS@

test_slice_classify_SOURCES=test_slice_classify.cpp
test_slice_classify_LDADD=../common/libcarve_ui.la ../common/libcarve_misc.la ../lib/libintersect.la @GL_LIBS@ @GLUT_LIBS@

test_triangulate_SOURCES=test_triangulate.cpp
test_triangulate_LDADD=../common/libcarve_ui.la ../common/libcarve_misc.la ../lib/libintersect.la @GL_LIBS@ @GLUT_LIBS@

test_hole_incorporate_SOURCES=test_hole_incorporate.cpp
test_hole_incorporate_LDADD=../common/libcarve_ui.la ../common/libcarve_misc.la ../lib/libintersect.la @GL_LIBS@ @GLUT_LIBS@

problem_SOURCES=problem.cpp
problem_LDADD=../common/libcarve_ui.la ../lib/libintersect.la @GL_LIBS@ @GLUT_LIBS@
