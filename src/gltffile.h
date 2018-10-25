#ifndef DUST3D_GLTF_FILE_H
#define DUST3D_GLTF_FILE_H
#include <QObject>
#include <QString>
#include <QByteArray>
#include <QMatrix4x4>
#include <vector>
#include <QQuaternion>
#include "outcome.h"
#include "json.hpp"
#include "document.h"

class GltfFileWriter : public QObject
{
    Q_OBJECT
public:
    GltfFileWriter(Outcome &outcome,
        const std::vector<AutoRiggerBone> *resultRigBones,
        const std::map<int, AutoRiggerVertexWeights> *resultRigWeights,
        const QString &filename);
    bool save();
    const QString &textureFilenameInGltf();
private:
    QString m_filename;
    QString m_textureFilename;
    bool m_outputNormal;
    bool m_outputAnimation;
    bool m_outputUv;
    bool m_testOutputAsWhole;
private:
    nlohmann::json m_json;
public:
    static bool m_enableComment;
};

#endif
