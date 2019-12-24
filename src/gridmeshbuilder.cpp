#include <queue>
#include "gridmeshbuilder.h"
#include "cyclefinder.h"
#include "regionfiller.h"
#include "util.h"

size_t GridMeshBuilder::addNode(const QVector3D &position, float radius)
{
    size_t newNodeIndex = m_nodes.size();
    
    Node node;
    node.position = position;
    node.radius = radius;
    node.source = newNodeIndex;
    m_nodes.push_back(node);
    
    return newNodeIndex;
}

size_t GridMeshBuilder::addEdge(size_t firstNodeIndex, size_t secondNodeIndex)
{
    size_t newEdgeIndex = m_edges.size();
    
    Edge edge;
    edge.firstNodeIndex = firstNodeIndex;
    edge.secondNodeIndex = secondNodeIndex;
    m_edges.push_back(edge);
    
    m_nodes[firstNodeIndex].neighborIndices.push_back(secondNodeIndex);
    m_nodes[secondNodeIndex].neighborIndices.push_back(firstNodeIndex);

    return newEdgeIndex;
}

const std::vector<QVector3D> &GridMeshBuilder::getGeneratedPositions()
{
    return m_generatedPositions;
}

const std::vector<size_t> &GridMeshBuilder::getGeneratedSources()
{
    return m_generatedSources;
}

const std::vector<std::vector<size_t>> &GridMeshBuilder::getGeneratedFaces()
{
    return m_generatedFaces;
}

void GridMeshBuilder::splitCycleToPolylines(const std::vector<size_t> &cycle,
        std::vector<std::vector<size_t>> *polylines)
{
    if (cycle.size() < 3) {
        qDebug() << "Invalid cycle size:" << cycle.size();
        return;
    }
    //qDebug() << "Cycle:" << cycle;
    std::vector<size_t> cornerIndices;
    for (size_t i = 0; i < cycle.size(); ++i) {
        size_t h = (i + cycle.size() - 1) % cycle.size();
        size_t j = (i + 1) % cycle.size();
        QVector3D hi = m_nodeVertices[cycle[i]].position - m_nodeVertices[cycle[h]].position;
        QVector3D ij = m_nodeVertices[cycle[j]].position - m_nodeVertices[cycle[i]].position;
        auto angle = degreesBetweenVectors(hi, ij);
        //qDebug() << "angle[" << i << "]:" << angle;
        if (angle >= m_polylineAngleChangeThreshold)
            cornerIndices.push_back(i);
    }
    if (cornerIndices.size() < 3) {
        qDebug() << "Invalid corners:" << cornerIndices.size();
        return;
    }
    for (size_t m = 0; m < cornerIndices.size(); ++m) {
        size_t n = (m + 1) % cornerIndices.size();
        std::vector<size_t> polyline;
        size_t i = cornerIndices[m];
        size_t j = cornerIndices[n];
        for (size_t p = i; p != j; p = (p + 1) % cycle.size())
            polyline.push_back(cycle[p]);
        polyline.push_back(cycle[j]);
        //qDebug() << "Polyline[m" << m << "n" << n << "i" << i << "j" << j << "]:" << polyline;
        polylines->push_back(polyline);
    }
}

void GridMeshBuilder::prepareNodeVertices()
{
    m_nodeVertices.resize(m_nodes.size());
    m_nodePositions.resize(m_nodes.size());
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        RegionFiller::Node node;
        node.position = m_nodes[i].position;
        node.radius = m_nodes[i].radius;
        node.source = i;
        m_nodeVertices[i] = node;
        m_nodePositions[i] = node.position;
    }
}

