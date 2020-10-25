#ifndef DUST3D_PLANE_MESH_H
#define DUST3D_PLANE_MESH_H
#include <QVector3D>
#include <vector>

class PlaneMesh
{
public:
    PlaneMesh(const QVector3D &normal, 
            const QVector3D &axis,
            const QVector3D &origin, 
            double radius,
            size_t halfColumns,
            size_t halfRows) :
        m_normal(normal),
        m_axis(axis),
        m_origin(origin),
        m_radius(radius),
        m_halfColumns(halfColumns),
        m_halfRows(halfRows)
    {
    }

    ~PlaneMesh()
    {
        delete m_resultVertices;
        delete m_resultQuads;
        delete m_resultFaces;
    }
    
    std::vector<QVector3D> *takeResultVertices()
    {
        std::vector<QVector3D> *resultVertices = m_resultVertices;
        m_resultVertices = nullptr;
        return resultVertices;
    }
    
    std::vector<std::vector<size_t>> *takeResultQuads()
    {
        std::vector<std::vector<size_t>> *resultQuads = m_resultQuads;
        m_resultQuads = nullptr;
        return resultQuads;
    }
    
    std::vector<std::vector<size_t>> *takeResultFaces()
    {
        std::vector<std::vector<size_t>> *resultFaces = m_resultFaces;
        m_resultFaces = nullptr;
        return resultFaces;
    }
    
    void build();
    
private:
    std::vector<QVector3D> *m_resultVertices = nullptr;
    std::vector<std::vector<size_t>> *m_resultQuads = nullptr;
    std::vector<std::vector<size_t>> *m_resultFaces = nullptr;
    QVector3D m_normal;
    QVector3D m_axis;
    QVector3D m_origin;
    double m_radius = 0.0;
    size_t m_halfColumns = 0;
    size_t m_halfRows = 0;
};

#endif
