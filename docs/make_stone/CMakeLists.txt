cmake_minimum_required(VERSION 2.8)
project(make_stone)
set(targetName make_stone)
include_directories(../../src)
add_executable(${targetName}
    ../../src/geometry.h
    ../../src/geometry.c
    ../../src/halfedge.h
    ../../src/halfedge.c
    ../../src/matrix.h
    ../../src/matrix.c
    make_stone.c
    ../../src/rbtree.h
    ../../src/rbtree.c
    ../../src/vector3d.h
    ../../src/vector3d.c
)
add_definitions(-DDUST3D_DEBUG)
