cmake_minimum_required(VERSION 3.8)
project(fbx-writer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -std=c++17 -lstdc++")

find_package( ZLIB REQUIRED )

include_directories( ${ZLIB_INCLUDE_DIRS} )
    
set(SOURCE_FILES fbxdocument.cpp fbxnode.cpp fbxutil.cpp fbxproperty.cpp)

add_executable(fbx-writer main.cpp ${SOURCE_FILES})
target_link_libraries(fbx-writer ${ZLIB_LIBRARIES})

add_executable(fbxdump fbxdump.cpp ${SOURCE_FILES})
target_link_libraries(fbxdump ${ZLIB_LIBRARIES})
