#ifndef DUST3D_COMBINE_MODE_H
#define DUST3D_COMBINE_MODE_H
#include <QString>

enum class CombineMode
{
    Normal = 0,
    Inversion,
    Count
};
CombineMode CombineModeFromString(const char *modeString);
#define IMPL_CombineModeFromString                                  \
CombineMode CombineModeFromString(const char *modeString)           \
{                                                                   \
    QString mode = modeString;                                      \
    if (mode == "Normal")                                           \
        return CombineMode::Normal;                                 \
    if (mode == "Inversion")                                        \
        return CombineMode::Inversion;                              \
    return CombineMode::Normal;                                     \
}
const char *CombineModeToString(CombineMode mode);
#define IMPL_CombineModeToString                                    \
const char *CombineModeToString(CombineMode mode)                   \
{                                                                   \
    switch (mode) {                                                 \
        case CombineMode::Normal:                                   \
            return "Normal";                                        \
        case CombineMode::Inversion:                                \
            return "Inversion";                                     \
        default:                                                    \
            return "Normal";                                        \
    }                                                               \
}
QString CombineModeToDispName(CombineMode mode);
#define IMPL_CombineModeToDispName                                  \
QString CombineModeToDispName(CombineMode mode)                     \
{                                                                   \
    switch (mode) {                                                 \
        case CombineMode::Normal:                                   \
            return QObject::tr("Normal");                           \
        case CombineMode::Inversion:                                \
            return QObject::tr("Inversion");                        \
        default:                                                    \
            return QObject::tr("Normal");                           \
    }                                                               \
}

#endif
