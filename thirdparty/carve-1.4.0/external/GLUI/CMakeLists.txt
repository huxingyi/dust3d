cmake_minimum_required(VERSION 2.6)

find_package(OpenGL)
find_package(GLUT)

project(glui)

include_directories("${glui_SOURCE_DIR}/include")

add_definitions(-DGLUI_NO_LIB_PRAGMA)
add_definitions(-DGLUI_USE_STATIC_LIB)

include_directories(${OPENGL_INCLUDE_DIR})
include_directories(${GLUT_INCLUDE_DIR})

add_library(glui STATIC 
            src/algebra3.cpp
            src/arcball.cpp
            src/arcball.h
            src/glui.cpp
            src/glui_add_controls.cpp
            src/glui_bitmap_img_data.cpp
            src/glui_bitmaps.cpp
            src/glui_button.cpp
            src/glui_checkbox.cpp
            src/glui_column.cpp
            src/glui_commandline.cpp
            src/glui_control.cpp
            src/glui_edittext.cpp
            src/glui_filebrowser.cpp
            src/glui_list.cpp
            src/glui_listbox.cpp
            src/glui_mouse_iaction.cpp
            src/glui_node.cpp
            src/glui_panel.cpp
            src/glui_radio.cpp
            src/glui_rollout.cpp
            src/glui_rotation.cpp
            src/glui_scrollbar.cpp
            src/glui_separator.cpp
            src/glui_spinner.cpp
            src/glui_statictext.cpp
            src/glui_string.cpp
            src/glui_textbox.cpp
            src/glui_translation.cpp
            src/glui_tree.cpp
            src/glui_treepanel.cpp
            src/glui_window.cpp
            src/quaternion.cpp
            src/viewmodel.cpp)

target_link_libraries(glui ${OPENGL_LIBRARIES} ${GLUT_LIBRARY})
