#include <map>
#include <vector>
#include <QDebug>
#include <set>
#include "theme.h"
#include "meshresultcontext.h"

struct HalfColorEdge
{
    int cornVertexIndex;
    std::pair<int, int> source;
};

struct CandidateEdge
{
    std::pair<int, int> source;
    int fromVertexIndex;
    int toVertexIndex;
    float dot;
    float length;
};

MeshResultContext::MeshResultContext() :
    m_triangleSourceResolved(false),
    m_triangleColorResolved(false),
    m_bmeshConnectivityResolved(false),
    m_triangleEdgeSourceMapResolved(false),
    m_bmeshNodeMapResolved(false),
    m_centerBmeshNodeResolved(false),
    m_bmeshEdgeDirectionsResolved(false),
    m_bmeshNodeNeighborsResolved(false),
    m_vertexWeightResolved(false),
    m_centerBmeshNode(nullptr),
    m_resultPartsResolved(false)
{
}

const std::vector<std::pair<int, int>> &MeshResultContext::triangleSourceNodes()
{
    if (!m_triangleSourceResolved) {
        calculateTriangleSourceNodes(m_triangleSourceNodes);
        m_triangleSourceResolved = true;
    }
    return m_triangleSourceNodes;
}

const std::vector<QColor> &MeshResultContext::triangleColors()
{
    if (!m_triangleColorResolved) {
        calculateTriangleColors(m_triangleColors);
        m_triangleColorResolved = true;
    }
    return m_triangleColors;
}

const std::map<std::pair<int, int>, std::pair<int, int>> &MeshResultContext::triangleEdgeSourceMap()
{
    if (!m_triangleEdgeSourceMapResolved) {
        calculateTriangleEdgeSourceMap(m_triangleEdgeSourceMap);
        m_triangleEdgeSourceMapResolved = true;
    }
    return m_triangleEdgeSourceMap;
}

const std::map<std::pair<int, int>, BmeshNode *> &MeshResultContext::bmeshNodeMap()
{
    if (!m_bmeshNodeMapResolved) {
        calculateBmeshNodeMap(m_bmeshNodeMap);
        m_bmeshNodeMapResolved = true;
    }
    return m_bmeshNodeMap;
}

const BmeshNode *MeshResultContext::centerBmeshNode()
{
    if (!m_centerBmeshNodeResolved) {
        m_centerBmeshNode = calculateCenterBmeshNode();
        m_centerBmeshNodeResolved = true;
    }
    return m_centerBmeshNode;
}

void MeshResultContext::resolveBmeshConnectivity()
{
    if (!m_bmeshConnectivityResolved) {
        calculateBmeshConnectivity();
        m_bmeshConnectivityResolved = true;
    }
}

