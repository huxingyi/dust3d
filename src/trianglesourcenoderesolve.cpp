#include <map>
#include "trianglesourcenoderesolve.h"
#include "positionkey.h"

struct HalfColorEdge
{
    int cornVertexIndex;
    std::pair<QUuid, QUuid> source;
};

struct CandidateEdge
{
    std::pair<QUuid, QUuid> source;
    int fromVertexIndex;
    int toVertexIndex;
    float dot;
    float length;
};

void triangleSourceNodeResolve(const Outcome &outcome, std::vector<std::pair<QUuid, QUuid>> &triangleSourceNodes,
    std::vector<std::pair<QUuid, QUuid>> *vertexSourceNodes)
{
    std::map<int, std::pair<QUuid, QUuid>> vertexSourceMap;
    std::map<PositionKey, std::pair<QUuid, QUuid>> positionMap;
    std::map<std::pair<int, int>, HalfColorEdge> halfColorEdgeMap;
    std::set<int> brokenTriangleSet;
    for (const auto &it: outcome.nodeVertices) {
        positionMap.insert({PositionKey(it.first), it.second});
    }
    if (nullptr != vertexSourceNodes)
        vertexSourceNodes->resize(outcome.vertices.size());
    for (auto x = 0u; x < outcome.vertices.size(); x++) {
        const QVector3D *resultVertex = &outcome.vertices[x];
        std::pair<QUuid, QUuid> source;
        auto findPosition = positionMap.find(PositionKey(*resultVertex));
        if (findPosition != positionMap.end()) {
            (*vertexSourceNodes)[x] = findPosition->second;
            vertexSourceMap[x] = findPosition->second;
        }
    }
    for (auto x = 0u; x < outcome.triangles.size(); x++) {
        const auto triangle = outcome.triangles[x];
        std::vector<std::pair<std::pair<QUuid, QUuid>, int>> colorTypes;
        for (int i = 0; i < 3; i++) {
            int index = triangle[i];
            const auto &findResult = vertexSourceMap.find(index);
            if (findResult != vertexSourceMap.end()) {
                std::pair<QUuid, QUuid> source = findResult->second;
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
            triangleSourceNodes.push_back(std::make_pair(QUuid(), QUuid()));
            brokenTriangleSet.insert(x);
            continue;
        }
        if (colorTypes.size() != 1 || 3 == colorTypes[0].second) {
            std::sort(colorTypes.begin(), colorTypes.end(), [](const std::pair<std::pair<QUuid, QUuid>, int> &a, const std::pair<std::pair<QUuid, QUuid>, int> &b) -> bool {
                return a.second > b.second;
            });
        }
        std::pair<QUuid, QUuid> choosenColor = colorTypes[0].first;
        triangleSourceNodes.push_back(choosenColor);
        for (int i = 0; i < 3; i++) {
            int oppositeStartIndex = triangle[(i + 1) % 3];
            int oppositeStopIndex = triangle[i];
            auto oppositePair = std::make_pair(oppositeStartIndex, oppositeStopIndex);
            if (halfColorEdgeMap.find(oppositePair) != halfColorEdgeMap.end()) {
                halfColorEdgeMap.erase(oppositePair);
                continue;
            }
            auto selfPair = std::make_pair(oppositeStopIndex, oppositeStartIndex);
            HalfColorEdge edge;
            edge.cornVertexIndex = triangle[(i + 2) % 3];
            edge.source = choosenColor;
            halfColorEdgeMap[selfPair] = edge;
        }
    }
    std::map<std::pair<int, int>, int> brokenTriangleMapByEdge;
    std::vector<CandidateEdge> candidateEdges;
    for (const auto &x: brokenTriangleSet) {
        const auto triangle = outcome.triangles[x];
        for (int i = 0; i < 3; i++) {
            int oppositeStartIndex = triangle[(i + 1) % 3];
            int oppositeStopIndex = triangle[i];
            auto selfPair = std::make_pair(oppositeStopIndex, oppositeStartIndex);
            brokenTriangleMapByEdge[selfPair] = x;
            auto oppositePair = std::make_pair(oppositeStartIndex, oppositeStopIndex);
            const auto &findOpposite = halfColorEdgeMap.find(oppositePair);
            if (findOpposite == halfColorEdgeMap.end())
                continue;
            QVector3D selfPositions[3] = {
                outcome.vertices[triangle[i]], // A
                outcome.vertices[triangle[(i + 1) % 3]], // B
                outcome.vertices[triangle[(i + 2) % 3]] // C
            };
            QVector3D oppositeCornPosition = outcome.vertices[findOpposite->second.cornVertexIndex]; // D
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
            candidate.fromVertexIndex = triangle[i];
            candidate.toVertexIndex = triangle[(i + 1) % 3];
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
            const auto triangle = outcome.triangles[x];
            for (int i = 0; i < 3; i++) {
                int oppositeStartIndex = triangle[(i + 1) % 3];
                int oppositeStopIndex = triangle[i];
                auto oppositePair = std::make_pair(oppositeStartIndex, oppositeStopIndex);
                toResolvePairs.push_back(oppositePair);
            }
        }
    }
}
