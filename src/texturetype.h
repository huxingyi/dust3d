#ifndef DUST3D_TEXTURE_TYPE_H
#define DUST3D_TEXTURE_TYPE_H
#include <QObject>
#include <QColor>
#include <QString>

enum class TextureType
{
    None,
    BaseColor,
    Normal,
    Metalness,
    Roughness,
    AmbientOcclusion,
    Count
};

QString TextureTypeToDispName(TextureType type);
#define IMPL_TextureTypeToDispName                                  \
QString TextureTypeToDispName(TextureType type)                     \
{                                                                   \
    switch (type) {                                                 \
        case TextureType::BaseColor:                                \
            return QObject::tr("Base Color");                       \
        case TextureType::Normal:                                   \
            return QObject::tr("Normal");                           \
        case TextureType::Metalness:                                \
            return QObject::tr("Metalness");                        \
        case TextureType::Roughness:                                \
            return QObject::tr("Roughness");                        \
        case TextureType::AmbientOcclusion:                         \
            return QObject::tr("Ambient Occlusion");                \
        case TextureType::None:                                     \
            return QObject::tr("None");                             \
        default:                                                    \
            return "";                                              \
    }                                                               \
}

const char *TextureTypeToString(TextureType type);
#define IMPL_TextureTypeToString                                    \
const char *TextureTypeToString(TextureType type)                   \
{                                                                   \
    switch (type) {                                                 \
        case TextureType::BaseColor:                                \
            return "BaseColor";                                     \
        case TextureType::Normal:                                   \
            return "Normal";                                        \
        case TextureType::Metalness:                                \
            return "Metalness";                                     \
        case TextureType::Roughness:                                \
            return "Roughness";                                     \
        case TextureType::AmbientOcclusion:                         \
            return "AmbientOcclusion";                              \
        case TextureType::None:                                     \
            return "None";                                          \
        default:                                                    \
            return "";                                              \
    }                                                               \
}
TextureType TextureTypeFromString(const char *typeString);
#define IMPL_TextureTypeFromString                                  \
TextureType TextureTypeFromString(const char *typeString)           \
{                                                                   \
    QString type = typeString;                                      \
    if (type == "BaseColor")                                        \
        return TextureType::BaseColor;                              \
    if (type == "Normal")                                           \
        return TextureType::Normal;                                 \
    if (type == "Metalness")                                        \
        return TextureType::Metalness;                              \
    if (type == "Roughness")                                        \
        return TextureType::Roughness;                              \
    if (type == "AmbientOcclusion")                                 \
        return TextureType::AmbientOcclusion;                       \
    return TextureType::None;                                       \
}

#endif
