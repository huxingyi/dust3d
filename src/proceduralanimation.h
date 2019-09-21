#ifndef DUST3D_PROCEDURAL_ANIMATION_H
#define DUST3D_PROCEDURAL_ANIMATION_H
#include <QString>

enum class ProceduralAnimation
{
    None = 0,
    FallToDeath,
    Count
};

ProceduralAnimation ProceduralAnimationFromString(const char *string);
#define IMPL_ProceduralAnimationFromString                                     \
ProceduralAnimation ProceduralAnimationFromString(const char *string)          \
{                                                                              \
    QString face = string;                                                     \
    if (face == "FallToDeath")                                                 \
        return ProceduralAnimation::FallToDeath;                               \
    return ProceduralAnimation::None;                                          \
}
QString ProceduralAnimationToString(ProceduralAnimation cutFace);
#define IMPL_ProceduralAnimationToString                                       \
QString ProceduralAnimationToString(ProceduralAnimation cutFace)               \
{                                                                              \
    switch (cutFace) {                                                         \
        case ProceduralAnimation::FallToDeath:                                 \
            return "FallToDeath";                                              \
        default:                                                               \
            return "";                                                         \
    }                                                                          \
}
QString ProceduralAnimationToDispName(ProceduralAnimation side);
#define IMPL_ProceduralAnimationToDispName                                     \
QString ProceduralAnimationToDispName(ProceduralAnimation side)                \
{                                                                              \
    switch (side) {                                                            \
        case ProceduralAnimation::FallToDeath:                                 \
            return QObject::tr("Fall to Death");                               \
        case ProceduralAnimation::None:                                        \
            return "";                                                         \
        default:                                                               \
            return "";                                                         \
    }                                                                          \
}

#endif