void MeshResultContext::calculateTriangleSourceNodes(std::vector<std::pair<int, int>> &triangleSourceNodes)
{
    PositionMap<std::pair<int, int>> positionMap;
    std::map<std::pair<int, int>, HalfColorEdge> halfColorEdgeMap;
    std::set<int> brokenTriangleSet;
    for (const auto &it: bmeshVertices) {
        positionMap.addPosition(it.position.x(), it.position.y(), it.position.z(),
            std::make_pair(it.bmeshId, it.nodeId));
    }
    for (auto x = 0u; x < resultTriangles.size(); x++) {
        const auto triangle = &resultTriangles[x];
        std::vector<std::pair<std::pair<int, int>, int>> colorTypes;
        for (int i = 0; i < 3; i++) {
            int index = triangle->indicies[i];
            ResultVertex *resultVertex = &resultVertices[index];
            std::pair<int, int> source;
            if (positionMap.findPosition(resultVertex->position.x(), resultVertex->position.y(), resultVertex->position.z(), &source)) {
                bool colorExisted = false;
                for (auto j = 0u; j < colorTypes.size(); j++) {
                    if (colorTypes[j].first == source) {
                        colorTypes[j].second++;
                        colorExisted = true;
                        break;
                    }
                }
                if (!colorExisted) {
                    colorTypes.push_back(std::make_pair(source, 1));
                }
            }
        }
        if (colorTypes.empty()) {
            //qDebug() << "All vertices of a triangle can't find a color";
            triangleSourceNodes.push_back(std::make_pair(0, 0));
            brokenTriangleSet.insert(x);
            continue;
        }
        if (colorTypes.size() != 1 || 3 == colorTypes[0].second) {
            std::sort(colorTypes.begin(), colorTypes.end(), [](const std::pair<std::pair<int, int>, int> &a, const std::pair<std::pair<int, int>, int> &b) -> bool {
                return a.second > b.second;
            });
        }
        std::pair<int, int> choosenColor = colorTypes[0].first;
        triangleSourceNodes.push_back(choosenColor);
        for (int i = 0; i < 3; i++) {
            int oppositeStartIndex = triangle->indicies[(i + 1) % 3];
            int oppositeStopIndex = triangle->indicies[i];
            auto oppositePair = std::make_pair(oppositeStartIndex, oppositeStopIndex);
            if (halfColorEdgeMap.find(oppositePair) != halfColorEdgeMap.end()) {
                halfColorEdgeMap.erase(oppositePair);
                continue;
            }
            auto selfPair = std::make_pair(oppositeStopIndex, oppositeStartIndex);
            HalfColorEdge edge;
            edge.cornVertexIndex = triangle->indicies[(i + 2) % 3];
            edge.source = choosenColor;
            halfColorEdgeMap[selfPair] = edge;
        }
    }
    std::map<std::pair<int, int>, int> brokenTriangleMapByEdge;
    std::vector<CandidateEdge> candidateEdges;
    for (const auto &x: brokenTriangleSet) {
        const auto triangle = &resultTriangles[x];
        for (int i = 0; i < 3; i++) {
            int oppositeStartIndex = triangle->indicies[(i + 1) % 3];
            int oppositeStopIndex = triangle->indicies[i];
            auto selfPair = std::make_pair(oppositeStopIndex, oppositeStartIndex);
            brokenTriangleMapByEdge[selfPair] = x;
            auto oppositePair = std::make_pair(oppositeStartIndex, oppositeStopIndex);
            const auto &findOpposite = halfColorEdgeMap.find(oppositePair);
            if (findOpposite == halfColorEdgeMap.end())
                continue;
            QVector3D selfPositions[3] = {
                resultVertices[triangle->indicies[i]].position, // A
                resultVertices[triangle->indicies[(i + 1) % 3]].position, // B
                resultVertices[triangle->indicies[(i + 2) % 3]].position // C
            };
            QVector3D oppositeCornPosition = resultVertices[findOpposite->second.cornVertexIndex].position; // D
            QVector3D AB = selfPositions[1] - selfPositions[0];
            float length = AB.length();
            QVector3D AC = selfPositions[2] - selfPositions[0];
            QVector3D AD = oppositeCornPosition - selfPositions[0];
            AB.normalize();
            AC.normalize();
            AD.normalize();
            QVector3D ABxAC = QVector3D::crossProduct(AB, AC);
            QVector3D ADxAB = QVector3D::crossProduct(AD, AB);
            ABxAC.normalize();
            ADxAB.normalize();
            float dot = QVector3D::dotProduct(ABxAC, ADxAB);
            //qDebug() << "dot:" << dot;
            CandidateEdge candidate;
            candidate.dot = dot;
            candidate.length = length;
            candidate.fromVertexIndex = triangle->indicies[i];
            candidate.toVertexIndex = triangle->indicies[(i + 1) % 3];
            candidate.source = findOpposite->second.source;
            candidateEdges.push_back(candidate);
        }
    }
    if (candidateEdges.empty())
        return;
    std::sort(candidateEdges.begin(), candidateEdges.end(), [](const CandidateEdge &a, const CandidateEdge &b) -> bool {
        if (a.dot > b.dot)
            return true;
        else if (a.dot < b.dot)
            return false;
        return a.length > b.length;
    });
    for (auto cand = 0u; cand < candidateEdges.size(); cand++) {
        const auto &candidate = candidateEdges[cand];
        if (brokenTriangleSet.empty())
            break;
        //qDebug() << "candidate dot[" << cand << "]:" << candidate.dot;
        std::vector<std::pair<int, int>> toResolvePairs;
        toResolvePairs.push_back(std::make_pair(candidate.fromVertexIndex, candidate.toVertexIndex));
        for (auto order = 0u; order < toResolvePairs.size(); order++) {
            const auto &findTriangle = brokenTriangleMapByEdge.find(toResolvePairs[order]);
            if (findTriangle == brokenTriangleMapByEdge.end())
                continue;
            int x = findTriangle->second;
            if (brokenTriangleSet.find(x) == brokenTriangleSet.end())
                continue;
            brokenTriangleSet.erase(x);
            triangleSourceNodes[x] = candidate.source;
            //qDebug() << "resolved triangle:" << x;
            const auto triangle = &resultTriangles[x];
            for (int i = 0; i < 3; i++) {
                int oppositeStartIndex = triangle->indicies[(i + 1) % 3];
                int oppositeStopIndex = triangle->indicies[i];
                auto oppositePair = std::make_pair(oppositeStartIndex, oppositeStopIndex);
                toResolvePairs.push_back(oppositePair);
            }
        }
    }
}

