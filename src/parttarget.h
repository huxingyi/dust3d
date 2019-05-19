#ifndef DUST3D_PART_TARGET_H
#define DUST3D_PART_TARGET_H
#include <QString>
#include <QObject>

enum class PartTarget
{
    Model = 0,
    CutFace,
    Count
};
PartTarget PartTargetFromString(const char *targetString);
#define IMPL_PartTargetFromString                                               \
PartTarget PartTargetFromString(const char *targetString)                       \
{                                                                               \
    QString target = targetString;                                              \
    if (target == "Model")                                                      \
        return PartTarget::Model;                                               \
    if (target == "CutFace")                                                    \
        return PartTarget::CutFace;                                             \
    return PartTarget::Model;                                                   \
}
const char *PartTargetToString(PartTarget target);
#define IMPL_PartTargetToString                                                 \
const char *PartTargetToString(PartTarget target)                               \
{                                                                               \
    switch (target) {                                                           \
        case PartTarget::Model:                                                 \
            return "Model";                                                     \
        case PartTarget::CutFace:                                               \
            return "CutFace";                                                   \
        default:                                                                \
            return "Model";                                                     \
    }                                                                           \
}
QString PartTargetToDispName(PartTarget target);
#define IMPL_PartTargetToDispName                                               \
QString PartTargetToDispName(PartTarget target)                                 \
{                                                                               \
    switch (target) {                                                           \
        case PartTarget::Model:                                                 \
            return QObject::tr("Model");                                        \
        case PartTarget::CutFace:                                               \
            return QObject::tr("Cut Face");                                     \
        default:                                                                \
            return QObject::tr("Model");                                        \
    }                                                                           \
}

#endif
