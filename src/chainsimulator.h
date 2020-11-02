#ifndef DUST3D_CHAIN_SIMULATOR_H
#define DUST3D_CHAIN_SIMULATOR_H
#include <vector>
#include <QVector3D>
#include <unordered_map>

class ChainSimulator
{
public:
    struct Parameters
    {
        double particleMass = 1;
        size_t iterations = 2;
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
    };

    ChainSimulator(const std::vector<QVector3D> *vertices) :
        m_vertices(vertices)
    {
    }
    
    void setExternalForce(const QVector3D &externalForce)
    {
        m_externalForce = externalForce;
    }
    
    void setGroundY(double groundY)
    {
        m_groundY = groundY;
    }
    
    void start();
    void simulate(double stepSize);
    const VertexMotion &getVertexMotion(size_t vertexIndex);
    void updateVertexPosition(size_t vertexIndex, const QVector3D &position);
    void fixVertexPosition(size_t vertexIndex);
    
private:
    Parameters m_parameters;
    std::vector<Chain> m_chains;
    const std::vector<QVector3D> *m_vertices = nullptr;
    std::unordered_map<size_t, VertexMotion> m_vertexMotions;
    QVector3D m_externalForce;
    double m_groundY = 0.0;
    
    void initializeVertexMotions();
    void prepareChains();
    void updateVertexForces();
    void applyConstraints();
    void doVerletIntegration(double stepSize);
    void applyBoundingConstraints(QVector3D *position);
    
    void outputChainsForDebug(const char *filename, const std::vector<Chain> &springs);
};

#endif
