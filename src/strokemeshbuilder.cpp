#include <QMatrix4x4>
#include <unordered_set>
#include <QDebug>
#include "strokemeshbuilder.h"
#include "meshstitcher.h"
#include "util.h"
#include "boxmesh.h"

size_t StrokeMeshBuilder::Node::nextOrNeighborOtherThan(size_t neighborIndex) const
{
    if (this->next != neighborIndex && this->next != this->index)
        return this->next;
    for (const auto &it: this->neighbors) {
        if (it != neighborIndex)
            return it;
    }
    return this->index;
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

void StrokeMeshBuilder::setNodeOriginInfo(size_t nodeIndex, int nearOriginNodeIndex, int farOriginNodeIndex)
{
    auto &node = m_nodes[nodeIndex];
    node.nearOriginNodeIndex = nearOriginNodeIndex;
    node.farOriginNodeIndex = farOriginNodeIndex;
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

const std::vector<StrokeMeshBuilder::Node> &StrokeMeshBuilder::nodes() const
{
    return m_nodes;
}

const std::vector<size_t> &StrokeMeshBuilder::nodeIndices() const
{
    return m_nodeIndices;
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
    return m_nodes[nodeIndex].traverseOrder;
}

size_t StrokeMeshBuilder::addNode(const QVector3D &position, float radius, const std::vector<QVector2D> &cutTemplate, float cutRotation)
{
    size_t nodeIndex = m_nodes.size();
    
    Node node;
    node.position = position;
    node.radius = radius;
    node.cutTemplate = cutTemplate;
    node.cutRotation = cutRotation;
    node.next = nodeIndex;
    node.index = nodeIndex;
    
    m_nodes.push_back(node);
    
    return nodeIndex;
}

void StrokeMeshBuilder::addEdge(size_t firstNodeIndex, 
    size_t secondNodeIndex)
{
    auto &fromNode = m_nodes[firstNodeIndex];
    fromNode.next = secondNodeIndex;
    fromNode.neighbors.push_back(secondNodeIndex);
    
    auto &toNode = m_nodes[secondNodeIndex];
    toNode.neighbors.push_back(firstNodeIndex);
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

std::vector<QVector3D> StrokeMeshBuilder::makeCut(const QVector3D &cutCenter, 
    float radius, 
    const std::vector<QVector2D> &cutTemplate, 
    const QVector3D &cutNormal,
    const QVector3D &baseNormal)
{
    std::vector<QVector3D> resultCut;
    QVector3D u = QVector3D::crossProduct(cutNormal, baseNormal).normalized();
    QVector3D v = QVector3D::crossProduct(u, cutNormal).normalized();
    auto uFactor = u * radius;
    auto vFactor = v * radius;
    for (const auto &t: cutTemplate) {
        resultCut.push_back(cutCenter + (uFactor * t.x() + vFactor * t.y()));
    }
    return resultCut;
}

void StrokeMeshBuilder::insertCutVertices(const std::vector<QVector3D> &cut,
    std::vector<size_t> *vertices,
    size_t nodeIndex,
    const QVector3D &cutNormal)
{
    size_t indexInCut = 0;
    for (const auto &position: cut) {
        size_t vertexIndex = m_generatedVertices.size();
        m_generatedVertices.push_back(position);
        m_generatedVerticesSourceNodeIndices.push_back(nodeIndex);
        m_generatedVerticesCutDirects.push_back(cutNormal);
        
        GeneratedVertexInfo info;
        info.orderInCut = indexInCut;
        info.cutSize = cut.size();
        m_generatedVerticesInfos.push_back(info);
        
        vertices->push_back(vertexIndex);
        
        ++indexInCut;
    }
}

std::vector<size_t> StrokeMeshBuilder::edgeloopFlipped(const std::vector<size_t> &edgeLoop)
{
    auto reversed = edgeLoop;
    std::reverse(reversed.begin(), reversed.end());
    std::rotate(reversed.rbegin(), reversed.rbegin() + 1, reversed.rend());
    return reversed;
}

void StrokeMeshBuilder::buildMesh()
{
    if (1 == m_nodes.size()) {
        const Node &node = m_nodes[0];
        int subdivideTimes = (int)(node.cutTemplate.size() / 4) - 1;
        if (subdivideTimes < 0)
            subdivideTimes = 0;
        boxmesh(node.position, node.radius, subdivideTimes, m_generatedVertices, m_generatedFaces);
        m_generatedVerticesSourceNodeIndices.resize(m_generatedVertices.size(), 0);
        m_generatedVerticesCutDirects.resize(m_generatedVertices.size(), node.traverseDirection);
        return;
    }
    
    std::vector<std::vector<size_t>> cuts;
    for (size_t i = 0; i < m_nodeIndices.size(); ++i) {
        auto &node = m_nodes[m_nodeIndices[i]];
        if (!qFuzzyIsNull(node.cutRotation)) {
            float degree = node.cutRotation * 180;
            QMatrix4x4 rotation;
            rotation.rotate(degree, node.traverseDirection);
            node.baseNormal = rotation * node.baseNormal;
        }
        auto cutVertices = makeCut(node.position, node.radius, node.cutTemplate,
            node.traverseDirection, node.baseNormal);
        std::vector<size_t> cut;
        insertCutVertices(cutVertices, &cut, node.index, node.traverseDirection);
        cuts.push_back(cut);
    }
    
    // Stich cuts
    for (size_t i = m_isRing ? 0 : 1; i < m_nodeIndices.size(); ++i) {
        size_t h = (i + m_nodeIndices.size() - 1) % m_nodeIndices.size();
        const auto &nodeH = m_nodes[m_nodeIndices[h]];
        const auto &nodeI = m_nodes[m_nodeIndices[i]];
        const auto &cutH = cuts[h];
        auto reversedCutI = edgeloopFlipped(cuts[i]);
        std::vector<std::pair<std::vector<size_t>, QVector3D>> edgeLoops = {
            {cutH, -nodeH.traverseDirection},
            {reversedCutI, nodeI.traverseDirection},
        };
        MeshStitcher stitcher;
        stitcher.setVertices(&m_generatedVertices);
        stitcher.stitch(edgeLoops);
        for (const auto &face: stitcher.newlyGeneratedFaces()) {
            m_generatedFaces.push_back(face);
        }
    }
    
    // Fill endpoints
    if (!m_isRing) {
        if (cuts.size() < 2)
            return;
        if (!qFuzzyIsNull(m_hollowThickness)) {
            // Generate mesh for hollow
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
            
            std::vector<std::vector<size_t>> revisedCuts = {cuts[0],
                edgeloopFlipped(cuts[cuts.size() - 1])};
            for (const auto &cut: revisedCuts) {
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
        } else {
            m_generatedFaces.push_back(cuts[0]);
            m_generatedFaces.push_back(edgeloopFlipped(cuts[cuts.size() - 1]));
        }
    }
}

void StrokeMeshBuilder::reviseTraverseDirections()
{
    std::vector<std::pair<size_t, QVector3D>> revised;
    for (size_t i = 0; i < m_nodeIndices.size(); ++i) {
        const auto &node = m_nodes[m_nodeIndices[i]];
        if (-1 != node.nearOriginNodeIndex && -1 != node.farOriginNodeIndex) {
            const auto &nearOriginNode = m_nodes[node.nearOriginNodeIndex];
            const auto &farOriginNode = m_nodes[node.farOriginNodeIndex];
            float nearDistance = node.position.distanceToPoint(nearOriginNode.position);
            float farDistance = node.position.distanceToPoint(farOriginNode.position);
            float totalDistance = nearDistance + farDistance;
            float distanceFactor = nearDistance / totalDistance;
            const QVector3D *revisedNearCutNormal = nullptr;
            const QVector3D *revisedFarCutNormal = nullptr;
            if (distanceFactor <= 0.5) {
                revisedNearCutNormal = &nearOriginNode.traverseDirection;
                revisedFarCutNormal = &node.traverseDirection;
            } else {
                distanceFactor = (1.0 - distanceFactor);
                revisedNearCutNormal = &farOriginNode.traverseDirection;
                revisedFarCutNormal = &node.traverseDirection;
            }
            distanceFactor *= 1.75;
            QVector3D newTraverseDirection;
            if (QVector3D::dotProduct(*revisedNearCutNormal, *revisedFarCutNormal) <= 0)
                newTraverseDirection = (*revisedNearCutNormal * (1.0 - distanceFactor) - *revisedFarCutNormal * distanceFactor).normalized();
            else
                newTraverseDirection = (*revisedNearCutNormal * (1.0 - distanceFactor) + *revisedFarCutNormal * distanceFactor).normalized();
            if (QVector3D::dotProduct(newTraverseDirection, node.traverseDirection) <= 0)
                newTraverseDirection = -newTraverseDirection;
            revised.push_back({node.index, newTraverseDirection});
        }
    }
    for (const auto &it: revised)
        m_nodes[it.first].traverseDirection = it.second;
}

void StrokeMeshBuilder::localAverageBaseNormals()
{
    std::vector<std::pair<size_t, QVector3D>> averaged;
    for (size_t i = 0; i < m_nodeIndices.size(); ++i) {
        size_t h, j;
        if (m_isRing) {
            h = (i + m_nodeIndices.size() - 1) % m_nodeIndices.size();
            j = (j + 1) % m_nodeIndices.size();
        } else {
            h = i > 0 ? i - 1 : i;
            j = i + 1 < m_nodeIndices.size() ? i + 1 : i;
        }
        const auto &nodeH = m_nodes[m_nodeIndices[h]];
        const auto &nodeI = m_nodes[m_nodeIndices[i]];
        const auto &nodeJ = m_nodes[m_nodeIndices[j]];
        averaged.push_back({
            nodeI.index,
            (nodeH.baseNormal + nodeI.baseNormal + nodeJ.baseNormal).normalized()
        });
    }
    for (const auto &it: averaged)
        m_nodes[it.first].baseNormal = it.second;
}

void StrokeMeshBuilder::unifyBaseNormals()
{
    for (size_t i = 1; i < m_nodeIndices.size(); ++i) {
        size_t h = m_nodeIndices[i - 1];
        const auto &nodeH = m_nodes[m_nodeIndices[h]];
        auto &nodeI = m_nodes[m_nodeIndices[i]];
        if (QVector3D::dotProduct(nodeI.baseNormal, nodeH.baseNormal) < 0)
            nodeI.baseNormal = -nodeI.baseNormal;
    }
}

void StrokeMeshBuilder::reviseNodeBaseNormal(Node &node)
{
    QVector3D orientedBaseNormal = QVector3D::dotProduct(node.traverseDirection, node.baseNormal) >= 0 ?
        node.baseNormal : -node.baseNormal;
    if (orientedBaseNormal.isNull()) {
        orientedBaseNormal = calculateBaseNormalFromTraverseDirection(node.traverseDirection);
    }
    node.baseNormal = orientedBaseNormal;
}

bool StrokeMeshBuilder::prepare()
{
    if (m_nodes.empty())
        return false;
    
    if (1 == m_nodes.size()) {
        auto &node = m_nodes[0];
        node.traverseOrder = 0;
        node.traverseDirection = QVector3D(0, 1, 0);
        node.baseNormal = QVector3D(0, 0, 1);
        return true;
    }
    
    m_nodeIndices = sortedNodeIndices(&m_isRing);
    
    if (m_nodeIndices.empty())
        return false;
    
    std::vector<QVector3D> edgeDirections;
    for (size_t i = 0; i < m_nodeIndices.size(); ++i) {
        m_nodes[m_nodeIndices[i]].traverseOrder = i;
        size_t j;
        if (m_isRing) {
            j = (i + 1) % m_nodeIndices.size();
        } else {
            j = i + 1 < m_nodeIndices.size() ? i + 1 : i;
        }
        edgeDirections.push_back((m_nodes[m_nodeIndices[j]].position - m_nodes[m_nodeIndices[i]].position).normalized());
    }
    
    for (size_t i = 0; i < m_nodeIndices.size(); ++i) {
        size_t h;
        if (m_isRing) {
            h = (i + m_nodeIndices.size() - 1) % m_nodeIndices.size();
        } else {
            h = i > 0 ? i - 1 : i;
        }
        m_nodes[m_nodeIndices[i]].traverseDirection = (edgeDirections[h] + edgeDirections[i]).normalized();
    }
    reviseTraverseDirections();
    
    // Base plane constraints
    if (!m_baseNormalOnX || !m_baseNormalOnY || !m_baseNormalOnZ) {
        for (auto &it: edgeDirections) {
            if (!m_baseNormalOnX)
                it.setX(0);
            if (!m_baseNormalOnY)
                it.setY(0);
            if (!m_baseNormalOnZ)
                it.setZ(0);
        }
    }
    std::vector<size_t> validBaseNormalPosArray;
    for (size_t i = m_isRing ? 0 : 1; i < m_nodeIndices.size(); ++i) {
        size_t h = (i + m_nodeIndices.size() - 1) % m_nodeIndices.size();
        // >15 degrees && < 165 degrees
        if (abs(QVector3D::dotProduct(edgeDirections[h], edgeDirections[i])) < 0.966) {
            auto baseNormal = QVector3D::crossProduct(edgeDirections[h], edgeDirections[i]);
            if (!baseNormal.isNull()) {
                if (!validBaseNormalPosArray.empty()) {
                    const auto &lastNode = m_nodes[m_nodeIndices[validBaseNormalPosArray[validBaseNormalPosArray.size() - 1]]];
                    if (QVector3D::dotProduct(lastNode.baseNormal, baseNormal) < 0)
                        baseNormal = -baseNormal;
                }
                auto &node = m_nodes[m_nodeIndices[i]];
                node.baseNormal = baseNormal;
                validBaseNormalPosArray.push_back(i);
                continue;
            }
        }
    }
    if (validBaseNormalPosArray.empty()) {
        QVector3D baseNormal;
        for (size_t i = 0; i < m_nodeIndices.size(); ++i) {
            const auto &node = m_nodes[m_nodeIndices[i]];
            baseNormal += calculateBaseNormalFromTraverseDirection(node.traverseDirection);
        }
        baseNormal.normalize();
        for (size_t i = 0; i < m_nodeIndices.size(); ++i) {
            auto &node = m_nodes[m_nodeIndices[i]];
            node.baseNormal = baseNormal;
        }
    } else if (1 == validBaseNormalPosArray.size()) {
        auto baseNormal = m_nodes[m_nodeIndices[validBaseNormalPosArray[0]]].baseNormal;
        for (size_t i = 0; i < m_nodeIndices.size(); ++i) {
            auto &node = m_nodes[m_nodeIndices[i]];
            node.baseNormal = baseNormal;
        }
    } else {
        if (!m_isRing) {
            auto prePos = validBaseNormalPosArray[0];
            const auto &preNode = m_nodes[m_nodeIndices[prePos]];
            auto preBaseNormal = preNode.baseNormal;
            auto afterPos = validBaseNormalPosArray[validBaseNormalPosArray.size() - 1];
            const auto &afterNode = m_nodes[m_nodeIndices[afterPos]];
            auto afterBaseNormal = afterNode.baseNormal;
            for (size_t i = 0; i < prePos; ++i) {
                auto &node = m_nodes[m_nodeIndices[i]];
                node.baseNormal = preBaseNormal;
            }
            for (size_t i = afterPos + 1; i < m_nodeIndices.size(); ++i) {
                auto &node = m_nodes[m_nodeIndices[i]];
                node.baseNormal = afterBaseNormal;
            }
        }
        auto updateInBetweenBaseNormal = [this](const Node &nodeU, const Node &nodeV, Node &updateNode) {
            float distanceU = updateNode.position.distanceToPoint(nodeU.position);
            float distanceV = updateNode.position.distanceToPoint(nodeV.position);
            float totalDistance = distanceU + distanceV;
            float factorU = 1.0 - distanceU / totalDistance;
            float factorV = 1.0 - factorU;
            auto baseNormal = (nodeU.baseNormal * factorU + nodeV.baseNormal * factorV).normalized();
            updateNode.baseNormal = baseNormal;
        };
        for (size_t k = m_isRing ? 0 : 1; k < validBaseNormalPosArray.size(); ++k) {
            size_t u = validBaseNormalPosArray[(k + validBaseNormalPosArray.size() - 1) % validBaseNormalPosArray.size()];
            size_t v = validBaseNormalPosArray[k];
            const auto &nodeU = m_nodes[m_nodeIndices[u]];
            const auto &nodeV = m_nodes[m_nodeIndices[v]];
            for (size_t i = (u + 1) % m_nodeIndices.size(); 
                    i != v; 
                    i = (i + 1) % m_nodeIndices.size()) {
                auto &node = m_nodes[m_nodeIndices[i]];
                updateInBetweenBaseNormal(nodeU, nodeV, node);
            }
        }
        if (m_baseNormalAverageEnabled) {
            QVector3D baseNormal;
            for (size_t i = 0; i < m_nodeIndices.size(); ++i) {
                const auto &node = m_nodes[m_nodeIndices[i]];
                baseNormal += node.baseNormal;
            }
            baseNormal.normalize();
            for (size_t i = 0; i < m_nodeIndices.size(); ++i) {
                auto &node = m_nodes[m_nodeIndices[i]];
                node.baseNormal = baseNormal;
            }
        } else {
            unifyBaseNormals();
            localAverageBaseNormals();
            for (size_t i = 0; i < m_nodeIndices.size(); ++i) {
                reviseNodeBaseNormal(m_nodes[m_nodeIndices[i]]);
            }
            unifyBaseNormals();
        }
    }

    return true;
}

std::vector<size_t> StrokeMeshBuilder::sortedNodeIndices(bool *isRing)
{
    std::vector<size_t> nodeIndices;
    
    size_t startingNodeIndex = 0;
    if (!calculateStartingNodeIndex(&startingNodeIndex, isRing))
        return nodeIndices;
    
    size_t fromNodeIndex = startingNodeIndex;
    std::unordered_set<size_t> visited;
    auto nodeIndex = fromNodeIndex;
    while (true) {
        if (visited.find(nodeIndex) != visited.end())
            break;
        visited.insert(nodeIndex);
        nodeIndices.push_back(nodeIndex);
        const auto &node = m_nodes[nodeIndex];
        size_t neighborIndex = node.nextOrNeighborOtherThan(fromNodeIndex);
        if (neighborIndex == nodeIndex)
            break;
        fromNodeIndex = nodeIndex;
        nodeIndex = neighborIndex;
    };
    
    return nodeIndices;
}

bool StrokeMeshBuilder::calculateStartingNodeIndex(size_t *startingNodeIndex, 
    bool *isRing)
{
    if (m_nodes.empty())
        return false;
    
    if (1 == m_nodes.size()) {
        *startingNodeIndex = 0;
        *isRing = false;
        return true;
    }
    
    auto findNearestNodeWithWorldCenter = [&](const std::vector<size_t> &nodeIndices) {
        std::vector<std::pair<size_t, float>> dist2Array(m_nodes.size());
        for (const auto &i: nodeIndices) {
            dist2Array.push_back({i, (float)m_nodes[i].position.lengthSquared()});
        }
        return std::min_element(dist2Array.begin(), dist2Array.end(), 
            [](const std::pair<size_t, float> &first,
                const std::pair<size_t, float> &second) {
            return first.second < second.second;
        })->first;
    };
    
    auto findEndpointNodeIndices = [&]() {
        std::vector<size_t> endpointIndices;
        for (const auto &it: m_nodes) {
            if (1 == it.neighbors.size())
                endpointIndices.push_back(it.index);
        }
        return endpointIndices;
    };
    
    auto endpointIndices = findEndpointNodeIndices();
    if (2 != endpointIndices.size()) {
        // Invalid endpoint count, there must be a ring, choose the node which is nearest with world center
        std::vector<size_t> nodeIndices(m_nodes.size());
        for (size_t i = 0; i < m_nodes.size(); ++i) {
            if (2 != m_nodes[i].neighbors.size())
                return false;
            nodeIndices[i] = i;
        }
        *startingNodeIndex = findNearestNodeWithWorldCenter(nodeIndices);
        *isRing = true;
        return true;
    }
    
    auto countAlignedDirections = [&](size_t nodeIndex) {
        size_t alignedCount = 0;
        size_t fromNodeIndex = nodeIndex;
        std::unordered_set<size_t> visited;
        while (true) {
            if (visited.find(nodeIndex) != visited.end())
                break;
            visited.insert(nodeIndex);
            const auto &node = m_nodes[nodeIndex];
            size_t neighborIndex = node.nextOrNeighborOtherThan(fromNodeIndex);
            if (neighborIndex == nodeIndex)
                break;
            if (node.next == neighborIndex)
                ++alignedCount;
            fromNodeIndex = nodeIndex;
            nodeIndex = neighborIndex;
        };
        return alignedCount;
    };
    
    auto chooseStartingEndpointByAlignedDirections = [&](const std::vector<size_t> &endpointIndices) {
        std::vector<std::pair<size_t, size_t>> alignedDirections(endpointIndices.size());
        for (size_t i = 0; i < endpointIndices.size(); ++i) {
            auto nodeIndex = endpointIndices[i];
            alignedDirections[i] = {nodeIndex, countAlignedDirections(nodeIndex)};
        }
        std::sort(alignedDirections.begin(), alignedDirections.end(), [](const std::pair<size_t, size_t> &first,
                const std::pair<size_t, size_t> &second) {
            return first.second > second.second;
        });
        if (alignedDirections[0].second > alignedDirections[1].second)
            return alignedDirections[0].first;
        std::vector<size_t> nodeIndices = {alignedDirections[0].first, alignedDirections[1].first};
        return findNearestNodeWithWorldCenter(nodeIndices);
    };

    *startingNodeIndex = chooseStartingEndpointByAlignedDirections(endpointIndices);
    *isRing = false;
    return true;
}

QVector3D StrokeMeshBuilder::calculateDeformPosition(const QVector3D &vertexPosition, const QVector3D &ray, const QVector3D &deformNormal, float deformFactor)
{
    QVector3D revisedNormal = QVector3D::dotProduct(ray, deformNormal) < 0.0 ? -deformNormal : deformNormal;
    QVector3D projectRayOnRevisedNormal = revisedNormal * (QVector3D::dotProduct(ray, revisedNormal) / revisedNormal.lengthSquared());
    auto scaledProjct = projectRayOnRevisedNormal * deformFactor;
    return vertexPosition + (scaledProjct - projectRayOnRevisedNormal);
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
            int x = (int)node.traverseOrder * m_deformMapImage->width() / m_nodes.size();
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

bool StrokeMeshBuilder::buildBaseNormalsOnly()
{
    return prepare();
}

bool StrokeMeshBuilder::build()
{
    if (!prepare())
        return false;
    
    buildMesh();
    applyDeform();
    return true;
}
