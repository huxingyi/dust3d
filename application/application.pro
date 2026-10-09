QT += core gui opengl widgets svg multimedia

greaterThan(QT_MAJOR_VERSION, 5) {
    QT += openglwidgets svgwidgets
}

TARGET = dust3d
TEMPLATE = app

HUMAN_VERSION = "1.1.7"
VERSION = 1.1.7.0

QMAKE_TARGET_COMPANY = Dust3D
QMAKE_TARGET_PRODUCT = Dust3D
QMAKE_TARGET_DESCRIPTION = "Dust3D is a cross-platform open-source 3D modeling software"
QMAKE_TARGET_COPYRIGHT = "Copyright (C) 2018-2026 Dust3D Project. All Rights Reserved."

HOMEPAGE_URL = "https://dust3d.org/"
REPOSITORY_URL = "https://github.com/huxingyi/dust3d"
ISSUES_URL = "https://github.com/huxingyi/dust3d/issues"
REFERENCE_GUIDE_URL = "https://docs.dust3d.org"

DEFINES += QT_MESSAGELOGCONTEXT
DEFINES += _USE_MATH_DEFINES
CONFIG += object_parallel_to_source
CONFIG += no_batch

CONFIG += c++17

CONFIG(release, debug|release) {
    DEFINES += NDEBUG
}

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
    QMAKE_CXXFLAGS += -Wno-error=implicit-function-declaration
}

unix:!macx {
    QMAKE_CXXFLAGS_RELEASE -= -O
    QMAKE_CXXFLAGS_RELEASE -= -O1
    QMAKE_CXXFLAGS_RELEASE -= -O2

    QMAKE_CXXFLAGS_RELEASE += -O3
}

QMAKE_LFLAGS += -Os

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

# The TBB stand-in must come before any system TBB (see third_party/autoremesher/README.md)
INCLUDEPATH += ../third_party/autoremesher/tbbshim
INCLUDEPATH += ../
INCLUDEPATH += ../third_party
INCLUDEPATH += ../third_party/rapidxml-1.13
INCLUDEPATH += ../third_party/earcut.hpp/include

include(third_party/QtAwesome/QtAwesome/QtAwesome.pri)