void GridMeshBuilder::generateFaces()
{
    auto createEdgeKey = [](size_t v1, size_t v2) {
        if (v1 > v2)
            std::swap(v1, v2);
        return std::make_pair(v1, v2);
    };
    
    std::map<std::pair<size_t, size_t>, std::vector<size_t>> edgeToCandidateFaceMap;
    m_generatedVertices = m_nodeVertices;
    std::vector<std::vector<size_t>> candidateFaces;
    std::vector<size_t> candidateFaceSourceCycles;
    m_generatedFaces.clear();
    
    auto addCandidateFaces = [&](const std::vector<std::vector<size_t>> &newFaces, size_t sourceCycle) {
        for (const auto &face: newFaces) {
            size_t candidateFaceIndex = candidateFaces.size();
            candidateFaces.push_back(face);
            candidateFaceSourceCycles.push_back(sourceCycle);
            for (size_t i = 0; i < face.size(); ++i) {
                size_t j = (i + 1) % face.size();
                auto edgeKey = createEdgeKey(face[i], face[j]);
                edgeToCandidateFaceMap[edgeKey].push_back(candidateFaceIndex);
            }
        }
    };
    
    for (size_t i = 0; i < m_cycles.size(); ++i) {
        const auto &it = m_cycles[i];
        std::vector<std::vector<size_t>> polylines;
        splitCycleToPolylines(it, &polylines);
        if (polylines.size() < 3) {
            std::vector<std::vector<size_t>> faces;
            faces.push_back(it);
            addCandidateFaces(faces, i);
            continue;
        }
        RegionFiller regionFiller(&m_generatedVertices, &polylines);
        if (regionFiller.fill()) {
            m_generatedVertices = regionFiller.getOldAndNewVertices();
        } else {
            regionFiller.fillWithoutPartition();
        }
        auto newFaces = regionFiller.getNewFaces();
        addCandidateFaces(newFaces, i);
    }
    
    if (candidateFaces.empty())
        return;
    
    std::set<size_t> visitedFaceIndices;
    std::queue<size_t> waitFaceIndices;
    waitFaceIndices.push(0);
    while (!waitFaceIndices.empty()) {
        auto faceIndex = waitFaceIndices.front();
        waitFaceIndices.pop();
        if (visitedFaceIndices.find(faceIndex) != visitedFaceIndices.end())
            continue;
        visitedFaceIndices.insert(faceIndex);
        const auto &face = candidateFaces[faceIndex];
        if (face.size() < 3) {
            qDebug() << "Invalid face, edges:" << face.size();
            continue;
        }
        bool shouldReverse = false;
        size_t checkIndex = 0;
        for (size_t i = 0; i < face.size(); ++i) {
            size_t j = (i + 1) % face.size();
            if (m_halfEdgeMap.find(std::make_pair(face[i], face[j])) != m_halfEdgeMap.end() ||
                    m_halfEdgeMap.find(std::make_pair(face[j], face[i])) != m_halfEdgeMap.end()) {
                checkIndex = i;
                break;
            }
        }
        size_t nextOfCheckIndex = (checkIndex + 1) % face.size();
        std::pair<size_t, size_t> edge = std::make_pair(face[checkIndex], face[nextOfCheckIndex]);
        if (m_halfEdgeMap.find(edge) != m_halfEdgeMap.end()) {
            std::pair<size_t, size_t> oppositeEdge = std::make_pair(face[nextOfCheckIndex], face[checkIndex]);
            if (m_halfEdgeMap.find(oppositeEdge) != m_halfEdgeMap.end()) {
                qDebug() << "Too many face share one edge, should not happen";
                continue;
            }
            shouldReverse = true;
        }
        auto finalFace = face;
        if (shouldReverse) {
            std::reverse(finalFace.begin(), finalFace.end());
        }
        size_t finalFaceIndex = m_generatedFaces.size();
        m_generatedFaces.push_back(finalFace);
        m_generatedFaceSourceCycles.push_back(candidateFaceSourceCycles[faceIndex]);
        for (size_t i = 0; i < finalFace.size(); ++i) {
            size_t j = (i + 1) % finalFace.size();
            auto insertResult = m_halfEdgeMap.insert({std::make_pair(finalFace[i], finalFace[j]), finalFaceIndex});
            if (!insertResult.second) {
                qDebug() << "Should not happend, half edge conflicts";
            }
            auto edgeKey = createEdgeKey(finalFace[i], finalFace[j]);
            auto findCandidates = edgeToCandidateFaceMap.find(edgeKey);
            if (findCandidates == edgeToCandidateFaceMap.end())
                continue;
            for (const auto &candidateFaceIndex: findCandidates->second) {
                if (visitedFaceIndices.find(candidateFaceIndex) != visitedFaceIndices.end())
                    continue;
                waitFaceIndices.push(candidateFaceIndex);
            }
        }
    }
}

void GridMeshBuilder::calculateNormals()
{
    std::vector<std::vector<QVector3D>> vertexNormals(m_generatedVertices.size());
    for (const auto &face: m_generatedFaces) {
        QVector3D sumOfNormals;
        for (size_t i = 0; i < face.size(); ++i) {
            size_t h = (i + face.size() - 1) % face.size();
            size_t j = (i + 1) % face.size();
            QVector3D vh = m_generatedVertices[face[h]].position;
            QVector3D vi = m_generatedVertices[face[i]].position;
            QVector3D vj = m_generatedVertices[face[j]].position;
            sumOfNormals += QVector3D::normal(vj - vi, vh - vi);
        }
        QVector3D faceNormal = sumOfNormals.normalized();
        for (size_t i = 0; i < face.size(); ++i) {
            vertexNormals[face[i]].push_back(faceNormal);
        }
    }
    m_nodeNormals.resize(m_generatedVertices.size());
    for (size_t i = 0; i < m_nodeNormals.size(); ++i) {
        const auto &normals = vertexNormals[i];
        if (normals.empty())
            continue;
        m_nodeNormals[i] = std::accumulate(normals.begin(), normals.end(), QVector3D()).normalized();
    }
}

