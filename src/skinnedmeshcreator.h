#ifndef SKINNED_MESH_CREATOR_H
#define SKINNED_MESH_CREATOR_H
#include <QMatrix4x4>
#include <vector>
#include <QVector3D>
#include "meshloader.h"
#include "meshresultcontext.h"
#include "jointnodetree.h"

class SkinnedMeshCreator
{
public:
    SkinnedMeshCreator(const MeshResultContext &meshResultContext,
        const std::map<int, AutoRiggerVertexWeights> &resultWeights);
    MeshLoader *createMeshFromTransform(const std::vector<QMatrix4x4> &matricies);
private:
    MeshResultContext m_meshResultContext;
    std::map<int, AutoRiggerVertexWeights> m_resultWeights;
    std::vector<QVector3D> m_verticesBindPositions;
    std::vector<QVector3D> m_verticesBindNormals;
};

#endif
