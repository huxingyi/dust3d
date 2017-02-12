cmake_minimum_required(VERSION 2.8)

project(dust3d)

file(GLOB_RECURSE SOURCES 
  src/glw_osx.m
  src/glw_internal.h
  src/glw_style.h
  src/glw.c
  src/glw.h
  src/dmemory.h
  src/dmemory.c
  src/3dstruct.h
  src/array.h
  src/array.c
  src/dict.h
  src/dict.c
  src/bmesh.h
  src/bmesh.c
  src/matrix.h
  src/matrix.c
  src/subdivide.h
  src/subdivide.c
  src/vector3d.h
  src/vector3d.c
  src/convexhull.h
  src/convexhull.c
  src/draw.h
  src/draw.c
  src/skeleton.h
  src/skeleton.c
  src/icons.h
  src/icons.c
  src/editor.c
  src/lodepng.h
  src/lodepng.c
)

IF(APPLE)
  file(GLOB_RECURSE RES_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/res/*.png")
  add_executable(dust3d MACOSX_BUNDLE ${SOURCES} ${RES_SOURCES})
  SET_SOURCE_FILES_PROPERTIES(${RES_SOURCES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  find_package(OpenGL REQUIRED)
  include_directories(${OPENGL_INCLUDE_DIRS})
  target_link_libraries(dust3d ${OPENGL_LIBRARIES})
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework AppKit -framework Quartz")
  set_target_properties(dust3d PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/src/Info.plist)
  install(TARGETS dust3d DESTINATION ".")
ENDIF (APPLE)
