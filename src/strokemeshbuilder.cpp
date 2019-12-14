#include <QString>
#include <QDebug>
#include <cmath>
#include <algorithm>
#include <set>
#include <QMatrix4x4>
#include <unordered_set>
#include <queue>
#include <cmath>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <unordered_map>
#include "strokemeshbuilder.h"
#include "meshstitcher.h"
#include "boxmesh.h"
#include "meshcombiner.h"
#include "util.h"

#define WRAP_STEP_BACK_FACTOR   0.1     // 0.1 ~ 0.9
#define WRAP_WELD_FACTOR        0.01    // Allowed distance: WELD_FACTOR * radius

size_t StrokeMeshBuilder::addNode(const QVector3D &position, float radius, const std::vector<QVector2D> &cutTemplate, float cutRotation)
{
    size_t nodeIndex = m_nodes.size();
    Node node;
    node.position = position;
    node.radius = radius;
    node.cutTemplate = cutTemplate;
    node.cutRotation = cutRotation;
    m_nodes.push_back(node);
    m_sortedNodeIndices.push_back(nodeIndex);
    //qDebug() << "addNode" << position << radius;
    return nodeIndex;
}

size_t StrokeMeshBuilder::addEdge(size_t firstNodeIndex, size_t secondNodeIndex)
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

const std::vector<QVector3D> &StrokeMeshBuilder::generatedVertices()
{
    return m_generatedVertices;
}

const std::vector<std::vector<size_t>> &StrokeMeshBuilder::generatedFaces()
{
    return m_generatedFaces;
}

const std::vector<size_t> &StrokeMeshBuilder::generatedVerticesSourceNodeIndices()
{
    return m_generatedVerticesSourceNodeIndices;
}

void StrokeMeshBuilder::layoutNodes()
{
    std::unordered_set<size_t> processedNodes;
    std::queue<size_t> waitNodes;
    std::vector<size_t> threeBranchNodes;
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        if (m_nodes[i].edges.size() == 1) {
            waitNodes.push(i);
            break;
        }
    }
    if (waitNodes.empty())
        return;
    m_sortedNodeIndices.clear();
    while (!waitNodes.empty()) {
        auto index = waitNodes.front();
        waitNodes.pop();
        if (processedNodes.find(index) != processedNodes.end())
            continue;
        const auto &node = m_nodes[index];
        for (const auto &edgeIndex: node.edges) {
            const auto &edge = m_edges[edgeIndex];
            for (const auto &nodeIndex: edge.nodes) {
                if (processedNodes.find(nodeIndex) == processedNodes.end())
                    waitNodes.push(nodeIndex);
            }
        }
        if (node.edges.size() < 3) {
            m_sortedNodeIndices.push_back(index);
        } else {
            threeBranchNodes.push_back(index);
        }
        processedNodes.insert(index);
    }
    
    if (m_sortedNodeIndices.size() > 1) {
        QVector3D sumOfDirections;
        for (size_t i = 1; i < m_sortedNodeIndices.size(); ++i) {
            auto firstNodeIndex = m_sortedNodeIndices[i - 1];
            auto nextNodeIndex = m_sortedNodeIndices[i];
            sumOfDirections += (m_nodes[nextNodeIndex].position - m_nodes[firstNodeIndex].position);
        }
       
        QVector3D layoutDirection = sumOfDirections.normalized();
        const std::vector<QVector3D> axisList = {
            QVector3D(1, 0, 0),
            QVector3D(0, 1, 0),
            QVector3D(0, 0, 1),
        };
        std::vector<std::pair<float, size_t>> dots;
        for (size_t i = 0; i < axisList.size(); ++i) {
            dots.push_back(std::make_pair(qAbs(QVector3D::dotProduct(layoutDirection, axisList[i])), i));
        }
        std::sort(dots.begin(), dots.end(), [](const std::pair<float, size_t> &first,
                const std::pair<float, size_t> &second) {
            return first.first > second.first;
        });
        
        const auto &headNode = m_nodes[m_sortedNodeIndices[0]];
        const auto &tailNode = m_nodes[m_sortedNodeIndices[m_sortedNodeIndices.size() - 1]];
        
        bool needReverse = false;
        const auto &choosenAxis = dots[0].second;
        switch (choosenAxis) {
        case 0: // x
            if (headNode.position.x() * headNode.position.x() > tailNode.position.x() * tailNode.position.x())
                needReverse = true;
            break;
        case 1: // y
            if (headNode.position.y() * headNode.position.y() > tailNode.position.y() * tailNode.position.y())
                needReverse = true;
            break;
        case 2: // z
        default:
            if (headNode.position.z() * headNode.position.z() > tailNode.position.z() * tailNode.position.z())
                needReverse = true;
            break;
        }
        
        if (needReverse)
            std::reverse(m_sortedNodeIndices.begin(), m_sortedNodeIndices.end());
    }
    
    std::sort(threeBranchNodes.begin(), threeBranchNodes.end(), [&](const size_t &firstIndex,
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
    
    m_sortedNodeIndices.insert(m_sortedNodeIndices.begin(), threeBranchNodes.begin(), threeBranchNodes.end());
}

void StrokeMeshBuilder::sortNodeIndices()
{
    layoutNodes();
}

void StrokeMeshBuilder::prepareNode(size_t nodeIndex)
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
    if (node.edges.size() == 1) {
        node.cutNormal = node.raysToNeibors[0];
    } else if (node.edges.size() == 2) {
        node.cutNormal = (node.raysToNeibors[0] - node.raysToNeibors[1]) * 0.5;
    }
    auto baseNormalResult = calculateBaseNormal(node.raysToNeibors,
        neighborPositions,
        neighborRadius);
    node.initialBaseNormal = baseNormalResult.first;
    node.hasInitialBaseNormal = baseNormalResult.second;
    if (node.hasInitialBaseNormal)
        node.initialBaseNormal = revisedBaseNormalAcordingToCutNormal(node.initialBaseNormal, node.traverseDirection);
}

