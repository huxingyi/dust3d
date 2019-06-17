#include <QObject>
#include "skeletonside.h"

IMPL_SkeletonSideToDispName

SkeletonSide SkeletonSideFromBoneName(const QString &boneName)
{
    if (boneName.startsWith("Left"))
        return SkeletonSide::Left;
    else if (boneName.startsWith("Right"))
        return SkeletonSide::Right;
    return SkeletonSide::None;
}
