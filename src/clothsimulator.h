#ifndef DUST3D_CLOTH_SIMULATOR_H
#define DUST3D_CLOTH_SIMULATOR_H
#include <QObject>
#include <QVector3D>
#include <vector>

struct mass_spring_system;
class MassSpringSolver;
class CgRootNode;
class CgSpringDeformationNode;
class CgMeshCollisionNode;
class CgPointFixNode;

class ClothSimulator : public QObject
{
    Q_OBJECT
public:
    ClothSimulator(const std::vector<QVector3D> &vertices,
        const std::vector<std::vector<size_t>> &faces,
        const std::vector<QVector3D> &collisionVertices,
        const std::vector<std::vector<size_t>> &collisionTriangles,
        const std::vector<QVector3D> &externalForces);
    ~ClothSimulator();
    void setStiffness(float stiffness);
    void create();
    void step();
    void getCurrentVertices(std::vector<QVector3D> *currentVertices);
private:
    std::vector<QVector3D> m_vertices;
    std::vector<std::vector<size_t>> m_faces;
    std::vector<QVector3D> m_collisionVertices;
    std::vector<std::vector<size_t>> m_collisionTriangles;
    std::vector<QVector3D> m_externalForces;
    std::vector<float> m_clothPointBuffer;
    std::vector<size_t> m_clothPointSources;
    std::vector<std::pair<size_t, size_t>> m_clothSprings;
    float m_stiffness = 1.0f;
    QVector3D m_offset;
    mass_spring_system *m_massSpringSystem = nullptr;
    MassSpringSolver *m_massSpringSolver = nullptr;
    CgRootNode *m_rootNode = nullptr;
    CgSpringDeformationNode *m_deformationNode = nullptr;
    CgMeshCollisionNode *m_meshCollisionNode = nullptr;
    CgPointFixNode *m_fixNode = nullptr;
    void convertMeshToCloth();
};

#endif
