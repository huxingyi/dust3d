#ifndef DUST3D_REMESHER_H
#define DUST3D_REMESHER_H
#include <QObject>
#include <vector>
#include <QVector3D>

class Remesher : public QObject
{
    Q_OBJECT
public:
    ~Remesher();
    void setMesh(const std::vector<QVector3D> &vertices,
        const std::vector<std::vector<size_t>> &triangles);
    void remesh();
    const std::vector<QVector3D> &getRemeshedVertices();
    const std::vector<std::vector<size_t>> &getRemeshedFaces();
private:
    std::vector<QVector3D> m_vertices;
    std::vector<std::vector<size_t>> m_triangles;
    std::vector<QVector3D> m_remeshedVertices;
    std::vector<std::vector<size_t>> m_remeshedFaces;
};

#endif
