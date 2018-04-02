#ifndef MODEL_MESH_BINDER_H
#define MODEL_MESH_BINDER_H
#include <QOpenGLVertexArrayObject>
#include <QMutex>
#include <QOpenGLBuffer>
#include <QString>
#include "mesh.h"

class ModelMeshBinder
{
public:
    ModelMeshBinder();
    ~ModelMeshBinder();
    void updateMesh(Mesh *mesh);
    void exportMeshAsObj(const QString &filename);
    void initialize();
    void paint();
    void cleanup();
private:
    QOpenGLVertexArrayObject m_vaoTriangle;
    QOpenGLBuffer m_vboTriangle;
    QOpenGLVertexArrayObject m_vaoEdge;
    QOpenGLBuffer m_vboEdge;
    Mesh *m_mesh;
    QMutex m_meshMutex;
    int m_renderTriangleVertexCount;
    int m_renderEdgeVertexCount;
    bool m_meshUpdated;
};

#endif
