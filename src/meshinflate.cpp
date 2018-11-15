#include <QDebug>
#include <set>
#include "meshinflate.h"
#include "meshutil.h"

void *meshInflate(void *combinableMesh, const std::vector<std::pair<QVector3D, float>> &inflateBalls, std::vector<std::pair<QVector3D, QVector3D>> &inflatedVertices)
{
    std::vector<QVector3D> oldPositions;
    std::vector<QVector3D> newPositions;
    std::vector<std::vector<int>> faces;
    std::set<int> changedVertices;
    
    loadCombinableMeshVerticesPositionsAndFacesIndices(combinableMesh, oldPositions, faces);
    newPositions = oldPositions;
    
    std::vector<float> inflateBallRadius2s(inflateBalls.size());
    for (size_t i = 0; i < inflateBalls.size(); ++i) {
        const auto &radius = inflateBalls[i].second;
        inflateBallRadius2s[i] = radius * radius;
    }
    
    for (size_t i = 0; i < oldPositions.size(); ++i) {
        const auto &oldPos = oldPositions[i];
        QVector3D sumOfNewPos;
        size_t pushedByBallNum = 0;
        for (size_t j = 0; j < inflateBalls.size(); ++j) {
            const auto &ball = inflateBalls[j];
            const auto &ballPos = ball.first;
            const auto &ballRadius = std::max(ball.second, (float)0.00001);
            const auto ballToPosVec = oldPos - ballPos;
            if (ballToPosVec.lengthSquared() > inflateBallRadius2s[j])
                continue;
            const auto distance = 1.0 - ballToPosVec.length() / ballRadius;
            const auto smoothedDist = 3.0 * std::pow(distance, 2) - 2.0 * std::pow(distance, 3);
            const auto newPosPushByBall = oldPos + ballToPosVec * smoothedDist;
            sumOfNewPos += newPosPushByBall;
            ++pushedByBallNum;
        }
        if (0 == pushedByBallNum)
            continue;
        newPositions[i] = sumOfNewPos / pushedByBallNum;
        changedVertices.insert(i);
    }
    
    for (const auto &index: changedVertices) {
        inflatedVertices.push_back({oldPositions[index], newPositions[index]});
    }
    
    return buildCombinableMeshFromVerticesPositionsAndFacesIndices(newPositions, faces);
}