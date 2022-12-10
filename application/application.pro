QT += core gui opengl widgets svg network

TARGET = dust3d
TEMPLATE = app

HUMAN_VERSION = "1.0.0-rc.7"
VERSION = 1.0.0.37

QMAKE_TARGET_COMPANY = Dust3D
QMAKE_TARGET_PRODUCT = Dust3D
QMAKE_TARGET_DESCRIPTION = "Dust3D is a cross-platform open-source 3D modeling software"
QMAKE_TARGET_COPYRIGHT = "Copyright (C) 2018-2021 Dust3D Project. All Rights Reserved."

HOMEPAGE_URL = "https://dust3d.org/"
REPOSITORY_URL = "https://github.com/huxingyi/dust3d"
ISSUES_URL = "https://github.com/huxingyi/dust3d/issues"
REFERENCE_GUIDE_URL = "https://docs.dust3d.org"
UPDATES_CHECKER_URL = "https://dust3d.org/dust3d-updateinfo.xml"

DEFINES += QT_MESSAGELOGCONTEXT
DEFINES += _USE_MATH_DEFINES
CONFIG += object_parallel_to_source
CONFIG += no_batch

CONFIG(release, debug|release) {
    win32 {
        QMAKE_CXXFLAGS += /MP
        QMAKE_CXXFLAGS += /O2
        QMAKE_CXXFLAGS += /bigobj
        
        CONFIG += force_debug_info
    }

    macx {
        QMAKE_CXXFLAGS_RELEASE -= -O
        QMAKE_CXXFLAGS_RELEASE -= -O1
        QMAKE_CXXFLAGS_RELEASE -= -O2

        QMAKE_CXXFLAGS_RELEASE += -O3
    }

    unix:!macx {
        QMAKE_CXXFLAGS_RELEASE -= -O
        QMAKE_CXXFLAGS_RELEASE -= -O1
        QMAKE_CXXFLAGS_RELEASE -= -O2

        QMAKE_CXXFLAGS_RELEASE += -O3
    }
}

PLATFORM = "Unknown"
macx {
    PLATFORM = "MacOS"
}
win32 {
    PLATFORM = "Win32"
}
unix:!macx {
    PLATFORM = "Linux"
}

DEFINES += "PROJECT_DEFINED_APP_COMPANY=\"\\\"$$QMAKE_TARGET_COMPANY\\\"\""
DEFINES += "PROJECT_DEFINED_APP_NAME=\"\\\"$$QMAKE_TARGET_PRODUCT\\\"\""
DEFINES += "PROJECT_DEFINED_APP_VER=\"\\\"$$VERSION\\\"\""
DEFINES += "PROJECT_DEFINED_APP_HUMAN_VER=\"\\\"$$HUMAN_VERSION\\\"\""
DEFINES += "PROJECT_DEFINED_APP_HOMEPAGE_URL=\"\\\"$$HOMEPAGE_URL\\\"\""
DEFINES += "PROJECT_DEFINED_APP_REPOSITORY_URL=\"\\\"$$REPOSITORY_URL\\\"\""
DEFINES += "PROJECT_DEFINED_APP_ISSUES_URL=\"\\\"$$ISSUES_URL\\\"\""
DEFINES += "PROJECT_DEFINED_APP_REFERENCE_GUIDE_URL=\"\\\"$$REFERENCE_GUIDE_URL\\\"\""
DEFINES += "PROJECT_DEFINED_APP_UPDATES_CHECKER_URL=\"\\\"$$UPDATES_CHECKER_URL\\\"\""
DEFINES += "PROJECT_DEFINED_APP_PLATFORM=\"\\\"$$PLATFORM\\\"\""

OBJECTS_DIR = obj
MOC_DIR = moc

win32 {
    RC_FILE = $${SOURCE_ROOT}dust3d.rc
}
macx {
    ICON = $${SOURCE_ROOT}dust3d.icns

    RESOURCE_FILES.files = $$ICON
    RESOURCE_FILES.path = Contents/Resources
    QMAKE_BUNDLE_DATA += RESOURCE_FILES
}

RESOURCES += resources.qrc

INCLUDEPATH += ../
INCLUDEPATH += ../third_party
INCLUDEPATH += ../third_party/rapidxml-1.13
INCLUDEPATH += ../third_party/earcut.hpp/include

