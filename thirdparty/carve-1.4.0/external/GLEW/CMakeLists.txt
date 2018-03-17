cmake_minimum_required(VERSION 2.6)

project(glew)

find_package(OpenGL)

include_directories("${glew_SOURCE_DIR}/include")

add_library(glew STATIC src/glew.c)

add_definitions(-DGLEW_STATIC)

include_directories(${OPENGL_INCLUDE_DIR})

target_link_libraries(glew ${OPENGL_LIBRARIES})
