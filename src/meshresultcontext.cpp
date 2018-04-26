#include <map>
#include <vector>
#include <QDebug>
#include <set>
#include "theme.h"
#include "meshresultcontext.h"

struct HalfColorEdge
{
    int cornVertexIndex;
    QColor color;
};

struct CandidateEdge
{
    QColor color;
    int fromVertexIndex;
    int toVertexIndex;
    float dot;
    float length;
};

void MeshResultContext::calculateTriangleColors(std::vector<QColor> &triangleColors)
{
    PositionMap<QColor> positionColorMap;
    std::map<std::pair<int, int>, QColor> nodeColorMap;
    for (const auto &it: bmeshNodes) {
        nodeColorMap[std::make_pair(it.bmeshId, it.nodeId)] = it.color;
    }
    for (const auto &it: bmeshVertices) {
        positionColorMap.addPosition(it.position.x(), it.position.y(), it.position.z(),
            nodeColorMap[std::make_pair(it.bmeshId, it.nodeId)]);
    }
    std::map<std::pair<int, int>, HalfColorEdge> halfColorEdgeMap;
    std::set<int> brokenTriangleSet;
    for (auto x = 0u; x < resultTriangles.size(); x++) {
        const auto triangle = &resultTriangles[x];
        std::vector<std::pair<QColor, int>> colorTypes;
        for (int i = 0; i < 3; i++) {
            int index = triangle->indicies[i];
            ResultVertex *resultVertex = &resultVertices[index];
            QColor color;
            if (positionColorMap.findPosition(resultVertex->position.x(), resultVertex->position.y(), resultVertex->position.z(), &color)) {
                bool colorExisted = false;
                for (auto j = 0u; j < colorTypes.size(); j++) {
                    if (colorTypes[j].first == color) {
                        colorTypes[j].second++;
                        colorExisted = true;
                        break;
                    }
                }
                if (!colorExisted) {
                    colorTypes.push_back(std::make_pair(color, 1));
                }
            }
        }
        if (colorTypes.empty()) {
            //qDebug() << "All vertices of a triangle can't find a color";
            triangleColors.push_back(Theme::white);
            brokenTriangleSet.insert(x);
            continue;
        }
        if (colorTypes.size() != 1 || 3 == colorTypes[0].second) {
            std::sort(colorTypes.begin(), colorTypes.end(), [](const std::pair<QColor, int> &a, const std::pair<QColor, int> &b) -> bool {
                return a.second > b.second;
            });
        }
        QColor choosenColor = colorTypes[0].first;
        triangleColors.push_back(choosenColor);
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
            edge.color = choosenColor;
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
            candidate.color = findOpposite->second.color;
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
            triangleColors[x] = candidate.color;
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
