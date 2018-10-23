#ifndef INTERPOLATION_TYPE_H
#define INTERPOLATION_TYPE_H
#include <QString>
#include <QEasingCurve>

enum class InterpolationType
{
    None = 0,
    Linear,
    EaseInQuad,
    EaseOutQuad,
    EaseInOutQuad,
    EaseOutInQuad,
    EaseInCubic,
    EaseOutCubic,
    EaseInOutCubic,
    EaseOutInCubic,
    EaseInQuart,
    EaseOutQuart,
    EaseInOutQuart,
    EaseOutInQuart,
    EaseInQuint,
    EaseOutQuint,
    EaseInOutQuint,
    EaseOutInQuint,
    EaseInSine,
    EaseOutSine,
    EaseInOutSine,
    EaseOutInSine,
    EaseInExpo,
    EaseOutExpo,
    EaseInOutExpo,
    EaseOutInExpo,
    EaseInCirc,
    EaseOutCirc,
    EaseInOutCirc,
    EaseOutInCirc,
    EaseInElastic,
    EaseOutElastic,
    EaseInOutElastic,
    EaseOutInElastic,
    EaseInBack,
    EaseOutBack,
    EaseInOutBack,
    EaseOutInBack,
    EaseInBounce,
    EaseOutBounce,
    EaseInOutBounce,
    EaseOutInBounce,
    Count
};
InterpolationType InterpolationTypeFromString(const char *typeString);
#define IMPL_InterpolationTypeFromString                                      \
InterpolationType InterpolationTypeFromString(const char *typeString)         \
{                                                                             \
    QString type = typeString;                                                \
    static std::map<QString, InterpolationType> s_map = {                     \
        {"None", InterpolationType::None},                                    \
        {"Linear", InterpolationType::Linear},                                \
        {"EaseInQuad", InterpolationType::EaseInQuad},                        \
        {"EaseOutQuad", InterpolationType::EaseOutQuad},                      \
        {"EaseInOutQuad", InterpolationType::EaseInOutQuad},                  \
        {"EaseOutInQuad", InterpolationType::EaseOutInQuad},                  \
        {"EaseInCubic", InterpolationType::EaseInCubic},                      \
        {"EaseOutCubic", InterpolationType::EaseOutCubic},                    \
        {"EaseInOutCubic", InterpolationType::EaseInOutCubic},                \
        {"EaseOutInCubic", InterpolationType::EaseOutInCubic},                \
        {"EaseInQuart", InterpolationType::EaseInQuart},                      \
        {"EaseOutQuart", InterpolationType::EaseOutQuart},                    \
        {"EaseInOutQuart", InterpolationType::EaseInOutQuart},                \
        {"EaseOutInQuart", InterpolationType::EaseOutInQuart},                \
        {"EaseInQuint", InterpolationType::EaseInQuint},                      \
        {"EaseOutQuint", InterpolationType::EaseOutQuint},                    \
        {"EaseInOutQuint", InterpolationType::EaseInOutQuint},                \
        {"EaseOutInQuint", InterpolationType::EaseOutInQuint},                \
        {"EaseInSine", InterpolationType::EaseInSine},                        \
        {"EaseOutSine", InterpolationType::EaseOutSine},                      \
        {"EaseInOutSine", InterpolationType::EaseInOutSine},                  \
        {"EaseOutInSine", InterpolationType::EaseOutInSine},                  \
        {"EaseInExpo", InterpolationType::EaseInExpo},                        \
        {"EaseOutExpo", InterpolationType::EaseOutExpo},                      \
        {"EaseInOutExpo", InterpolationType::EaseInOutExpo},                  \
        {"EaseOutInExpo", InterpolationType::EaseOutInExpo},                  \
        {"EaseInCirc", InterpolationType::EaseInCirc},                        \
        {"EaseOutCirc", InterpolationType::EaseOutCirc},                      \
        {"EaseInOutCirc", InterpolationType::EaseInOutCirc},                  \
        {"EaseOutInCirc", InterpolationType::EaseOutInCirc},                  \
        {"EaseInElastic", InterpolationType::EaseInElastic},                  \
        {"EaseOutElastic", InterpolationType::EaseOutElastic},                \
        {"EaseInOutElastic", InterpolationType::EaseInOutElastic},            \
        {"EaseOutInElastic", InterpolationType::EaseOutInElastic},            \
        {"EaseInBack", InterpolationType::EaseInBack},                        \
        {"EaseOutBack", InterpolationType::EaseOutBack},                      \
        {"EaseInOutBack", InterpolationType::EaseInOutBack},                  \
        {"EaseOutInBack", InterpolationType::EaseOutInBack},                  \
        {"EaseInBounce", InterpolationType::EaseInBounce},                    \
        {"EaseOutBounce", InterpolationType::EaseOutBounce},                  \
        {"EaseInOutBounce", InterpolationType::EaseInOutBounce},              \
        {"EaseOutInBounce", InterpolationType::EaseOutInBounce}               \
    };                                                                        \
    auto findResult = s_map.find(type);                                       \
    if (findResult != s_map.end())                                            \
        return findResult->second;                                            \
    return InterpolationType::None;                                           \
}
const char *InterpolationTypeToString(InterpolationType type);
#define IMPL_InterpolationTypeToString                                       \
const char *InterpolationTypeToString(InterpolationType type)                \
{                                                                            \
    static const char *s_names[] = {                                         \
        "None",                                                              \
        "Linear",                                                            \
        "EaseInQuad",                                                        \
        "EaseOutQuad",                                                       \
        "EaseInOutQuad",                                                     \
        "EaseOutInQuad",                                                     \
        "EaseInCubic",                                                       \
        "EaseOutCubic",                                                      \
        "EaseInOutCubic",                                                    \
        "EaseOutInCubic",                                                    \
        "EaseInQuart",                                                       \
        "EaseOutQuart",                                                      \
        "EaseInOutQuart",                                                    \
        "EaseOutInQuart",                                                    \
        "EaseInQuint",                                                       \
        "EaseOutQuint",                                                      \
        "EaseInOutQuint",                                                    \
        "EaseOutInQuint",                                                    \
        "EaseInSine",                                                        \
        "EaseOutSine",                                                       \
        "EaseInOutSine",                                                     \
        "EaseOutInSine",                                                     \
        "EaseInExpo",                                                        \
        "EaseOutExpo",                                                       \
        "EaseInOutExpo",                                                     \
        "EaseOutInExpo",                                                     \
        "EaseInCirc",                                                        \
        "EaseOutCirc",                                                       \
        "EaseInOutCirc",                                                     \
        "EaseOutInCirc",                                                     \
        "EaseInElastic",                                                     \
        "EaseOutElastic",                                                    \
        "EaseInOutElastic",                                                  \
        "EaseOutInElastic",                                                  \
        "EaseInBack",                                                        \
        "EaseOutBack",                                                       \
        "EaseInOutBack",                                                     \
        "EaseOutInBack",                                                     \
        "EaseInBounce",                                                      \
        "EaseOutBounce",                                                     \
        "EaseInOutBounce",                                                   \
        "EaseOutInBounce"                                                    \
    };                                                                       \
    size_t index = (size_t)type;                                             \
    if (index < sizeof(s_names) / sizeof(s_names[0]))                        \
        return s_names[index];                                               \
    return "";                                                               \
}
QString InterpolationTypeToDispName(InterpolationType type);
#define IMPL_InterpolationTypeToDispName                                     \
QString InterpolationTypeToDispName(InterpolationType type)                  \
{                                                                            \
    return InterpolationTypeToString(type);                                  \
}
QEasingCurve::Type InterpolationTypeToEasingCurveType(InterpolationType type);
#define IMPL_InterpolationTypeToEasingCurveType                              \
QEasingCurve::Type InterpolationTypeToEasingCurveType(InterpolationType type)\
{                                                                            \
    static QEasingCurve::Type s_types[] = {                                  \
        QEasingCurve::Linear,                                                \
        QEasingCurve::Linear,                                                \
        QEasingCurve::InQuad,                                                \
        QEasingCurve::OutQuad,                                               \
        QEasingCurve::InOutQuad,                                             \
        QEasingCurve::OutInQuad,                                             \
        QEasingCurve::InCubic,                                               \
        QEasingCurve::OutCubic,                                              \
        QEasingCurve::InOutCubic,                                            \
        QEasingCurve::OutInCubic,                                            \
        QEasingCurve::InQuart,                                               \
        QEasingCurve::OutQuart,                                              \
        QEasingCurve::InOutQuart,                                            \
        QEasingCurve::OutInQuart,                                            \
        QEasingCurve::InQuint,                                               \
        QEasingCurve::OutQuint,                                              \
        QEasingCurve::InOutQuint,                                            \
        QEasingCurve::OutInQuint,                                            \
        QEasingCurve::InSine,                                                \
        QEasingCurve::OutSine,                                               \
        QEasingCurve::InOutSine,                                             \
        QEasingCurve::OutInSine,                                             \
        QEasingCurve::InExpo,                                                \
        QEasingCurve::OutExpo,                                               \
        QEasingCurve::InOutExpo,                                             \
        QEasingCurve::OutInExpo,                                             \
        QEasingCurve::InCirc,                                                \
        QEasingCurve::OutCirc,                                               \
        QEasingCurve::InOutCirc,                                             \
        QEasingCurve::OutInCirc,                                             \
        QEasingCurve::InElastic,                                             \
        QEasingCurve::OutElastic,                                            \
        QEasingCurve::InOutElastic,                                          \
        QEasingCurve::OutInElastic,                                          \
        QEasingCurve::InBack,                                                \
        QEasingCurve::OutBack,                                               \
        QEasingCurve::InOutBack,                                             \
        QEasingCurve::OutInBack,                                             \
        QEasingCurve::InBounce,                                              \
        QEasingCurve::OutBounce,                                             \
        QEasingCurve::InOutBounce,                                           \
        QEasingCurve::OutInBounce                                            \
    };                                                                       \
    size_t index = (size_t)type;                                             \
    if (index < sizeof(s_types) / sizeof(s_types[0]))                        \
        return s_types[index];                                               \
    return QEasingCurve::Linear;                                             \
}
bool InterpolationIsBouncingBegin(InterpolationType type);
bool InterpolationIsBouncingEnd(InterpolationType type);
InterpolationType InterpolationMakeBouncingType(InterpolationType type, bool boucingBegin, bool bouncingEnd);

float calculateInterpolation(InterpolationType type, float knot);

#endif
