#ifndef DUST3D_PART_BASE_H
#define DUST3D_PART_BASE_H
#include <QString>
#include <QObject>

enum class PartBase
{
    XYZ = 0,
    YZ,
    XY,
    ZX,
    Count
};
PartBase PartBaseFromString(const char *baseString);
#define IMPL_PartBaseFromString                                                 \
PartBase PartBaseFromString(const char *baseString)                             \
{                                                                               \
    QString base = baseString;                                                  \
    if (base == "XYZ")                                                          \
        return PartBase::XYZ;                                                   \
    if (base == "YZ")                                                           \
        return PartBase::YZ;                                                    \
    if (base == "XY")                                                           \
        return PartBase::XY;                                                    \
    if (base == "ZX")                                                           \
        return PartBase::ZX;                                                    \
    return PartBase::XYZ;                                                       \
}
const char *PartBaseToString(PartBase base);
#define IMPL_PartBaseToString                                                   \
const char *PartBaseToString(PartBase base)                                     \
{                                                                               \
    switch (base) {                                                             \
        case PartBase::XYZ:                                                     \
            return "XYZ";                                                       \
        case PartBase::YZ:                                                      \
            return "YZ";                                                        \
        case PartBase::XY:                                                      \
            return "XY";                                                        \
        case PartBase::ZX:                                                      \
            return "ZX";                                                        \
        default:                                                                \
            return "XYZ";                                                       \
    }                                                                           \
}
QString PartBaseToDispName(PartBase base);
#define IMPL_PartBaseToDispName                                                 \
QString PartBaseToDispName(PartBase base)                                       \
{                                                                               \
    switch (base) {                                                             \
        case PartBase::XYZ:                                                     \
            return QObject::tr("Dynamic");                                      \
        case PartBase::YZ:                                                      \
            return QObject::tr("Side Plane");                                   \
        case PartBase::XY:                                                      \
            return QObject::tr("Front Plane");                                  \
        case PartBase::ZX:                                                      \
            return QObject::tr("Top Plane");                                    \
        default:                                                                \
            return QObject::tr("Dynamic");                                      \
    }                                                                           \
}

#endif
