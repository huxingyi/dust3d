#ifndef RIG_TYPE_H
#define RIG_TYPE_H
#include <QString>

enum class RigType
{
    None = 0,
    Tetrapod,
    Count
};
RigType RigTypeFromString(const char *typeString);
#define IMPL_RigTypeFromString                                      \
RigType RigTypeFromString(const char *typeString)                   \
{                                                                   \
    QString type = typeString;                                      \
    if (type == "Tetrapod")                                         \
        return RigType::Tetrapod;                                   \
    return RigType::None;                                           \
}
const char *RigTypeToString(RigType type);
#define IMPL_RigTypeToString                                        \
const char *RigTypeToString(RigType type)                           \
{                                                                   \
    switch (type) {                                                 \
        case RigType::Tetrapod:                                     \
            return "Tetrapod";                                      \
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
        case RigType::Tetrapod:                                     \
            return QObject::tr("Tetrapod");                         \
        case RigType::None:                                         \
            return QObject::tr("None");                             \
        default:                                                    \
            return "";                                              \
    }                                                               \
}

#endif
