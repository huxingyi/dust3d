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
    src/dust3d.h
    src/dust3d.c
    src/functionlist.h
    src/geometry.h
    src/geometry.c
    src/halfedge.h
    src/halfedge.c
    src/matrix.h
    src/matrix.c
    src/selector.h
    src/selector.c
    src/rbtree.h
    src/rbtree.c
    src/t_add.c
    src/t_chamfer.c
    src/t_extrude.c
    src/t_save.c
    src/t_show.c
    src/t_slice.c
    src/t_subdiv.c
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
add_definitions(-DDUST3D_DEBUG)
add_test(test-add-cube test-add-cube)
