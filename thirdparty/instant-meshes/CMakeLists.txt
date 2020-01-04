cmake_minimum_required (VERSION 2.8.3)
project(InstantMeshes)

set(INSTANT_MESHES_RELATIVE_ROOT instant-meshes-dust3d)
set(INSTANT_MESHES_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/${INSTANT_MESHES_RELATIVE_ROOT})

option(INSTANT_MESHES_DEV_MODE "Instant meshes developer mode" OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()
string(TOUPPER "${CMAKE_BUILD_TYPE}" U_CMAKE_BUILD_TYPE)

# Enable folders for projects in Visual Studio
if (CMAKE_GENERATOR MATCHES "Visual Studio")
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

# Sanitize build environment for static build with C++11
if (MSVC)
  add_definitions (/D "_CRT_SECURE_NO_WARNINGS")
  add_definitions (/D "__TBB_NO_IMPLICIT_LINKAGE")
  add_definitions (/D "DUST3D_INSTANT_MESHES_API_EXPORTS")
  add_definitions (/D "_USE_MATH_DEFINES")
  set(DUST3D_INSTANT_MESHES_API_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/instant-meshes-api.h
    ${CMAKE_CURRENT_SOURCE_DIR}/instant-meshes-api.cpp
    ${INSTANT_MESHES_ROOT}/../../simpleuv/simpleuv/triangulate.h
    ${INSTANT_MESHES_ROOT}/../../simpleuv/simpleuv/triangulate.cpp)

  # Parallel build on MSVC (all targets)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

  if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")

    # Disable Eigen vectorization for Windows 32 bit builds (issues with unaligned access segfaults)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DEIGEN_DONT_ALIGN")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Od")
  endif()

  # Static build
  set(CompilerFlags
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
  foreach(CompilerFlag ${CompilerFlags})
    string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
  endforeach()
elseif(APPLE)
  # Try to auto-detect a suitable SDK
  # Commented out for now -- causes a too new SDK to be selected on AppVeyor
  #execute_process(COMMAND bash -c "xcodebuild -version -sdk | grep MacOSX | grep Path | head -n 1 | cut -f 2 -d ' '" OUTPUT_VARIABLE CMAKE_OSX_SYSROOT)
  #string(REGEX REPLACE "(\r?\n)+$" "" CMAKE_OSX_SYSROOT "${CMAKE_OSX_SYSROOT}")
  #string(REGEX REPLACE "^.*X([0-9.]*).sdk$" "\\1" CMAKE_OSX_DEPLOYMENT_TARGET "${CMAKE_OSX_SYSROOT}")
endif()

# Enable C++11 mode on GCC / Clang
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

# Build TBB
if(MSVC)
    set(TBB_BUILD_STATIC OFF CACHE BOOL " " FORCE)
    set(TBB_BUILD_SHARED ON CACHE BOOL " " FORCE)
else()
    set(TBB_BUILD_STATIC ON CACHE BOOL " " FORCE)
    set(TBB_BUILD_SHARED OFF CACHE BOOL " " FORCE)
endif()
set(TBB_BUILD_TBBMALLOC OFF CACHE BOOL " " FORCE)
set(TBB_BUILD_TBBMALLOC_PROXY OFF CACHE BOOL " " FORCE)
set(TBB_BUILD_TESTS OFF CACHE BOOL " " FORCE)

add_subdirectory(${INSTANT_MESHES_ROOT}/ext/tbb ext_build/tbb)
if(MSVC)
    # Build dll on windows
else()
    set_property(TARGET tbb_static tbb_def_files PROPERTY FOLDER "dependencies")
endif()

# Compile instant meshes with various compiler warnings turned on
if(MSVC)
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-gnu-anonymous-struct -Wno-c99-extensions -Wno-nested-anon-types")
  endif()
endif()

# Header file directories of dependency libraries
include_directories(
  # Intel Thread Building Blocks
  ${INSTANT_MESHES_ROOT}/ext/tbb/include
  # Disjoint set data structure
  ${INSTANT_MESHES_ROOT}/ext/dset
  # Parallel stable sort
  ${INSTANT_MESHES_ROOT}/ext/pss
  # Pseudorandom number generator
  ${INSTANT_MESHES_ROOT}/ext/pcg32
  # RPLY mesh I/O library
  ${INSTANT_MESHES_ROOT}/ext/rply
  # Half precision type
  ${INSTANT_MESHES_ROOT}/ext/half
  # Eigen
  ${INSTANT_MESHES_ROOT}/../../simpleuv/thirdparty/eigen
  # Export API
  ${INSTANT_MESHES_ROOT}/../../simpleuv
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${INSTANT_MESHES_ROOT}/src
)

if (INSTANT_MESHES_DEV_MODE)
  add_definitions(-DDEV_MODE)
endif()

if (WIN32)
    add_library(instant-meshes SHARED "")
else()
    add_library(instant-meshes STATIC "")
endif()

target_sources(instant-meshes PRIVATE
  ${INSTANT_MESHES_RELATIVE_ROOT}/src/aabb.h
  ${INSTANT_MESHES_RELATIVE_ROOT}/src/common.h
  ${INSTANT_MESHES_RELATIVE_ROOT}/src/meshio.h ${INSTANT_MESHES_RELATIVE_ROOT}/src/meshio.cpp
  ${INSTANT_MESHES_RELATIVE_ROOT}/src/normal.h ${INSTANT_MESHES_RELATIVE_ROOT}/src/normal.cpp
  ${INSTANT_MESHES_RELATIVE_ROOT}/src/adjacency.h ${INSTANT_MESHES_RELATIVE_ROOT}/src/adjacency.cpp
  ${INSTANT_MESHES_RELATIVE_ROOT}/src/meshstats.h ${INSTANT_MESHES_RELATIVE_ROOT}/src/meshstats.cpp
  ${INSTANT_MESHES_RELATIVE_ROOT}/src/hierarchy.h ${INSTANT_MESHES_RELATIVE_ROOT}/src/hierarchy.cpp
  ${INSTANT_MESHES_RELATIVE_ROOT}/src/extract.h ${INSTANT_MESHES_RELATIVE_ROOT}/src/extract.cpp
  ${INSTANT_MESHES_RELATIVE_ROOT}/src/field.h ${INSTANT_MESHES_RELATIVE_ROOT}/src/field.cpp
  ${INSTANT_MESHES_RELATIVE_ROOT}/src/bvh.h ${INSTANT_MESHES_RELATIVE_ROOT}/src/bvh.cpp
  ${INSTANT_MESHES_RELATIVE_ROOT}/src/subdivide.h ${INSTANT_MESHES_RELATIVE_ROOT}/src/subdivide.cpp
  ${INSTANT_MESHES_RELATIVE_ROOT}/src/reorder.h ${INSTANT_MESHES_RELATIVE_ROOT}/src/reorder.cpp
  ${INSTANT_MESHES_RELATIVE_ROOT}/src/serializer.h ${INSTANT_MESHES_RELATIVE_ROOT}/src/serializer.cpp
  ${INSTANT_MESHES_RELATIVE_ROOT}/src/smoothcurve.h ${INSTANT_MESHES_RELATIVE_ROOT}/src/smoothcurve.cpp
  ${INSTANT_MESHES_RELATIVE_ROOT}/src/cleanup.h ${INSTANT_MESHES_RELATIVE_ROOT}/src/cleanup.cpp
  ${INSTANT_MESHES_RELATIVE_ROOT}/src/dedge.h ${INSTANT_MESHES_RELATIVE_ROOT}/src/dedge.cpp
  ${INSTANT_MESHES_RELATIVE_ROOT}/ext/rply/rply.c
  ${DUST3D_INSTANT_MESHES_API_SOURCES})

if (WIN32)
    target_link_libraries(instant-meshes tbb)
else()
    target_link_libraries(instant-meshes tbb_static)
endif()

if (WIN32)
  # Quench some warnings on MSVC
  if (MSVC)
    set_source_files_properties(${INSTANT_MESHES_RELATIVE_ROOT}/ext/rply/rply.c PROPERTIES COMPILE_FLAGS "/wd4127")
  endif()
elseif (APPLE)
  # Build an application bundle on OSX
  # set_target_properties(instant-meshes PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "Instant Meshes")
  # set_target_properties(instant-meshes PROPERTIES MACOSX_BUNDLE_BUNDLE_GUI_IDENTIFIER "com.im.InstantMeshes")
  # set_target_properties(instant-meshes PROPERTIES MACOSX_BUNDLE_ICON_FILE im.icns)
  # set_target_properties(instant-meshes PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/resources/MacOSXBundleInfo.plist.in)
  # set_source_files_properties(resources/im.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
else()
  # Insulate from a few types of ABI changes by statically linking against libgcc and libstdc++
  set_target_properties(instant-meshes PROPERTIES LINK_FLAGS "-static-libgcc")
  #set_target_properties(instant-meshes PROPERTIES LINK_FLAGS "-static-libgcc -static-libstdc++")
endif()
