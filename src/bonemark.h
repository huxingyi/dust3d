#ifndef DUST3D_BONE_MARK_H
#define DUST3D_BONE_MARK_H
#include <QColor>
#include <QString>

enum class BoneMark
{
    None = 0,
    Neck,
    Limb,
    Tail,
    Joint,
    Count
};
#define BoneMarkHasSide(mark) ((mark) == BoneMark::Limb)
QColor BoneMarkToColor(BoneMark mark);
#define IMPL_BoneMarkToColor                                        \
QColor BoneMarkToColor(BoneMark mark)                               \
{                                                                   \
    switch (mark) {                                                 \
        case BoneMark::Neck:                                        \
            return QColor(0x51, 0xba, 0xf2);                        \
        case BoneMark::Limb:                                        \
            return QColor(0x29, 0xfd, 0x2f);                        \
        case BoneMark::Tail:                                        \
            return QColor(0xff, 0xfd, 0x38);                        \
        case BoneMark::Joint:                                       \
            return QColor(0xcf, 0x83, 0xe1);                        \
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
        case BoneMark::Limb:                                        \
            return "Limb";                                          \
        case BoneMark::Tail:                                        \
            return "Tail";                                          \
        case BoneMark::Joint:                                       \
            return "Joint";                                         \
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
    if (mark == "Limb")                                             \
        return BoneMark::Limb;                                      \
    if (mark == "Tail")                                             \
        return BoneMark::Tail;                                      \
    if (mark == "Joint")                                            \
        return BoneMark::Joint;                                     \
    return BoneMark::None;                                          \
}
QString BoneMarkToDispName(BoneMark mark);
#define IMPL_BoneMarkToDispName                                     \
QString BoneMarkToDispName(BoneMark mark)                           \
{                                                                   \
    switch (mark) {                                                 \
        case BoneMark::Neck:                                        \
            return QObject::tr("Neck");                             \
        case BoneMark::Limb:                                        \
            return QObject::tr("Limb");                             \
        case BoneMark::Tail:                                        \
            return QObject::tr("Tail");                             \
        case BoneMark::Joint:                                       \
            return QObject::tr("Joint");                            \
        case BoneMark::None:                                        \
            return QObject::tr("None");                             \
        default:                                                    \
            return "";                                              \
    }                                                               \
}

#endif

