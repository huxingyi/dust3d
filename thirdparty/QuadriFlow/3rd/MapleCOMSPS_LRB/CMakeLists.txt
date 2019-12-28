cmake_minimum_required(VERSION 3.1)
project(MapleCOMSPS_LRB)

find_package(ZLIB)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(${ZLIB_INCLUDE_DIRS})

set(
    minisat_SRC
    core/Solver.cc
    simp/Main.cc
    simp/SimpSolver.cc
    utils/Options.cc
    utils/System.cc
)

add_executable(
    minisat
    ${minisat_SRC}
)

target_link_libraries(
    minisat
    ${ZLIB_LIBRARIES}
)
