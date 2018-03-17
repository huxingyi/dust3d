lib_LTLIBRARIES=libintersect.la

noinst_HEADERS=csg_collector.hpp intersect_classify_common.hpp	\
	intersect_classify_common_impl.hpp intersect_common.hpp	\
	intersect_debug.hpp

AM_CPPFLAGS=@CPPFLAGS@ -I$(top_srcdir)/include

libintersect_la_SOURCES=aabb.cpp carve.cpp convex_hull.cpp csg.cpp	\
	csg_collector.cpp geom2d.cpp geom3d.cpp polyhedron.cpp		\
	intersect.cpp intersection.cpp intersect_debug.cpp		\
	intersect_group.cpp intersect_classify_group.cpp		\
	intersect_half_classify_group.cpp intersect_face_division.cpp	\
	intersect_classify_edge.cpp octree.cpp polyline.cpp math.cpp	\
	edge.cpp face.cpp tag.cpp timing.cpp triangulator.cpp		\
	pointset.cpp