void MeshResultContext::calculateTriangleColors(std::vector<QColor> &triangleColors)
{
    PositionMap<QColor> positionColorMap;
    std::map<std::pair<int, int>, QColor> nodeColorMap;
    for (const auto &it: bmeshNodes) {
        nodeColorMap[std::make_pair(it.bmeshId, it.nodeId)] = it.color;
    }
    const auto sourceNodes = triangleSourceNodes();
    for (const auto &it: sourceNodes) {
        triangleColors.push_back(nodeColorMap[it]);
    }
}

struct BmeshConnectionPossible
{
    BmeshEdge bmeshEdge;
    float score;
};

void MeshResultContext::calculateBmeshConnectivity()
{
    const std::map<std::pair<int, int>, std::pair<int, int>> edgeSourceMap = triangleEdgeSourceMap();
    std::set<std::pair<int, int>> processedSet;
    qDebug() << "start calculateBmeshConnectivity";
    std::vector<BmeshConnectionPossible> possibles;
    for (const auto &it: edgeSourceMap) {
        if (processedSet.find(it.first) != processedSet.end())
            continue;
        const auto pairKey = std::make_pair(it.first.second, it.first.first);
        const auto &paired = edgeSourceMap.find(pairKey);
        if (paired == edgeSourceMap.end())
            continue;
        processedSet.insert(pairKey);
        if (it.second == paired->second)
            continue;
        BmeshConnectionPossible possible;
        possible.bmeshEdge.fromBmeshId = it.second.first;
        possible.bmeshEdge.fromNodeId = it.second.second;
        possible.bmeshEdge.toBmeshId = paired->second.first;
        possible.bmeshEdge.toNodeId = paired->second.second;
        possible.score = 100000000; // just as bigger than all the lengthSquared
        const auto &fromBmeshNode = bmeshNodeMap().find(std::make_pair(possible.bmeshEdge.fromBmeshId, possible.bmeshEdge.fromNodeId));
        const auto &toBmeshNode = bmeshNodeMap().find(std::make_pair(possible.bmeshEdge.toBmeshId, possible.bmeshEdge.toNodeId));
        if (fromBmeshNode == bmeshNodeMap().end())
            qDebug() << "There is noexists node:" << possible.bmeshEdge.fromBmeshId << possible.bmeshEdge.fromNodeId;
        else if (toBmeshNode == bmeshNodeMap().end())
            qDebug() << "There is noexists node:" << possible.bmeshEdge.toBmeshId << possible.bmeshEdge.toNodeId;
        else
            possible.score = (fromBmeshNode->second->origin - toBmeshNode->second->origin).lengthSquared();
        possibles.push_back(possible);
    }
    std::sort(possibles.begin(), possibles.end(), [](const BmeshConnectionPossible &a, const BmeshConnectionPossible &b) -> bool {
        return a.score < b.score;
    });
    std::set<std::pair<int, int>> connectedBmeshSet;
    for (auto i = 0u; i < possibles.size(); i++) {
        BmeshConnectionPossible *possible = &possibles[i];
        int fromBmeshId = possible->bmeshEdge.fromBmeshId;
        int toBmeshId = possible->bmeshEdge.toBmeshId;
        if (connectedBmeshSet.find(std::make_pair(fromBmeshId, toBmeshId)) != connectedBmeshSet.end())
            continue;
        if (possible->bmeshEdge.fromBmeshId != -possible->bmeshEdge.toBmeshId) {
            // Don't connect each other mirrored parts
            bmeshEdges.push_back(possible->bmeshEdge);
        }
        connectedBmeshSet.insert(std::make_pair(fromBmeshId, toBmeshId));
        connectedBmeshSet.insert(std::make_pair(toBmeshId, fromBmeshId));
    }
    qDebug() << "finish calculateBmeshConnectivity";
}

