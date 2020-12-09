#ifndef DUST3D_RAGDOLL_H
#define DUST3D_RAGDOLL_H
#include <vector>
#include <QVector3D>
#include <unordered_map>

class Ragdoll
{
public:
    struct Parameters
    {
        size_t iterations = 1;
    };
    
    struct Chain
    {
        size_t from;
        size_t to;
        double restLength;
    };
    
    struct VertexMotion
    {
        QVector3D position;
        QVector3D lastPosition;
        QVector3D force;
        bool fixed = false;
        double radius = 0.0;
    };
    
    struct JointConstraint
    {
        size_t joint;
        size_t first;
        size_t second;
        double minDegrees;
        double maxDegrees;
    };

    Ragdoll(const std::vector<QVector3D> *vertices,
            const std::vector<std::pair<size_t, size_t>> *links) :
        m_vertices(vertices),
        m_links(links)
    {
    }
    
    void addJointConstraint(size_t joint, 
            size_t first, 
            size_t second, 
            double minDegrees, 
            double maxDegrees)
    {
        JointConstraint jointConstraint = {
            joint, first, second, minDegrees, maxDegrees
        };
        m_jointConstraints.push_back(jointConstraint);
    }
    
    void setExternalForce(const QVector3D &externalForce)
    {
        m_externalForce = externalForce;
    }
    
    void start();
    void simulate(double stepSize);
    const VertexMotion &getVertexMotion(size_t vertexIndex);
    void updateVertexPosition(size_t vertexIndex, const QVector3D &position);
    void updateVertexRadius(size_t vertexIndex, double radius);
    void fixVertexPosition(size_t vertexIndex);
    
private:
    Parameters m_parameters;
    std::vector<Chain> m_chains;
    const std::vector<QVector3D> *m_vertices = nullptr;
    const std::vector<std::pair<size_t, size_t>> *m_links = nullptr;
    std::vector<JointConstraint> m_jointConstraints;
    std::unordered_map<size_t, VertexMotion> m_vertexMotions;
    QVector3D m_externalForce;
    double m_groundY = 0.0;
    
    void initializeVertexMotions();
    void prepareChains();
    void updateVertexForces();
    void applyConstraints();
    void doVerletIntegration(double stepSize);
    void applyBoundingConstraints(VertexMotion *vertexMotion);
    
    void outputChainsForDebug(const char *filename, const std::vector<Chain> &springs);
};

#endif