HEADERS += sources/about_widget.h
SOURCES += sources/about_widget.cc
HEADERS += sources/bone_structure.h
HEADERS += sources/bone_manage_widget.h
SOURCES += sources/bone_manage_widget.cc
HEADERS += sources/bone_property_widget.h
SOURCES += sources/bone_property_widget.cc
HEADERS += sources/rig_skeleton_mesh_generator.h
SOURCES += sources/rig_skeleton_mesh_generator.cc
HEADERS += sources/rig_skeleton_mesh_worker.h
SOURCES += sources/rig_skeleton_mesh_worker.cc
HEADERS += sources/rig_generator_worker.h
HEADERS += sources/ccd_ik_resolver.h
SOURCES += sources/ccd_ik_resolver.cc
HEADERS += sources/component_breadcrumb_widget.h
SOURCES += sources/component_breadcrumb_widget.cc
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
HEADERS += sources/debug.h
SOURCES += sources/debug.cc
HEADERS += sources/document.h
SOURCES += sources/document.cc
SOURCES += sources/document_component.cc
SOURCES += sources/document_edge.cc
SOURCES += sources/document_node.cc
SOURCES += sources/document_part.cc
HEADERS += sources/document_saver.h
SOURCES += sources/document_saver.cc
HEADERS += sources/document_window.h
SOURCES += sources/document_window.cc
HEADERS += sources/steps_replay_window.h
SOURCES += sources/steps_replay_window.cc
HEADERS += sources/turnaround_overlay_widget.h
SOURCES += sources/turnaround_overlay_widget.cc
HEADERS += sources/fbx_file.h
SOURCES += sources/fbx_file.cc
HEADERS += sources/float_number_widget.h
SOURCES += sources/float_number_widget.cc
HEADERS += sources/flow_layout.h
SOURCES += sources/flow_layout.cc
HEADERS += sources/glb_file.h
SOURCES += sources/glb_file.cc
HEADERS += sources/glb_forever.h
SOURCES += sources/glb_forever.cc
HEADERS += sources/glb_reader.h
SOURCES += sources/glb_reader.cc
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
HEADERS += sources/shadow_opengl_program.h
SOURCES += sources/shadow_opengl_program.cc
HEADERS += sources/scene_outline_opengl_program.h
SOURCES += sources/scene_outline_opengl_program.cc
HEADERS += sources/world_opengl_program.h
SOURCES += sources/world_opengl_program.cc
HEADERS += sources/world_ground_opengl_vertex.h
HEADERS += sources/world_ground_opengl_object.h
SOURCES += sources/world_ground_opengl_object.cc
HEADERS += sources/world_ground_opengl_program.h
SOURCES += sources/world_ground_opengl_program.cc
HEADERS += sources/world_widget.h
SOURCES += sources/world_widget.cc
HEADERS += sources/scene_widget.h
SOURCES += sources/scene_widget.cc
HEADERS += sources/scene_opengl_program.h
SOURCES += sources/scene_opengl_program.cc
HEADERS += sources/scene_ground_opengl_program.h
SOURCES += sources/scene_ground_opengl_program.cc
HEADERS += sources/monochrome_mesh.h
SOURCES += sources/monochrome_mesh.cc
HEADERS += sources/monochrome_opengl_program.h
SOURCES += sources/monochrome_opengl_program.cc
HEADERS += sources/monochrome_opengl_object.h
SOURCES += sources/monochrome_opengl_object.cc
HEADERS += sources/monochrome_opengl_vertex.h
HEADERS += sources/part_manage_widget.h
SOURCES += sources/part_manage_widget.cc
HEADERS += sources/animation_manage_widget.h
SOURCES += sources/animation_manage_widget.cc
HEADERS += sources/animation_preview_worker.h
SOURCES += sources/animation_preview_worker.cc
HEADERS += sources/preview_overlay_controller.h
SOURCES += sources/preview_overlay_controller.cc
HEADERS += sources/export_animation_worker.h
SOURCES += sources/export_animation_worker.cc
HEADERS += sources/export_progress_widget.h
SOURCES += sources/export_progress_widget.cc
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
HEADERS += sources/turnaround_image_editor_dialog.h
SOURCES += sources/turnaround_image_editor_dialog.cc
HEADERS += sources/turnaround_loader.h
SOURCES += sources/turnaround_loader.cc
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
HEADERS += ../dust3d/base/bone_binding.h
# Animation infrastructure
HEADERS += ../dust3d/animation/animation_generator.h
HEADERS += ../dust3d/animation/animation_catalog.h
SOURCES += ../dust3d/animation/animation_catalog.cc
SOURCES += ../dust3d/animation/animation_generator.cc
HEADERS += ../dust3d/animation/arthropod_die.h
HEADERS += ../dust3d/animation/common.h
HEADERS += ../dust3d/animation/sound_event_detector.h
SOURCES += ../dust3d/animation/sound_event_detector.cc
HEADERS += ../dust3d/animation/sound_generator.h
SOURCES += ../dust3d/animation/sound_generator.cc

# Biped shared motion
HEADERS += ../dust3d/animation/biped/action.h
SOURCES += ../dust3d/animation/biped/action.cc
HEADERS += ../dust3d/animation/biped/clip_catalog.h
SOURCES += ../dust3d/animation/biped/clip_catalog.cc
HEADERS += ../dust3d/animation/biped/combat_motion.h
SOURCES += ../dust3d/animation/biped/combat_motion.cc
HEADERS += ../dust3d/animation/biped/emote_motion.h
SOURCES += ../dust3d/animation/biped/emote_motion.cc
HEADERS += ../dust3d/animation/biped/floor_transition.h
SOURCES += ../dust3d/animation/biped/floor_transition.cc
HEADERS += ../dust3d/animation/biped/gait.h
SOURCES += ../dust3d/animation/biped/gait.cc
HEADERS += ../dust3d/animation/biped/gait_variant.h
SOURCES += ../dust3d/animation/biped/gait_variant.cc
HEADERS += ../dust3d/animation/biped/interaction_motion.h
SOURCES += ../dust3d/animation/biped/interaction_motion.cc
HEADERS += ../dust3d/animation/biped/pose.h
HEADERS += ../dust3d/animation/biped/pose_sequence.h
SOURCES += ../dust3d/animation/biped/pose_sequence.cc
HEADERS += ../dust3d/animation/biped/secondary_motion.h
SOURCES += ../dust3d/animation/biped/secondary_motion.cc
HEADERS += ../dust3d/animation/biped/spell_transition.h
SOURCES += ../dust3d/animation/biped/spell_transition.cc
HEADERS += ../dust3d/animation/biped/stance_motion.h
SOURCES += ../dust3d/animation/biped/stance_motion.cc
HEADERS += ../dust3d/animation/biped/swim_motion.h
SOURCES += ../dust3d/animation/biped/swim_motion.cc

