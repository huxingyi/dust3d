#include <QString>
#include <QDebug>
#include <cmath>
#include <algorithm>
#include <set>
#include <nodemesh/builder.h>
#include <nodemesh/stitcher.h>
#include <nodemesh/box.h>
#include <nodemesh/combiner.h>
#include <nodemesh/misc.h>
#include <QMatrix4x4>

#define WRAP_STEP_BACK_FACTOR   0.1     // 0.1 ~ 0.9
#define WRAP_WELD_FACTOR        0.01    // Allowed distance: WELD_FACTOR * radius

namespace nodemesh 
{

size_t Builder::addNode(const QVector3D &position, float radius, const std::vector<QVector2D> &cutTemplate)
{
    size_t nodeIndex = m_nodes.size();
    Node node;
    node.position = position;
    node.radius = radius;
    node.cutTemplate = cutTemplate;
    m_nodes.push_back(node);
    m_sortedNodeIndices.push_back(nodeIndex);
    //qDebug() << "addNode" << position << radius;
    return nodeIndex;
}

size_t Builder::addEdge(size_t firstNodeIndex, size_t secondNodeIndex)
{
    size_t edgeIndex = m_edges.size();
    Edge edge;
    edge.nodes = {firstNodeIndex, secondNodeIndex};
    m_edges.push_back(edge);
    m_nodes[firstNodeIndex].edges.push_back(edgeIndex);
    m_nodes[secondNodeIndex].edges.push_back(edgeIndex);
    //qDebug() << "addEdge" << firstNodeIndex << secondNodeIndex;
    return edgeIndex;
}

const std::vector<QVector3D> &Builder::generatedVertices()
{
    return m_generatedVertices;
}

const std::vector<std::vector<size_t>> &Builder::generatedFaces()
{
    return m_generatedFaces;
}

const std::vector<size_t> &Builder::generatedVerticesSourceNodeIndices()
{
    return m_generatedVerticesSourceNodeIndices;
}

void Builder::sortNodeIndices()
{
    std::sort(m_sortedNodeIndices.begin(), m_sortedNodeIndices.end(), [&](const size_t &firstIndex,
            const size_t &secondIndex) {
        const Node &firstNode = m_nodes[firstIndex];
        const Node &secondNode = m_nodes[secondIndex];
        if (firstNode.edges.size() > secondNode.edges.size())
            return true;
        if (firstNode.edges.size() < secondNode.edges.size())
            return false;
        if (firstNode.radius > secondNode.radius)
            return true;
        if (firstNode.radius < secondNode.radius)
            return false;
        if (firstNode.position.y() > secondNode.position.y())
            return true;
        if (firstNode.position.y() < secondNode.position.y())
            return false;
        if (firstNode.position.z() > secondNode.position.z())
            return true;
        if (firstNode.position.z() < secondNode.position.z())
            return false;
        if (firstNode.position.x() > secondNode.position.x())
            return true;
        if (firstNode.position.x() < secondNode.position.x())
            return false;
        return false;
    });
}

void Builder::prepareNode(size_t nodeIndex)
{
    auto &node = m_nodes[nodeIndex];
    node.raysToNeibors.resize(node.edges.size());
    std::vector<QVector3D> neighborPositions(node.edges.size());
    std::vector<float> neighborRadius(node.edges.size());
    for (size_t i = 0; i < node.edges.size(); ++i) {
        size_t neighborIndex = m_edges[node.edges[i]].neiborOf(nodeIndex);
        const auto &neighbor = m_nodes[neighborIndex];
        node.raysToNeibors[i] = (neighbor.position - node.position).normalized();
        neighborPositions[i] = neighbor.position;
        neighborRadius[i] = neighbor.radius;
    }
    auto baseNormalResult = calculateBaseNormal(node.raysToNeibors,
        neighborPositions,
        neighborRadius);
    node.initialBaseNormal = baseNormalResult.first;
    node.hasInitialBaseNormal = baseNormalResult.second;
    if (node.hasInitialBaseNormal)
        node.initialBaseNormal = revisedBaseNormalAcordingToCutNormal(node.initialBaseNormal, node.traverseDirection);
}

void Builder::resolveBaseNormalRecursively(size_t nodeIndex)
{
    auto &node = m_nodes[nodeIndex];
    if (node.baseNormalResolved)
        return;
    
    if (node.hasInitialBaseNormal) {
        resolveBaseNormalForLeavesRecursively(nodeIndex, node.initialBaseNormal);
    } else {
        node.baseNormalSearched = true;
        auto searchResult = searchBaseNormalFromNeighborsRecursively(nodeIndex);
        if (searchResult.second) {
            resolveBaseNormalForLeavesRecursively(nodeIndex, searchResult.first);
        } else {
            resolveBaseNormalForLeavesRecursively(nodeIndex, QVector3D {0, 0, 1});
        }
    }
}

void Builder::resolveBaseNormalForLeavesRecursively(size_t nodeIndex, const QVector3D &baseNormal)
{
    auto &node = m_nodes[nodeIndex];
    if (node.baseNormalResolved)
        return;
    node.baseNormalResolved = true;
    node.baseNormal = baseNormal;
    for (size_t i = 0; i < node.edges.size(); ++i) {
        size_t neighborIndex = m_edges[node.edges[i]].neiborOf(nodeIndex);
        const auto &neighbor = m_nodes[neighborIndex];
        switch (neighbor.edges.size()) {
            case 1:
                resolveBaseNormalForLeavesRecursively(neighborIndex, baseNormal);
                break;
            case 2:
                neighbor.hasInitialBaseNormal ?
                        resolveBaseNormalForLeavesRecursively(neighborIndex, neighbor.initialBaseNormal) :
                        resolveBaseNormalForLeavesRecursively(neighborIndex, baseNormal);
                break;
            default:
                // void
                break;
        }
    }
}

void Builder::resolveInitialTraverseDirectionRecursively(size_t nodeIndex, const QVector3D *from, std::set<size_t> *visited)
{
    if (visited->find(nodeIndex) != visited->end())
        return;
    auto &node = m_nodes[nodeIndex];
    node.reversedTraverseOrder = visited->size();
    visited->insert(nodeIndex);
    if (nullptr != from) {
        node.initialTraverseDirection = (node.position - *from).normalized();
        node.hasInitialTraverseDirection = true;
    }
    for (size_t i = 0; i < node.edges.size(); ++i) {
        size_t neighborIndex = m_edges[node.edges[i]].neiborOf(nodeIndex);
        resolveInitialTraverseDirectionRecursively(neighborIndex, &node.position, visited);
    }
}

void Builder::resolveTraverseDirection(size_t nodeIndex)
{
    auto &node = m_nodes[nodeIndex];
    if (!node.hasInitialTraverseDirection) {
        if (node.edges.size() > 0) {
            size_t neighborIndex = m_edges[node.edges[0]].neiborOf(nodeIndex);
            const auto &neighbor = m_nodes[neighborIndex];
            node.initialTraverseDirection = neighbor.initialTraverseDirection;
            node.hasInitialTraverseDirection = true;
        }
    }
    if (node.edges.size() == 2) {
        std::vector<size_t> neighborIndices = {m_edges[node.edges[0]].neiborOf(nodeIndex),
            m_edges[node.edges[1]].neiborOf(nodeIndex)};
        std::sort(neighborIndices.begin(), neighborIndices.end(), [&](const size_t &firstIndex, const size_t &secondIndex) {
            return m_nodes[firstIndex].reversedTraverseOrder < m_nodes[secondIndex].reversedTraverseOrder;
        });
        node.traverseDirection = (node.initialTraverseDirection + m_nodes[neighborIndices[1]].initialTraverseDirection).normalized();
    } else {
        node.traverseDirection = node.initialTraverseDirection;
    }
}

std::pair<QVector3D, bool> Builder::searchBaseNormalFromNeighborsRecursively(size_t nodeIndex)
{
    auto &node = m_nodes[nodeIndex];
    node.baseNormalSearched = true;
    for (size_t i = 0; i < node.edges.size(); ++i) {
        size_t neighborIndex = m_edges[node.edges[i]].neiborOf(nodeIndex);
        const auto &neighbor = m_nodes[neighborIndex];
        if (neighbor.baseNormalResolved)
            return {neighbor.baseNormal, true};
    }
    for (size_t i = 0; i < node.edges.size(); ++i) {
        size_t neighborIndex = m_edges[node.edges[i]].neiborOf(nodeIndex);
        const auto &neighbor = m_nodes[neighborIndex];
        if (neighbor.hasInitialBaseNormal)
            return {neighbor.initialBaseNormal, true};
    }
    for (size_t i = 0; i < node.edges.size(); ++i) {
        size_t neighborIndex = m_edges[node.edges[i]].neiborOf(nodeIndex);
        const auto &neighbor = m_nodes[neighborIndex];
        if (neighbor.baseNormalSearched)
            continue;
        auto searchResult = searchBaseNormalFromNeighborsRecursively(neighborIndex);
        if (searchResult.second)
            return searchResult;
    }
    
    return {{}, false};
}

bool Builder::build()
{
    bool succeed = true;
    
    sortNodeIndices();
    
    if (m_sortedNodeIndices.size() < 2) {
        if (m_sortedNodeIndices.size() == 1) {
            const Node &node = m_nodes[0];
            int subdivideTimes = (node.cutTemplate.size() / 4) - 1;
            if (subdivideTimes < 0)
                subdivideTimes = 0;
            box(node.position, node.radius, subdivideTimes, m_generatedVertices, m_generatedFaces);
            m_generatedVerticesSourceNodeIndices.resize(m_generatedVertices.size(), 0);
        }
        return true;
    }
    
    {
        std::set<size_t> visited;
        for (auto nodeIndex = m_sortedNodeIndices.rbegin();
                nodeIndex != m_sortedNodeIndices.rend();
                ++nodeIndex) {
            resolveInitialTraverseDirectionRecursively(*nodeIndex, nullptr, &visited);
        }
    }
    {
        for (auto nodeIndex = m_sortedNodeIndices.rbegin();
                nodeIndex != m_sortedNodeIndices.rend();
                ++nodeIndex) {
            resolveTraverseDirection(*nodeIndex);
        }
    }
    
    for (const auto &nodeIndex: m_sortedNodeIndices) {
        prepareNode(nodeIndex);
    }
    for (const auto &nodeIndex: m_sortedNodeIndices) {
        resolveBaseNormalRecursively(nodeIndex);
    }
    
    unifyBaseNormals();
    
    for (const auto &nodeIndex: m_sortedNodeIndices) {
        if (!generateCutsForNode(nodeIndex))
            succeed = false;
    }
    
    stitchEdgeCuts();
    
    applyWeld();
    applyDeform();
    
    return succeed;
}

bool Builder::validateNormal(const QVector3D &normal)
{
    if (normal.isNull()) {
        return false;
    }
    if (!validatePosition(normal)) {
        return false;
    }
    return true;
}

std::pair<QVector3D, bool> Builder::calculateBaseNormal(const std::vector<QVector3D> &directs,
        const std::vector<QVector3D> &positions,
        const std::vector<float> &weights)
{
    auto calculateTwoPointsNormal = [&](size_t i0, size_t i1) -> std::pair<QVector3D, bool> {
        auto normal = QVector3D::crossProduct(directs[i0], directs[i1]).normalized();
        if (validateNormal(normal)) {
            return {normal, true};
        }
        return {{}, false};
    };
    auto calculateThreePointsNormal = [&](size_t i0, size_t i1, size_t i2) -> std::pair<QVector3D, bool> {
        auto normal = QVector3D::normal(positions[i0], positions[i1], positions[i2]);
        if (validateNormal(normal)) {
            return {normal, true};
        }
        // >=15 degrees && <= 165 degrees
        if (abs(QVector3D::dotProduct(directs[i0], directs[i1])) < 0.966) {
            auto twoPointsResult = calculateTwoPointsNormal(i0, i1);
            if (twoPointsResult.second)
                return twoPointsResult;
        }
        if (abs(QVector3D::dotProduct(directs[i1], directs[i2])) < 0.966) {
            auto twoPointsResult = calculateTwoPointsNormal(i1, i2);
            if (twoPointsResult.second)
                return twoPointsResult;
        }
        if (abs(QVector3D::dotProduct(directs[i2], directs[i0])) < 0.966) {
            auto twoPointsResult = calculateTwoPointsNormal(i2, i0);
            if (twoPointsResult.second)
                return twoPointsResult;
        }
        return {{}, false};
    };
    
    if (directs.size() <= 1) {
        return {{}, false};
    } else if (directs.size() <= 2) {
        // >=15 degrees && <= 165 degrees
        if (abs(QVector3D::dotProduct(directs[0], directs[1])) < 0.966) {
            auto twoPointsResult = calculateTwoPointsNormal(0, 1);
            if (twoPointsResult.second)
                return twoPointsResult;
        }
        return {{}, false};
    } else if (directs.size() <= 3) {
        return calculateThreePointsNormal(0, 1, 2);
    } else {
        std::vector<std::pair<size_t, float>> weightedIndices;
        for (size_t i = 0; i < weights.size(); ++i) {
            weightedIndices.push_back({i, weights[i]});
        }
        std::sort(weightedIndices.begin(), weightedIndices.end(), [](const std::pair<size_t, size_t> &first, const std::pair<size_t, size_t> &second) {
            return first.second > second.second;
        });
        return calculateThreePointsNormal(weightedIndices[0].first,
            weightedIndices[1].first,
            weightedIndices[2].first);
    }
}

void Builder::insertCutVertices(const std::vector<QVector3D> &cut, std::vector<size_t> &vertices, size_t nodeIndex, const QVector3D &cutDirect)
{
    for (const auto &position: cut) {
        size_t vertexIndex = m_generatedVertices.size();
        m_generatedVertices.push_back(position);
        m_generatedVerticesSourceNodeIndices.push_back(nodeIndex);
        m_generatedVerticesCutDirects.push_back(cutDirect);
        vertices.push_back(vertexIndex);
    }
}

bool Builder::generateCutsForNode(size_t nodeIndex)
{
    if (m_swallowedNodes.find(nodeIndex) != m_swallowedNodes.end()) {
        //qDebug() << "node" << nodeIndex << "ignore cuts generating because of been swallowed";
        return true;
    }
    
    auto &node = m_nodes[nodeIndex];
    size_t neighborsCount = node.edges.size();
    //qDebug() << "Generate cuts for node" << nodeIndex << "with neighbor count" << neighborsCount;
    if (1 == neighborsCount) {
        const QVector3D &cutNormal = node.raysToNeibors[0];
        std::vector<QVector3D> cut;
        makeCut(node.position, node.radius, node.cutTemplate, node.baseNormal, cutNormal, node.traverseDirection, cut);
        std::vector<size_t> vertices;
        insertCutVertices(cut, vertices, nodeIndex, cutNormal);
        m_generatedFaces.push_back(vertices);
        m_edges[node.edges[0]].cuts.push_back({vertices, -cutNormal});
    } else if (2 == neighborsCount) {
        const QVector3D cutNormal = (node.raysToNeibors[0].normalized() - node.raysToNeibors[1].normalized()) / 2;
        std::vector<QVector3D> cut;
        makeCut(node.position, node.radius, node.cutTemplate, node.baseNormal, cutNormal, node.traverseDirection, cut);
        std::vector<size_t> vertices;
        insertCutVertices(cut, vertices, nodeIndex, cutNormal);
        std::vector<size_t> verticesReversed;
        verticesReversed = vertices;
        std::reverse(verticesReversed.begin(), verticesReversed.end());
        m_edges[node.edges[0]].cuts.push_back({vertices, -cutNormal});
        m_edges[node.edges[1]].cuts.push_back({verticesReversed, cutNormal});
    } else if (neighborsCount >= 3) {
        std::vector<float> offsets(node.edges.size(), 0.0);
        bool offsetChanged = false;
        size_t tries = 0;
        do {
            offsetChanged = false;
            //qDebug() << "Try wrap #" << tries;
            if (tryWrapMultipleBranchesForNode(nodeIndex, offsets, offsetChanged)) {
                //qDebug() << "Wrap succeed";
                return true;
            }
            ++tries;
        } while (offsetChanged);
        return false;
    }
    
    return true;
}

bool Builder::tryWrapMultipleBranchesForNode(size_t nodeIndex, std::vector<float> &offsets, bool &offsetsChanged)
{
    auto backupVertices = m_generatedVertices;
    auto backupFaces = m_generatedFaces;
    auto backupSourceIndices = m_generatedVerticesSourceNodeIndices;
    auto backupVerticesCutDirects = m_generatedVerticesCutDirects;
    
    auto &node = m_nodes[nodeIndex];
    std::vector<std::pair<std::vector<size_t>, QVector3D>> cutsForWrapping;
    std::vector<std::pair<std::vector<size_t>, QVector3D>> cutsForEdges;
    bool directSwallowed = false;
    for (size_t i = 0; i < node.edges.size(); ++i) {
        const QVector3D &cutNormal = node.raysToNeibors[i];
        size_t edgeIndex = node.edges[i];
        size_t neighborIndex = m_edges[edgeIndex].neiborOf(nodeIndex);
        const auto &neighbor = m_nodes[neighborIndex];
        float distance = (neighbor.position - node.position).length();
        if (qFuzzyIsNull(distance))
            distance = 0.0001f;
        float radiusFactor = 1.0 - (node.radius / distance);
        float radius = node.radius * radiusFactor + neighbor.radius * (1.0 - radiusFactor);
        std::vector<QVector3D> cut;
        float offsetDistance = 0;
        offsetDistance = offsets[i] * (distance - node.radius - neighbor.radius);
        if (offsetDistance < 0)
            offsetDistance = 0;
        float finalDistance = node.radius + offsetDistance;
        if (finalDistance >= distance) {
            if (swallowEdgeForNode(nodeIndex, i)) {
                //qDebug() << "Neighbor too near to wrap, swallow it";
                offsets[i] = 0;
                offsetsChanged = true;
                directSwallowed = true;
                continue;
            }
        }
        makeCut(node.position + cutNormal * finalDistance, radius, node.cutTemplate, node.baseNormal, cutNormal, neighbor.traverseDirection, cut);
        std::vector<size_t> vertices;
        insertCutVertices(cut, vertices, nodeIndex, cutNormal);
        cutsForEdges.push_back({vertices, -cutNormal});
        std::vector<size_t> verticesReversed;
        verticesReversed = vertices;
        std::reverse(verticesReversed.begin(), verticesReversed.end());
        cutsForWrapping.push_back({verticesReversed, cutNormal});
    }
    if (directSwallowed) {
        m_generatedVertices = backupVertices;
        m_generatedFaces = backupFaces;
        m_generatedVerticesSourceNodeIndices = backupSourceIndices;
        m_generatedVerticesCutDirects = backupVerticesCutDirects;
        return false;
    }
    Stitcher stitcher;
    stitcher.setVertices(&m_generatedVertices);
    std::vector<size_t> failedEdgeLoops;
    bool stitchSucceed = stitcher.stitch(cutsForWrapping);
    std::vector<std::vector<size_t>> testFaces = stitcher.newlyGeneratedFaces();
    for (const auto &cuts: cutsForWrapping) {
        testFaces.push_back(cuts.first);
    }
    if (stitchSucceed) {
        stitchSucceed = nodemesh::isManifold(testFaces);
        if (!stitchSucceed) {
            //qDebug() << "Mesh stitch but not manifold";
        }
    }
    if (stitchSucceed) {
        nodemesh::Combiner::Mesh mesh(m_generatedVertices, testFaces, false);
        if (mesh.isNull()) {
            //qDebug() << "Mesh stitched but not not pass test";
            //nodemesh::exportMeshAsObj(m_generatedVertices, testFaces, "/Users/jeremy/Desktop/test.obj");
            stitchSucceed = false;
            for (size_t i = 0; i < node.edges.size(); ++i) {
                failedEdgeLoops.push_back(i);
            }
        }
    } else {
        //nodemesh::exportMeshAsObj(m_generatedVertices, testFaces, "/Users/jeremy/Desktop/test.obj");
        stitcher.getFailedEdgeLoops(failedEdgeLoops);
    }
    if (!stitchSucceed) {
        for (const auto &edgeLoop: failedEdgeLoops) {
            if (offsets[edgeLoop] + WRAP_STEP_BACK_FACTOR < 1.0) {
                offsets[edgeLoop] += WRAP_STEP_BACK_FACTOR;
                offsetsChanged = true;
            }
        }
        if (!offsetsChanged) {
            for (const auto &edgeLoop: failedEdgeLoops) {
                if (offsets[edgeLoop] + WRAP_STEP_BACK_FACTOR >= 1.0) {
                    if (swallowEdgeForNode(nodeIndex, edgeLoop)) {
                        //qDebug() << "No offset to step back, swallow neighbor instead";
                        offsets[edgeLoop] = 0;
                        offsetsChanged = true;
                        break;
                    }
                }
            }
        }
        m_generatedVertices = backupVertices;
        m_generatedFaces = backupFaces;
        m_generatedVerticesSourceNodeIndices = backupSourceIndices;
        m_generatedVerticesCutDirects = backupVerticesCutDirects;
        return false;
    }
    
    // Weld nearby vertices
    float weldThreshold = node.radius * WRAP_WELD_FACTOR;
    float allowedMinDist2 = weldThreshold * weldThreshold;
    for (size_t i = 0; i < node.edges.size(); ++i) {
        for (size_t j = i + 1; j < node.edges.size(); ++j) {
            const auto &first = cutsForEdges[i];
            const auto &second = cutsForEdges[j];
            for (const auto &u: first.first) {
                for (const auto &v: second.first) {
                    if ((m_generatedVertices[u] - m_generatedVertices[v]).lengthSquared() < allowedMinDist2) {
                        //qDebug() << "Weld" << v << "to" << u;
                        m_weldMap.insert({v, u});
                    }
                }
            }
        }
    }
    
    for (const auto &face: stitcher.newlyGeneratedFaces()) {
        m_generatedFaces.push_back(face);
    }
    for (size_t i = 0; i < node.edges.size(); ++i) {
        m_edges[node.edges[i]].cuts.push_back(cutsForEdges[i]);
    }
    
    return true;
}

bool Builder::swallowEdgeForNode(size_t nodeIndex, size_t edgeOrder)
{
    auto &node = m_nodes[nodeIndex];
    size_t edgeIndex = node.edges[edgeOrder];
    if (m_swallowedEdges.find(edgeIndex) != m_swallowedEdges.end()) {
        //qDebug() << "No more edge to swallow";
        return false;
    }
    size_t neighborIndex = m_edges[edgeIndex].neiborOf(nodeIndex);
    const auto &neighbor = m_nodes[neighborIndex];
    if (neighbor.edges.size() != 2) {
        //qDebug() << "Neighbor is not a simple two edges node to swallow, edges:" << neighbor.edges.size() << "neighbor:" << neighborIndex << "node" << nodeIndex;
        return false;
    }
    size_t anotherEdgeIndex = neighbor.anotherEdge(edgeIndex);
    if (m_swallowedEdges.find(anotherEdgeIndex) != m_swallowedEdges.end()) {
        //qDebug() << "Desired edge already been swallowed";
        return false;
    }
    node.edges[edgeOrder] = anotherEdgeIndex;
    //qDebug() << "Nodes of edge" << anotherEdgeIndex << "before update:";
    //for (const auto &it: m_edges[anotherEdgeIndex].nodes)
    //    qDebug() << it;
    m_edges[anotherEdgeIndex].updateNodeIndex(neighborIndex, nodeIndex);
    //qDebug() << "Nodes of edge" << anotherEdgeIndex << "after update:";
    //for (const auto &it: m_edges[anotherEdgeIndex].nodes)
    //    qDebug() << it;
    m_swallowedEdges.insert(edgeIndex);
    m_swallowedNodes.insert(neighborIndex);
    //qDebug() << "Swallow edge" << edgeIndex << "for node" << nodeIndex << "neighbor" << neighborIndex << "got eliminated, choosen edge" << anotherEdgeIndex;
    return true;
}

void Builder::unifyBaseNormals()
{
    std::vector<size_t> nodeIndices(m_nodes.size());
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        const auto &node = m_nodes[i];
        nodeIndices[node.reversedTraverseOrder] = i;
    }
    for (size_t i = 1; i < nodeIndices.size(); ++i) {
        size_t lastIndex = nodeIndices[i - 1];
        size_t nodeIndex = nodeIndices[i];
        auto &node = m_nodes[nodeIndex];
        const auto &last = m_nodes[lastIndex];
        if (QVector3D::dotProduct(node.baseNormal, last.baseNormal) <= 0)
            node.baseNormal = -node.baseNormal;
    }
}

