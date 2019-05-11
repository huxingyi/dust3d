#ifndef DUST3D_COMBINE_MODE_H
#define DUST3D_COMBINE_MODE_H
#include <QString>

enum class CombineMode
{
    Normal = 0,
    Inversion,
    Uncombined,
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
    if (mode == "Uncombined")                                       \
        return CombineMode::Uncombined;                             \
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
        case CombineMode::Uncombined:                               \
            return "Uncombined";                                    \
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
        case CombineMode::Uncombined:                               \
            return QObject::tr("Uncombined");                       \
        default:                                                    \
            return QObject::tr("Normal");                           \
    }                                                               \
}

#endif
