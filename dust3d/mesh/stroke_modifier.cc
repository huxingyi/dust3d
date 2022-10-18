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

#include <dust3d/mesh/centripetal_catmull_rom_spline.h>
#include <dust3d/mesh/stroke_modifier.h>
#include <map>
#include <unordered_map>

namespace dust3d {

void StrokeModifier::enableIntermediateAddition()
{
    m_intermediateAdditionEnabled = true;
}

void StrokeModifier::enableSmooth()
{
    m_smooth = true;
}

size_t StrokeModifier::addNode(const Vector3& position, float radius, const std::vector<Vector2>& cutTemplate, float cutRotation)
{
    size_t nodeIndex = m_nodes.size();

    Node node;
    node.isOriginal = true;
    node.position = position;
    node.radius = radius;
    node.cutTemplate = cutTemplate;
    node.cutRotation = cutRotation;
    node.originNodeIndex = nodeIndex;
    m_nodes.push_back(node);

    return nodeIndex;
}

size_t StrokeModifier::addEdge(size_t firstNodeIndex, size_t secondNodeIndex)
{
    size_t edgeIndex = m_edges.size();

    Edge edge;
    edge.firstNodeIndex = firstNodeIndex;
    edge.secondNodeIndex = secondNodeIndex;
    m_edges.push_back(edge);

    return edgeIndex;
}

void StrokeModifier::createIntermediateNode(const Node& firstNode, const Node& secondNode, float factor, Node* resultNode)
{
    float firstFactor = 1.0 - factor;
    resultNode->position = firstNode.position * firstFactor + secondNode.position * factor;
    resultNode->radius = firstNode.radius * firstFactor + secondNode.radius * factor;
    if (factor <= 0.5) {
        resultNode->originNodeIndex = firstNode.originNodeIndex;
        resultNode->nearOriginNodeIndex = firstNode.originNodeIndex;
        resultNode->farOriginNodeIndex = secondNode.originNodeIndex;
        resultNode->cutRotation = firstNode.cutRotation;
        resultNode->cutTemplate = firstNode.cutTemplate;
    } else {
        resultNode->originNodeIndex = secondNode.originNodeIndex;
        resultNode->nearOriginNodeIndex = secondNode.originNodeIndex;
        resultNode->farOriginNodeIndex = firstNode.originNodeIndex;
        resultNode->cutRotation = secondNode.cutRotation;
        resultNode->cutTemplate = secondNode.cutTemplate;
    }
}

void StrokeModifier::subdivide()
{
    for (auto& node : m_nodes) {
        subdivideFace(&node.cutTemplate);
    }
}

void StrokeModifier::subdivideFace(std::vector<Vector2>* face)
{
    auto oldFace = *face;
    face->resize(oldFace.size() * 2);
    for (size_t i = 0, n = 0; i < oldFace.size(); ++i) {
        size_t h = (i + oldFace.size() - 1) % oldFace.size();
        size_t j = (i + 1) % oldFace.size();
        (*face)[n++] = oldFace[h] * 0.125 + oldFace[i] * 0.75 + oldFace[j] * 0.125;
        (*face)[n++] = (oldFace[i] + oldFace[j]) * 0.5;
    }
}

float StrokeModifier::averageCutTemplateEdgeLength(const std::vector<Vector2>& cutTemplate)
{
    if (cutTemplate.empty())
        return 0;

    float sum = 0;
    for (size_t i = 0; i < cutTemplate.size(); ++i) {
        size_t j = (i + 1) % cutTemplate.size();
        sum += (cutTemplate[i] - cutTemplate[j]).length();
    }
    return sum / cutTemplate.size();
}

void StrokeModifier::roundEnd()
{
    std::map<size_t, std::vector<size_t>> neighbors;
    for (const auto& edge : m_edges) {
        neighbors[edge.firstNodeIndex].push_back(edge.secondNodeIndex);
        neighbors[edge.secondNodeIndex].push_back(edge.firstNodeIndex);
    }
    for (const auto& it : neighbors) {
        if (1 == it.second.size()) {
            const Node& currentNode = m_nodes[it.first];
            const Node& neighborNode = m_nodes[it.second[0]];
            Node endNode;
            endNode.radius = currentNode.radius * 0.5;
            endNode.position = currentNode.position + (currentNode.position - neighborNode.position).normalized() * endNode.radius;
            endNode.cutTemplate = currentNode.cutTemplate;
            endNode.cutRotation = currentNode.cutRotation;
            endNode.originNodeIndex = currentNode.originNodeIndex;
            size_t endNodeIndex = m_nodes.size();
            m_nodes.push_back(endNode);
            addEdge(endNode.originNodeIndex, endNodeIndex);
        }
    }
}

void StrokeModifier::createIntermediateCutTemplateEdges(std::vector<Vector2>& cutTemplate, float averageCutTemplateLength)
{
    std::vector<Vector2> newCutTemplate;
    auto pointCount = cutTemplate.size();
    float targetLength = averageCutTemplateLength * 1.2;
    for (size_t index = 0; index < pointCount; ++index) {
        size_t nextIndex = (index + 1) % pointCount;
        newCutTemplate.push_back(cutTemplate[index]);
        float oldEdgeLength = (cutTemplate[index] - cutTemplate[nextIndex]).length();
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
            newCutTemplate.push_back(cutTemplate[index] * firstFactor + cutTemplate[nextIndex] * factor);
        }
    }
    cutTemplate = newCutTemplate;
}