void StrokeMeshBuilder::setNodeOriginInfo(size_t nodeIndex, int nearOriginNodeIndex, int farOriginNodeIndex)
{
    auto &node = m_nodes[nodeIndex];
    node.nearOriginNodeIndex = nearOriginNodeIndex;
    node.farOriginNodeIndex = farOriginNodeIndex;
}

QVector3D StrokeMeshBuilder::calculateBaseNormalFromTraverseDirection(const QVector3D &traverseDirection)
{
    const std::vector<QVector3D> axisList = {
        QVector3D {1, 0, 0},
        QVector3D {0, 1, 0},
        QVector3D {0, 0, 1},
    };
    float maxDot = -1;
    size_t nearAxisIndex = 0;
    bool reversed = false;
    for (size_t i = 0; i < axisList.size(); ++i) {
        const auto axis = axisList[i];
        auto dot = QVector3D::dotProduct(axis, traverseDirection);
        auto positiveDot = abs(dot);
        if (positiveDot >= maxDot) {
            reversed = dot < 0;
            maxDot = positiveDot;
            nearAxisIndex = i;
        }
    }
    // axisList[nearAxisIndex] align with the traverse direction,
    // So we pick the next axis to do cross product with traverse direction
    const auto& choosenAxis = axisList[(nearAxisIndex + 1) % 3];
    auto baseNormal = QVector3D::crossProduct(traverseDirection, choosenAxis).normalized();
    return reversed ? -baseNormal : baseNormal;
}

void StrokeMeshBuilder::resolveBaseNormalRecursively(size_t nodeIndex)
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
            resolveBaseNormalForLeavesRecursively(nodeIndex,
                calculateBaseNormalFromTraverseDirection(node.traverseDirection));
        }
    }
}

void StrokeMeshBuilder::resolveBaseNormalForLeavesRecursively(size_t nodeIndex, const QVector3D &baseNormal)
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

void StrokeMeshBuilder::resolveInitialTraverseDirectionRecursively(size_t nodeIndex, const QVector3D *from, std::set<size_t> *visited)
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

void StrokeMeshBuilder::resolveTraverseDirection(size_t nodeIndex)
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

std::pair<QVector3D, bool> StrokeMeshBuilder::searchBaseNormalFromNeighborsRecursively(size_t nodeIndex)
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

