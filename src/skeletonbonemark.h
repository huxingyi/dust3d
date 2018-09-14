#ifndef SKELETON_BONE_MARK_H
#define SKELETON_BONE_MARK_H
#include <QColor>
#include <QString>

enum class SkeletonSide
{
    None = 0,
    Left,
    Right
};

QString SkeletonSideToDispName(SkeletonSide side);
#define IMPL_SkeletonSideToDispName                                 \
QString SkeletonSideToDispName(SkeletonSide side)                   \
{                                                                   \
    switch (side) {                                                 \
        case SkeletonSide::Left:                                    \
            return QObject::tr("Left");                             \
        case SkeletonSide::Right:                                   \
            return QObject::tr("Right");                            \
        case SkeletonSide::None:                                    \
            return "";                                              \
        default:                                                    \
            return "";                                              \
    }                                                               \
}

enum class SkeletonBoneMark
{
    None = 0,
    Neck,
    Shoulder,
    Elbow,
    Wrist,
    Hip,
    Knee,
    Ankle,
    Count
};
#define SkeletonBoneMarkHasSide(mark) ((mark) != SkeletonBoneMark::Neck)
QColor SkeletonBoneMarkToColor(SkeletonBoneMark mark);
#define IMPL_SkeletonBoneMarkToColor                                \
QColor SkeletonBoneMarkToColor(SkeletonBoneMark mark)               \
{                                                                   \
    switch (mark) {                                                 \
        case SkeletonBoneMark::Neck:                                \
            return QColor(0xfc, 0x0d, 0x1b);                        \
        case SkeletonBoneMark::Shoulder:                            \
            return QColor(0xfd, 0x80, 0x23);                        \
        case SkeletonBoneMark::Elbow:                               \
            return QColor(0x29, 0xfd, 0x2f);                        \
        case SkeletonBoneMark::Wrist:                               \
            return QColor(0xff, 0xfd, 0x38);                        \
        case SkeletonBoneMark::Hip:                                 \
            return QColor(0x2c, 0xff, 0xfe);                        \
        case SkeletonBoneMark::Knee:                                \
            return QColor(0x0b, 0x24, 0xfb);                        \
        case SkeletonBoneMark::Ankle:                               \
            return QColor(0xfc, 0x28, 0xfc);                        \
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
        case SkeletonBoneMark::Neck:                                \
            return "Neck";                                          \
        case SkeletonBoneMark::Shoulder:                            \
            return "Shoulder";                                      \
        case SkeletonBoneMark::Elbow:                               \
            return "Elbow";                                         \
        case SkeletonBoneMark::Wrist:                               \
            return "Wrist";                                         \
        case SkeletonBoneMark::Hip:                                 \
            return "Hip";                                           \
        case SkeletonBoneMark::Knee:                                \
            return "Knee";                                          \
        case SkeletonBoneMark::Ankle:                               \
            return "Ankle";                                         \
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
    if (mark == "Neck")                                             \
        return SkeletonBoneMark::Neck;                              \
    if (mark == "Shoulder")                                         \
        return SkeletonBoneMark::Shoulder;                          \
    if (mark == "Elbow")                                            \
        return SkeletonBoneMark::Elbow;                             \
    if (mark == "Wrist")                                            \
        return SkeletonBoneMark::Wrist;                             \
    if (mark == "Hip")                                              \
        return SkeletonBoneMark::Hip;                               \
    if (mark == "Knee")                                             \
        return SkeletonBoneMark::Knee;                              \
    if (mark == "Ankle")                                            \
        return SkeletonBoneMark::Ankle;                             \
    return SkeletonBoneMark::None;                                  \
}
QString SkeletonBoneMarkToDispName(SkeletonBoneMark mark);
#define IMPL_SkeletonBoneMarkToDispName                             \
QString SkeletonBoneMarkToDispName(SkeletonBoneMark mark)           \
{                                                                   \
    switch (mark) {                                                 \
        case SkeletonBoneMark::Neck:                                \
            return QObject::tr("Neck");                             \
        case SkeletonBoneMark::Shoulder:                            \
            return QObject::tr("Shoulder (Arm Start)");             \
        case SkeletonBoneMark::Elbow:                               \
            return QObject::tr("Elbow");                            \
        case SkeletonBoneMark::Wrist:                               \
            return QObject::tr("Wrist");                            \
        case SkeletonBoneMark::Hip:                                 \
            return QObject::tr("Hip (Leg Start)");                  \
        case SkeletonBoneMark::Knee:                                \
            return QObject::tr("Knee");                             \
        case SkeletonBoneMark::Ankle:                               \
            return QObject::tr("Ankle");                            \
        case SkeletonBoneMark::None:                                \
            return QObject::tr("None");                             \
        default:                                                    \
            return "";                                              \
    }                                                               \
}

#endif