void MeshResultContext::calculateTriangleEdgeSourceMap(std::map<std::pair<int, int>, std::pair<int, int>> &triangleEdgeSourceMap)
{
    const std::vector<std::pair<int, int>> sourceNodes = triangleSourceNodes();
    for (auto x = 0u; x < resultTriangles.size(); x++) {
        const auto triangle = &resultTriangles[x];
        for (int i = 0; i < 3; i++) {
            int startIndex = triangle->indicies[i];
            int stopIndex = triangle->indicies[(i + 1) % 3];
            triangleEdgeSourceMap[std::make_pair(startIndex, stopIndex)] = sourceNodes[x];
        }
    }
}

void MeshResultContext::calculateBmeshNodeMap(std::map<std::pair<int, int>, BmeshNode *> &bmeshNodeMap) {
    for (auto i = 0u; i < bmeshNodes.size(); i++) {
        BmeshNode *bmeshNode = &bmeshNodes[i];
        bmeshNodeMap[std::make_pair(bmeshNode->bmeshId, bmeshNode->nodeId)] = bmeshNode;
    }
}

struct BmeshNodeDistWithWorldCenter
{
    BmeshNode *bmeshNode;
    float dist2;
};

BmeshNode *MeshResultContext::calculateCenterBmeshNode()
{
    // Sort all the nodes by distance with world center.
    std::vector<BmeshNodeDistWithWorldCenter> nodesOrderByDistWithWorldCenter;
    for (auto i = 0u; i < bmeshNodes.size(); i++) {
        BmeshNode *bmeshNode = &bmeshNodes[i];
        BmeshNodeDistWithWorldCenter distNode;
        distNode.bmeshNode = bmeshNode;
        distNode.dist2 = bmeshNode->origin.lengthSquared();
        nodesOrderByDistWithWorldCenter.push_back(distNode);
    }
    if (nodesOrderByDistWithWorldCenter.empty())
        return nullptr;
    std::sort(nodesOrderByDistWithWorldCenter.begin(), nodesOrderByDistWithWorldCenter.end(), [](const BmeshNodeDistWithWorldCenter &a, const BmeshNodeDistWithWorldCenter &b) -> bool {
        return a.dist2 < b.dist2;
    });
    return nodesOrderByDistWithWorldCenter[0].bmeshNode;
}

const std::map<std::pair<int, int>, std::vector<std::pair<int, int>>> &MeshResultContext::nodeNeighbors()
{
    if (!m_bmeshNodeNeighborsResolved) {
        calculateBmeshNodeNeighbors(m_nodeNeighbors);
        m_bmeshNodeNeighborsResolved = true;
    }
    return m_nodeNeighbors;
}

void MeshResultContext::calculateBmeshNodeNeighbors(std::map<std::pair<int, int>, std::vector<std::pair<int, int>>> &nodeNeighbors)
{
    for (const auto &it: bmeshEdges) {
        nodeNeighbors[std::make_pair(it.fromBmeshId, it.fromNodeId)].push_back(std::make_pair(it.toBmeshId, it.toNodeId));
        nodeNeighbors[std::make_pair(it.toBmeshId, it.toNodeId)].push_back(std::make_pair(it.fromBmeshId, it.fromNodeId));
    }
}

void MeshResultContext::calculateBmeshEdgeDirectionsFromNode(std::pair<int, int> node, std::set<std::pair<int, int>> &visitedNodes, std::set<std::pair<std::pair<int, int>, std::pair<int, int>>> &connections, std::vector<BmeshEdge> &rearrangedEdges)
{
    if (visitedNodes.find(node) != visitedNodes.end())
        return;
    visitedNodes.insert(node);
    const auto &neighbors = nodeNeighbors().find(node);
    if (neighbors == nodeNeighbors().end())
        return;
    for (const auto &it: neighbors->second) {
        if (connections.find(std::make_pair(std::make_pair(node.first, node.second), std::make_pair(it.first, it.second))) != connections.end())
            continue;
        BmeshEdge bmeshEdge;
        bmeshEdge.fromBmeshId = node.first;
        bmeshEdge.fromNodeId = node.second;
        bmeshEdge.toBmeshId = it.first;
        bmeshEdge.toNodeId = it.second;
        rearrangedEdges.push_back(bmeshEdge);
        connections.insert(std::make_pair(std::make_pair(node.first, node.second), std::make_pair(it.first, it.second)));
        connections.insert(std::make_pair(std::make_pair(it.first, it.second), std::make_pair(node.first, node.second)));
        calculateBmeshEdgeDirectionsFromNode(it, visitedNodes, connections, rearrangedEdges);
    }
}