# Biped animation clips
HEADERS += ../dust3d/animation/biped/block.h
SOURCES += ../dust3d/animation/biped/block.cc
HEADERS += ../dust3d/animation/biped/bow.h
SOURCES += ../dust3d/animation/biped/bow.cc
HEADERS += ../dust3d/animation/biped/bow_aim.h
SOURCES += ../dust3d/animation/biped/bow_aim.cc
HEADERS += ../dust3d/animation/biped/bow_draw.h
SOURCES += ../dust3d/animation/biped/bow_draw.cc
HEADERS += ../dust3d/animation/biped/bow_shot.h
SOURCES += ../dust3d/animation/biped/bow_shot.cc
HEADERS += ../dust3d/animation/biped/cast.h
SOURCES += ../dust3d/animation/biped/cast.cc
HEADERS += ../dust3d/animation/biped/cast_recover.h
SOURCES += ../dust3d/animation/biped/cast_recover.cc
HEADERS += ../dust3d/animation/biped/cast_release.h
SOURCES += ../dust3d/animation/biped/cast_release.cc
HEADERS += ../dust3d/animation/biped/cast_start.h
SOURCES += ../dust3d/animation/biped/cast_start.cc
HEADERS += ../dust3d/animation/biped/channel.h
SOURCES += ../dust3d/animation/biped/channel.cc
HEADERS += ../dust3d/animation/biped/channel_enter.h
SOURCES += ../dust3d/animation/biped/channel_enter.cc
HEADERS += ../dust3d/animation/biped/channel_exit.h
SOURCES += ../dust3d/animation/biped/channel_exit.cc
HEADERS += ../dust3d/animation/biped/channel_interrupt.h
SOURCES += ../dust3d/animation/biped/channel_interrupt.cc
HEADERS += ../dust3d/animation/biped/cheer.h
SOURCES += ../dust3d/animation/biped/cheer.cc
HEADERS += ../dust3d/animation/biped/chop.h
SOURCES += ../dust3d/animation/biped/chop.cc
HEADERS += ../dust3d/animation/biped/clap.h
SOURCES += ../dust3d/animation/biped/clap.cc
HEADERS += ../dust3d/animation/biped/combat_idle.h
SOURCES += ../dust3d/animation/biped/combat_idle.cc
HEADERS += ../dust3d/animation/biped/crouch_enter.h
SOURCES += ../dust3d/animation/biped/crouch_enter.cc
HEADERS += ../dust3d/animation/biped/crouch_exit.h
SOURCES += ../dust3d/animation/biped/crouch_exit.cc
HEADERS += ../dust3d/animation/biped/crouch_idle.h
SOURCES += ../dust3d/animation/biped/crouch_idle.cc
HEADERS += ../dust3d/animation/biped/dance.h
SOURCES += ../dust3d/animation/biped/dance.cc
HEADERS += ../dust3d/animation/biped/die.h
SOURCES += ../dust3d/animation/biped/die.cc
HEADERS += ../dust3d/animation/biped/dodge.h
SOURCES += ../dust3d/animation/biped/dodge.cc
HEADERS += ../dust3d/animation/biped/dodge_roll.h
SOURCES += ../dust3d/animation/biped/dodge_roll.cc
HEADERS += ../dust3d/animation/biped/drink.h
SOURCES += ../dust3d/animation/biped/drink.cc
HEADERS += ../dust3d/animation/biped/eat.h
SOURCES += ../dust3d/animation/biped/eat.cc
HEADERS += ../dust3d/animation/biped/fall.h
SOURCES += ../dust3d/animation/biped/fall.cc
HEADERS += ../dust3d/animation/biped/gather.h
SOURCES += ../dust3d/animation/biped/gather.cc
HEADERS += ../dust3d/animation/biped/get_up.h
SOURCES += ../dust3d/animation/biped/get_up.cc
HEADERS += ../dust3d/animation/biped/hop.h
SOURCES += ../dust3d/animation/biped/hop.cc
HEADERS += ../dust3d/animation/biped/hurt.h
SOURCES += ../dust3d/animation/biped/hurt.cc
HEADERS += ../dust3d/animation/biped/idle.h
SOURCES += ../dust3d/animation/biped/idle.cc
HEADERS += ../dust3d/animation/biped/interact.h
SOURCES += ../dust3d/animation/biped/interact.cc
HEADERS += ../dust3d/animation/biped/jump.h
SOURCES += ../dust3d/animation/biped/jump.cc
HEADERS += ../dust3d/animation/biped/jump_start.h
SOURCES += ../dust3d/animation/biped/jump_start.cc
HEADERS += ../dust3d/animation/biped/kick.h
SOURCES += ../dust3d/animation/biped/kick.cc
HEADERS += ../dust3d/animation/biped/knockdown.h
SOURCES += ../dust3d/animation/biped/knockdown.cc
HEADERS += ../dust3d/animation/biped/land.h
SOURCES += ../dust3d/animation/biped/land.cc
HEADERS += ../dust3d/animation/biped/mine.h
SOURCES += ../dust3d/animation/biped/mine.cc
HEADERS += ../dust3d/animation/biped/mounted_idle.h
SOURCES += ../dust3d/animation/biped/mounted_idle.cc
HEADERS += ../dust3d/animation/biped/mounted_ride.h
SOURCES += ../dust3d/animation/biped/mounted_ride.cc
HEADERS += ../dust3d/animation/biped/one_hand_slash.h
SOURCES += ../dust3d/animation/biped/one_hand_slash.cc
HEADERS += ../dust3d/animation/biped/parry.h
SOURCES += ../dust3d/animation/biped/parry.cc
HEADERS += ../dust3d/animation/biped/pick_up.h
SOURCES += ../dust3d/animation/biped/pick_up.cc
HEADERS += ../dust3d/animation/biped/point.h
SOURCES += ../dust3d/animation/biped/point.cc
HEADERS += ../dust3d/animation/biped/roar.h
SOURCES += ../dust3d/animation/biped/roar.cc
HEADERS += ../dust3d/animation/biped/run.h
SOURCES += ../dust3d/animation/biped/run.cc
HEADERS += ../dust3d/animation/biped/sit_down.h
SOURCES += ../dust3d/animation/biped/sit_down.cc
HEADERS += ../dust3d/animation/biped/sit_idle.h
SOURCES += ../dust3d/animation/biped/sit_idle.cc
HEADERS += ../dust3d/animation/biped/slam.h
SOURCES += ../dust3d/animation/biped/slam.cc
HEADERS += ../dust3d/animation/biped/slash.h
SOURCES += ../dust3d/animation/biped/slash.cc
HEADERS += ../dust3d/animation/biped/sleep_idle.h
SOURCES += ../dust3d/animation/biped/sleep_idle.cc
HEADERS += ../dust3d/animation/biped/sleep_lie_down.h
SOURCES += ../dust3d/animation/biped/sleep_lie_down.cc
HEADERS += ../dust3d/animation/biped/sneak.h
SOURCES += ../dust3d/animation/biped/sneak.cc
HEADERS += ../dust3d/animation/biped/sprint.h
SOURCES += ../dust3d/animation/biped/sprint.cc
HEADERS += ../dust3d/animation/biped/stab.h
SOURCES += ../dust3d/animation/biped/stab.cc
HEADERS += ../dust3d/animation/biped/stand_up.h
SOURCES += ../dust3d/animation/biped/stand_up.cc
HEADERS += ../dust3d/animation/biped/strafe_left.h
SOURCES += ../dust3d/animation/biped/strafe_left.cc
HEADERS += ../dust3d/animation/biped/strafe_right.h
SOURCES += ../dust3d/animation/biped/strafe_right.cc
HEADERS += ../dust3d/animation/biped/stunned.h
SOURCES += ../dust3d/animation/biped/stunned.cc
HEADERS += ../dust3d/animation/biped/swim_backward.h
SOURCES += ../dust3d/animation/biped/swim_backward.cc
HEADERS += ../dust3d/animation/biped/swim_forward.h
SOURCES += ../dust3d/animation/biped/swim_forward.cc
HEADERS += ../dust3d/animation/biped/swim_idle.h
SOURCES += ../dust3d/animation/biped/swim_idle.cc
HEADERS += ../dust3d/animation/biped/swim_left.h
SOURCES += ../dust3d/animation/biped/swim_left.cc
HEADERS += ../dust3d/animation/biped/swim_right.h
SOURCES += ../dust3d/animation/biped/swim_right.cc
HEADERS += ../dust3d/animation/biped/talk.h
SOURCES += ../dust3d/animation/biped/talk.cc
HEADERS += ../dust3d/animation/biped/throw.h
SOURCES += ../dust3d/animation/biped/throw.cc
HEADERS += ../dust3d/animation/biped/turn_left.h
SOURCES += ../dust3d/animation/biped/turn_left.cc
HEADERS += ../dust3d/animation/biped/turn_right.h
SOURCES += ../dust3d/animation/biped/turn_right.cc
HEADERS += ../dust3d/animation/biped/two_hand_swing.h
SOURCES += ../dust3d/animation/biped/two_hand_swing.cc
HEADERS += ../dust3d/animation/biped/wake_up.h
SOURCES += ../dust3d/animation/biped/wake_up.cc
HEADERS += ../dust3d/animation/biped/walk.h
SOURCES += ../dust3d/animation/biped/walk.cc
HEADERS += ../dust3d/animation/biped/walk_backward.h
SOURCES += ../dust3d/animation/biped/walk_backward.cc
HEADERS += ../dust3d/animation/biped/wave.h
SOURCES += ../dust3d/animation/biped/wave.cc

