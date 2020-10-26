#include <QDebug>
#include "simplerendermeshgenerator.h"

void SimpleRenderMeshGenerator::process()
{
    generate();
    
    emit finished();
}

void SimpleRenderMeshGenerator::generate()
{
    //qDebug() << "SimpleRenderMeshGenerator generating...";
    
    std::vector<std::vector<QVector3D>> *triangleCornerNormals = 
        new std::vector<std::vector<QVector3D>>(m_triangles->size());
    for (size_t i = 0; i < m_triangles->size(); ++i) {
        const auto &triangle = (*m_triangles)[i];
        QVector3D triangleNormal = QVector3D::normal(
            (*m_vertices)[triangle[0]],
            (*m_vertices)[triangle[1]],
            (*m_vertices)[triangle[2]]
        );
        (*triangleCornerNormals)[i] = {triangleNormal,
            triangleNormal, triangleNormal
        };
    }
    delete m_renderMesh;
    m_renderMesh = new SimpleShaderMesh(m_vertices, m_triangles, triangleCornerNormals);
    m_vertices = nullptr;
    m_triangles = nullptr;
    
    //qDebug() << "SimpleRenderMeshGenerator generation done";
}