bool StrokeMeshBuilder::build()
{
    bool succeed = true;
    
    sortNodeIndices();
    
    if (m_sortedNodeIndices.size() < 2) {
        if (m_sortedNodeIndices.size() == 1) {
            const Node &node = m_nodes[0];
            int subdivideTimes = (node.cutTemplate.size() / 4) - 1;
            if (subdivideTimes < 0)
                subdivideTimes = 0;
            boxmesh(node.position, node.radius, subdivideTimes, m_generatedVertices, m_generatedFaces);
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
    if (m_baseNormalAverageEnabled) {
        QVector3D sumNormal;
        for (const auto &node: m_nodes) {
            if (node.hasInitialBaseNormal) {
                sumNormal += node.initialBaseNormal * node.radius;
            }
        }
        QVector3D averageNormal = sumNormal.normalized();
        if (!averageNormal.isNull()) {
            for (auto &node: m_nodes) {
                if (node.hasInitialBaseNormal) {
                    node.initialBaseNormal = revisedBaseNormalAcordingToCutNormal(averageNormal, node.traverseDirection);
                }
            }
        }
    }
    for (const auto &nodeIndex: m_sortedNodeIndices) {
        resolveBaseNormalRecursively(nodeIndex);
    }
    
    unifyBaseNormals();
    localAverageBaseNormals();
    unifyBaseNormals();
    
    for (const auto &nodeIndex: m_sortedNodeIndices) {
        if (!generateCutsForNode(nodeIndex))
            succeed = false;
    }
    
    stitchEdgeCuts();
    
    applyWeld();
    applyDeform();
    finalizeHollow();
    
    return succeed;
}

void StrokeMeshBuilder::localAverageBaseNormals()
{
    std::vector<QVector3D> localAverageNormals;
    for (size_t nodeIndex = 0; nodeIndex < m_nodes.size(); ++nodeIndex) {
        const auto &node = m_nodes[nodeIndex];
        QVector3D sumOfNormals = node.baseNormal;
        for (const auto &edgeIndex: node.edges) {
            const auto &edge = m_edges[edgeIndex];
            size_t neighborIndex = edge.neiborOf(nodeIndex);
            const auto &neighbor = m_nodes[neighborIndex];
            sumOfNormals += neighbor.baseNormal;
        }
        localAverageNormals.push_back(sumOfNormals.normalized());
    }
    for (size_t nodeIndex = 0; nodeIndex < m_nodes.size(); ++nodeIndex) {
        m_nodes[nodeIndex].baseNormal = localAverageNormals[nodeIndex];
    }
}

bool StrokeMeshBuilder::validateNormal(const QVector3D &normal)
{
    if (normal.isNull()) {
        return false;
    }
    return true;
}

void StrokeMeshBuilder::enableBaseNormalOnX(bool enabled)
{
    m_baseNormalOnX = enabled;
}

void StrokeMeshBuilder::enableBaseNormalOnY(bool enabled)
{
    m_baseNormalOnY = enabled;
}

void StrokeMeshBuilder::enableBaseNormalOnZ(bool enabled)
{
    m_baseNormalOnZ = enabled;
}

void StrokeMeshBuilder::enableBaseNormalAverage(bool enabled)
{
    m_baseNormalAverageEnabled = enabled;
}

std::pair<QVector3D, bool> StrokeMeshBuilder::calculateBaseNormal(const std::vector<QVector3D> &inputDirects,
        const std::vector<QVector3D> &inputPositions,
        const std::vector<float> &weights)
{
    std::vector<QVector3D> directs = inputDirects;
    std::vector<QVector3D> positions = inputPositions;
    if (!m_baseNormalOnX || !m_baseNormalOnY || !m_baseNormalOnZ) {
        for (auto &it: directs) {
            if (!m_baseNormalOnX)
                it.setX(0);
            if (!m_baseNormalOnY)
                it.setY(0);
            if (!m_baseNormalOnZ)
                it.setZ(0);
        }
        for (auto &it: positions) {
            if (!m_baseNormalOnX)
                it.setX(0);
            if (!m_baseNormalOnY)
                it.setY(0);
            if (!m_baseNormalOnZ)
                it.setZ(0);
        }
    }
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

void StrokeMeshBuilder::insertCutVertices(const std::vector<QVector3D> &cut,
    std::vector<size_t> &vertices,
    size_t nodeIndex,
    const QVector3D &cutDirect,
    bool cutFlipped)
{
    size_t indexInCut = 0;
    for (const auto &position: cut) {
        size_t vertexIndex = m_generatedVertices.size();
        m_generatedVertices.push_back(position);
        m_generatedVerticesSourceNodeIndices.push_back(nodeIndex);
        m_generatedVerticesCutDirects.push_back(cutDirect);
        
        GeneratedVertexInfo info;
        info.orderInCut = cutFlipped ? ((cut.size() - indexInCut) % cut.size()) : indexInCut;
        info.cutSize = cut.size();
        m_generatedVerticesInfos.push_back(info);
        
        vertices.push_back(vertexIndex);
        
        ++indexInCut;
    }
}

const StrokeMeshBuilder::CutFaceTransform *StrokeMeshBuilder::nodeAdjustableCutFaceTransform(size_t nodeIndex)
{
    if (nodeIndex >= m_nodes.size())
        return nullptr;
    const auto &node = m_nodes[nodeIndex];
    if (!node.hasAdjustableCutFace)
        return nullptr;
    return &node.cutFaceTransform;
}

bool StrokeMeshBuilder::generateCutsForNode(size_t nodeIndex)
{
    if (m_swallowedNodes.find(nodeIndex) != m_swallowedNodes.end()) {
        //qDebug() << "node" << nodeIndex << "ignore cuts generating because of been swallowed";
        return true;
    }
    
    auto &node = m_nodes[nodeIndex];
    size_t neighborsCount = node.edges.size();
    //qDebug() << "Generate cuts for node" << nodeIndex << "with neighbor count" << neighborsCount;
    if (1 == neighborsCount) {
        QVector3D cutNormal = node.cutNormal;
        std::vector<QVector3D> cut;
        bool cutFlipped = false;
        makeCut(node.position, node.radius, node.cutTemplate, node.cutRotation, node.baseNormal, cutNormal, node.traverseDirection, cut, &node.cutFaceTransform, &cutFlipped);
        node.hasAdjustableCutFace = true;
        std::vector<size_t> vertices;
        insertCutVertices(cut, vertices, nodeIndex, cutNormal, cutFlipped);
        if (qFuzzyIsNull(m_hollowThickness)) {
            m_generatedFaces.push_back(vertices);
        } else {
            m_endCuts.push_back(vertices);
        }
        m_edges[node.edges[0]].cuts.push_back({vertices, -cutNormal});
    } else if (2 == neighborsCount) {
        QVector3D cutNormal = node.cutNormal;
        if (-1 != node.nearOriginNodeIndex && -1 != node.farOriginNodeIndex) {
            const auto &nearOriginNode = m_nodes[node.nearOriginNodeIndex];
            const auto &farOriginNode = m_nodes[node.farOriginNodeIndex];
            if (nearOriginNode.edges.size() <= 2 && farOriginNode.edges.size() <= 2) {
                float nearDistance = node.position.distanceToPoint(nearOriginNode.position);
                float farDistance = node.position.distanceToPoint(farOriginNode.position);
                float totalDistance = nearDistance + farDistance;
                float distanceFactor = nearDistance / totalDistance;
                const QVector3D *revisedNearCutNormal = nullptr;
                const QVector3D *revisedFarCutNormal = nullptr;
                if (distanceFactor <= 0.5) {
                    revisedNearCutNormal = &nearOriginNode.cutNormal;
                    revisedFarCutNormal = &node.cutNormal;
                } else {
                    distanceFactor = (1.0 - distanceFactor);
                    revisedNearCutNormal = &farOriginNode.cutNormal;
                    revisedFarCutNormal = &node.cutNormal;
                }
                distanceFactor *= 1.75;
                if (QVector3D::dotProduct(*revisedNearCutNormal, *revisedFarCutNormal) <= 0)
                    cutNormal = (*revisedNearCutNormal * (1.0 - distanceFactor) - *revisedFarCutNormal * distanceFactor).normalized();
                else
                    cutNormal = (*revisedNearCutNormal * (1.0 - distanceFactor) + *revisedFarCutNormal * distanceFactor).normalized();
                if (QVector3D::dotProduct(cutNormal, node.cutNormal) <= 0)
                    cutNormal = -cutNormal;
            }
        }
        std::vector<QVector3D> cut;
        bool cutFlipped = false;
        makeCut(node.position, node.radius, node.cutTemplate, node.cutRotation, node.baseNormal, cutNormal, node.traverseDirection, cut, &node.cutFaceTransform, &cutFlipped);
        node.hasAdjustableCutFace = true;
        std::vector<size_t> vertices;
        insertCutVertices(cut, vertices, nodeIndex, cutNormal, cutFlipped);
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

bool StrokeMeshBuilder::tryWrapMultipleBranchesForNode(size_t nodeIndex, std::vector<float> &offsets, bool &offsetsChanged)
{
    auto backupVertices = m_generatedVertices;
    auto backupFaces = m_generatedFaces;
    auto backupSourceIndices = m_generatedVerticesSourceNodeIndices;
    auto backupVerticesCutDirects = m_generatedVerticesCutDirects;
    auto backupVerticesInfos = m_generatedVerticesInfos;
    
    auto &node = m_nodes[nodeIndex];
    std::vector<std::pair<std::vector<size_t>, QVector3D>> cutsForWrapping;
    std::vector<std::pair<std::vector<size_t>, QVector3D>> cutsForEdges;
    bool directSwallowed = false;
    for (size_t i = 0; i < node.edges.size(); ++i) {
        QVector3D cutNormal = node.raysToNeibors[i];
        size_t edgeIndex = node.edges[i];
        size_t neighborIndex = m_edges[edgeIndex].neiborOf(nodeIndex);
        const auto &neighbor = m_nodes[neighborIndex];
        if (neighbor.edges.size() == 2) {
            size_t anotherEdgeIndex = neighbor.anotherEdge(edgeIndex);
            size_t neighborsNeighborIndex = m_edges[anotherEdgeIndex].neiborOf(neighborIndex);
            const auto &neighborsNeighbor = m_nodes[neighborsNeighborIndex];
            cutNormal = ((cutNormal + (neighborsNeighbor.position - neighbor.position).normalized()) * 0.5).normalized();
        }
        float distance = (neighbor.position - node.position).length();
        if (qFuzzyIsNull(distance))
            distance = 0.0001f;
        float radiusRate = neighbor.radius / node.radius;
        float tangentTriangleLongEdgeLength = distance + (radiusRate * distance / (1.0 - radiusRate));
        float segmentLength = node.radius * (tangentTriangleLongEdgeLength - node.radius) / tangentTriangleLongEdgeLength;
        float radius = segmentLength / std::sin(std::acos(node.radius / tangentTriangleLongEdgeLength));
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
        bool cutFlipped = false;
        makeCut(node.position + cutNormal * finalDistance, radius, node.cutTemplate, node.cutRotation, node.baseNormal, cutNormal, neighbor.traverseDirection, cut, nullptr, &cutFlipped);
        std::vector<size_t> vertices;
        insertCutVertices(cut, vertices, nodeIndex, cutNormal, cutFlipped);
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
        m_generatedVerticesInfos = backupVerticesInfos;
        return false;
    }
    MeshStitcher stitcher;
    stitcher.setVertices(&m_generatedVertices);
    std::vector<size_t> failedEdgeLoops;
    bool stitchSucceed = stitcher.stitch(cutsForWrapping);
    std::vector<std::vector<size_t>> testFaces = stitcher.newlyGeneratedFaces();
    for (const auto &cuts: cutsForWrapping) {
        testFaces.push_back(cuts.first);
    }
    if (stitchSucceed) {
        stitchSucceed = isManifold(testFaces);
        if (!stitchSucceed) {
            //qDebug() << "Mesh stitch but not manifold";
        }
    }
    if (stitchSucceed) {
        MeshCombiner::Mesh mesh(m_generatedVertices, testFaces, false);
        if (mesh.isNull()) {
            stitchSucceed = false;
            for (size_t i = 0; i < node.edges.size(); ++i) {
                failedEdgeLoops.push_back(i);
            }
        }
    } else {
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
        m_generatedVerticesInfos = backupVerticesInfos;
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

bool StrokeMeshBuilder::swallowEdgeForNode(size_t nodeIndex, size_t edgeOrder)
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

void StrokeMeshBuilder::unifyBaseNormals()
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

QVector3D StrokeMeshBuilder::revisedBaseNormalAcordingToCutNormal(const QVector3D &baseNormal, const QVector3D &cutNormal)
{
    QVector3D orientedBaseNormal = QVector3D::dotProduct(cutNormal, baseNormal) > 0 ?
        baseNormal : -baseNormal;
    // 0.966: < 15 degress
    if (QVector3D::dotProduct(cutNormal, orientedBaseNormal) > 0.966) {
        orientedBaseNormal = calculateBaseNormalFromTraverseDirection(cutNormal);
    }
    return orientedBaseNormal.normalized();
}

void StrokeMeshBuilder::makeCut(const QVector3D &position,
        float radius,
        const std::vector<QVector2D> &cutTemplate,
        float cutRotation,
        QVector3D &baseNormal,
        QVector3D &cutNormal,
        const QVector3D &traverseDirection,
        std::vector<QVector3D> &resultCut,
        CutFaceTransform *cutFaceTransform,
        bool *cutFlipped)
{
    auto finalCutTemplate = cutTemplate;
    float degree = 0;
    if (!qFuzzyIsNull(cutRotation)) {
        degree = cutRotation * 180;
    }
    if (QVector3D::dotProduct(cutNormal, traverseDirection) <= 0) {
        cutNormal = -cutNormal;
        std::reverse(finalCutTemplate.begin(), finalCutTemplate.end());
        std::rotate(finalCutTemplate.begin(), finalCutTemplate.begin() + finalCutTemplate.size() - 1, finalCutTemplate.end());
        if (nullptr != cutFaceTransform)
            cutFaceTransform->reverse = true;
        if (nullptr != cutFlipped)
            *cutFlipped = true;
    } else {
        if (nullptr != cutFlipped)
            *cutFlipped = false;
    }
    QVector3D u = QVector3D::crossProduct(cutNormal, baseNormal).normalized();
    QVector3D v = QVector3D::crossProduct(u, cutNormal).normalized();
    auto uFactor = u * radius;
    auto vFactor = v * radius;
    if (nullptr != cutFaceTransform) {
        cutFaceTransform->scale = radius;
        cutFaceTransform->translation = position;
        cutFaceTransform->uFactor = uFactor;
        cutFaceTransform->vFactor = vFactor;
    }
    for (const auto &t: finalCutTemplate) {
        resultCut.push_back(uFactor * t.x() + vFactor * t.y());
    }
    if (!qFuzzyIsNull(degree)) {
        QMatrix4x4 rotation;
        rotation.rotate(degree, cutNormal);
        baseNormal = rotation * baseNormal;
        for (auto &positionOnCut: resultCut) {
            positionOnCut = rotation * positionOnCut;
        }
        if (nullptr != cutFaceTransform)
            cutFaceTransform->rotation = rotation;
    }
    for (auto &positionOnCut: resultCut) {
        positionOnCut += position;
    }
}

void StrokeMeshBuilder::stitchEdgeCuts()
{
    for (size_t edgeIndex = 0; edgeIndex < m_edges.size(); ++edgeIndex) {
        auto &edge = m_edges[edgeIndex];
        if (2 == edge.cuts.size()) {
            MeshStitcher stitcher;
            stitcher.setVertices(&m_generatedVertices);
            stitcher.stitch(edge.cuts);
            for (const auto &face: stitcher.newlyGeneratedFaces()) {
                m_generatedFaces.push_back(face);
            }
        }
    }
}

void StrokeMeshBuilder::applyWeld()
{
    if (m_weldMap.empty())
        return;
    
    std::vector<QVector3D> newVertices;
    std::vector<size_t> newSourceIndices;
    std::vector<std::vector<size_t>> newFaces;
    std::vector<QVector3D> newVerticesCutDirects;
    std::vector<GeneratedVertexInfo> newVerticesInfos;
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
                newVerticesInfos.push_back(m_generatedVerticesInfos[useOldVertex]);
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
    m_generatedVerticesInfos = newVerticesInfos;
}

void StrokeMeshBuilder::setDeformThickness(float thickness)
{
    m_deformThickness = thickness;
}

void StrokeMeshBuilder::setDeformWidth(float width)
{
    m_deformWidth = width;
}

void StrokeMeshBuilder::setDeformMapImage(const QImage *image)
{
    m_deformMapImage = image;
}

void StrokeMeshBuilder::setHollowThickness(float hollowThickness)
{
    m_hollowThickness = hollowThickness;
}

void StrokeMeshBuilder::setDeformMapScale(float scale)
{
    m_deformMapScale = scale;
}

QVector3D StrokeMeshBuilder::calculateDeformPosition(const QVector3D &vertexPosition, const QVector3D &ray, const QVector3D &deformNormal, float deformFactor)
{
    QVector3D revisedNormal = QVector3D::dotProduct(ray, deformNormal) < 0.0 ? -deformNormal : deformNormal;
    QVector3D projectRayOnRevisedNormal = revisedNormal * (QVector3D::dotProduct(ray, revisedNormal) / revisedNormal.lengthSquared());
    auto scaledProjct = projectRayOnRevisedNormal * deformFactor;
    return vertexPosition + (scaledProjct - projectRayOnRevisedNormal);
}

void StrokeMeshBuilder::finalizeHollow()
{
    if (qFuzzyIsNull(m_hollowThickness))
        return;
    
    size_t startVertexIndex = m_generatedVertices.size();
    for (size_t i = 0; i < startVertexIndex; ++i) {
        const auto &position = m_generatedVertices[i];
        const auto &node = m_nodes[m_generatedVerticesSourceNodeIndices[i]];
        auto ray = position - node.position;
        
        auto newPosition = position - ray * m_hollowThickness;
        m_generatedVertices.push_back(newPosition);
        m_generatedVerticesCutDirects.push_back(m_generatedVerticesCutDirects[i]);
        m_generatedVerticesSourceNodeIndices.push_back(m_generatedVerticesSourceNodeIndices[i]);
        m_generatedVerticesInfos.push_back(m_generatedVerticesInfos[i]);
    }
    
    size_t oldFaceNum = m_generatedFaces.size();
    for (size_t i = 0; i < oldFaceNum; ++i) {
        auto newFace = m_generatedFaces[i];
        std::reverse(newFace.begin(), newFace.end());
        for (auto &it: newFace)
            it += startVertexIndex;
        m_generatedFaces.push_back(newFace);
    }
    
    for (const auto &cut: m_endCuts) {
        for (size_t i = 0; i < cut.size(); ++i) {
            size_t j = (i + 1) % cut.size();
            std::vector<size_t> quad;
            quad.push_back(cut[i]);
            quad.push_back(cut[j]);
            quad.push_back(startVertexIndex + cut[j]);
            quad.push_back(startVertexIndex + cut[i]);
            m_generatedFaces.push_back(quad);
        }
    }
}

void StrokeMeshBuilder::applyDeform()
{
    for (size_t i = 0; i < m_generatedVertices.size(); ++i) {
        auto &position = m_generatedVertices[i];
        const auto &node = m_nodes[m_generatedVerticesSourceNodeIndices[i]];
        const auto &cutDirect = m_generatedVerticesCutDirects[i];
        auto ray = position - node.position;
        if (nullptr != m_deformMapImage) {
            float degrees = angleInRangle360BetweenTwoVectors(node.baseNormal, ray.normalized(), node.traverseDirection);
            int x = node.reversedTraverseOrder * m_deformMapImage->width() / m_nodes.size();
            int y = degrees * m_deformMapImage->height() / 360.0;
            if (y >= m_deformMapImage->height())
                y = m_deformMapImage->height() - 1;
            float gray = (float)(qGray(m_deformMapImage->pixelColor(x, y).rgb()) - 127) / 127;
            position += m_deformMapScale * gray * ray;
            ray = position - node.position;
        }
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

const QVector3D &StrokeMeshBuilder::nodeTraverseDirection(size_t nodeIndex) const
{
    return m_nodes[nodeIndex].traverseDirection;
}

const QVector3D &StrokeMeshBuilder::nodeBaseNormal(size_t nodeIndex) const
{
    return m_nodes[nodeIndex].baseNormal;
}

size_t StrokeMeshBuilder::nodeTraverseOrder(size_t nodeIndex) const
{
    return m_nodes[nodeIndex].reversedTraverseOrder;
}

float radianToDegree(float r)
{
    return r * 180.0 / M_PI;
}


