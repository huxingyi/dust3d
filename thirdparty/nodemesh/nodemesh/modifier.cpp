#include <nodemesh/modifier.h>
#include <nodemesh/misc.h>
#include <QVector2D>
#include <QDebug>

namespace nodemesh
{

size_t Modifier::addNode(const QVector3D &position, float radius, const std::vector<QVector2D> &cutTemplate)
{
    size_t nodeIndex = m_nodes.size();
    
    Node node;
    node.isOriginal = true;
    node.position = position;
    node.radius = radius;
    node.cutTemplate = cutTemplate;
    node.originNodeIndex = nodeIndex;
    m_nodes.push_back(node);
    
    return nodeIndex;
}

size_t Modifier::addEdge(size_t firstNodeIndex, size_t secondNodeIndex)
{
    size_t edgeIndex = m_edges.size();
    
    Edge edge;
    edge.firstNodeIndex = firstNodeIndex;
    edge.secondNodeIndex = secondNodeIndex;
    m_edges.push_back(edge);
    
    return edgeIndex;
}

void Modifier::createIntermediateNode(const Node &firstNode, const Node &secondNode, float factor, Node *resultNode)
{
    float firstFactor = 1.0 - factor;
    resultNode->position = firstNode.position * firstFactor + secondNode.position * factor;
    resultNode->radius = firstNode.radius * firstFactor + secondNode.radius * factor;
    resultNode->cutTemplate = firstNode.cutTemplate;
    for (size_t i = 0; i < secondNode.cutTemplate.size(); ++i) {
        if (i >= resultNode->cutTemplate.size())
            break;
        resultNode->cutTemplate[i] = resultNode->cutTemplate[i] * firstFactor + secondNode.cutTemplate[i] * factor;
    }
}

void Modifier::subdivide()
{
    for (auto &node: m_nodes) {
        subdivideFace2D(&node.cutTemplate);
    }
}

float Modifier::averageCutTemplateEdgeLength(const std::vector<QVector2D> &cutTemplate)
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

void Modifier::roundEnd()
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
            endNode.originNodeIndex = currentNode.originNodeIndex;
            size_t endNodeIndex = m_nodes.size();
            m_nodes.push_back(endNode);
            addEdge(endNode.originNodeIndex, endNodeIndex);
        }
    }
}

void Modifier::finalize()
{
    auto oldEdges = m_edges;
    m_edges.clear();
    for (const auto &edge: oldEdges) {
        const Node &firstNode = m_nodes[edge.firstNodeIndex];
        const Node &secondNode = m_nodes[edge.secondNodeIndex];
        float targetEdgeLength = (averageCutTemplateEdgeLength(firstNode.cutTemplate) * firstNode.radius +
            averageCutTemplateEdgeLength(secondNode.cutTemplate) * secondNode.radius) * 0.5;
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
            if (factor <= 0.5) {
                intermediateNode.originNodeIndex = firstNode.originNodeIndex;
                intermediateNode.nearOriginNodeIndex = firstNode.originNodeIndex;
                intermediateNode.farOriginNodeIndex = secondNode.originNodeIndex;
            } else {
                intermediateNode.originNodeIndex = secondNode.originNodeIndex;
                intermediateNode.nearOriginNodeIndex = secondNode.originNodeIndex;
                intermediateNode.farOriginNodeIndex = firstNode.originNodeIndex;
            }
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

const std::vector<Modifier::Node> &Modifier::nodes()
{
    return m_nodes;
}

const std::vector<Modifier::Edge> &Modifier::edges()
{
    return m_edges;
}

}
