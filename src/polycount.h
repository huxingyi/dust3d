#ifndef DUST3D_POLY_COUNT_H
#define DUST3D_POLY_COUNT_H
#include <QString>

enum class PolyCount
{
    LowPoly,
    Original,
    HighPoly,
    ExtremeHighPoly,
    Count
};
PolyCount PolyCountFromString(const char *countString);
#define IMPL_PolyCountFromString                                    \
PolyCount PolyCountFromString(const char *countString)              \
{                                                                   \
    QString count = countString;                                    \
    if (count == "LowPoly")                                         \
        return PolyCount::LowPoly;                                  \
    if (count == "Original")                                        \
        return PolyCount::Original;                                 \
    if (count == "HighPoly")                                        \
        return PolyCount::HighPoly;                                 \
    if (count == "ExtremeHighPoly")                                 \
        return PolyCount::ExtremeHighPoly;                          \
    return PolyCount::Original;                                     \
}
const char *PolyCountToString(PolyCount count);
#define IMPL_PolyCountToString                                      \
const char *PolyCountToString(PolyCount count)                      \
{                                                                   \
    switch (count) {                                                \
        case PolyCount::LowPoly:                                    \
            return "LowPoly";                                       \
        case PolyCount::Original:                                   \
            return "Original";                                      \
        case PolyCount::HighPoly:                                   \
            return "HighPoly";                                      \
        case PolyCount::ExtremeHighPoly:                            \
            return "ExtremeHighPoly";                               \
        default:                                                    \
            return "Original";                                      \
    }                                                               \
}
QString PolyCountToDispName(PolyCount count);
#define IMPL_PolyCountToDispName                                    \
QString PolyCountToDispName(PolyCount count)                        \
{                                                                   \
    switch (count) {                                                \
        case PolyCount::LowPoly:                                    \
            return QObject::tr("Low Poly");                         \
        case PolyCount::Original:                                   \
            return QObject::tr("Original");                         \
        case PolyCount::HighPoly:                                   \
            return QObject::tr("High Poly");                        \
        case PolyCount::ExtremeHighPoly:                            \
            return QObject::tr("Extreme High Poly");                \
        default:                                                    \
            return QObject::tr("Original");                         \
    }                                                               \
}
float PolyCountToValue(PolyCount count);
#define IMPL_PolyCountToValue                                       \
float PolyCountToValue(PolyCount count)                             \
{                                                                   \
    switch (count) {                                                \
        case PolyCount::LowPoly:                                    \
            return 0.6f;                                            \
        case PolyCount::Original:                                   \
            return 1.0f;                                            \
        case PolyCount::HighPoly:                                   \
            return 1.2f;                                            \
        case PolyCount::ExtremeHighPoly:                            \
            return 1.8f;                                            \
        default:                                                    \
            return 1.0f;                                            \
    }                                                               \
}

#endif