# Bird animations
HEADERS += ../dust3d/animation/bird/attack.h
SOURCES += ../dust3d/animation/bird/attack.cc
HEADERS += ../dust3d/animation/bird/die.h
SOURCES += ../dust3d/animation/bird/die.cc
HEADERS += ../dust3d/animation/bird/eat.h
SOURCES += ../dust3d/animation/bird/eat.cc
HEADERS += ../dust3d/animation/bird/fly.h
SOURCES += ../dust3d/animation/bird/fly.cc
HEADERS += ../dust3d/animation/bird/glide.h
SOURCES += ../dust3d/animation/bird/glide.cc
HEADERS += ../dust3d/animation/bird/hurt.h
SOURCES += ../dust3d/animation/bird/hurt.cc
HEADERS += ../dust3d/animation/bird/idle.h
SOURCES += ../dust3d/animation/bird/idle.cc
HEADERS += ../dust3d/animation/bird/run.h
SOURCES += ../dust3d/animation/bird/run.cc
HEADERS += ../dust3d/animation/bird/strike.h
SOURCES += ../dust3d/animation/bird/strike.cc
HEADERS += ../dust3d/animation/bird/walk.h
SOURCES += ../dust3d/animation/bird/walk.cc

# Fish animations
HEADERS += ../dust3d/animation/fish/attack.h
SOURCES += ../dust3d/animation/fish/attack.cc
SOURCES += ../dust3d/animation/fish/die.cc
HEADERS += ../dust3d/animation/fish/hurt.h
SOURCES += ../dust3d/animation/fish/hurt.cc
HEADERS += ../dust3d/animation/fish/idle.h
SOURCES += ../dust3d/animation/fish/idle.cc
HEADERS += ../dust3d/animation/fish/swim.h
SOURCES += ../dust3d/animation/fish/swim.cc