QVector3D Builder::revisedBaseNormalAcordingToCutNormal(const QVector3D &baseNormal, const QVector3D &cutNormal)
{
    QVector3D orientedBaseNormal = QVector3D::dotProduct(cutNormal, baseNormal) > 0 ?
        baseNormal : -baseNormal;
    // 0.966: < 15 degress
    if (QVector3D::dotProduct(cutNormal, orientedBaseNormal) > 0.966) {
        if (QVector3D::dotProduct(cutNormal, QVector3D(1, 0, 0)) > 0.966) {
            orientedBaseNormal = QVector3D(0, 1, 0);
        } else {
            orientedBaseNormal = QVector3D(1, 0, 0);
        }
    }
    return orientedBaseNormal.normalized();
}

void Builder::makeCut(const QVector3D &position,
        float radius,
        const std::vector<QVector2D> &cutTemplate,
        QVector3D &baseNormal,
        const QVector3D &cutNormal,
        const QVector3D &traverseDirection,
        std::vector<QVector3D> &resultCut)
{
    baseNormal = revisedBaseNormalAcordingToCutNormal(baseNormal, cutNormal);
    auto finalCutTemplate = cutTemplate;
    auto finalCutNormal = cutNormal;
    float degree = 0;
    if (!qFuzzyIsNull(m_cutRotation)) {
        degree = m_cutRotation * 180;
    }
    if (QVector3D::dotProduct(cutNormal, traverseDirection) <= 0) {
        baseNormal = -baseNormal;
        finalCutNormal = -finalCutNormal;
        std::reverse(finalCutTemplate.begin(), finalCutTemplate.end());
        degree = ((int)degree + 180) % 360;
        //for (auto &it: finalCutTemplate) {
        //    it.setX(-it.x());
        //    it.setY(-it.y());
        //}
    }
    QVector3D u = QVector3D::crossProduct(finalCutNormal, baseNormal).normalized();
    QVector3D v = QVector3D::crossProduct(u, finalCutNormal).normalized();
    auto uFactor = u * radius;
    auto vFactor = v * radius;
    for (const auto &t: finalCutTemplate) {
        resultCut.push_back(uFactor * t.x() + vFactor * t.y());
    }
    if (!qFuzzyIsNull(degree)) {
        QMatrix4x4 rotation;
        rotation.rotate(degree, traverseDirection);
        baseNormal = rotation * baseNormal;
        for (auto &positionOnCut: resultCut) {
            positionOnCut = rotation * positionOnCut;
        }
    }
    for (auto &positionOnCut: resultCut) {
        positionOnCut += position;
    }
}

