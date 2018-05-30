#include <map>
#include <vector>
#include <QDebug>
#include <set>
#include "theme.h"
#include "meshresultcontext.h"
#include "thekla_atlas.h"
#include "positionmap.h"

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
    m_vertexWeightsResolved(false),
    m_centerBmeshNode(nullptr),
    m_resultPartsResolved(false),
    m_resultTriangleUvsResolved(false),
    m_resultRearrangedVerticesResolved(false)
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
    for (auto x = 0u; x < triangles.size(); x++) {
        const auto triangle = &triangles[x];
        std::vector<std::pair<std::pair<int, int>, int>> colorTypes;
        for (int i = 0; i < 3; i++) {
            int index = triangle->indicies[i];
            ResultVertex *resultVertex = &vertices[index];
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
        const auto triangle = &triangles[x];
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
                vertices[triangle->indicies[i]].position, // A
                vertices[triangle->indicies[(i + 1) % 3]].position, // B
                vertices[triangle->indicies[(i + 2) % 3]].position // C
            };
            QVector3D oppositeCornPosition = vertices[findOpposite->second.cornVertexIndex].position; // D
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
            const auto triangle = &triangles[x];
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
    std::map<std::pair<int, int>, QColor> nodeColorMap;
    for (const auto &it: bmeshNodes) {
        nodeColorMap[std::make_pair(it.bmeshId, it.nodeId)] = it.color;
    }
    const auto sourceNodes = triangleSourceNodes();
    for (const auto &it: sourceNodes) {
        triangleColors.push_back(nodeColorMap[it]);
    }
}

void MeshResultContext::calculateConnectionPossibleRscore(BmeshConnectionPossible &possible)
{
    possible.rscore = 100000000; // just as bigger than all the lengthSquared
    const auto &fromBmeshNode = bmeshNodeMap().find(std::make_pair(possible.bmeshEdge.fromBmeshId, possible.bmeshEdge.fromNodeId));
    const auto &toBmeshNode = bmeshNodeMap().find(std::make_pair(possible.bmeshEdge.toBmeshId, possible.bmeshEdge.toNodeId));
    if (fromBmeshNode == bmeshNodeMap().end())
        qDebug() << "There is noexists node:" << possible.bmeshEdge.fromBmeshId << possible.bmeshEdge.fromNodeId;
    else if (toBmeshNode == bmeshNodeMap().end())
        qDebug() << "There is noexists node:" << possible.bmeshEdge.toBmeshId << possible.bmeshEdge.toNodeId;
    else
        possible.rscore = (fromBmeshNode->second->origin - toBmeshNode->second->origin).lengthSquared();
}