# Insect animations
HEADERS += ../dust3d/animation/insect/attack.h
SOURCES += ../dust3d/animation/insect/attack.cc
HEADERS += ../dust3d/animation/insect/bite.h
SOURCES += ../dust3d/animation/insect/bite.cc
HEADERS += ../dust3d/animation/insect/common.h
HEADERS += ../dust3d/animation/insect/die.h
SOURCES += ../dust3d/animation/insect/die.cc
HEADERS += ../dust3d/animation/insect/fly.h
SOURCES += ../dust3d/animation/insect/fly.cc
HEADERS += ../dust3d/animation/insect/hurt.h
SOURCES += ../dust3d/animation/insect/hurt.cc
HEADERS += ../dust3d/animation/insect/idle.h
SOURCES += ../dust3d/animation/insect/idle.cc
HEADERS += ../dust3d/animation/insect/rub_hands.h
SOURCES += ../dust3d/animation/insect/rub_hands.cc
HEADERS += ../dust3d/animation/insect/walk.h
SOURCES += ../dust3d/animation/insect/walk.cc

# Quadruped animations
HEADERS += ../dust3d/animation/quadruped/attack.h
SOURCES += ../dust3d/animation/quadruped/attack.cc
HEADERS += ../dust3d/animation/quadruped/die.h
SOURCES += ../dust3d/animation/quadruped/die.cc
HEADERS += ../dust3d/animation/quadruped/eat.h
SOURCES += ../dust3d/animation/quadruped/eat.cc
HEADERS += ../dust3d/animation/quadruped/hurt.h
SOURCES += ../dust3d/animation/quadruped/hurt.cc
HEADERS += ../dust3d/animation/quadruped/idle.h
SOURCES += ../dust3d/animation/quadruped/idle.cc
HEADERS += ../dust3d/animation/quadruped/roar.h
SOURCES += ../dust3d/animation/quadruped/roar.cc
HEADERS += ../dust3d/animation/quadruped/run.h
SOURCES += ../dust3d/animation/quadruped/run.cc
HEADERS += ../dust3d/animation/quadruped/walk.h
SOURCES += ../dust3d/animation/quadruped/walk.cc