void Builder::stitchEdgeCuts()
{
    for (size_t edgeIndex = 0; edgeIndex < m_edges.size(); ++edgeIndex) {
        auto &edge = m_edges[edgeIndex];
        if (2 == edge.cuts.size()) {
            Stitcher stitcher;
            stitcher.setVertices(&m_generatedVertices);
            stitcher.stitch(edge.cuts);
            for (const auto &face: stitcher.newlyGeneratedFaces()) {
                m_generatedFaces.push_back(face);
            }
        }
    }
}

void Builder::applyWeld()
{
    if (m_weldMap.empty())
        return;
    
    std::vector<QVector3D> newVertices;
    std::vector<size_t> newSourceIndices;
    std::vector<std::vector<size_t>> newFaces;
    std::vector<QVector3D> newVerticesCutDirects;
    std::map<size_t, size_t> oldVertexToNewMap;
    for (const auto &face: m_generatedFaces) {
        std::vector<size_t> newIndices;
        std::set<size_t> inserted;
        for (const auto &oldVertex: face) {
            size_t useOldVertex = oldVertex;
            size_t newIndex = 0;
            auto findWeld = m_weldMap.find(useOldVertex);
            if (findWeld != m_weldMap.end()) {
                useOldVertex = findWeld->second;
            }
            auto findResult = oldVertexToNewMap.find(useOldVertex);
            if (findResult == oldVertexToNewMap.end()) {
                newIndex = newVertices.size();
                oldVertexToNewMap.insert({useOldVertex, newIndex});
                newVertices.push_back(m_generatedVertices[useOldVertex]);
                newSourceIndices.push_back(m_generatedVerticesSourceNodeIndices[useOldVertex]);
                newVerticesCutDirects.push_back(m_generatedVerticesCutDirects[useOldVertex]);
            } else {
                newIndex = findResult->second;
            }
            if (inserted.find(newIndex) != inserted.end())
                continue;
            inserted.insert(newIndex);
            newIndices.push_back(newIndex);
        }
        if (newIndices.size() < 3) {
            //qDebug() << "Face been welded";
            continue;
        }
        newFaces.push_back(newIndices);
    }
    
    m_generatedVertices = newVertices;
    m_generatedFaces = newFaces;
    m_generatedVerticesSourceNodeIndices = newSourceIndices;
    m_generatedVerticesCutDirects = newVerticesCutDirects;
}

