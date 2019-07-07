#ifndef DUST3D_TOOL_MESH_H
#define DUST3D_TOOL_MESH_H
#include <QVector3D>
#include <QMatrix4x4>
#include <vector>
#include <map>
#include "shadervertex.h"

class ToolMesh
{
public:
    ~ToolMesh();
    void addNode(const QString &nodeId, const QVector3D &position);
    void generate();
    ShaderVertex *takeShaderVertices(int *shaderVertexCount);
    
private:
    struct Node
    {
        QVector3D position;
    };

    std::map<QString, Node> m_nodes;
    std::map<QString, std::vector<ShaderVertex>> m_nodesVertices;
    
    static const std::vector<QVector3D> m_predefinedPoints;
    static const std::vector<QVector3D> m_predefinedNormals;
    
    void fillCube(std::vector<ShaderVertex> *vertices, const QVector3D &position);
};

#endif