include(third_party/QtAwesome/QtAwesome/QtAwesome.pri)

HEADERS += sources/about_widget.h
SOURCES += sources/about_widget.cc
HEADERS += sources/bone_generator.h
SOURCES += sources/bone_generator.cc
HEADERS += sources/bone_list_model.h
SOURCES += sources/bone_list_model.cc
HEADERS += sources/bone_manage_widget.h
SOURCES += sources/bone_manage_widget.cc
HEADERS += sources/bone_preview_grid_widget.h
SOURCES += sources/bone_preview_grid_widget.cc
HEADERS += sources/bone_property_widget.h
SOURCES += sources/bone_property_widget.cc
HEADERS += sources/ccd_ik_resolver.h
SOURCES += sources/ccd_ik_resolver.cc
HEADERS += sources/component_list_model.h
SOURCES += sources/component_list_model.cc
HEADERS += sources/component_preview_grid_widget.h
SOURCES += sources/component_preview_grid_widget.cc
HEADERS += sources/component_preview_images_decorator.h
SOURCES += sources/component_preview_images_decorator.cc
HEADERS += sources/component_property_widget.h
SOURCES += sources/component_property_widget.cc
HEADERS += sources/cut_face_preview.h
SOURCES += sources/cut_face_preview.cc
HEADERS += sources/dds_file.h
SOURCES += sources/dds_file.cc
HEADERS += sources/debug.h
SOURCES += sources/debug.cc
HEADERS += sources/document.h
SOURCES += sources/document.cc
SOURCES += sources/document_bone.cc
SOURCES += sources/document_component.cc
SOURCES += sources/document_edge.cc
SOURCES += sources/document_node.cc
SOURCES += sources/document_part.cc
HEADERS += sources/document_saver.h
SOURCES += sources/document_saver.cc
HEADERS += sources/document_window.h
SOURCES += sources/document_window.cc
HEADERS += sources/fbx_file.h
SOURCES += sources/fbx_file.cc
HEADERS += sources/float_number_widget.h
SOURCES += sources/float_number_widget.cc
HEADERS += sources/flow_layout.h
SOURCES += sources/flow_layout.cc
HEADERS += sources/glb_file.h
SOURCES += sources/glb_file.cc
HEADERS += sources/graphics_container_widget.h
SOURCES += sources/graphics_container_widget.cc
HEADERS += sources/horizontal_line_widget.h
SOURCES += sources/horizontal_line_widget.cc
HEADERS += sources/image_forever.h
SOURCES += sources/image_forever.cc
HEADERS += sources/image_preview_widget.h
SOURCES += sources/image_preview_widget.cc
HEADERS += sources/info_label.h
SOURCES += sources/info_label.cc
HEADERS += sources/int_number_widget.h
SOURCES += sources/int_number_widget.cc
HEADERS += sources/log_browser.h
SOURCES += sources/log_browser.cc
HEADERS += sources/log_browser_dialog.h
SOURCES += sources/log_browser_dialog.cc
SOURCES += sources/main.cc
HEADERS += sources/mesh_generator.h
SOURCES += sources/mesh_generator.cc
HEADERS += sources/mesh_preview_images_generator.h
SOURCES += sources/mesh_preview_images_generator.cc
HEADERS += sources/model_mesh.h
SOURCES += sources/model_mesh.cc
HEADERS += sources/model_offscreen_render.h
SOURCES += sources/model_offscreen_render.cc
HEADERS += sources/model_opengl_program.h
SOURCES += sources/model_opengl_program.cc
HEADERS += sources/model_opengl_object.h
SOURCES += sources/model_opengl_object.cc
HEADERS += sources/model_opengl_vertex.h
HEADERS += sources/model_widget.h
SOURCES += sources/model_widget.cc
HEADERS += sources/monochrome_mesh.h
SOURCES += sources/monochrome_mesh.cc
HEADERS += sources/monochrome_opengl_program.h
SOURCES += sources/monochrome_opengl_program.cc
HEADERS += sources/monochrome_opengl_object.h
SOURCES += sources/monochrome_opengl_object.cc
HEADERS += sources/monochrome_opengl_vertex.h
HEADERS += sources/part_manage_widget.h
SOURCES += sources/part_manage_widget.cc
HEADERS += sources/preferences.h
SOURCES += sources/preferences.cc
HEADERS += sources/preview_grid_view.h
SOURCES += sources/preview_grid_view.cc
HEADERS += sources/skeleton_graphics_edge_item.h
SOURCES += sources/skeleton_graphics_edge_item.cc
HEADERS += sources/skeleton_graphics_node_item.h
SOURCES += sources/skeleton_graphics_node_item.cc
HEADERS += sources/skeleton_graphics_origin_item.h
SOURCES += sources/skeleton_graphics_origin_item.cc
HEADERS += sources/skeleton_graphics_selection_item.h
SOURCES += sources/skeleton_graphics_selection_item.cc
HEADERS += sources/skeleton_graphics_widget.h
SOURCES += sources/skeleton_graphics_widget.cc
HEADERS += sources/skeleton_ik_mover.h
SOURCES += sources/skeleton_ik_mover.cc
HEADERS += sources/spinnable_toolbar_icon.h
SOURCES += sources/spinnable_toolbar_icon.cc
HEADERS += sources/theme.h
SOURCES += sources/theme.cc
HEADERS += sources/toolbar_button.h
SOURCES += sources/toolbar_button.cc
HEADERS += sources/turnaround_loader.h
SOURCES += sources/turnaround_loader.cc
HEADERS += sources/updates_check_widget.h
SOURCES += sources/updates_check_widget.cc
HEADERS += sources/updates_checker.h
SOURCES += sources/updates_checker.cc
HEADERS += sources/uv_map_generator.h
SOURCES += sources/uv_map_generator.cc
HEADERS += sources/version.h
INCLUDEPATH += third_party/QtWaitingSpinner
SOURCES += third_party/QtWaitingSpinner/waitingspinnerwidget.cpp
HEADERS += third_party/QtWaitingSpinner/waitingspinnerwidget.h
INCLUDEPATH += third_party/fbx/src
SOURCES += third_party/fbx/src/fbxdocument.cpp
HEADERS += third_party/fbx/src/fbxdocument.h
SOURCES += third_party/fbx/src/fbxnode.cpp
HEADERS += third_party/fbx/src/fbxnode.h
SOURCES += third_party/fbx/src/fbxproperty.cpp
HEADERS += third_party/fbx/src/fbxproperty.h
SOURCES += third_party/fbx/src/fbxutil.cpp
HEADERS += third_party/fbx/src/fbxutil.h
INCLUDEPATH += third_party/json
INCLUDEPATH += third_party/miniz
SOURCES += third_party/miniz/miniz.c
HEADERS += third_party/miniz/miniz.h

