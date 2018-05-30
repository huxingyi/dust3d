#ifndef SKELETON_BONE_MARK_H
#define SKELETON_BONE_MARK_H
#include <QColor>
#include <QString>

enum class SkeletonBoneMark
{
    None = 0,
    LegStart,
    LegJoint,
    LegEnd,
    Spine,
    Max
};
#define SKELETON_BONE_MARK_TYPE_NUM    ((int)SkeletonBoneMark::Max - 1)
#define SkeletonBoneMarkIsStart(mark) (SkeletonBoneMark::LegStart == (mark))
QColor SkeletonBoneMarkToColor(SkeletonBoneMark mark);
#define IMPL_SkeletonBoneMarkToColor                                \
QColor SkeletonBoneMarkToColor(SkeletonBoneMark mark)               \
{                                                                   \
    switch (mark) {                                                 \
        case SkeletonBoneMark::LegStart:                            \
            return QColor(0xf4, 0xcd, 0x56);                        \
        case SkeletonBoneMark::LegJoint:                            \
            return QColor(0x51, 0xba, 0xf2);                        \
        case SkeletonBoneMark::LegEnd:                              \
            return QColor(0xd0, 0x8c, 0xe0);                        \
        case SkeletonBoneMark::Spine:                               \
            return QColor(0xa4, 0xa4, 0xa7);                        \
        case SkeletonBoneMark::None:                                \
            return Qt::transparent;                                 \
        default:                                                    \
            return Qt::transparent;                                 \
    }                                                               \
}
const char *SkeletonBoneMarkToString(SkeletonBoneMark mark);
#define IMPL_SkeletonBoneMarkToString                               \
const char *SkeletonBoneMarkToString(SkeletonBoneMark mark)         \
{                                                                   \
    switch (mark) {                                                 \
        case SkeletonBoneMark::LegStart:                            \
            return "LegStart";                                      \
        case SkeletonBoneMark::LegJoint:                            \
            return "LegJoint";                                      \
        case SkeletonBoneMark::LegEnd:                              \
            return "LegEnd";                                        \
        case SkeletonBoneMark::Spine:                               \
            return "Spine";                                         \
        case SkeletonBoneMark::None:                                \
            return "None";                                          \
        default:                                                    \
            return "None";                                          \
    }                                                               \
}
SkeletonBoneMark SkeletonBoneMarkFromString(const char *markString);
#define IMPL_SkeletonBoneMarkFromString                             \
SkeletonBoneMark SkeletonBoneMarkFromString(const char *markString) \
{                                                                   \
    QString mark = markString;                                      \
    if (mark == "LegStart")                                         \
        return SkeletonBoneMark::LegStart;                          \
    if (mark == "LegJoint")                                         \
        return SkeletonBoneMark::LegJoint;                          \
    if (mark == "LegEnd")                                           \
        return SkeletonBoneMark::LegEnd;                            \
    if (mark == "Spine")                                            \
        return SkeletonBoneMark::Spine;                             \
    if (mark == "None")                                             \
        return SkeletonBoneMark::None;                              \
    return SkeletonBoneMark::None;                                  \
}
const char *SkeletonBoneMarkToDispName(SkeletonBoneMark mark);
#define IMPL_SkeletonBoneMarkToDispName                             \
const char *SkeletonBoneMarkToDispName(SkeletonBoneMark mark)       \
{                                                                   \
    switch (mark) {                                                 \
        case SkeletonBoneMark::LegStart:                            \
            return "Leg (Start)";                                   \
        case SkeletonBoneMark::LegJoint:                            \
            return "Leg (Joint)";                                   \
        case SkeletonBoneMark::LegEnd:                              \
            return "Leg (End)";                                     \
        case SkeletonBoneMark::Spine:                               \
            return "Spine";                                         \
        case SkeletonBoneMark::None:                                \
            return "";                                              \
        default:                                                    \
            return "";                                              \
    }                                                               \
}

#endif

