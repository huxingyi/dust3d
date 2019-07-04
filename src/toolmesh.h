#ifndef DUST3D_TOOL_MESH_H
#define DUST3D_TOOL_MESH_H
#include <QVector3D>
#include <QMatrix4x4>
#include <vector>
#include "shadervertex.h"

class ToolMesh
{
public:
    void addNode(const QVector3D &position, float radius, const QMatrix4x4 &transform);
    void generate();
    ShaderVertex *takeShaderVertices(int *shaderVertexCount);
    
private:
    struct Node
    {
        QVector3D position;
        float radius;
        QMatrix4x4 transform;
    };

    std::vector<Node> m_nodes;
    ShaderVertex *m_shaderVertices = nullptr;
};

#endif