HEADERS += ../dust3d/base/axis_aligned_bounding_box.h
HEADERS += ../dust3d/base/axis_aligned_bounding_box_tree.h
SOURCES += ../dust3d/base/axis_aligned_bounding_box_tree.cc
HEADERS += ../dust3d/base/color.h
HEADERS += ../dust3d/base/combine_mode.h
SOURCES += ../dust3d/base/combine_mode.cc
HEADERS += ../dust3d/base/cut_face.h
SOURCES += ../dust3d/base/cut_face.cc
HEADERS += ../dust3d/base/debug.h
HEADERS += ../dust3d/base/ds3_file.h
SOURCES += ../dust3d/base/ds3_file.cc
HEADERS += ../dust3d/base/math.h
HEADERS += ../dust3d/base/matrix4x4.h
HEADERS += ../dust3d/base/object.h
HEADERS += ../dust3d/base/part_target.h
SOURCES += ../dust3d/base/part_target.cc
HEADERS += ../dust3d/base/position_key.h
SOURCES += ../dust3d/base/position_key.cc
HEADERS += ../dust3d/base/quaternion.h
HEADERS += ../dust3d/base/rectangle.h
HEADERS += ../dust3d/base/snapshot.h
HEADERS += ../dust3d/base/snapshot_xml.h
SOURCES += ../dust3d/base/snapshot_xml.cc
HEADERS += ../dust3d/base/string.h
SOURCES += ../dust3d/base/string.cc
HEADERS += ../dust3d/base/texture_type.h
SOURCES += ../dust3d/base/texture_type.cc
HEADERS += ../dust3d/base/vector3.h
SOURCES += ../dust3d/base/vector3.cc
HEADERS += ../dust3d/base/vector2.h
HEADERS += ../dust3d/base/uuid.h
SOURCES += ../dust3d/base/uuid.cc
HEADERS += ../dust3d/mesh/base_normal.h
SOURCES += ../dust3d/mesh/base_normal.cc
HEADERS += ../dust3d/mesh/box_mesh.h
SOURCES += ../dust3d/mesh/box_mesh.cc
HEADERS += ../dust3d/mesh/centripetal_catmull_rom_spline.h
SOURCES += ../dust3d/mesh/centripetal_catmull_rom_spline.cc
HEADERS += ../dust3d/mesh/solid_mesh.h
SOURCES += ../dust3d/mesh/solid_mesh.cc
HEADERS += ../dust3d/mesh/solid_mesh_boolean_operation.h
SOURCES += ../dust3d/mesh/solid_mesh_boolean_operation.cc
HEADERS += ../dust3d/mesh/hole_stitcher.h
SOURCES += ../dust3d/mesh/hole_stitcher.cc
HEADERS += ../dust3d/mesh/hole_wrapper.h
SOURCES += ../dust3d/mesh/hole_wrapper.cc
HEADERS += ../dust3d/mesh/isotropic_halfedge_mesh.h
SOURCES += ../dust3d/mesh/isotropic_halfedge_mesh.cc
HEADERS += ../dust3d/mesh/isotropic_remesher.h
SOURCES += ../dust3d/mesh/isotropic_remesher.cc
HEADERS += ../dust3d/mesh/mesh_combiner.h
SOURCES += ../dust3d/mesh/mesh_combiner.cc
HEADERS += ../dust3d/mesh/mesh_generator.h
SOURCES += ../dust3d/mesh/mesh_generator.cc
HEADERS += ../dust3d/mesh/mesh_node.h
HEADERS += ../dust3d/mesh/mesh_recombiner.h
SOURCES += ../dust3d/mesh/mesh_recombiner.cc
HEADERS += ../dust3d/mesh/mesh_state.h
SOURCES += ../dust3d/mesh/mesh_state.cc
HEADERS += ../dust3d/mesh/re_triangulator.h
SOURCES += ../dust3d/mesh/re_triangulator.cc
HEADERS += ../dust3d/mesh/resolve_triangle_tangent.h
SOURCES += ../dust3d/mesh/resolve_triangle_tangent.cc
HEADERS += ../dust3d/mesh/rope_mesh.h
SOURCES += ../dust3d/mesh/rope_mesh.cc
HEADERS += ../dust3d/mesh/section_preview_mesh_builder.h
SOURCES += ../dust3d/mesh/section_preview_mesh_builder.cc
HEADERS += ../dust3d/mesh/section_remesher.h
SOURCES += ../dust3d/mesh/section_remesher.cc
HEADERS += ../dust3d/mesh/smooth_normal.h
SOURCES += ../dust3d/mesh/smooth_normal.cc
HEADERS += ../dust3d/mesh/stitch_mesh_builder.h
SOURCES += ../dust3d/mesh/stitch_mesh_builder.cc
HEADERS += ../dust3d/mesh/triangulate.h
SOURCES += ../dust3d/mesh/triangulate.cc
HEADERS += ../dust3d/mesh/trim_vertices.h
SOURCES += ../dust3d/mesh/trim_vertices.cc
HEADERS += ../dust3d/mesh/tube_mesh_builder.h
SOURCES += ../dust3d/mesh/tube_mesh_builder.cc
HEADERS += ../dust3d/mesh/weld_vertices.h
SOURCES += ../dust3d/mesh/weld_vertices.cc
HEADERS += ../dust3d/rig/bone_generator.h
SOURCES += ../dust3d/rig/bone_generator.cc
HEADERS += ../dust3d/uv/chart_packer.h
SOURCES += ../dust3d/uv/chart_packer.cc
HEADERS += ../dust3d/uv/max_rectangles.h
SOURCES += ../dust3d/uv/max_rectangles.cc
HEADERS += ../dust3d/uv/uv_map_packer.h
SOURCES += ../dust3d/uv/uv_map_packer.cc
HEADERS += ../third_party/GuigueDevillers03/tri_tri_intersect.h
SOURCES += ../third_party/GuigueDevillers03/tri_tri_intersect.c

win32 {
    LIBS += -luser32
    LIBS += -lopengl32
}