# Snake animations
HEADERS += ../dust3d/animation/snake/die.h
SOURCES += ../dust3d/animation/snake/die.cc
HEADERS += ../dust3d/animation/snake/hurt.h
SOURCES += ../dust3d/animation/snake/hurt.cc
HEADERS += ../dust3d/animation/snake/idle.h
SOURCES += ../dust3d/animation/snake/idle.cc
HEADERS += ../dust3d/animation/snake/slither.h
SOURCES += ../dust3d/animation/snake/slither.cc
HEADERS += ../dust3d/animation/snake/strike.h
SOURCES += ../dust3d/animation/snake/strike.cc

# Spider animations
HEADERS += ../dust3d/animation/spider/attack.h
SOURCES += ../dust3d/animation/spider/attack.cc
HEADERS += ../dust3d/animation/spider/die.h
SOURCES += ../dust3d/animation/spider/die.cc
HEADERS += ../dust3d/animation/spider/hurt.h
SOURCES += ../dust3d/animation/spider/hurt.cc
HEADERS += ../dust3d/animation/spider/idle.h
SOURCES += ../dust3d/animation/spider/idle.cc
HEADERS += ../dust3d/animation/spider/run.h
SOURCES += ../dust3d/animation/spider/run.cc
HEADERS += ../dust3d/animation/spider/walk.h
SOURCES += ../dust3d/animation/spider/walk.cc

# Mesh processing
HEADERS += ../dust3d/mesh/base_normal.h
SOURCES += ../dust3d/mesh/base_normal.cc
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
HEADERS += ../dust3d/mesh/section_remesher.h
SOURCES += ../dust3d/mesh/section_remesher.cc
HEADERS += ../dust3d/mesh/smooth_normal.h
SOURCES += ../dust3d/mesh/smooth_normal.cc
HEADERS += ../dust3d/mesh/spine_deformer.h
HEADERS += ../dust3d/mesh/stitch_mesh_builder.h
SOURCES += ../dust3d/mesh/stitch_mesh_builder.cc

HEADERS += ../dust3d/mesh/wrap_mesh_builder.h
SOURCES += ../dust3d/mesh/wrap_mesh_builder.cc

