#ifndef DUST3D_REMESHER_H
#define DUST3D_REMESHER_H
#include <QObject>
#include <vector>
#include <QVector3D>
#include <QUuid>

class Remesher : public QObject
{
    Q_OBJECT
public:
    ~Remesher();
    void setMesh(const std::vector<QVector3D> &vertices,
        const std::vector<std::vector<size_t>> &triangles);
    void setNodes(const std::vector<std::pair<QVector3D, float>> &nodes,
        const std::vector<std::pair<QUuid, QUuid>> &sourceIds);
    void remesh();
    const std::vector<QVector3D> &getRemeshedVertices() const;
    const std::vector<std::vector<size_t>> &getRemeshedFaces() const;
    const std::vector<std::pair<QUuid, QUuid>> &getRemeshedVertexSources() const;
private:
    std::vector<QVector3D> m_vertices;
    std::vector<std::vector<size_t>> m_triangles;
    std::vector<QVector3D> m_remeshedVertices;
    std::vector<std::vector<size_t>> m_remeshedFaces;
    std::vector<std::pair<QUuid, QUuid>> m_remeshedVertexSources;
    std::vector<std::pair<QVector3D, float>> m_nodes;
    std::vector<std::pair<QUuid, QUuid>> m_sourceIds;
    void resolveSources();
    void isotropicRemesh(float targetEdgeLength=0.02f, unsigned int iterationNum=3);
};

#endif