void GridMeshBuilder::removeBigRingFaces()
{
    if (m_generatedFaces.size() != m_generatedFaceSourceCycles.size()) {
        qDebug() << "Generated source cycles invalid";
        return;
    }
    auto maxBigRingSize = m_maxBigRingSize;
    if (m_subdived)
        maxBigRingSize *= 2;
    std::set<size_t> invalidCycles;
    for (size_t faceIndex = 0; faceIndex < m_generatedFaces.size(); ++faceIndex) {
        const auto &face = m_generatedFaces[faceIndex];
        size_t sourceCycle = m_generatedFaceSourceCycles[faceIndex];
        size_t oppositeCycles = 0;
        for (size_t i = 0; i < face.size(); ++i) {
            size_t j = (i + 1) % face.size();
            auto oppositeEdge = std::make_pair(face[j], face[i]);
            auto findOpposite = m_halfEdgeMap.find(oppositeEdge);
            if (findOpposite == m_halfEdgeMap.end())
                continue;
            size_t oppositeFaceIndex = findOpposite->second;
            size_t oppositeFaceSourceCycle = m_generatedFaceSourceCycles[oppositeFaceIndex];
            if (sourceCycle == oppositeFaceSourceCycle)
                continue;
            ++oppositeCycles;
        }
        if (oppositeCycles > maxBigRingSize)
            invalidCycles.insert(sourceCycle);
    }
    
    if (invalidCycles.empty())
        return;
    
    auto oldFaces = m_generatedFaces;
    m_halfEdgeMap.clear();
    m_generatedFaces.clear();
    for (size_t faceIndex = 0; faceIndex < oldFaces.size(); ++faceIndex) {
        size_t sourceCycle = m_generatedFaceSourceCycles[faceIndex];
        if (invalidCycles.find(sourceCycle) != invalidCycles.end())
            continue;
        const auto &face = oldFaces[faceIndex];
        for (size_t i = 0; i < face.size(); ++i) {
            size_t j = (i + 1) % face.size();
            auto edge = std::make_pair(face[i], face[j]);
            m_halfEdgeMap.insert({edge, m_generatedFaces.size()});
        }
        m_generatedFaces.push_back(face);
    }
}

void GridMeshBuilder::extrude()
{
    removeBigRingFaces();
    calculateNormals();

    bool hasHalfEdge = false;
    for (const auto &halfEdge: m_halfEdgeMap) {
        auto oppositeHalfEdge = std::make_pair(halfEdge.first.second, halfEdge.first.first);
        if (m_halfEdgeMap.find(oppositeHalfEdge) != m_halfEdgeMap.end())
            continue;
        hasHalfEdge = true;
        break;
    }
    
    if (m_generatedVertices.empty())
        return;
    
    m_generatedPositions.resize(m_generatedVertices.size() * 2);
    m_generatedSources.resize(m_generatedPositions.size(), 0);
    for (size_t i = 0; i < m_generatedVertices.size(); ++i) {
        const auto &vertex = m_generatedVertices[i];
        const auto &normal = m_nodeNormals[i];
        m_generatedPositions[i] = vertex.position;
        m_generatedPositions[i] += normal * vertex.radius;
        m_generatedSources[i] = m_nodes[vertex.source].source;
        size_t j = m_generatedVertices.size() + i;
        m_generatedPositions[j] = vertex.position;
        m_generatedPositions[j] -= normal * vertex.radius;
        m_generatedSources[j] = m_generatedSources[i];
    }
    
    bool pickSecondMesh = false;
    // The outter faces should have longer edges
    float sumOfFirstMeshEdgeLength = 0;
    float sumOfSecondMeshEdgeLength = 0;
    for (size_t i = 0; i < m_generatedFaces.size(); ++i) {
        const auto &face = m_generatedFaces[i];
        for (size_t m = 0; m < face.size(); ++m) {
            size_t n = (m + 1) % face.size();
            sumOfFirstMeshEdgeLength += (m_generatedPositions[face[m]] - m_generatedPositions[face[n]]).length();
            sumOfSecondMeshEdgeLength += (m_generatedPositions[m_generatedVertices.size() + face[m]] - m_generatedPositions[m_generatedVertices.size() + face[n]]).length();
        }
    }
    if (sumOfFirstMeshEdgeLength < sumOfSecondMeshEdgeLength)
        pickSecondMesh = true;
    
    size_t faceNumPerLayer = m_generatedFaces.size();
    if (hasHalfEdge) {
        m_generatedFaces.resize(faceNumPerLayer * 2);
        for (size_t i = faceNumPerLayer; i < m_generatedFaces.size(); ++i) {
            auto &face = m_generatedFaces[i];
            face = m_generatedFaces[i - faceNumPerLayer];
            for (auto &it: face)
                it += m_generatedVertices.size();
            std::reverse(face.begin(), face.end());
        }
        for (const auto &halfEdge: m_halfEdgeMap) {
            auto oppositeHalfEdge = std::make_pair(halfEdge.first.second, halfEdge.first.first);
            if (m_halfEdgeMap.find(oppositeHalfEdge) != m_halfEdgeMap.end())
                continue;
            std::vector<size_t> face = {
                oppositeHalfEdge.first,
                oppositeHalfEdge.second,
                halfEdge.first.first + m_generatedVertices.size(),
                halfEdge.first.second + m_generatedVertices.size()
            };
            m_generatedFaces.push_back(face);
        }
    } else {
        if (pickSecondMesh) {
            for (auto &face: m_generatedFaces) {
                for (auto &it: face)
                    it += m_generatedVertices.size();
                std::reverse(face.begin(), face.end());
            }
        }
    }
}

