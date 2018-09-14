#ifndef GLTF_FILE_H
#define GLTF_FILE_H
#include <QObject>
#include <QString>
#include <QByteArray>
#include <QMatrix4x4>
#include <vector>
#include <QQuaternion>
#include "meshresultcontext.h"
#include "json.hpp"
#include "skeletondocument.h"

struct GltfNode
{
    int parentIndex;
    QString name;
    QVector3D position;
    QVector3D translation;
    QQuaternion rotation;
    QMatrix4x4 bindMatrix;
    QMatrix4x4 inverseBindMatrix;
    std::vector<int> children;
};

class GltfFileWriter : public QObject
{
    Q_OBJECT
public:
    GltfFileWriter(MeshResultContext &resultContext,
        const std::vector<AutoRiggerBone> *resultRigBones,
        const std::map<int, AutoRiggerVertexWeights> *resultRigWeights,
        const QString &filename);
    bool save();
    const QString &textureFilenameInGltf();
private:
    void calculateMatrices(const std::vector<AutoRiggerBone> *resultRigBones);
    QString m_filename;
    QString m_textureFilename;
    bool m_outputNormal;
    bool m_outputAnimation;
    bool m_outputUv;
    bool m_testOutputAsWhole;
    std::vector<GltfNode> m_boneNodes;
private:
    nlohmann::json m_json;
public:
    static bool m_enableComment;
};

#endif
