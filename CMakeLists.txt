cmake_minimum_required(VERSION 2.8)
enable_testing()
project(dust3d)
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
include_directories(
    src
    ${OPENGL_INCLUDE_DIRS}
    ${GLUT_INCLUDE_DIRS}
)
link_libraries(
    ${OPENGL_LIBRARIES}
    ${GLUT_LIBRARY}
)
set(SOURCES
    src/array.h
    src/array.c
    src/color.h
    src/color.c
    src/dust3d.h
    src/dust3d.c
    src/functionlist.h
    src/halfedge.h
    src/halfedge.c
    src/math3d.h
    src/math3d.c
    src/matrix.h
    src/matrix.c
    src/selector.h
    src/selector.c
    src/strutil.h
    src/strutil.c
    src/rbtree.h
    src/rbtree.c
    src/t_add.c
    src/t_chamfer.c
    src/t_diff.c
    src/t_extrude.c
    src/t_inter.c
    src/t_move.c
    src/t_new.c
    src/t_save.c
    src/t_sel.c
    src/t_show.c
    src/t_slice.c
    src/t_subdiv.c
    src/t_union.c
    src/util.h
    src/util.c
    src/vector3d.h
    src/vector3d.c
)
add_executable(dust3d
    src/main.c
    ${SOURCES}
)
add_executable(test-add-cube
    tests/test-add-cube.c
    ${SOURCES}
)
add_executable(test-add-obj
    tests/test-add-obj.c
    ${SOURCES}
)
add_executable(test-slice-by-face
    tests/test-slice-by-face.c
    ${SOURCES}
)
add_executable(test-chamfer-edge
    tests/test-chamfer-edge.c
    ${SOURCES}
)
add_executable(test-inter
    tests/test-inter.c
    ${SOURCES}
)
add_definitions(-DDUST3D_DEBUG)
add_test(test-add-cube test-add-cube)
add_test(test-add-obj test-add-obj)
add_test(test-slice-by-face test-slice-by-face)
add_test(test-chamfer-edge test-chamfer-edge)
add_test(test-inter test-inter)
