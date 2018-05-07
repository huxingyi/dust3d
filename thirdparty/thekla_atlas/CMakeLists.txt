cmake_minimum_required(VERSION 2.8.8)
project(thekla_atlas)

#add_definitions(-DNV_OS_DARWIN=1)

include_directories(
    src
    src/nvcore
    extern/tinyobj
    extern/poshlib)

add_subdirectory(extern/poshlib)
add_subdirectory(src/nvcore)
add_subdirectory(src/nvimage)
add_subdirectory(src/nvmath)
add_subdirectory(src/nvmesh)

add_executable(
    thekla_atlas_test
    src/thekla/thekla_atlas_test.cpp
    src/thekla/thekla_atlas.cpp)

target_link_libraries(
    thekla_atlas_test
    nvmesh)
