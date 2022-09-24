/*
 *  Copyright (c) 2016-2021 Jeremy HU <jeremy-at-dust3d dot org>. All rights reserved. 
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:

 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.

 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#include <unordered_set>
#include <algorithm>
#include <dust3d/mesh/stroke_mesh_builder.h>
#include <dust3d/mesh/triangulate.h>
#include <dust3d/mesh/box_mesh.h>
#include <dust3d/mesh/hole_stitcher.h>
#include <dust3d/mesh/isotropic_remesher.h>

namespace dust3d
{

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
    m_deformThickness = std::max((float)0.01, thickness);
}

void StrokeMeshBuilder::setDeformWidth(float width)
{
    m_deformWidth = std::max((float)0.01, width);
}

void StrokeMeshBuilder::setDeformUnified(bool unified)
{
    m_deformUnified = unified;
}

void StrokeMeshBuilder::setHollowThickness(float hollowThickness)
{
    m_hollowThickness = hollowThickness;
}

void StrokeMeshBuilder::setNodeOriginInfo(size_t nodeIndex, int nearOriginNodeIndex, int farOriginNodeIndex, int sourceNodeIndex)
{
    auto &node = m_nodes[nodeIndex];
    node.nearOriginNodeIndex = nearOriginNodeIndex;
    node.farOriginNodeIndex = farOriginNodeIndex;
    node.sourceNodeIndex = sourceNodeIndex;
}

const std::vector<Vector3> &StrokeMeshBuilder::generatedVertices()
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

const Vector3 &StrokeMeshBuilder::nodeTraverseDirection(size_t nodeIndex) const
{
    return m_nodes[nodeIndex].traverseDirection;
}

const Vector3 &StrokeMeshBuilder::nodeBaseNormal(size_t nodeIndex) const
{
    return m_nodes[nodeIndex].baseNormal;
}

size_t StrokeMeshBuilder::nodeTraverseOrder(size_t nodeIndex) const
{
    return m_nodes[nodeIndex].traverseOrder;
}

size_t StrokeMeshBuilder::addNode(const Vector3 &position, float radius, const std::vector<Vector2> &cutTemplate, float cutRotation)
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

Vector3 StrokeMeshBuilder::calculateBaseNormalFromTraverseDirection(const Vector3 &traverseDirection)
{
    const std::vector<Vector3> axisList = {
        Vector3 {1, 0, 0},
        Vector3 {0, 1, 0},
        Vector3 {0, 0, 1},
    };
    float maxDot = -1;
    size_t nearAxisIndex = 0;
    bool reversed = false;
    for (size_t i = 0; i < axisList.size(); ++i) {
        const auto axis = axisList[i];
        auto dot = Vector3::dotProduct(axis, traverseDirection);
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
    auto baseNormal = Vector3::crossProduct(traverseDirection, choosenAxis).normalized();
    return reversed ? -baseNormal : baseNormal;
}

std::vector<Vector3> StrokeMeshBuilder::makeCut(const Vector3 &cutCenter, 
    float radius, 
    const std::vector<Vector2> &cutTemplate, 
    const Vector3 &cutNormal,
    const Vector3 &baseNormal)
{
    std::vector<Vector3> resultCut;
    Vector3 u = Vector3::crossProduct(cutNormal, baseNormal).normalized();
    Vector3 v = Vector3::crossProduct(u, cutNormal).normalized();
    auto uFactor = u * radius;
    auto vFactor = v * radius;
    for (const auto &t: cutTemplate) {
        resultCut.push_back(cutCenter + (uFactor * t.x() + vFactor * t.y()));
    }
    return resultCut;
}

void StrokeMeshBuilder::insertCutVertices(const std::vector<Vector3> &cut,
    std::vector<size_t> *vertices,
    size_t nodeIndex,
    const Vector3 &cutNormal)
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
        buildBoxMesh(node.position, node.radius, subdivideTimes, m_generatedVertices, m_generatedFaces);
        m_generatedVerticesSourceNodeIndices.resize(m_generatedVertices.size(), 0);
        m_generatedVerticesCutDirects.resize(m_generatedVertices.size(), node.traverseDirection);
        return;
    }
    
    for (size_t i = 0; i < m_nodeIndices.size(); ++i) {
        auto &node = m_nodes[m_nodeIndices[i]];
        if (!Math::isZero(node.cutRotation)) {
            float degree = node.cutRotation * 180;
            node.baseNormal = node.baseNormal.rotated(node.traverseDirection, Math::radiansFromDegrees(degree));
        }
        auto cutVertices = makeCut(node.position, node.radius, node.cutTemplate,
            node.traverseDirection, node.baseNormal);
        std::vector<size_t> cut;
        insertCutVertices(cutVertices, &cut, node.index, node.traverseDirection);
        m_cuts.push_back(cut);
    }
}

void StrokeMeshBuilder::interpolateCutEdges()
{
    if (m_cuts.empty())
        return;
    
    size_t bigCutIndex = 0;
    double maxRadius = 0.0;
    for (size_t i = 0; i < m_cuts.size(); ++i) {
        size_t j = (i + 1) % m_cuts.size();
        if (m_cuts[i].size() != m_cuts[j].size())
            return;
        const auto &nodeIndex = m_generatedVerticesSourceNodeIndices[m_cuts[i][0]];
        if (m_nodes[nodeIndex].radius > maxRadius) {
            maxRadius = m_nodes[nodeIndex].radius;
            bigCutIndex = i;
        }
    }
    
    const auto &cut = m_cuts[bigCutIndex];
    double sumOfLegnth = 0;
    std::vector<double> edgeLengths;
    edgeLengths.reserve(cut.size());
    for (size_t i = 0; i < cut.size(); ++i) {
        size_t j = (i + 1) % cut.size();
        double length = (m_generatedVertices[cut[i]] - m_generatedVertices[cut[j]]).length();
        edgeLengths.push_back(length);
        sumOfLegnth += length;
    }
    double targetLength = 1.2 * sumOfLegnth / cut.size();
    std::vector<std::vector<size_t>> newCuts(m_cuts.size());
    for (size_t index = 0; index < cut.size(); ++index) {
        size_t nextIndex = (index + 1) % cut.size();
        for (size_t cutIndex = 0; cutIndex < m_cuts.size(); ++cutIndex) {
            newCuts[cutIndex].push_back(m_cuts[cutIndex][index]);
        }
        const auto &oldEdgeLength = edgeLengths[index];
        if (targetLength >= oldEdgeLength)
            continue;
        size_t newInsertNum = oldEdgeLength / targetLength;
        if (newInsertNum < 1)
            newInsertNum = 1;
        if (newInsertNum > 100)
            continue;
        float stepFactor = 1.0 / (newInsertNum + 1);
        float factor = stepFactor;
        for (size_t i = 0; i < newInsertNum && factor < 1.0; factor += stepFactor, ++i) {
            float firstFactor = 1.0 - factor;
            for (size_t cutIndex = 0; cutIndex < m_cuts.size(); ++cutIndex) {
                auto newPosition = m_generatedVertices[m_cuts[cutIndex][index]] * firstFactor + m_generatedVertices[m_cuts[cutIndex][nextIndex]] * factor;
                newCuts[cutIndex].push_back(m_generatedVertices.size());
                m_generatedVertices.push_back(newPosition);
                const auto &oldIndex = m_cuts[cutIndex][index];
                m_generatedVerticesCutDirects.push_back(m_generatedVerticesCutDirects[oldIndex]);
                m_generatedVerticesSourceNodeIndices.push_back(m_generatedVerticesSourceNodeIndices[oldIndex]);
                m_generatedVerticesInfos.push_back(m_generatedVerticesInfos[oldIndex]);
            }
        }
    }

    m_cuts = newCuts;
}

void StrokeMeshBuilder::stitchCuts()
{
    for (size_t i = m_isRing ? 0 : 1; i < m_nodeIndices.size(); ++i) {
        size_t h = (i + m_nodeIndices.size() - 1) % m_nodeIndices.size();
        const auto &nodeH = m_nodes[m_nodeIndices[h]];
        const auto &nodeI = m_nodes[m_nodeIndices[i]];
        const auto &cutH = m_cuts[h];
        auto reversedCutI = edgeloopFlipped(m_cuts[i]);
        std::vector<std::pair<std::vector<size_t>, Vector3>> edgeLoops = {
            {cutH, -nodeH.traverseDirection},
            {reversedCutI, nodeI.traverseDirection},
        };
        HoleStitcher stitcher;
        stitcher.setVertices(&m_generatedVertices);
        stitcher.stitch(edgeLoops);
        for (const auto &face: stitcher.newlyGeneratedFaces()) {
            m_generatedFaces.push_back(face);
        }
    }
    
    // Fill endpoints
    if (!m_isRing) {
        if (m_cuts.size() < 2)
            return;
        if (!Math::isZero(m_hollowThickness)) {
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
            
            std::vector<std::vector<size_t>> revisedCuts = {m_cuts[0],
                edgeloopFlipped(m_cuts[m_cuts.size() - 1])};
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
            if (m_cuts[0].size() <= 4) {
                m_generatedFaces.push_back(m_cuts[0]);
                m_generatedFaces.push_back(edgeloopFlipped(m_cuts[m_cuts.size() - 1]));
            } else {
                auto remeshAndAddCut = [&](const std::vector<size_t> &inputFace) {
                    std::vector<std::vector<size_t>> remeshedFaces;
                    size_t oldVertexCount = m_generatedVertices.size();
                    isotropicTriangulate(m_generatedVertices, inputFace, &remeshedFaces);
                    size_t oldIndex = inputFace[0];
                    for (size_t i = oldVertexCount; i < m_generatedVertices.size(); ++i) {
                        m_generatedVerticesCutDirects.push_back(m_generatedVerticesCutDirects[oldIndex]);
                        m_generatedVerticesSourceNodeIndices.push_back(m_generatedVerticesSourceNodeIndices[oldIndex]);
                        m_generatedVerticesInfos.push_back(m_generatedVerticesInfos[oldIndex]);
                    }
                    for (const auto &it: remeshedFaces)
                        m_generatedFaces.push_back(it);
                };
                remeshAndAddCut(m_cuts[0]);
                remeshAndAddCut(edgeloopFlipped(m_cuts[m_cuts.size() - 1]));
            }
        }
    }
}

void StrokeMeshBuilder::reviseTraverseDirections()
{
    std::vector<std::pair<size_t, Vector3>> revised;
    for (size_t i = 0; i < m_nodeIndices.size(); ++i) {
        const auto &node = m_nodes[m_nodeIndices[i]];
        if (-1 != node.nearOriginNodeIndex && -1 != node.farOriginNodeIndex) {
            const auto &nearOriginNode = m_nodes[node.nearOriginNodeIndex];
            const auto &farOriginNode = m_nodes[node.farOriginNodeIndex];
            float nearDistance = (node.position - nearOriginNode.position).length();
            float farDistance = (node.position - farOriginNode.position).length();
            float totalDistance = nearDistance + farDistance;
            float distanceFactor = nearDistance / totalDistance;
            const Vector3 *revisedNearCutNormal = nullptr;
            const Vector3 *revisedFarCutNormal = nullptr;
            if (distanceFactor <= 0.5) {
                revisedNearCutNormal = &nearOriginNode.traverseDirection;
                revisedFarCutNormal = &node.traverseDirection;
            } else {
                distanceFactor = (1.0 - distanceFactor);
                revisedNearCutNormal = &farOriginNode.traverseDirection;
                revisedFarCutNormal = &node.traverseDirection;
            }
            distanceFactor *= 1.75;
            Vector3 newTraverseDirection;
            if (Vector3::dotProduct(*revisedNearCutNormal, *revisedFarCutNormal) <= 0)
                newTraverseDirection = (*revisedNearCutNormal * (1.0 - distanceFactor) - *revisedFarCutNormal * distanceFactor).normalized();
            else
                newTraverseDirection = (*revisedNearCutNormal * (1.0 - distanceFactor) + *revisedFarCutNormal * distanceFactor).normalized();
            if (Vector3::dotProduct(newTraverseDirection, node.traverseDirection) <= 0)
                newTraverseDirection = -newTraverseDirection;
            revised.push_back({node.index, newTraverseDirection});
        }
    }
    for (const auto &it: revised)
        m_nodes[it.first].traverseDirection = it.second;
}

void StrokeMeshBuilder::localAverageBaseNormals()
{
    std::vector<std::pair<size_t, Vector3>> averaged;
    for (size_t i = 0; i < m_nodeIndices.size(); ++i) {
        size_t h, j;
        if (m_isRing) {
            h = (i + m_nodeIndices.size() - 1) % m_nodeIndices.size();
            j = (i + 1) % m_nodeIndices.size();
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
        if (Vector3::dotProduct(nodeI.baseNormal, nodeH.baseNormal) < 0)
            nodeI.baseNormal = -nodeI.baseNormal;
    }
}

void StrokeMeshBuilder::reviseNodeBaseNormal(Node &node)
{
    Vector3 orientedBaseNormal = Vector3::dotProduct(node.traverseDirection, node.baseNormal) >= 0 ?
        node.baseNormal : -node.baseNormal;
    if (orientedBaseNormal.isZero()) {
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
        node.traverseDirection = Vector3(0, 1, 0);
        node.baseNormal = Vector3(0, 0, 1);
        return true;
    }
    
    m_nodeIndices = sortedNodeIndices(&m_isRing);
    
    if (m_nodeIndices.empty())
        return false;

    std::vector<Vector3> edgeDirections;
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
        if (abs(Vector3::dotProduct(edgeDirections[h], edgeDirections[i])) < 0.966) {
            auto baseNormal = Vector3::crossProduct(edgeDirections[h], edgeDirections[i]);
            if (!baseNormal.isZero()) {
                if (!validBaseNormalPosArray.empty()) {
                    const auto &lastNode = m_nodes[m_nodeIndices[validBaseNormalPosArray[validBaseNormalPosArray.size() - 1]]];
                    if (Vector3::dotProduct(lastNode.baseNormal, baseNormal) < 0)
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
        Vector3 baseNormal;
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
            float distanceU = (updateNode.position - nodeU.position).length();
            float distanceV = (updateNode.position - nodeV.position).length();
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
            Vector3 baseNormal;
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
        std::vector<std::pair<size_t, float>> dist2Array;
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

Vector3 StrokeMeshBuilder::calculateDeformPosition(const Vector3 &vertexPosition, const Vector3 &ray, const Vector3 &deformNormal, float deformFactor)
{
    Vector3 revisedNormal = Vector3::dotProduct(ray, deformNormal) < 0.0 ? -deformNormal : deformNormal;
    Vector3 projectRayOnRevisedNormal = revisedNormal * (Vector3::dotProduct(ray, revisedNormal) / revisedNormal.lengthSquared());
    auto scaledProjct = projectRayOnRevisedNormal * deformFactor;
    return vertexPosition + (scaledProjct - projectRayOnRevisedNormal);
}

void StrokeMeshBuilder::applyDeform()
{
    float maxRadius = 0.0;
    if (m_deformUnified) {
        for (const auto &node: m_nodes) {
            if (node.radius > maxRadius)
                maxRadius = node.radius;
        }
    }
    for (size_t i = 0; i < m_generatedVertices.size(); ++i) {
        auto &position = m_generatedVertices[i];
        const auto &node = m_nodes[m_generatedVerticesSourceNodeIndices[i]];
        const auto &cutDirect = m_generatedVerticesCutDirects[i];
        auto ray = position - node.position;
        Vector3 sum;
        size_t count = 0;
        float deformUnifyFactor = m_deformUnified ? maxRadius / node.radius : 1.0;
        if (!Math::isEqual(m_deformThickness, (float)1.0)) {
            auto deformedPosition = calculateDeformPosition(position, ray, node.baseNormal, m_deformThickness * deformUnifyFactor);
            sum += deformedPosition;
            ++count;
        }
        if (!Math::isEqual(m_deformWidth, (float)1.0)) {
            auto deformedPosition = calculateDeformPosition(position, ray, Vector3::crossProduct(node.baseNormal, cutDirect), m_deformWidth * deformUnifyFactor);
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
    interpolateCutEdges();
    stitchCuts();
    return true;
}

}
