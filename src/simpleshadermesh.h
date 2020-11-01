#ifndef DUST3D_SIMPLE_SHADER_MESH_H
#define DUST3D_SIMPLE_SHADER_MESH_H
#include <QVector3D>
#include <vector>

class SimpleShaderMesh
{
public:
    SimpleShaderMesh(std::vector<QVector3D> *vertices,
            std::vector<std::vector<size_t>> *triangles,
            std::vector<std::vector<QVector3D>> *triangleCornerNormals) :
        m_vertices(vertices),
        m_triangles(triangles),
        m_triangleCornerNormals(triangleCornerNormals)
    {
    }
    
    SimpleShaderMesh(const SimpleShaderMesh &mesh)
    {
        if (nullptr != mesh.m_vertices)
            m_vertices = new std::vector<QVector3D>(*mesh.m_vertices);
        if (nullptr != mesh.m_triangles)
            m_triangles = new std::vector<std::vector<size_t>>(*mesh.m_triangles);
        if (nullptr != mesh.m_triangleCornerNormals)
            m_triangleCornerNormals = new std::vector<std::vector<QVector3D>>(*mesh.m_triangleCornerNormals);
    }
    
    ~SimpleShaderMesh()
    {
        delete m_vertices;
        delete m_triangles;
        delete m_triangleCornerNormals;
    }
    
    const std::vector<QVector3D> *vertices()
    {
        return m_vertices;
    }
    
    const std::vector<std::vector<size_t>> *triangles()
    {
        return m_triangles;
    }
    
    const std::vector<std::vector<QVector3D>> *triangleCornerNormals()
    {
        return m_triangleCornerNormals;
    }
    
private:
    std::vector<QVector3D> *m_vertices = nullptr;
    std::vector<std::vector<size_t>> *m_triangles = nullptr;
    std::vector<std::vector<QVector3D>> *m_triangleCornerNormals = nullptr;
};

#endif
