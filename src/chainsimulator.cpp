#include <set>
#include <unordered_set>
#include <unordered_map>
#include <QDebug>
#include "chainsimulator.h"

void ChainSimulator::prepareChains()
{
    for (size_t i = 1; i < m_vertices->size(); ++i) {
        size_t h = i - 1;
        m_chains.push_back({h, i,
            ((*m_vertices)[h] - (*m_vertices)[i]).length()});
    }
}

void ChainSimulator::initializeVertexMotions()
{
    for (size_t i = 0; i < m_vertices->size(); ++i)
        m_vertexMotions[i].position = m_vertexMotions[i].lastPosition = (*m_vertices)[i];
}

void ChainSimulator::outputChainsForDebug(const char *filename, const std::vector<Chain> &springs)
{
    FILE *fp = fopen(filename, "wb");
    for (const auto &it: *m_vertices) {
        fprintf(fp, "v %f %f %f\n", it[0], it[1], it[2]);
    }
    for (const auto &it: springs) {
        fprintf(fp, "l %zu %zu\n", it.from + 1, it.to + 1);
    }
    fclose(fp);
}

void ChainSimulator::start()
{
    initializeVertexMotions();
    prepareChains();

    outputChainsForDebug("debug-chains.obj", m_chains);
}

const ChainSimulator::VertexMotion &ChainSimulator::getVertexMotion(size_t vertexIndex)
{
    return m_vertexMotions[vertexIndex];
}

void ChainSimulator::updateVertexForces()
{
    for (auto &it: m_vertexMotions)
        it.second.force = QVector3D();
    
    QVector3D combinedForce = QVector3D(0.0, -9.80665, 0.0) + m_externalForce;
    for (auto &it: m_vertexMotions) {
        it.second.force += combinedForce;
    }
}

void ChainSimulator::doVerletIntegration(double stepSize)
{
    for (auto &it: m_vertexMotions) {
        if (it.second.fixed)
            continue;
        QVector3D &x = it.second.position;
        QVector3D temp = x;
        QVector3D &oldX = it.second.lastPosition;
        QVector3D a = it.second.force / m_parameters.particleMass;
        x += x - oldX + a * stepSize * stepSize;
        oldX = temp;
    }
}

void ChainSimulator::applyBoundingConstraints(QVector3D *position)
{
    if (position->y() < m_groundY)
        position->setY(m_groundY);
}

void ChainSimulator::applyConstraints()
{
    for (size_t iteration = 0; iteration < m_parameters.iterations; ++iteration) {
        for (auto &it: m_chains) {
            auto &from = m_vertexMotions[it.from];
            auto &to = m_vertexMotions[it.to];
            auto delta = from.position - to.position;
            auto deltaLength = delta.length();
            if (qFuzzyIsNull(deltaLength))
                continue;
            auto diff = (it.restLength - deltaLength) / deltaLength;
            auto offset = delta * 0.5 * diff;
            if (!from.fixed) {
                from.position += offset;
                applyBoundingConstraints(&from.position);
            }
            if (!to.fixed) {
                to.position += -offset;
                applyBoundingConstraints(&to.position);
            }
        }
    }
}

void ChainSimulator::updateVertexPosition(size_t vertexIndex, const QVector3D &position)
{
    m_vertexMotions[vertexIndex].position = position;
}

void ChainSimulator::fixVertexPosition(size_t vertexIndex)
{
    m_vertexMotions[vertexIndex].fixed = true;
}

void ChainSimulator::simulate(double stepSize)
{
    updateVertexForces();
    doVerletIntegration(stepSize);
    applyConstraints();
}
