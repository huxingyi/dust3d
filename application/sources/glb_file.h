#ifndef DUST3D_APPLICATION_GLB_FILE_H_
#define DUST3D_APPLICATION_GLB_FILE_H_

#include "bone_structure.h"
#include "document.h"
#include "json.hpp"
#include <QByteArray>
#include <QImage>
#include <QMatrix4x4>
#include <QObject>
#include <QQuaternion>
#include <QString>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/object.h>
#include <map>
#include <vector>

class GlbFileWriter : public QObject {
    Q_OBJECT
public:
    GlbFileWriter(dust3d::Object& object,
        const QString& filename,
        QImage* textureImage = nullptr,
        QImage* normalImage = nullptr,
        QImage* ormImage = nullptr,
        const RigStructure* rigStructure = nullptr,
        const std::map<std::string, dust3d::Matrix4x4>* inverseBindMatrices = nullptr,
        const dust3d::Object* uvObject = nullptr,
        const std::vector<dust3d::RigAnimationClip>* animationClips = nullptr);
    bool save();
    bool save(QDataStream& output);

private:
    QString m_filename;
    bool m_outputNormal = true;
    bool m_outputUv = true;
    QByteArray m_binByteArray;
    QByteArray m_jsonByteArray;

private:
    nlohmann::json m_json;

public:
    static bool m_enableComment;
};

#endif

