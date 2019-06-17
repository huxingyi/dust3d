#ifndef DUST3D_SKELETON_SIDE_H
#define DUST3D_SKELETON_SIDE_H
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
SkeletonSide SkeletonSideFromBoneName(const QString &boneName);

#endif
