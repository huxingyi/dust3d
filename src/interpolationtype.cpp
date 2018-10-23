#include <map>
#include <QEasingCurve>
#include "interpolationtype.h"

IMPL_InterpolationTypeFromString
IMPL_InterpolationTypeToString
IMPL_InterpolationTypeToDispName
IMPL_InterpolationTypeToEasingCurveType

float calculateInterpolation(InterpolationType type, float knot)
{
    QEasingCurve easing;
    easing.setType(InterpolationTypeToEasingCurveType(type));
    return easing.valueForProgress(knot);
}

bool InterpolationIsBouncingBegin(InterpolationType type)
{
    QString name = InterpolationTypeToString(type);
    if (-1 == name.indexOf("InBack") && -1 == name.indexOf("InOutBack"))
        return false;
    return true;
}

bool InterpolationIsBouncingEnd(InterpolationType type)
{
    QString name = InterpolationTypeToString(type);
    if (-1 == name.indexOf("OutBack") && -1 == name.indexOf("InOutBack"))
        return false;
    return true;
}

InterpolationType InterpolationMakeBouncingType(InterpolationType type, bool boucingBegin, bool bouncingEnd)
{
    Q_UNUSED(type);
    if (boucingBegin && bouncingEnd)
        return InterpolationType::EaseInOutBack;
    else if (boucingBegin)
        return InterpolationType::EaseInBack;
    else if (bouncingEnd)
        return InterpolationType::EaseOutBack;
    else
        return InterpolationType::EaseInOutCubic;
}

