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

bool InterpolationIsLinear(InterpolationType type)
{
    return QEasingCurve::Linear == InterpolationTypeToEasingCurveType(type);
}

bool InterpolationHasAccelerating(InterpolationType type)
{
    QString name = InterpolationTypeToString(type);
    if (-1 != name.indexOf("In"))
        return true;
    return false;
}

bool InterpolationHasDecelerating(InterpolationType type)
{
    QString name = InterpolationTypeToString(type);
    if (-1 != name.indexOf("Out"))
        return true;
    return false;
}

bool InterpolationIsBouncingBegin(InterpolationType type)
{
    QString name = InterpolationTypeToString(type);
    if (-1 != name.indexOf("InBack") || -1 != name.indexOf("InOutBack"))
        return true;
    return false;
}

bool InterpolationIsBouncingEnd(InterpolationType type)
{
    QString name = InterpolationTypeToString(type);
    if (-1 != name.indexOf("OutBack") || -1 != name.indexOf("InOutBack"))
        return true;
    return false;
}

InterpolationType InterpolationMakeFromOptions(bool isLinear,
    bool hasAccelerating, bool hasDecelerating,
    bool boucingBegin, bool bouncingEnd)
{
    if (isLinear)
        return InterpolationType::Linear;
    if (boucingBegin && bouncingEnd) {
        return InterpolationType::EaseInOutBack;
    } else if (boucingBegin) {
        return InterpolationType::EaseInBack;
    } else if (bouncingEnd) {
        return InterpolationType::EaseOutBack;
    } else {
        if (hasAccelerating && hasDecelerating) {
            return InterpolationType::EaseInOutCubic;
        } else if (hasAccelerating) {
            return InterpolationType::EaseInCubic;
        } else if (hasDecelerating) {
            return InterpolationType::EaseOutCubic;
        } else {
            return InterpolationType::EaseInOutCubic;
        }
    }
}