void MeshResultContext::resolveBmeshEdgeDirections()
{
    if (!m_bmeshEdgeDirectionsResolved) {
        calculateBmeshEdgeDirections();
        m_bmeshEdgeDirectionsResolved = true;
    }
}

void MeshResultContext::calculateBmeshEdgeDirections()
{
    const BmeshNode *rootNode = centerBmeshNode();
    if (!rootNode)
        return;
    std::set<std::pair<int, int>> visitedNodes;
    std::vector<BmeshEdge> rearrangedEdges;
    std::set<std::pair<std::pair<int, int>, std::pair<int, int>>> connections;
    calculateBmeshEdgeDirectionsFromNode(std::make_pair(rootNode->bmeshId, rootNode->nodeId), visitedNodes, connections, rearrangedEdges);
    bmeshEdges = rearrangedEdges;
}

const std::vector<std::vector<ResultVertexWeight>> &MeshResultContext::resultVertexWeights()
{
    if (!m_vertexWeightResolved) {
        calculateVertexWeights(m_resultVertexWeights);
        m_vertexWeightResolved = true;
    }
    return m_resultVertexWeights;
}

void MeshResultContext::calculateVertexWeights(std::vector<std::vector<ResultVertexWeight>> &vertexWeights)
{
    vertexWeights.clear();
    vertexWeights.resize(resultVertices.size());
    for (auto i = 0u; i < resultTriangles.size(); i++) {
        std::pair<int, int> sourceNode = triangleSourceNodes()[i];
        for (int j = 0; j < 3; j++) {
            int vertexIndex = resultTriangles[i].indicies[j];
            Q_ASSERT(vertexIndex < (int)vertexWeights.size());
            int foundSourceNodeIndex = -1;
            for (auto k = 0u; k < vertexWeights[vertexIndex].size(); k++) {
                if (vertexWeights[vertexIndex][k].sourceNode == sourceNode) {
                    foundSourceNodeIndex = k;
                    break;
                }
            }
            if (-1 == foundSourceNodeIndex) {
                ResultVertexWeight vertexWeight;
                vertexWeight.sourceNode = sourceNode;
                vertexWeight.count = 1;
                continue;
            }
            vertexWeights[vertexIndex][foundSourceNodeIndex].count++;
        }
    }
    for (auto &it: vertexWeights) {
        int total = 0;
        for (auto i = 0u; i < MAX_WEIGHT_NUM && i < it.size(); i++) {
            total += it[i].count;
        }
        for (auto i = 0u; i < MAX_WEIGHT_NUM && i < it.size(); i++) {
            it[i].weight = (float)it[i].count / total;
        }
    }
}

const std::map<int, ResultPart> &MeshResultContext::resultParts()
{
    if (!m_resultPartsResolved) {
        calculateResultParts(m_resultParts);
        m_resultPartsResolved = true;
    }
    return m_resultParts;
}

void MeshResultContext::calculateResultParts(std::map<int, ResultPart> &parts)
{
    std::map<std::pair<int, int>, int> oldVertexToNewMap;
    for (auto x = 0u; x < resultTriangles.size(); x++) {
        const auto &triangle = resultTriangles[x];
        const auto &sourceNode = triangleSourceNodes()[x];
        auto it = parts.find(sourceNode.first);
        if (it == parts.end()) {
            ResultPart newPart;
            newPart.color = triangleColors()[x];
            parts.insert(std::make_pair(sourceNode.first, newPart));
        }
        auto &resultPart = parts[sourceNode.first];
        ResultTriangle newTriangle;
        newTriangle.normal = triangle.normal;
        for (auto i = 0u; i < 3; i++) {
            auto key = std::make_pair(sourceNode.first, triangle.indicies[i]);
            const auto &it = oldVertexToNewMap.find(key);
            if (it == oldVertexToNewMap.end()) {
                int newIndex = resultPart.vertices.size();
                resultPart.vertices.push_back(resultVertices[triangle.indicies[i]]);
                resultPart.weights.push_back(resultVertexWeights()[triangle.indicies[i]]);
                oldVertexToNewMap.insert(std::make_pair(key, newIndex));
                newTriangle.indicies[i] = newIndex;
            } else {
                newTriangle.indicies[i] = it->second;
            }
        }
        resultPart.triangles.push_back(newTriangle);
    }
}
