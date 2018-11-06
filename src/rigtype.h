#ifndef DUST3D_RIG_TYPE_H
#define DUST3D_RIG_TYPE_H
#include <QString>

enum class RigType
{
    None = 0,
    Animal,
    Count
};
RigType RigTypeFromString(const char *typeString);
#define IMPL_RigTypeFromString                                      \
RigType RigTypeFromString(const char *typeString)                   \
{                                                                   \
    QString type = typeString;                                      \
    if (type == "Animal")                                           \
        return RigType::Animal;                                     \
    return RigType::None;                                           \
}
const char *RigTypeToString(RigType type);
#define IMPL_RigTypeToString                                        \
const char *RigTypeToString(RigType type)                           \
{                                                                   \
    switch (type) {                                                 \
        case RigType::Animal:                                       \
            return "Animal";                                        \
        case RigType::None:                                         \
            return "None";                                          \
        default:                                                    \
            return "None";                                          \
    }                                                               \
}
QString RigTypeToDispName(RigType type);
#define IMPL_RigTypeToDispName                                      \
QString RigTypeToDispName(RigType type)                             \
{                                                                   \
    switch (type) {                                                 \
        case RigType::Animal:                                       \
            return QObject::tr("Animal");                           \
        case RigType::None:                                         \
            return QObject::tr("None");                             \
        default:                                                    \
            return "";                                              \
    }                                                               \
}

#endif
