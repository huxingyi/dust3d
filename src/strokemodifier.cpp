#include <QVector2D>
#include <QDebug>
#include <unordered_map>
#include "strokemodifier.h"
#include "util.h"
#include "centripetalcatmullromspline.h"

void StrokeModifier::enableIntermediateAddition()
{
    m_intermediateAdditionEnabled = true;
}

void StrokeModifier::enableSmooth()
{
    m_smooth = true;
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
    
    if (m_smooth)
        smooth();
}

void StrokeModifier::smooth()
{
    std::unordered_map<int, std::vector<int>> neighborMap;
    for (const auto &edge: m_edges) {
        neighborMap[edge.firstNodeIndex].push_back(edge.secondNodeIndex);
        neighborMap[edge.secondNodeIndex].push_back(edge.firstNodeIndex);
    }
    
    int startEndpoint = 0;
    for (const auto &edge: m_edges) {
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
        for (const auto &index: findNeighbor->second) {
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
        const auto &nodeIndex = loop[i];
        const auto &node = m_nodes[nodeIndex];
        bool isKnot = node.originNodeIndex == nodeIndex;
        qDebug() << "Loop[" << i << "]: isKnot:" << (isKnot ? "TRUE" : "false") << "nodeIndex:" << nodeIndex;
        spline.addPoint((int)nodeIndex, node.position, isKnot);
    }
    if (!spline.interpolate())
        return;
    for (const auto &it: spline.splineNodes()) {
        if (-1 == it.source)
            continue;
        auto &node = m_nodes[it.source];
        node.position = it.position;
    }
    
    /*
    auto atKnot = [](float t, const QVector3D &p0, const QVector3D &p1) {
        const float alpha = 0.5f;
	    QVector3D d = p1 - p0;
        float a = QVector3D::dotProduct(d, d);
        float b = std::pow(a, alpha * 0.5f);
	    return (b + t);
	};
    
    auto centripetalCatmullRom = [&](std::vector<int> &knots,
            size_t segment, const std::vector<float> &times, const std::vector<size_t> &nodeIndices) {
        const QVector3D &p0 = m_nodes[knots[0]].position;
		const QVector3D &p1 = m_nodes[knots[1]].position;
		const QVector3D &p2 = m_nodes[knots[2]].position;
		const QVector3D &p3 = m_nodes[knots[3]].position;

		float t0 = 0.0f;
		float t1 = atKnot(t0, p0, p1);
		float t2 = atKnot(t1, p1, p2);
		float t3 = atKnot(t2, p2, p3);
        
        qDebug() << "t0:" << t0;
        qDebug() << "t1:" << t1;
        qDebug() << "t2:" << t2;
        qDebug() << "t3:" << t3;
        
        qDebug() << "p0:" << p0;
        qDebug() << "p1:" << p1;
        qDebug() << "p2:" << p2;
        qDebug() << "p3:" << p3;
        
        for (size_t i = 0; i < times.size(); ++i) {
            const auto &factor = times[i];
            float t = 0.0;
            if (0 == segment)
                t = t0 * (1.0f - factor) + t1 * factor;
            else if (1 == segment)
                t = t1 * (1.0f - factor) + t2 * factor;
            else
                t = t2 * (1.0f - factor) + t3 * factor;
            QVector3D a1 = (t1 - t) / (t1 - t0) * p0 + (t - t0) / (t1 - t0) * p1;
		    QVector3D a2 = (t2 - t) / (t2 - t1) * p1 + (t - t1) / (t2 - t1) * p2;
		    QVector3D a3 = (t3 - t) / (t3 - t2) * p2 + (t - t2) / (t3 - t2) * p3;
		    
		    QVector3D b1 = (t2 - t) / (t2 - t0) * a1 + (t - t0) / (t2 - t0) * a2;
		    QVector3D b2 = (t3 - t) / (t3 - t1) * a2 + (t - t1) / (t3 - t1) * a3;
		    
		    m_nodes[nodeIndices[i]].position = ((t2 - t) / (t2 - t1) * b1 + (t - t1) / (t2 - t1) * b2);
            
            qDebug() << "Update node position:" << m_nodes[nodeIndices[i]].position << "factor:" << factor << "t:" << t;
        }
    };
    
    struct SplineNode
    {
        size_t order;
        int nodeIndex;
        float distance;
    };
    std::vector<SplineNode> splineNodes;
    std::vector<size_t> splineKnots;
    float distance = 0;
    for (size_t i = 0; i < loop.size(); ++i) {
        const auto &nodeIndex = loop[i];
        const auto &node = m_nodes[nodeIndex];
        if (i > 0)
            distance += (m_nodes[loop[i - 1]].position - node.position).length();
        if (node.originNodeIndex == nodeIndex)
            splineKnots.push_back(splineNodes.size());
        splineNodes.push_back({
            i, nodeIndex, distance
        });
    }
    
    qDebug() << "splineNodes.size():" << splineNodes.size();
    qDebug() << "splineKnots.size():" << splineKnots.size();
    
    for (size_t i = 0; i < splineKnots.size(); ++i) {
        size_t h = i > 0 ? i - 1 : i;
        size_t j = i + 1 < splineKnots.size() ? i + 1 : i;
        size_t k = i + 2 < splineKnots.size() ? i + 2 : j;
        qDebug() << "h:" << h;
        qDebug() << "i:" << i;
        qDebug() << "j:" << j;
        qDebug() << "k:" << k;
        if (h == i || i == j || j == k)
            continue;
        float beginDistance = splineNodes[splineKnots[i]].distance;
        float endDistance = splineNodes[splineKnots[j]].distance;
        float totalDistance = endDistance - beginDistance;
        std::vector<float> times;
        std::vector<size_t> nodeIndices;
        for (size_t splineNodeIndex = splineKnots[i] + 1; 
                splineNodeIndex < splineKnots[j]; ++splineNodeIndex) {
            qDebug() << "splineNodeIndex:" << splineNodeIndex;
            times.push_back((splineNodes[splineNodeIndex].distance - beginDistance) / totalDistance);
            nodeIndices.push_back(splineNodes[splineNodeIndex].nodeIndex);
        }
        std::vector<int> knots = {
            splineNodes[splineKnots[h]].nodeIndex,
            splineNodes[splineKnots[i]].nodeIndex,
            splineNodes[splineKnots[j]].nodeIndex,
            splineNodes[splineKnots[k]].nodeIndex
        };
        centripetalCatmullRom(knots, 1, times, nodeIndices);
    }
    */
}

const std::vector<StrokeModifier::Node> &StrokeModifier::nodes() const
{
    return m_nodes;
}

const std::vector<StrokeModifier::Edge> &StrokeModifier::edges() const
{
    return m_edges;
}

