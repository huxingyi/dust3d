#include <memory>
#include <QDebug>
#include "meshstitcher.h"

MeshStitcher::~MeshStitcher()
{
    delete m_wrapper;
}

void MeshStitcher::setVertices(const std::vector<QVector3D> *vertices)
{
    m_positions = vertices;
}

const std::vector<std::vector<size_t>> &MeshStitcher::newlyGeneratedFaces()
{
    if (m_wrapper)
        return m_wrapper->newlyGeneratedFaces();
    
    return m_newlyGeneratedFaces;
}

void MeshStitcher::getFailedEdgeLoops(std::vector<size_t> &failedEdgeLoops)
{
    if (m_wrapper)
        m_wrapper->getFailedEdgeLoops(failedEdgeLoops);
}

bool MeshStitcher::stitchByQuads(const std::vector<std::pair<std::vector<size_t>, QVector3D>> &edgeLoops)
{
    if (2 != edgeLoops.size())
        return false;
    const auto &firstEdgeLoop = edgeLoops[0].first;
    const auto &secondEdgeLoop = edgeLoops[1].first;
    if (firstEdgeLoop.size() < 3 || firstEdgeLoop.size() != secondEdgeLoop.size())
        return false;
    float minDist2 = std::numeric_limits<float>::max();
    size_t choosenStartIndex = 0;
    for (size_t k = 0; k < secondEdgeLoop.size(); ++k) {
        float sumOfDist2 = 0;
        std::vector<QVector3D> edges;
        for (size_t i = 0, j = k; i < firstEdgeLoop.size(); ++i, --j) {
            const auto &positionOnFirstEdgeLoop = (*m_positions)[firstEdgeLoop[i % firstEdgeLoop.size()]];
            const auto &positionOnSecondEdgeLoop = (*m_positions)[secondEdgeLoop[(j + secondEdgeLoop.size()) % secondEdgeLoop.size()]];
            auto edge = positionOnSecondEdgeLoop - positionOnFirstEdgeLoop;
            sumOfDist2 += edge.lengthSquared();
        }
        if (sumOfDist2 < minDist2) {
            minDist2 = sumOfDist2;
            choosenStartIndex = k;
        }
    }
    for (size_t i = 0, j = choosenStartIndex; i < firstEdgeLoop.size(); ++i, --j) {
        m_newlyGeneratedFaces.push_back({
            secondEdgeLoop[(j + secondEdgeLoop.size()) % secondEdgeLoop.size()],
            secondEdgeLoop[(j + secondEdgeLoop.size() - 1) % secondEdgeLoop.size()],
            firstEdgeLoop[(i + 1) % firstEdgeLoop.size()],
            firstEdgeLoop[i % firstEdgeLoop.size()],
        });
    }
    return true;
}

bool MeshStitcher::stitch(const std::vector<std::pair<std::vector<size_t>, QVector3D>> &edgeLoops)
{
    if (edgeLoops.size() == 2 &&
            edgeLoops[0].first.size() == edgeLoops[1].first.size())
        return stitchByQuads(edgeLoops);
    
    m_wrapper = new MeshWrapper;
    m_wrapper->setVertices(m_positions);
    m_wrapper->wrap(edgeLoops);
    if (!m_wrapper->finished()) {
        //qDebug() << "Wrapping failed";
        return false;
    }
    return true;
}