void MeshResultContext::calculateBmeshConnectivity()
{
    const std::map<std::pair<int, int>, std::pair<int, int>> edgeSourceMap = triangleEdgeSourceMap();
    std::set<std::pair<int, int>> processedSet;
    std::set<std::pair<int, int>> connectedBmeshSet;
    std::set<int> legBmeshIdSet;
    std::vector<BmeshConnectionPossible> possibles;
    qDebug() << "start calculateBmeshConnectivity";
    // Check Leg (Start) && Spine
    std::set<std::pair<int, int>> legStartNodes;
    std::set<std::pair<int, int>> spineNodes;
    for (const auto &it: bmeshNodes) {
        switch (it.boneMark) {
            case SkeletonBoneMark::LegStart:
                legBmeshIdSet.insert(it.bmeshId);
                legStartNodes.insert(std::make_pair(it.bmeshId, it.nodeId));
                break;
            case SkeletonBoneMark::Spine:
                spineNodes.insert(std::make_pair(it.bmeshId, it.nodeId));
                break;
            default:
                break;
        }
    }
    for (const auto &legStart: legStartNodes) {
        possibles.clear();
        for (const auto &spine: spineNodes) {
            if (legStart.first != spine.first && legStart.first != -spine.first) {
                BmeshConnectionPossible possible;
                possible.bmeshEdge.fromBmeshId = legStart.first;
                possible.bmeshEdge.fromNodeId = legStart.second;
                possible.bmeshEdge.toBmeshId = spine.first;
                possible.bmeshEdge.toNodeId = spine.second;
                calculateConnectionPossibleRscore(possible);
                possibles.push_back(possible);
            }
        }
        std::sort(possibles.begin(), possibles.end(), [](const BmeshConnectionPossible &a, const BmeshConnectionPossible &b) -> bool {
            return a.rscore < b.rscore;
        });
        if (!possibles.empty()) {
            BmeshConnectionPossible *possible = &possibles[0];
            int fromBmeshId = possible->bmeshEdge.fromBmeshId;
            int toBmeshId = possible->bmeshEdge.toBmeshId;
            if (connectedBmeshSet.find(std::make_pair(fromBmeshId, toBmeshId)) == connectedBmeshSet.end()) {
                bmeshEdges.push_back(possible->bmeshEdge);
                connectedBmeshSet.insert(std::make_pair(fromBmeshId, toBmeshId));
                connectedBmeshSet.insert(std::make_pair(toBmeshId, fromBmeshId));
            }
        }
    }
    // Check connectivity by mesh connection
    possibles.clear();
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
        calculateConnectionPossibleRscore(possible);
        possibles.push_back(possible);
    }
    std::sort(possibles.begin(), possibles.end(), [](const BmeshConnectionPossible &a, const BmeshConnectionPossible &b) -> bool {
        return a.rscore < b.rscore;
    });
    for (auto i = 0u; i < possibles.size(); i++) {
        BmeshConnectionPossible *possible = &possibles[i];
        int fromBmeshId = possible->bmeshEdge.fromBmeshId;
        int toBmeshId = possible->bmeshEdge.toBmeshId;
        if (connectedBmeshSet.find(std::make_pair(fromBmeshId, toBmeshId)) != connectedBmeshSet.end())
            continue;
        // Don't connect each other mirrored parts and two seperate legs
        if (possible->bmeshEdge.fromBmeshId != -possible->bmeshEdge.toBmeshId &&
                !(legBmeshIdSet.find(possible->bmeshEdge.fromBmeshId) != legBmeshIdSet.end() &&
                    legBmeshIdSet.find(possible->bmeshEdge.toBmeshId) != legBmeshIdSet.end())) {
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
    for (auto x = 0u; x < triangles.size(); x++) {
        const auto triangle = &triangles[x];
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

const std::vector<std::vector<ResultVertexWeight>> &MeshResultContext::vertexWeights()
{
    if (!m_vertexWeightsResolved) {
        calculateVertexWeights(m_resultVertexWeights);
        m_vertexWeightsResolved = true;
    }
    return m_resultVertexWeights;
}

void MeshResultContext::calculateVertexWeights(std::vector<std::vector<ResultVertexWeight>> &vertexWeights)
{
    vertexWeights.clear();
    vertexWeights.resize(vertices.size());
    for (auto i = 0u; i < triangles.size(); i++) {
        std::pair<int, int> sourceNode = triangleSourceNodes()[i];
        for (int j = 0; j < 3; j++) {
            int vertexIndex = triangles[i].indicies[j];
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

const std::map<int, ResultPart> &MeshResultContext::parts()
{
    if (!m_resultPartsResolved) {
        calculateResultParts(m_resultParts);
        m_resultPartsResolved = true;
    }
    return m_resultParts;
}

const std::vector<ResultTriangleUv> &MeshResultContext::triangleUvs()
{
    if (!m_resultTriangleUvsResolved) {
        calculateResultTriangleUvs(m_resultTriangleUvs, m_seamVertices);
        m_resultTriangleUvsResolved = true;
    }
    return m_resultTriangleUvs;
}

void MeshResultContext::calculateResultParts(std::map<int, ResultPart> &parts)
{
    std::map<std::pair<int, int>, int> oldVertexToNewMap;
    for (auto x = 0u; x < triangles.size(); x++) {
        const auto &triangle = triangles[x];
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
            if (it == oldVertexToNewMap.end() || m_seamVertices.end() != m_seamVertices.find(triangle.indicies[i])) {
                int newIndex = resultPart.vertices.size();
                resultPart.interpolatedVertexNormals.push_back(newTriangle.normal);
                resultPart.vertices.push_back(vertices[triangle.indicies[i]]);
                ResultVertexUv vertexUv;
                vertexUv.uv[0] = triangleUvs()[x].uv[i][0];
                vertexUv.uv[1] = triangleUvs()[x].uv[i][1];
                resultPart.vertexUvs.push_back(vertexUv);
                resultPart.weights.push_back(vertexWeights()[triangle.indicies[i]]);
                if (it == oldVertexToNewMap.end())
                    oldVertexToNewMap.insert(std::make_pair(key, newIndex));
                newTriangle.indicies[i] = newIndex;
            } else {
                newTriangle.indicies[i] = it->second;
                resultPart.interpolatedVertexNormals[it->second] += newTriangle.normal;
            }
        }
        resultPart.triangles.push_back(newTriangle);
        resultPart.uvs.push_back(triangleUvs()[x]);
    }
    for (auto &partIt: parts) {
        for (auto &normalIt: partIt.second.interpolatedVertexNormals) {
            normalIt.normalize();
        }
    }
}

void MeshResultContext::calculateResultTriangleUvs(std::vector<ResultTriangleUv> &uvs, std::set<int> &seamVertices)
{
    using namespace Thekla;
    
    const std::vector<ResultRearrangedVertex> &choosenVertices = rearrangedVertices();
    const std::vector<ResultRearrangedTriangle> &choosenTriangles = rearrangedTriangles();
    
    Atlas_Input_Mesh inputMesh;
 
    inputMesh.vertex_count = choosenVertices.size();
    inputMesh.vertex_array = new Atlas_Input_Vertex[inputMesh.vertex_count];
    inputMesh.face_count = choosenTriangles.size();
    inputMesh.face_array = new Atlas_Input_Face[inputMesh.face_count];
    for (auto i = 0; i < inputMesh.vertex_count; i++) {
        const ResultRearrangedVertex *src = &choosenVertices[i];
        Atlas_Input_Vertex *dest = &inputMesh.vertex_array[i];
        dest->position[0] = src->position.x();
        dest->position[1] = src->position.y();
        dest->position[2] = src->position.z();
        dest->normal[0] = 0;
        dest->normal[1] = 0;
        dest->normal[2] = 0;
        dest->uv[0] = 0;
        dest->uv[1] = 0;
        dest->first_colocal = i;
    }
    std::map<std::pair<int, int>, int> edgeToFaceIndexMap;
    for (auto i = 0; i < inputMesh.face_count; i++) {
        const ResultRearrangedTriangle *src = &choosenTriangles[i];
        Atlas_Input_Face *dest = &inputMesh.face_array[i];
        dest->material_index = abs(triangleSourceNodes()[src->originalIndex].first);
        dest->vertex_index[0] = src->indicies[0];
        dest->vertex_index[1] = src->indicies[1];
        dest->vertex_index[2] = src->indicies[2];
        edgeToFaceIndexMap[std::make_pair(src->indicies[0], src->indicies[1])] = src->originalIndex;
        edgeToFaceIndexMap[std::make_pair(src->indicies[1], src->indicies[2])] = src->originalIndex;
        edgeToFaceIndexMap[std::make_pair(src->indicies[2], src->indicies[0])] = src->originalIndex;
        for (auto j = 0; j < 3; j++) {
            Atlas_Input_Vertex *vertex = &inputMesh.vertex_array[src->indicies[j]];
            vertex->normal[0] += src->normal.x();
            vertex->normal[1] += src->normal.y();
            vertex->normal[2] += src->normal.z();
        }
    }
    for (auto i = 0; i < inputMesh.vertex_count; i++) {
        Atlas_Input_Vertex *dest = &inputMesh.vertex_array[i];
        QVector3D normal(dest->normal[0], dest->normal[1], dest->normal[2]);
        normal.normalize();
        dest->normal[0] = normal.x();
        dest->normal[1] = normal.y();
        dest->normal[2] = normal.z();
    }
    
    Atlas_Options atlasOptions;
    atlas_set_default_options(&atlasOptions);
    
    atlasOptions.packer_options.witness.packing_quality = 4;
    
    Atlas_Error error = Atlas_Error_Success;
    Atlas_Output_Mesh *outputMesh = atlas_generate(&inputMesh, &atlasOptions, &error);
    
    PositionMap<int> uvPositionAndIndexMap;
    
    uvs.resize(triangles.size());
    std::set<int> refs;
    for (auto i = 0; i < outputMesh->index_count; i += 3) {
        Atlas_Output_Vertex *outputVertices[] = {
            &outputMesh->vertex_array[outputMesh->index_array[i + 0]],
            &outputMesh->vertex_array[outputMesh->index_array[i + 1]],
            &outputMesh->vertex_array[outputMesh->index_array[i + 2]]
        };
        int faceIndex = edgeToFaceIndexMap[std::make_pair(outputVertices[0]->xref, outputVertices[1]->xref)];
        //Q_ASSERT(faceIndex == edgeToFaceIndexMap[std::make_pair(outputVertices[1]->xref, outputVertices[2]->xref)]);
        //Q_ASSERT(faceIndex == edgeToFaceIndexMap[std::make_pair(outputVertices[2]->xref, outputVertices[0]->xref)]);
        int firstIndex = 0;
        for (auto j = 0; j < 3; j++) {
            if (choosenVertices[outputVertices[0]->xref].originalIndex == triangles[faceIndex].indicies[j]) {
                firstIndex = j;
                break;
            }
        }
        for (auto j = 0; j < 3; j++) {
            Atlas_Output_Vertex *from = outputVertices[j];
            ResultTriangleUv *to = &uvs[faceIndex];
            to->resolved = true;
            int toIndex = (firstIndex + j) % 3;
            to->uv[toIndex][0] = (float)from->uv[0] / outputMesh->atlas_width;
            to->uv[toIndex][1] = (float)from->uv[1] / outputMesh->atlas_height;
            int originalRef = choosenVertices[from->xref].originalIndex;
            if (refs.find(originalRef) == refs.end()) {
                refs.insert(originalRef);
                uvPositionAndIndexMap.addPosition(from->uv[0], from->uv[1], (float)originalRef, 0);
            } else {
                if (!uvPositionAndIndexMap.findPosition(from->uv[0], from->uv[1], (float)originalRef, nullptr)) {
                    seamVertices.insert(originalRef);
                }
            }
        }
    }
    int unresolvedUvFaceCount = 0;
    for (auto i = 0u; i < uvs.size(); i++) {
        ResultTriangleUv *uv = &uvs[i];
        if (!uv->resolved) {
            unresolvedUvFaceCount++;
        }
    }
    qDebug() << "unresolvedUvFaceCount:" << unresolvedUvFaceCount;
    
    atlas_free(outputMesh);
}

const std::vector<ResultRearrangedVertex> &MeshResultContext::rearrangedVertices()
{
    if (!m_resultRearrangedVerticesResolved) {
        calculateResultRearrangedVertices(m_rearrangedVertices, m_rearrangedTriangles);
        m_resultRearrangedVerticesResolved = true;
    }
    return m_rearrangedVertices;
}

const std::vector<ResultRearrangedTriangle> &MeshResultContext::rearrangedTriangles()
{
    if (!m_resultRearrangedVerticesResolved) {
        calculateResultRearrangedVertices(m_rearrangedVertices, m_rearrangedTriangles);
        m_resultRearrangedVerticesResolved = true;
    }
    return m_rearrangedTriangles;
}

void MeshResultContext::calculateResultRearrangedVertices(std::vector<ResultRearrangedVertex> &rearrangedVertices, std::vector<ResultRearrangedTriangle> &rearrangedTriangles)
{
    std::map<std::pair<int, int>, int> oldVertexToNewMap;
    rearrangedVertices.clear();
    rearrangedTriangles.clear();
    for (auto x = 0u; x < triangles.size(); x++) {
        const auto &triangle = triangles[x];
        const auto &sourceNode = triangleSourceNodes()[x];
        ResultRearrangedTriangle newTriangle;
        newTriangle.normal = triangle.normal;
        newTriangle.originalIndex = x;
        for (auto i = 0u; i < 3; i++) {
            auto key = std::make_pair(sourceNode.first, triangle.indicies[i]);
            const auto &it = oldVertexToNewMap.find(key);
            if (it == oldVertexToNewMap.end()) {
                ResultRearrangedVertex rearrangedVertex;
                rearrangedVertex.originalIndex = triangle.indicies[i];
                rearrangedVertex.position = vertices[triangle.indicies[i]].position;
                int newIndex = rearrangedVertices.size();
                rearrangedVertices.push_back(rearrangedVertex);
                oldVertexToNewMap.insert(std::make_pair(key, newIndex));
                newTriangle.indicies[i] = newIndex;
            } else {
                newTriangle.indicies[i] = it->second;
            }
        }
        rearrangedTriangles.push_back(newTriangle);
    }
}
