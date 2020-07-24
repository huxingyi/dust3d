cmake_minimum_required(VERSION 3.10)

project(nodes2mesh)

link_directories(
    ../../release
)

add_executable(nodes2mesh 
    nodes2mesh.cpp
)

target_include_directories(nodes2mesh PUBLIC
    ../../../include
)

target_link_libraries(nodes2mesh PUBLIC 
    libdust3d
)
