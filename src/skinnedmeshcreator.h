#ifndef DUST3D_SKINNED_MESH_CREATOR_H
#define DUST3D_SKINNED_MESH_CREATOR_H
#include <QMatrix4x4>
#include <vector>
#include <QVector3D>
#include <QColor>
#include "model.h"
#include "object.h"
#include "jointnodetree.h"

class SkinnedMeshCreator
{
public:
    SkinnedMeshCreator(const Object &object,
        const std::map<int, RiggerVertexWeights> &resultWeights);
    Model *createMeshFromTransform(const std::vector<QMatrix4x4> &matricies);
private:
    Object m_object;
    std::map<int, RiggerVertexWeights> m_resultWeights;
    std::vector<std::vector<int>> m_verticesOldIndices;
    std::vector<std::vector<QVector3D>> m_verticesBindPositions;
    std::vector<std::vector<QVector3D>> m_verticesBindNormals;
    std::vector<QColor> m_triangleColors;
};

#endif
