#ifndef DUST3D_COMPONENT_LAYER_H
#define DUST3D_COMPONENT_LAYER_H
#include <QString>

enum class ComponentLayer
{
    Body = 0,
    Cloth,
    Count
};
ComponentLayer ComponentLayerFromString(const char *layerString);
#define IMPL_ComponentLayerFromString                                       \
ComponentLayer ComponentLayerFromString(const char *layerString)            \
{                                                                           \
    QString layer = layerString;                                            \
    if (layer == "Body")                                                    \
        return ComponentLayer::Body;                                        \
    if (layer == "Cloth")                                                   \
        return ComponentLayer::Cloth;                                       \
    return ComponentLayer::Body;                                            \
}
const char *ComponentLayerToString(ComponentLayer layer);
#define IMPL_ComponentLayerToString                                         \
const char *ComponentLayerToString(ComponentLayer layer)                    \
{                                                                           \
    switch (layer) {                                                        \
        case ComponentLayer::Body:                                          \
            return "Body";                                                  \
        case ComponentLayer::Cloth:                                         \
            return "Cloth";                                                 \
        default:                                                            \
            return "Body";                                                  \
    }                                                                       \
}
QString ComponentLayerToDispName(ComponentLayer layer);
#define IMPL_ComponentLayerToDispName                                       \
QString ComponentLayerToDispName(ComponentLayer layer)                      \
{                                                                           \
    switch (layer) {                                                        \
        case ComponentLayer::Body:                                          \
            return QObject::tr("Body");                                     \
        case ComponentLayer::Cloth:                                         \
            return QObject::tr("Cloth");                                    \
        default:                                                            \
            return QObject::tr("Body");                                     \
    }                                                                       \
}

#endif
