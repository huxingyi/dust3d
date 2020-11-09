#ifndef DUST3D_RENDER_MESH_GENERATOR_H
#define DUST3D_RENDER_MESH_GENERATOR_H
#include <QObject>
#include <QVector3D>
#include "simpleshadermesh.h"

class SimpleRenderMeshGenerator: public QObject
{
    Q_OBJECT
public:
    SimpleRenderMeshGenerator(const std::vector<QVector3D> &vertices,
            const std::vector<std::vector<size_t>> &triangles,
            const std::vector<std::vector<QVector3D>> *triangleCornerNormals=nullptr) :
        m_vertices(new std::vector<QVector3D>(vertices)),
        m_triangles(new std::vector<std::vector<size_t>>(triangles)),
        m_triangleCornerNormals(nullptr != triangleCornerNormals ? 
            new std::vector<std::vector<QVector3D>>(*triangleCornerNormals) : 
            nullptr)
    {
    }
    
    ~SimpleRenderMeshGenerator()
    {
        delete m_vertices;
        delete m_triangles;
        delete m_renderMesh;
        delete m_triangleCornerNormals;
    }

    SimpleShaderMesh *takeRenderMesh()
    {
        SimpleShaderMesh *renderMesh = m_renderMesh;
        m_renderMesh = nullptr;
        return renderMesh;
    }
    
    void generate();
    
signals:
    void finished();
public slots:
    void process();
    
private:
    std::vector<QVector3D> *m_vertices = nullptr;
    std::vector<std::vector<size_t>> *m_triangles = nullptr;
    SimpleShaderMesh *m_renderMesh = nullptr;
    std::vector<std::vector<QVector3D>> *m_triangleCornerNormals = nullptr;
};

#endif
