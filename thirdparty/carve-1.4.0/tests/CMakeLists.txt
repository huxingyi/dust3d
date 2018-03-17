SET(GTEST_DIR "${carve_SOURCE_DIR}/external/gtest-1.5.0")

include_directories(${GTEST_DIR})
include_directories(${GTEST_DIR}/include)

function(cxx_test_with_flags name cxx_flags libs)
  add_executable(${name} ${ARGN})
  set_target_properties(${name}
    PROPERTIES
    COMPILE_FLAGS "${cxx_flags}")
  # To support mixing linking in static and dynamic libraries, link each
  # library in with an extra call to target_link_libraries.
  foreach (lib "${libs}")
    target_link_libraries(${name} ${lib})
  endforeach()
  add_test(${name} ${name})
endfunction()

# cxx_test(name libs srcs...)
#
# creates a named test target that depends on the given libs and is
# built from the given source files.  Unlike cxx_test_with_flags,
# test/name.cc is already implicitly included in the source file list.
function(cxx_test name libs)
  cxx_test_with_flags("${name}" "${cxx_default}" "${libs}" "${name}.cpp" ${ARGN})
endfunction()

cxx_test(geom2d_unittest gtest_main)
target_link_libraries(geom2d_unittest carve)
cxx_test(geom3d_unittest gtest_main)
target_link_libraries(geom3d_unittest carve)
# not committed yet.
# cxx_test(exact_unittest gtest_main)
# target_link_libraries(exact_unittest carve)
# cxx_test(shewchuk_unittest gtest_main)

include_directories("${carve_SOURCE_DIR}/include")
include_directories("${carve_SOURCE_DIR}/external/GLOOP/include")
include_directories("${carve_SOURCE_DIR}/external/GLEW/include")
include_directories("${carve_SOURCE_DIR}/external/GLUI/include")
include_directories("${carve_SOURCE_DIR}/common")
include_directories(${OPENGL_INCLUDE_DIR})

link_directories("${gloop_BINARY_DIR}")
link_directories("${carve_BINARY_DIR}/lib")
link_directories("${carve_BINARY_DIR}/common")

add_executable       (test_carve_polyhedrons_2          test_carve_polyhedrons_2.cpp)
target_link_libraries(test_carve_polyhedrons_2          carve carve_fileformats gloop_model)

add_executable       (test_geom          test_geom.cpp)
target_link_libraries(test_geom          carve)

add_executable       (test_eigen         test_eigen.cpp)
target_link_libraries(test_eigen         carve)

add_executable       (test_spacetree     test_spacetree.cpp)
target_link_libraries(test_spacetree     carve)

add_executable       (test_aabb          test_aabb.cpp)
target_link_libraries(test_aabb          carve)

add_executable       (test_rescale       test_rescale.cpp)
target_link_libraries(test_rescale       carve)

add_executable       (tetrahedron        tetrahedron.cpp)
target_link_libraries(tetrahedron        carve)

if(CARVE_WITH_GUI)
  add_executable       (test_intersect     test_intersect.cpp)
  target_link_libraries(test_intersect     carve carve_fileformats carve_ui carve_misc glui gloop_model ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES})

  add_executable       (test_interpolate   test_interpolate.cpp)
  target_link_libraries(test_interpolate   carve carve_ui glui ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES})

  add_executable       (test_csg_interpolate   test_csg_interpolate.cpp)
  target_link_libraries(test_csg_interpolate   carve carve_ui carve_misc carve_fileformats gloop_model glui ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES})

  add_executable       (test_slice             test_slice.cpp)
  target_link_libraries(test_slice             carve carve_ui carve_misc carve_fileformats gloop_model glui ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES})

  add_executable       (test_slice_classify    test_slice_classify.cpp)
  target_link_libraries(test_slice_classify    carve carve_ui carve_misc carve_fileformats gloop_model glui ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES})

  add_executable       (test_triangulate       test_triangulate.cpp)
  target_link_libraries(test_triangulate       carve carve_ui carve_misc glui ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES})

  add_executable       (test_hole_incorporate  test_hole_incorporate.cpp)
  target_link_libraries(test_hole_incorporate  carve carve_ui carve_misc glui ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES})
endif(CARVE_WITH_GUI)
