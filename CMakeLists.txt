cmake_minimum_required(VERSION 2.8)
enable_testing()
project(dust3d)
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_package(glfw3 REQUIRED)
include_directories(
    ${OPENGL_INCLUDE_DIRS}
    ${GLUT_INCLUDE_DIRS}
    src
    3rdparty/glad/include
    #deps/imgui
    #deps/imgui/examples/opengl2_example
)
link_libraries(
    ${OPENGL_LIBRARIES}
    ${GLUT_LIBRARY}
)
set(SOURCES
    src/array.h
    src/array.c
    src/color.h
    src/color.c
    src/convexhull.h
    src/convexhull.c
    src/dust3d.h
    src/dust3d.c
    src/editor.h
    src/editor.c
    src/functionlist.h
    src/halfedge.h
    src/halfedge.c
    src/math3d.h
    src/math3d.c
    src/matrix4x4.h
    src/matrix4x4.c
    src/selector.h
    src/selector.c
    src/standard.h
    src/standard.c
    src/rbtree.h
    src/rbtree.c
    src/subdivision.h
    src/subdivision.c
    src/t_add.c
    src/t_chamfer.c
    src/t_diff.c
    src/t_extrude.c
    src/t_inter.c
    src/t_move.c
    src/t_new.c
    src/t_save.c
    src/t_scale.c
    src/t_sel.c
    src/t_show.c
    src/t_slice.c
    src/t_subdiv.c
    src/t_union.c
    src/t_wrap.c
    src/util.h
    src/util.c
    src/vector3d.h
    src/vector3d.c
)
add_executable(dust3d
    #deps/imgui/imconfig.h
    #deps/imgui/imgui_demo.cpp
    #deps/imgui/imgui_draw.cpp
    #deps/imgui/imgui_internal.h
    #deps/imgui/imgui.cpp
    #deps/imgui/imgui.h
    #deps/imgui/stb_rect_pack.h
    #deps/imgui/stb_textedit.h
    #deps/imgui/stb_truetype.h
    #deps/imgui/examples/opengl2_example/imgui_impl_glfw.h
    #deps/imgui/examples/opengl2_example/imgui_impl_glfw.cpp
    3rdparty/glad/src/glad.c
    src/render.h
    src/render.c
    #src/viewer.cpp
    ${SOURCES}
)
target_link_libraries(dust3d glfw)
add_executable(test-add-cube
    tests/test-add-cube.c
    ${SOURCES}
)
add_executable(test-add-obj
    tests/test-add-obj.c
    ${SOURCES}
)
add_executable(test-slice-by-face
    tests/test-slice-by-face.c
    ${SOURCES}
)
add_executable(test-chamfer-edge
    tests/test-chamfer-edge.c
    ${SOURCES}
)
add_executable(test-inter
    tests/test-inter.c
    ${SOURCES}
)
add_executable(test-add-stone
    tests/test-add-stone.c
    ${SOURCES}
)
add_definitions(-DDUST3D_DEBUG)
add_test(test-add-cube test-add-cube)
add_test(test-add-obj test-add-obj)
add_test(test-slice-by-face test-slice-by-face)
add_test(test-chamfer-edge test-chamfer-edge)
add_test(test-inter test-inter)
add_test(test-add-stone test-add-stone)