void GridMeshBuilder::applyModifiers()
{
    std::vector<Node> oldNodes = m_nodes;
    std::vector<Edge> oldEdges = m_edges;
    
    m_edges.clear();
    
    float distance2Threshold = m_meshTargetEdgeSize * m_meshTargetEdgeSize;
    
    std::set<std::pair<size_t, size_t>> visitedEdges;
    for (const auto &oldEdge: oldEdges) {
        auto key = std::make_pair(oldEdge.firstNodeIndex, oldEdge.secondNodeIndex);
        if (visitedEdges.find(key) != visitedEdges.end())
            continue;
        visitedEdges.insert(std::make_pair(oldEdge.firstNodeIndex, oldEdge.secondNodeIndex));
        visitedEdges.insert(std::make_pair(oldEdge.secondNodeIndex, oldEdge.firstNodeIndex));
        const auto &oldStartNode = oldNodes[oldEdge.firstNodeIndex];
        const auto &oldStopNode = oldNodes[oldEdge.secondNodeIndex];
        if ((oldStartNode.position - oldStopNode.position).lengthSquared() <= distance2Threshold) {
            m_edges.push_back(oldEdge);
            continue;
        }
        auto oldEdgeLength = (oldStartNode.position - oldStopNode.position).length();
        size_t newInsertNum = oldEdgeLength / m_meshTargetEdgeSize;
        if (newInsertNum < 1)
            newInsertNum = 1;
        if (newInsertNum > 100)
            continue;
        float stepFactor = 1.0 / (newInsertNum + 1);
        float factor = stepFactor;
        std::vector<size_t> edgeNodeIndices;
        edgeNodeIndices.push_back(oldEdge.firstNodeIndex);
        for (size_t i = 0; i < newInsertNum && factor < 1.0; factor += stepFactor, ++i) {
            float firstFactor = 1.0 - factor;
            Node newNode;
            newNode.position = oldStartNode.position * firstFactor + oldStopNode.position * factor;
            newNode.radius = oldStartNode.radius * firstFactor + oldStopNode.radius * factor;
            if (firstFactor >= 0.5) {
                newNode.source = oldEdge.firstNodeIndex;
            } else {
                newNode.source = oldEdge.secondNodeIndex;
            }
            edgeNodeIndices.push_back(m_nodes.size());
            m_nodes.push_back(newNode);
        }
        edgeNodeIndices.push_back(oldEdge.secondNodeIndex);
        for (size_t i = 1; i < edgeNodeIndices.size(); ++i) {
            size_t h = i - 1;
            m_edges.push_back(Edge {edgeNodeIndices[h], edgeNodeIndices[i]});
        }
    }
}

void GridMeshBuilder::setSubdived(bool subdived)
{
    m_subdived = subdived;
}

bool GridMeshBuilder::build()
{
    if (m_subdived)
        applyModifiers();
    prepareNodeVertices();
    findCycles();
    if (m_cycles.empty())
        return false;
    generateFaces();
    extrude();
    return true;
}

void GridMeshBuilder::findCycles()
{
    std::vector<std::pair<size_t, size_t>> edges(m_edges.size());
    for (size_t i = 0; i < m_edges.size(); ++i) {
        const auto &source = m_edges[i];
        edges[i] = std::make_pair(source.firstNodeIndex, source.secondNodeIndex);
    }
    CycleFinder cycleFinder(m_nodePositions, edges);
    cycleFinder.find();
    m_cycles = cycleFinder.getCycles();
}

