#ifndef DUST3D_BONE_MARK_H
#define DUST3D_BONE_MARK_H
#include <QColor>
#include <QString>

enum class BoneMark
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
#define BoneMarkHasSide(mark) ((mark) != BoneMark::Neck)
QColor BoneMarkToColor(BoneMark mark);
#define IMPL_BoneMarkToColor                                        \
QColor BoneMarkToColor(BoneMark mark)                               \
{                                                                   \
    switch (mark) {                                                 \
        case BoneMark::Neck:                                        \
            return QColor(0xfc, 0x0d, 0x1b);                        \
        case BoneMark::Shoulder:                                    \
            return QColor(0xfd, 0x80, 0x23);                        \
        case BoneMark::Elbow:                                       \
            return QColor(0x29, 0xfd, 0x2f);                        \
        case BoneMark::Wrist:                                       \
            return QColor(0xff, 0xfd, 0x38);                        \
        case BoneMark::Hip:                                         \
            return QColor(0x2c, 0xff, 0xfe);                        \
        case BoneMark::Knee:                                        \
            return QColor(0x0b, 0x24, 0xfb);                        \
        case BoneMark::Ankle:                                       \
            return QColor(0xfc, 0x28, 0xfc);                        \
        case BoneMark::None:                                        \
            return Qt::transparent;                                 \
        default:                                                    \
            return Qt::transparent;                                 \
    }                                                               \
}
const char *BoneMarkToString(BoneMark mark);
#define IMPL_BoneMarkToString                                       \
const char *BoneMarkToString(BoneMark mark)                         \
{                                                                   \
    switch (mark) {                                                 \
        case BoneMark::Neck:                                        \
            return "Neck";                                          \
        case BoneMark::Shoulder:                                    \
            return "Shoulder";                                      \
        case BoneMark::Elbow:                                       \
            return "Elbow";                                         \
        case BoneMark::Wrist:                                       \
            return "Wrist";                                         \
        case BoneMark::Hip:                                         \
            return "Hip";                                           \
        case BoneMark::Knee:                                        \
            return "Knee";                                          \
        case BoneMark::Ankle:                                       \
            return "Ankle";                                         \
        case BoneMark::None:                                        \
            return "None";                                          \
        default:                                                    \
            return "None";                                          \
    }                                                               \
}
BoneMark BoneMarkFromString(const char *markString);
#define IMPL_BoneMarkFromString                                     \
BoneMark BoneMarkFromString(const char *markString)                 \
{                                                                   \
    QString mark = markString;                                      \
    if (mark == "Neck")                                             \
        return BoneMark::Neck;                                      \
    if (mark == "Shoulder")                                         \
        return BoneMark::Shoulder;                                  \
    if (mark == "Elbow")                                            \
        return BoneMark::Elbow;                                     \
    if (mark == "Wrist")                                            \
        return BoneMark::Wrist;                                     \
    if (mark == "Hip")                                              \
        return BoneMark::Hip;                                       \
    if (mark == "Knee")                                             \
        return BoneMark::Knee;                                      \
    if (mark == "Ankle")                                            \
        return BoneMark::Ankle;                                     \
    return BoneMark::None;                                          \
}
QString BoneMarkToDispName(BoneMark mark);
#define IMPL_BoneMarkToDispName                                     \
QString BoneMarkToDispName(BoneMark mark)                           \
{                                                                   \
    switch (mark) {                                                 \
        case BoneMark::Neck:                                        \
            return QObject::tr("Neck");                             \
        case BoneMark::Shoulder:                                    \
            return QObject::tr("Shoulder (Arm Start)");             \
        case BoneMark::Elbow:                                       \
            return QObject::tr("Elbow");                            \
        case BoneMark::Wrist:                                       \
            return QObject::tr("Wrist");                            \
        case BoneMark::Hip:                                         \
            return QObject::tr("Hip (Leg Start)");                  \
        case BoneMark::Knee:                                        \
            return QObject::tr("Knee");                             \
        case BoneMark::Ankle:                                       \
            return QObject::tr("Ankle");                            \
        case BoneMark::None:                                        \
            return QObject::tr("None");                             \
        default:                                                    \
            return "";                                              \
    }                                                               \
}

#endif

