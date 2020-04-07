#ifndef DUST3D_POSE_MESH_CREATOR_H
#define DUST3D_POSE_MESH_CREATOR_H
#include <QObject>
#include "model.h"
#include "jointnodetree.h"
#include "outcome.h"

class PoseMeshCreator : public QObject
{
    Q_OBJECT
signals:
    void finished();
public:
    PoseMeshCreator(const std::vector<JointNode> &resultNodes,
        const Outcome &outcome,
        const std::map<int, RiggerVertexWeights> &resultWeights);
    ~PoseMeshCreator();
    void createMesh();
    Model *takeResultMesh();
public slots:
    void process();
private:
    std::vector<JointNode> m_resultNodes;
    Outcome m_outcome;
    std::map<int, RiggerVertexWeights> m_resultWeights;
    Model *m_resultMesh = nullptr;
};

#endif
