#include <set>
#include <unordered_set>
#include <unordered_map>
#include <QDebug>
#include "ragdoll.h"
#include "util.h"

void Ragdoll::prepareChains()
{
    for (const auto &it: *m_links) {
        m_chains.push_back({it.first, it.second,
            ((*m_vertices)[it.first] - (*m_vertices)[it.second]).length()});
    }
}

void Ragdoll::initializeVertexMotions()
{
    for (size_t i = 0; i < m_vertices->size(); ++i)
        m_vertexMotions[i].position = m_vertexMotions[i].lastPosition = (*m_vertices)[i];
}

void Ragdoll::outputChainsForDebug(const char *filename, const std::vector<Chain> &springs)
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

void Ragdoll::start()
{
    initializeVertexMotions();
    prepareChains();

    //outputChainsForDebug("debug-chains.obj", m_chains);
}

const Ragdoll::VertexMotion &Ragdoll::getVertexMotion(size_t vertexIndex)
{
    return m_vertexMotions[vertexIndex];
}

void Ragdoll::updateVertexForces()
{
    for (auto &it: m_vertexMotions)
        it.second.force = QVector3D();
    
    QVector3D combinedForce = QVector3D(0.0, -9.80665, 0.0) + m_externalForce;
    for (auto &it: m_vertexMotions) {
        it.second.force += combinedForce;
    }
}

void Ragdoll::doVerletIntegration(double stepSize)
{
    for (auto &it: m_vertexMotions) {
        if (it.second.fixed)
            continue;
        QVector3D &x = it.second.position;
        QVector3D temp = x;
        QVector3D &oldX = it.second.lastPosition;
        QVector3D a = it.second.force;
        x += x - oldX + a * stepSize * stepSize;
        oldX = temp;
        applyBoundingConstraints(&it.second);
    }
}

void Ragdoll::applyBoundingConstraints(VertexMotion *vertexMotion)
{
    if (vertexMotion->position.y() < m_groundY + vertexMotion->radius)
        vertexMotion->position.setY(m_groundY + vertexMotion->radius);
}

void Ragdoll::applyConstraints()
{
    for (size_t iteration = 0; iteration < m_parameters.iterations; ++iteration) {
        for (auto &it: m_chains) {
            auto &from = m_vertexMotions[it.from];
            auto &to = m_vertexMotions[it.to];
            auto delta = from.position - to.position;
            auto deltaLength = delta.length();
            if (qFuzzyCompare(deltaLength, 0.0f))
                continue;
            auto diff = (it.restLength - deltaLength) / deltaLength;
            auto offset = delta * 0.5 * diff;
            if (!from.fixed) {
                from.position += offset;
                applyBoundingConstraints(&from);
            }
            if (!to.fixed) {
                to.position += -offset;
                applyBoundingConstraints(&to);
            }
        }
        
        for (const auto &it: m_jointConstraints) {
            auto &from = m_vertexMotions[it.first];
            auto &to = m_vertexMotions[it.second];
            auto &joint = m_vertexMotions[it.joint];
            double degrees = degreesBetweenVectors(from.position - joint.position,
                to.position - joint.position);
            if (degrees < it.minDegrees) {
                QVector3D straightPosition = (from.position + to.position) * 0.5;
                joint.position += (it.minDegrees - degrees) * (straightPosition - joint.position) / (180 - degrees);
                applyBoundingConstraints(&joint);
            } else if (degrees > it.maxDegrees) {
                // TODO:
            }
        }
    }
}

void Ragdoll::updateVertexPosition(size_t vertexIndex, const QVector3D &position)
{
    m_vertexMotions[vertexIndex].position = position;
}

void Ragdoll::fixVertexPosition(size_t vertexIndex)
{
    m_vertexMotions[vertexIndex].fixed = true;
}

void Ragdoll::updateVertexRadius(size_t vertexIndex, double radius)
{
    m_vertexMotions[vertexIndex].radius = radius;
}

void Ragdoll::simulate(double stepSize)
{
    updateVertexForces();
    doVerletIntegration(stepSize);
    applyConstraints();
}