void StrokeModifier::finalize()
{
    if (!m_intermediateAdditionEnabled)
        return;

    for (auto& node : m_nodes) {
        node.averageCutTemplateLength = averageCutTemplateEdgeLength(node.cutTemplate);
        createIntermediateCutTemplateEdges(node.cutTemplate, node.averageCutTemplateLength);
    }

    auto oldEdges = m_edges;
    m_edges.clear();
    for (const auto& edge : oldEdges) {
        const Node& firstNode = m_nodes[edge.firstNodeIndex];
        const Node& secondNode = m_nodes[edge.secondNodeIndex];
        auto firstAverageCutTemplateEdgeLength = firstNode.averageCutTemplateLength * firstNode.radius;
        auto secondAverageCutTemplateEdgeLength = secondNode.averageCutTemplateLength * secondNode.radius;
        float targetEdgeLength = (firstAverageCutTemplateEdgeLength + secondAverageCutTemplateEdgeLength) * 0.5;
        float currentEdgeLength = (firstNode.position - secondNode.position).length();
        if (targetEdgeLength >= currentEdgeLength) {
            addEdge(edge.firstNodeIndex, edge.secondNodeIndex);
            continue;
        }
        size_t newInsertNum = currentEdgeLength / targetEdgeLength;
        if (newInsertNum < 1)
            newInsertNum = 1;
        if (newInsertNum > 100) {
            addEdge(edge.firstNodeIndex, edge.secondNodeIndex);
            continue;
        }
        float stepFactor = 1.0 / (newInsertNum + 1);
        std::vector<size_t> nodeIndices;
        nodeIndices.push_back(edge.firstNodeIndex);
        float factor = stepFactor;
        for (size_t i = 0; i < newInsertNum && factor < 1.0; factor += stepFactor, ++i) {
            Node intermediateNode;
            const Node& firstNode = m_nodes[edge.firstNodeIndex];
            const Node& secondNode = m_nodes[edge.secondNodeIndex];
            createIntermediateNode(firstNode, secondNode, factor, &intermediateNode);
            size_t intermedidateNodeIndex = m_nodes.size();
            nodeIndices.push_back(intermedidateNodeIndex);
            m_nodes.push_back(intermediateNode);
        }
        nodeIndices.push_back(edge.secondNodeIndex);
        for (size_t i = 1; i < nodeIndices.size(); ++i) {
            addEdge(nodeIndices[i - 1], nodeIndices[i]);
        }
    }

    if (m_smooth)
        smooth();
}

void StrokeModifier::smooth()
{
    std::unordered_map<int, std::vector<int>> neighborMap;
    for (const auto& edge : m_edges) {
        neighborMap[edge.firstNodeIndex].push_back(edge.secondNodeIndex);
        neighborMap[edge.secondNodeIndex].push_back(edge.firstNodeIndex);
    }

    int startEndpoint = 0;
    for (const auto& edge : m_edges) {
        auto findNeighbor = neighborMap.find(edge.firstNodeIndex);
        if (findNeighbor == neighborMap.end())
            continue;
        if (1 != findNeighbor->second.size()) {
            auto findNeighborNeighbor = neighborMap.find(edge.secondNodeIndex);
            if (findNeighborNeighbor == neighborMap.end())
                continue;
            if (1 != findNeighborNeighbor->second.size())
                continue;
            startEndpoint = edge.secondNodeIndex;
        } else {
            startEndpoint = edge.firstNodeIndex;
        }
        break;
    }
    if (-1 == startEndpoint)
        return;

    int loopIndex = startEndpoint;
    int previousIndex = -1;
    bool isRing = false;
    std::vector<int> loop;
    while (-1 != loopIndex) {
        loop.push_back(loopIndex);
        auto findNeighbor = neighborMap.find(loopIndex);
        if (findNeighbor == neighborMap.end())
            return;
        int nextIndex = -1;
        for (const auto& index : findNeighbor->second) {
            if (index == previousIndex)
                continue;
            if (index == startEndpoint) {
                isRing = true;
                break;
            }
            nextIndex = index;
            break;
        }
        previousIndex = loopIndex;
        loopIndex = nextIndex;
    }

    CentripetalCatmullRomSpline spline(isRing);
    for (size_t i = 0; i < loop.size(); ++i) {
        const auto& nodeIndex = loop[i];
        const auto& node = m_nodes[nodeIndex];
        bool isKnot = node.originNodeIndex == nodeIndex;
        spline.addPoint((int)nodeIndex, node.position, isKnot);
    }
    if (!spline.interpolate())
        return;
    for (const auto& it : spline.splineNodes()) {
        if (-1 == it.source)
            continue;
        auto& node = m_nodes[it.source];
        node.position = it.position;
    }
}

const std::vector<StrokeModifier::Node>& StrokeModifier::nodes() const
{
    return m_nodes;
}

const std::vector<StrokeModifier::Edge>& StrokeModifier::edges() const
{
    return m_edges;
}

}
