#ifndef DUST3D_CLOTH_FORCE_H
#define DUST3D_CLOTH_FORCE_H
#include <QString>

enum class ClothForce
{
    Gravitational = 0,
    Centripetal,
    Count
};
ClothForce ClothForceFromString(const char *forceString);
#define IMPL_ClothForceFromString                                  \
ClothForce ClothForceFromString(const char *forceString)           \
{                                                                  \
    QString force = forceString;                                   \
    if (force == "Gravitational")                                  \
        return ClothForce::Gravitational;                          \
    if (force == "Centripetal")                                    \
        return ClothForce::Centripetal;                            \
    return ClothForce::Gravitational;                              \
}
const char *ClothForceToString(ClothForce force);
#define IMPL_ClothForceToString                                    \
const char *ClothForceToString(ClothForce force)                   \
{                                                                  \
    switch (force) {                                               \
        case ClothForce::Gravitational:                            \
            return "Gravitational";                                \
        case ClothForce::Centripetal:                              \
            return "Centripetal";                                  \
        default:                                                   \
            return "Gravitational";                                \
    }                                                              \
}
QString ClothForceToDispName(ClothForce force);
#define IMPL_ClothForceToDispName                                  \
QString ClothForceToDispName(ClothForce force)                     \
{                                                                  \
    switch (force) {                                               \
        case ClothForce::Gravitational:                            \
            return QObject::tr("Gravitational");                   \
        case ClothForce::Centripetal:                              \
            return QObject::tr("Centripetal");                     \
        default:                                                   \
            return QObject::tr("Gravitational");                   \
    }                                                              \
}

#endif
