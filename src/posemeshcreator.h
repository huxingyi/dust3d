#ifndef POSE_MESH_CREATOR_H
#define POSE_MESH_CREATOR_H
#include <QObject>
#include "meshloader.h"
#include "jointnodetree.h"
#include "meshresultcontext.h"

class PoseMeshCreator : public QObject
{
    Q_OBJECT
signals:
    void finished();
public:
    PoseMeshCreator(const std::vector<JointNode> &resultNodes,
        const MeshResultContext &meshResultContext,
        const std::map<int, AutoRiggerVertexWeights> &resultWeights);
    ~PoseMeshCreator();
    void createMesh();
    MeshLoader *takeResultMesh();
public slots:
    void process();
private:
    std::vector<JointNode> m_resultNodes;
    MeshResultContext m_meshResultContext;
    std::map<int, AutoRiggerVertexWeights> m_resultWeights;
    MeshLoader *m_resultMesh = nullptr;
};

#endif