void Builder::setDeformThickness(float thickness)
{
    m_deformThickness = thickness;
}

void Builder::setDeformWidth(float width)
{
    m_deformWidth = width;
}

void Builder::setCutRotation(float cutRotation)
{
    m_cutRotation = cutRotation;
}

QVector3D Builder::calculateDeformPosition(const QVector3D &vertexPosition, const QVector3D &ray, const QVector3D &deformNormal, float deformFactor)
{
    QVector3D revisedNormal = QVector3D::dotProduct(ray, deformNormal) < 0.0 ? -deformNormal : deformNormal;
    QVector3D projectRayOnRevisedNormal = revisedNormal * (QVector3D::dotProduct(ray, revisedNormal) / revisedNormal.lengthSquared());
    auto scaledProjct = projectRayOnRevisedNormal * deformFactor;
    return vertexPosition + (scaledProjct - projectRayOnRevisedNormal);
}

void Builder::applyDeform()
{
    for (size_t i = 0; i < m_generatedVertices.size(); ++i) {
        auto &position = m_generatedVertices[i];
        const auto &node = m_nodes[m_generatedVerticesSourceNodeIndices[i]];
        const auto &cutDirect = m_generatedVerticesCutDirects[i];
        auto ray = position - node.position;
        QVector3D sum;
        size_t count = 0;
        if (!qFuzzyCompare(m_deformThickness, (float)1.0)) {
            auto deformedPosition = calculateDeformPosition(position, ray, node.baseNormal, m_deformThickness);
            sum += deformedPosition;
            ++count;
        }
        if (!qFuzzyCompare(m_deformWidth, (float)1.0)) {
            auto deformedPosition = calculateDeformPosition(position, ray, QVector3D::crossProduct(node.baseNormal, cutDirect), m_deformWidth);
            sum += deformedPosition;
            ++count;
        }
        if (count > 0)
            position = sum / count;
    }
}

}
