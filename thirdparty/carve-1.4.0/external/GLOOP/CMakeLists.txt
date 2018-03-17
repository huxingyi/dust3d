cmake_minimum_required(VERSION 2.6)

project(gloop)

include_directories("${gloop_SOURCE_DIR}/include")
include_directories("${glew_SOURCE_DIR}/include")

find_package(OpenGL)
include_directories(${OPENGL_INCLUDE_DIR})

add_library(gloop_math STATIC src/matrix.cpp src/quaternion.cpp)

add_library(gloop_image STATIC src/radiance.cpp)

add_library(gloop_model STATIC  src/model/stream.cpp src/model/ply_format.cpp src/model/obj_format.cpp src/model/vtk_format.cpp)

if(CARVE_WITH_GUI)
  add_library(gloop STATIC src/shader.cpp src/surface.cpp src/texparam.cpp src/vbo.cpp src/fbo.cpp)
  target_link_libraries(gloop ${OPENGL_LIBRARIES})
endif(CARVE_WITH_GUI)
