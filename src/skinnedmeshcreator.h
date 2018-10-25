#ifndef DUST3D_SKINNED_MESH_CREATOR_H
#define DUST3D_SKINNED_MESH_CREATOR_H
#include <QMatrix4x4>
#include <vector>
#include <QVector3D>
#include "meshloader.h"
#include "outcome.h"
#include "jointnodetree.h"

class SkinnedMeshCreator
{
public:
    SkinnedMeshCreator(const Outcome &outcome,
        const std::map<int, AutoRiggerVertexWeights> &resultWeights);
    MeshLoader *createMeshFromTransform(const std::vector<QMatrix4x4> &matricies);
private:
    Outcome m_outcome;
    std::map<int, AutoRiggerVertexWeights> m_resultWeights;
    std::vector<QVector3D> m_verticesBindPositions;
    std::vector<QVector3D> m_verticesBindNormals;
};

#endif
