#include <QVector2D>
#include <QDebug>
#include "strokemodifier.h"
#include "util.h"

void StrokeModifier::enableIntermediateAddition()
{
    m_intermediateAdditionEnabled = true;
}

size_t StrokeModifier::addNode(const QVector3D &position, float radius, const std::vector<QVector2D> &cutTemplate, float cutRotation)
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

void StrokeModifier::createIntermediateNode(const Node &firstNode, const Node &secondNode, float factor, Node *resultNode)
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
    for (auto &node: m_nodes) {
        subdivideFace2D(&node.cutTemplate);
    }
}

float StrokeModifier::averageCutTemplateEdgeLength(const std::vector<QVector2D> &cutTemplate)
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
    for (const auto &edge: m_edges) {
        neighbors[edge.firstNodeIndex].push_back(edge.secondNodeIndex);
        neighbors[edge.secondNodeIndex].push_back(edge.firstNodeIndex);
    }
    for (const auto &it: neighbors) {
        if (1 == it.second.size()) {
            const Node &currentNode = m_nodes[it.first];
            const Node &neighborNode = m_nodes[it.second[0]];
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

void StrokeModifier::createIntermediateCutTemplateEdges(std::vector<QVector2D> &cutTemplate, float averageCutTemplateLength)
{
    std::vector<QVector2D> newCutTemplate;
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
    
    for (auto &node: m_nodes) {
        node.averageCutTemplateLength = averageCutTemplateEdgeLength(node.cutTemplate);
        createIntermediateCutTemplateEdges(node.cutTemplate, node.averageCutTemplateLength);
    }
    
    auto oldEdges = m_edges;
    m_edges.clear();
    for (const auto &edge: oldEdges) {
        const Node &firstNode = m_nodes[edge.firstNodeIndex];
        const Node &secondNode = m_nodes[edge.secondNodeIndex];
        //float edgeLengthThreshold = (firstNode.radius + secondNode.radius) * 0.75;
        auto firstAverageCutTemplateEdgeLength = firstNode.averageCutTemplateLength * firstNode.radius;
        auto secondAverageCutTemplateEdgeLength = secondNode.averageCutTemplateLength * secondNode.radius;
        float targetEdgeLength = (firstAverageCutTemplateEdgeLength + secondAverageCutTemplateEdgeLength) * 0.5;
        //if (targetEdgeLength < edgeLengthThreshold)
        //    targetEdgeLength = edgeLengthThreshold;
        float currentEdgeLength = (firstNode.position - secondNode.position).length();
        if (targetEdgeLength >= currentEdgeLength) {
            addEdge(edge.firstNodeIndex, edge.secondNodeIndex);
            continue;
        }
        size_t newInsertNum = currentEdgeLength / targetEdgeLength;
        if (newInsertNum < 1)
            newInsertNum = 1;
        if (newInsertNum > 100)
            continue;
        float stepFactor = 1.0 / (newInsertNum + 1);
        std::vector<size_t> nodeIndices;
        nodeIndices.push_back(edge.firstNodeIndex);
        float factor = stepFactor;
        for (size_t i = 0; i < newInsertNum && factor < 1.0; factor += stepFactor, ++i) {
            Node intermediateNode;
            const Node &firstNode = m_nodes[edge.firstNodeIndex];
            const Node &secondNode = m_nodes[edge.secondNodeIndex];
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
}

const std::vector<StrokeModifier::Node> &StrokeModifier::nodes()
{
    return m_nodes;
}

const std::vector<StrokeModifier::Edge> &StrokeModifier::edges()
{
    return m_edges;
}

