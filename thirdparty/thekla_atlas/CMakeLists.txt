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

set(CMAKE_DEBUG_POSTFIX d)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--export-all-symbols")

add_library(libthekla_atlas
	src/thekla/thekla_atlas.cpp
)

install(TARGETS libthekla_atlas DESTINATION lib/static)

get_property(HEADER_DIRS DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)

configure_file(theklaAtlasConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/theklaAtlasConfig.cmake @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/theklaAtlasConfig.cmake DESTINATION .)

file(GLOB HEADERS src/*.h)
install(FILES ${HEADERS} DESTINATION include)
file(GLOB HEADERS src/thekla/*.h)
install(FILES ${HEADERS} DESTINATION include/thekla)
file(GLOB HEADERS src/nvcore/*.h)
install(FILES ${HEADERS} DESTINATION include/nvcore)
file(GLOB HEADERS src/nvimage/*.h)
install(FILES ${HEADERS} DESTINATION include/nvimage)
file(GLOB HEADERS src/nvmesh/*.h)
install(FILES ${HEADERS} DESTINATION include/nvmesh)
file(GLOB HEADERS src/nvmath/*.h)
install(FILES ${HEADERS} DESTINATION include/nvmath)
