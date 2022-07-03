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

#include <algorithm>    // std::reverse
#include <dust3d/base/vector2.h>
#include <dust3d/base/vector3.h>
#include <dust3d/base/cut_face.h>

namespace dust3d
{
    
IMPL_CutFaceFromString
IMPL_CutFaceToString
TMPL_CutFaceToPoints

static void correctFlippedNormal(std::vector<Vector2> *points)
{
    if (points->size() < 3)
        return;
    std::vector<Vector3> referenceFacePoints = {
        Vector3((float)-1.0, (float)-1.0, (float)0.0),
        Vector3((float)1.0, (float)-1.0, (float)0.0),
        Vector3((float)1.0,  (float)1.0, (float)0.0)
    };
    Vector3 referenceNormal = Vector3::normal(referenceFacePoints[0],
        referenceFacePoints[1], referenceFacePoints[2]);
    Vector3 normal;
    for (size_t i = 0; i < points->size(); ++i) {
        size_t j = (i + 1) % points->size();
        size_t k = (j + 1) % points->size();
        std::vector<Vector3> facePoints = {
            Vector3(((*points)[i]).x(), ((*points)[i]).y(), (float)0.0),
            Vector3(((*points)[j]).x(), ((*points)[j]).y(), (float)0.0),
            Vector3(((*points)[k]).x(), ((*points)[k]).y(), (float)0.0)
        };
        normal += Vector3::normal(facePoints[0],
            facePoints[1], facePoints[2]);
    }
    normal.normalize();
    if (Vector3::dotProduct(referenceNormal, normal) > 0)
        return;
    std::reverse(points->begin() + 1, points->end());
}

void normalizeCutFacePoints(std::vector<Vector2> *points)
{
    Vector2 center;
    if (nullptr == points || points->empty())
        return;
    float xLow = std::numeric_limits<float>::max();
    float xHigh = std::numeric_limits<float>::lowest();
    float yLow = std::numeric_limits<float>::max();
    float yHigh = std::numeric_limits<float>::lowest();
    for (const auto &position: *points) {
        if (position.x() < xLow)
            xLow = position.x();
        else if (position.x() > xHigh)
            xHigh = position.x();
        if (position.y() < yLow)
            yLow = position.y();
        else if (position.y() > yHigh)
            yHigh = position.y();
    }
    float xMiddle = (xHigh + xLow) * 0.5;
    float yMiddle = (yHigh + yLow) * 0.5;
    float xSize = xHigh - xLow;
    float ySize = yHigh - yLow;
    float longSize = ySize;
    if (xSize > longSize)
        longSize = xSize;
    if (Math::isZero(longSize))
        longSize = 0.000001;
    for (auto &position: *points) {
        position.setX((position.x() - xMiddle) * 2 / longSize);
        position.setY((position.y() - yMiddle) * 2 / longSize);
    }
    correctFlippedNormal(points);
}

void cutFacePointsFromNodes(std::vector<Vector2> &points, const std::vector<std::tuple<float, float, float, std::string>> &nodes, bool isRing,
    std::vector<std::string> *pointsIds)
{
    if (isRing) {
        if (nodes.size() < 3)
            return;
        for (const auto &it: nodes) {
            points.push_back(Vector2(std::get<1>(it), std::get<2>(it)));
        }
        if (nullptr != pointsIds) {
            for (const auto &it: nodes) {
                pointsIds->push_back(std::get<3>(it));
            }
        }
        normalizeCutFacePoints(&points);
        return;
    }
    if (nodes.size() < 2)
        return;
    std::vector<Vector2> edges;
    for (size_t i = 1; i < nodes.size(); ++i) {
        const auto &previous = nodes[i - 1];
        const auto &current = nodes[i];
        edges.push_back((Vector2(std::get<1>(current), std::get<2>(current)) -
            Vector2(std::get<1>(previous), std::get<2>(previous))).normalized());
    }
    std::vector<Vector2> nodeDirections;
    nodeDirections.push_back(edges[0]);
    for (size_t i = 1; i < nodes.size() - 1; ++i) {
        const auto &previousEdge = edges[i - 1];
        const auto &nextEdge = edges[i];
        nodeDirections.push_back((previousEdge + nextEdge).normalized());
    }
    nodeDirections.push_back(edges[edges.size() - 1]);
    std::vector<std::pair<Vector2, Vector2>> cutPoints;
    for (size_t i = 0; i < nodes.size(); ++i) {
        const auto &current = nodes[i];
        const auto &direction = nodeDirections[i];
        const auto &radius = std::get<0>(current);
        Vector3 origin = Vector3(std::get<1>(current), std::get<2>(current), 0);
        Vector3 pointer = Vector3(direction.x(), direction.y(), 0);
        Vector3 rotateAxis = Vector3(0, 0, 1);
        Vector3 u = Vector3::crossProduct(pointer, rotateAxis).normalized();
        Vector3 upPoint = origin + u * radius;
        Vector3 downPoint = origin - u * radius;
        cutPoints.push_back({Vector2(upPoint.x(), upPoint.y()),
            Vector2(downPoint.x(), downPoint.y())});
    }
    for (const auto &it: cutPoints) {
        points.push_back(it.first);
    }
    if (nullptr != pointsIds) {
        for (const auto &it: nodes) {
            pointsIds->push_back(std::get<3>(it) + std::string("/1"));
        }
    }
    for (auto it = cutPoints.rbegin(); it != cutPoints.rend(); ++it) {
        points.push_back(it->second);
    }
    if (nullptr != pointsIds) {
        for (auto it = nodes.rbegin(); it != nodes.rend(); ++it) {
            pointsIds->push_back(std::get<3>(*it) + std::string("/2"));
        }
    }
    normalizeCutFacePoints(&points);
}

}