# AutoRemesher core: the quad remesher of the wrap modifier
INCLUDEPATH += ../third_party/autoremesher/include
INCLUDEPATH += ../third_party/autoremesher/thirdparty/isotropicremesher
INCLUDEPATH += ../third_party/eigen
HEADERS += ../third_party/autoremesher/src/AutoRemesher/parameterizer.h
SOURCES += ../third_party/autoremesher/src/AutoRemesher/parameterizer.cpp
HEADERS += ../third_party/autoremesher/src/AutoRemesher/framefield.h
SOURCES += ../third_party/autoremesher/src/AutoRemesher/framefield.cpp
HEADERS += ../third_party/autoremesher/src/AutoRemesher/singularitysimplifier.h
SOURCES += ../third_party/autoremesher/src/AutoRemesher/singularitysimplifier.cpp
HEADERS += ../third_party/autoremesher/src/AutoRemesher/quadparameterizer.h
SOURCES += ../third_party/autoremesher/src/AutoRemesher/quadparameterizer.cpp
HEADERS += ../third_party/autoremesher/src/AutoRemesher/mixedintegerleastsquares.h
SOURCES += ../third_party/autoremesher/src/AutoRemesher/mixedintegerleastsquares.cpp
HEADERS += ../third_party/autoremesher/src/AutoRemesher/constrainedleastsquares.h
SOURCES += ../third_party/autoremesher/src/AutoRemesher/constrainedleastsquares.cpp
HEADERS += ../third_party/autoremesher/src/AutoRemesher/surfacemesh.h
SOURCES += ../third_party/autoremesher/src/AutoRemesher/surfacemesh.cpp
HEADERS += ../third_party/autoremesher/src/AutoRemesher/quadextractor.h
SOURCES += ../third_party/autoremesher/src/AutoRemesher/quadextractor.cpp
HEADERS += ../third_party/autoremesher/src/AutoRemesher/positionkey.h
SOURCES += ../third_party/autoremesher/src/AutoRemesher/positionkey.cpp
HEADERS += ../third_party/autoremesher/src/AutoRemesher/meshseparator.h
SOURCES += ../third_party/autoremesher/src/AutoRemesher/meshseparator.cpp
HEADERS += ../third_party/autoremesher/src/AutoRemesher/isotropicremesher.h
SOURCES += ../third_party/autoremesher/src/AutoRemesher/isotropicremesher.cpp
HEADERS += ../third_party/autoremesher/thirdparty/isotropicremesher/isotropicremesher.h
SOURCES += ../third_party/autoremesher/thirdparty/isotropicremesher/isotropicremesher.cpp
HEADERS += ../third_party/autoremesher/thirdparty/isotropicremesher/isotropichalfedgemesh.h
SOURCES += ../third_party/autoremesher/thirdparty/isotropicremesher/isotropichalfedgemesh.cpp
HEADERS += ../third_party/autoremesher/thirdparty/isotropicremesher/axisalignedboundingboxtree.h
SOURCES += ../third_party/autoremesher/thirdparty/isotropicremesher/axisalignedboundingboxtree.cpp
HEADERS += ../dust3d/mesh/stitch_loop_mesh_builder.h
SOURCES += ../dust3d/mesh/stitch_loop_mesh_builder.cc
HEADERS += ../dust3d/mesh/triangulate.h
SOURCES += ../dust3d/mesh/triangulate.cc
HEADERS += ../dust3d/mesh/trim_vertices.h
SOURCES += ../dust3d/mesh/trim_vertices.cc
HEADERS += ../dust3d/mesh/tube_mesh_builder.h
SOURCES += ../dust3d/mesh/tube_mesh_builder.cc
HEADERS += ../dust3d/rig/rig_generator.h
SOURCES += ../dust3d/rig/rig_generator.cc
HEADERS += ../dust3d/uv/chart_packer.h
SOURCES += ../dust3d/uv/chart_packer.cc

HEADERS += ../dust3d/uv/surface_pattern.h
SOURCES += ../dust3d/uv/surface_pattern.cc

HEADERS += ../dust3d/uv/procedural_noise.h

HEADERS += ../dust3d/uv/cloth_folds.h
SOURCES += ../dust3d/uv/cloth_folds.cc
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

HEADERS += sources/background_task_group.h
