cmake_minimum_required(VERSION 2.8)
project(dust3d)
set(targetName dust3d)
include_directories(src)
add_executable(${targetName}
    src/console.c
    src/geometry.h
    src/geometry.c
    src/halfedge.h
    src/halfedge.c
    src/matrix.h
    src/matrix.c
    src/parser.h
    src/parser.c
    src/rbtree.h
    src/rbtree.c
    src/vector3d.h
    src/vector3d.c
)
add_definitions(-DDUST3D_DEBUG)
