#ifndef GLTF_FILE_H
#define GLTF_FILE_H
#include <QObject>
#include <QString>
#include <QByteArray>
#include <QMatrix4x4>
#include <vector>
#include "meshresultcontext.h"
#include "json.hpp"

struct JointInfo
{
    int jointIndex;
    QVector3D position;
    QVector3D direction;
    QVector3D translation;
    QQuaternion rotation;
    QMatrix4x4 worldMatrix;
    QMatrix4x4 inverseBindMatrix;
    std::vector<int> children;
};

class GLTFFileWriter : public QObject
{
    Q_OBJECT
public:
    GLTFFileWriter(MeshResultContext &resultContext);
    bool save(const QString &filename);
    void traceBones(MeshResultContext &resultContext);
    void traceBoneFromJoint(MeshResultContext &resultContext, std::pair<int, int> node, std::set<std::pair<int, int>> &visitedNodes, std::set<std::pair<std::pair<int, int>, std::pair<int, int>>> &connections, int parentIndex);
private:
    QByteArray m_data;
    std::vector<JointInfo> m_tracedJoints;
    std::map<std::pair<int, int>, int> m_tracedNodeToJointIndexMap;
    QString getMatrixStringInColumnOrder(const QMatrix4x4 &mat);
private:
    nlohmann::json m_json;
};

#endif
