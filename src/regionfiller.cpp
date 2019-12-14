#include <numeric>
#include <cmath>
#include <QDebug>
#include <set>
#include "regionfiller.h"

//
// This is an implementation of the following paper
//
// <Filling N-Sided Regions by Quad Meshes for Subdivision Surfaces>
// A. Nasri1∗, M. Sabin2 and Z. Yasseen1
//

#define isEven(n)   (0 == (n) % 2)
#define isOdd(n)    (!isEven(n))

RegionFiller::RegionFiller(const std::vector<Node> *vertices,
        const std::vector<std::vector<size_t>> *polylines) :
    m_sourceVertices(vertices),
    m_sourcePolylines(new std::vector<std::vector<size_t>>(*polylines))
{
}

RegionFiller::~RegionFiller()
{
    delete m_sourcePolylines;
}

bool RegionFiller::resolveEvenSidedEvenSumOfSegmentsAndBothL2L2AreEven()
{
    return resolveOddSidedEvenSumOfSegments(m_sideNum / 2);
}

std::vector<size_t> RegionFiller::createPointsToMapBetween(size_t fromIndex,
        size_t toIndex,
        size_t segments,
        std::map<std::pair<size_t, size_t>, std::vector<size_t>> *map)
{
    size_t a = fromIndex;
    size_t b = toIndex;
    bool needReverse = false;
    if (a > b) {
        std::swap(a, b);
        needReverse = true;
    }
    auto key = std::make_pair(a, b);
    auto findPoints = map->find(key);
    if (findPoints != map->end()) {
        return needReverse ? reversed(findPoints->second) : findPoints->second;
    }
    std::vector<size_t> newPointIndices;
    createPointsBetween(a, b, segments, &newPointIndices);
    map->insert({key, newPointIndices});
    return needReverse ? reversed(newPointIndices) : newPointIndices;
}

void RegionFiller::createPointsBetween(size_t fromIndex,
        size_t toIndex,
        size_t segments,
        std::vector<size_t> *newPointIndices)
{
    if (fromIndex == toIndex || 0 == segments)
        return;
    auto fromVertex = m_oldAndNewVertices[fromIndex];
    auto toVertex = m_oldAndNewVertices[toIndex];
    float stepFactor = 1.0 / segments;
    newPointIndices->push_back(fromIndex);
    for (size_t i = 1; i <= segments - 1; ++i) {
        float factor = stepFactor * i;
        RegionFiller::Node node;
        node.position = fromVertex.position * (1.0 - factor) + toVertex.position * factor;
        node.radius = fromVertex.radius * (1.0 - factor) + toVertex.radius * factor;
        if (m_centerSources.find(toVertex.source) != m_centerSources.end()) {
            node.source = fromVertex.source;
        } else if (m_centerSources.find(fromVertex.source) != m_centerSources.end()) {
            node.source = toVertex.source;
        } else {
            if (factor > 0.5) {
                node.source = toVertex.source;
            } else {
                node.source = fromVertex.source;
            }
        }
        size_t newIndex = m_oldAndNewVertices.size();
        m_oldAndNewVertices.push_back(node);
        newPointIndices->push_back(newIndex);
    }
    newPointIndices->push_back(toIndex);
}

std::vector<size_t> RegionFiller::createPointsBetween(size_t fromIndex,
        size_t toIndex,
        size_t segments)
{
    std::vector<size_t> newPointIndices;
    createPointsBetween(fromIndex, toIndex, segments, &newPointIndices);
    return newPointIndices;
}

void RegionFiller::collectEdgePoints(size_t polyline, int startPos, int stopPos,
    std::vector<size_t> *pointIndices)
{
    const auto &edgeIndices = (*m_sourcePolylines)[polyline];
    int step = startPos < stopPos ? 1 : -1;
    int totalSteps = std::abs(startPos - stopPos) + 1;
    int current = startPos;
    for (int i = 0; i < totalSteps; ++i) {
        pointIndices->push_back(edgeIndices[current]);
        current += step;
    }
}

std::vector<size_t> RegionFiller::collectEdgePoints(size_t polyline, int startPos, int stopPos)
{
    std::vector<size_t> pointIndices;
    collectEdgePoints(polyline, startPos, stopPos, &pointIndices);
    return pointIndices;
}

std::vector<size_t> RegionFiller::reversed(const std::vector<size_t> &pointIndices)
{
    std::vector<size_t> newPointIndices = pointIndices;
    std::reverse(newPointIndices.begin(), newPointIndices.end());
    return newPointIndices;
}

float RegionFiller::averageRadius(size_t *maxNodeIndex)
{
    float sumOfRadius = 0;
    size_t num = 0;
    float maxRadius = 0;
    for (const auto &polyline: *m_sourcePolylines) {
        for (const auto &it: polyline) {
            const auto &vertex = (*m_sourceVertices)[it];
            if (vertex.radius >= maxRadius) {
                maxRadius = vertex.radius;
                if (nullptr != maxNodeIndex)
                    *maxNodeIndex = it;
            }
            sumOfRadius += vertex.radius;
            ++num;
        }
    }
    if (0 == num)
        return 0;
    auto radius = sumOfRadius / num;
    return radius;
}

bool RegionFiller::resolveOddSidedEvenSumOfSegments(int siUpperBound)
{
    auto si = [&](int i) {
        int sum = 0;
        for (int j = 1; j <= siUpperBound; ++j) {
            sum += std::pow(-1, j + 1) *
                m_sideSegmentNums[(i + 2 * j - 1) % m_sideSegmentNums.size()];
        }
        return sum;
    };
    auto di = [&](int i) {
        return std::ceil(si(i) * 0.5);
    };
    
    std::vector<int> gRaySegmentNums(m_sideNum);
    for (int i = 0; i < m_sideNum; ++i) {
        gRaySegmentNums[i] = di(i);
        if (gRaySegmentNums[i] <= 0) {
            qDebug() << "resolveOddSidedEvenSumOfSegments failed, di:" << gRaySegmentNums[i];
            return false;
        }
    }
 
    std::vector<int> eIndices(m_sideNum);
    for (int i = 0; i < m_sideNum; ++i) {
        auto length = gRaySegmentNums[(i + 1) % gRaySegmentNums.size()];
        eIndices[i] = m_sideSegmentNums[i] - length;
    }
    
    std::set<size_t> cornerVertices;
    std::vector<std::tuple<int, int, int>> edgeSegments; // tuple<begin, end, edgeIndex>
    for (int i = 0; i < m_sideNum; ++i) {
        if (gRaySegmentNums[i] <= 0) {
            edgeSegments.push_back(std::make_tuple(0, (*m_sourcePolylines)[i].size() - 1, i));
            continue;
        }
        const auto &pointEdgeIndex = eIndices[i];
        cornerVertices.insert((*m_sourcePolylines)[i][pointEdgeIndex]);
        if (pointEdgeIndex > 0)
            edgeSegments.push_back(std::make_tuple(0, pointEdgeIndex, i));
        if (pointEdgeIndex < (int)(*m_sourcePolylines)[i].size() - 1)
            edgeSegments.push_back(std::make_tuple(pointEdgeIndex, (*m_sourcePolylines)[i].size() - 1, i));
    }
    
    std::vector<std::pair<QVector3D, float>> gPositionAndAreas;
    for (size_t i = 0; i < edgeSegments.size(); ++i) {
        size_t j = (i + 1) % edgeSegments.size();
        size_t m = (j + 2) % edgeSegments.size();
        const auto &segmentI = edgeSegments[i];
        const auto &segmentJ = edgeSegments[j];
        const auto &segmentM = edgeSegments[m];
        std::vector<size_t> firstSegmentPoints;
        std::vector<size_t> secondSegmentPoints;
        std::vector<size_t> forthSegmentPoints;
        collectEdgePoints(std::get<2>(segmentI), std::get<0>(segmentI), std::get<1>(segmentI),
            &firstSegmentPoints);
        collectEdgePoints(std::get<2>(segmentJ), std::get<0>(segmentJ), std::get<1>(segmentJ),
            &secondSegmentPoints);
        collectEdgePoints(std::get<2>(segmentM), std::get<0>(segmentM), std::get<1>(segmentM),
            &forthSegmentPoints);
        if (cornerVertices.find((*m_sourcePolylines)[std::get<2>(segmentI)][std::get<0>(segmentI)]) != cornerVertices.end() &&
                cornerVertices.find((*m_sourcePolylines)[std::get<2>(segmentI)][std::get<1>(segmentI)]) != cornerVertices.end()) {
            const auto &p1 = (*m_sourceVertices)[(*m_sourcePolylines)[std::get<2>(segmentI)][std::get<0>(segmentI)]];
            const auto &p2 = (*m_sourceVertices)[(*m_sourcePolylines)[std::get<2>(segmentI)][std::get<1>(segmentI)]];
            gPositionAndAreas.push_back(std::make_pair((p1.position + p2.position) * 0.5,
                (firstSegmentPoints.size() - 1) * (secondSegmentPoints.size() - 1) * 0.5));
            continue;
        }
        if (cornerVertices.find((*m_sourcePolylines)[std::get<2>(segmentI)][std::get<0>(segmentI)]) != cornerVertices.end() &&
                cornerVertices.find((*m_sourcePolylines)[std::get<2>(segmentJ)][std::get<1>(segmentJ)]) != cornerVertices.end()) {
            ++i;
            const auto &p1 = (*m_sourceVertices)[(*m_sourcePolylines)[std::get<2>(segmentI)][std::get<0>(segmentI)]];
            const auto &p2 = (*m_sourceVertices)[(*m_sourcePolylines)[std::get<2>(segmentI)][std::get<1>(segmentI)]];
            const auto &p3 = (*m_sourceVertices)[(*m_sourcePolylines)[std::get<2>(segmentJ)][std::get<1>(segmentJ)]];
            gPositionAndAreas.push_back(std::make_pair(p1.position + p2.position - p3.position,
                (firstSegmentPoints.size() - 1) * (secondSegmentPoints.size() - 1)));
        }
    }
    
    QVector3D gTop;
    float gBottom = 0;
    for (const auto &it: gPositionAndAreas) {
        gTop += it.first / it.second;
        gBottom += 1.0 / it.second;
    }
    auto gPosition = gTop / gBottom;
    size_t gIndex = m_oldAndNewVertices.size();
    Node node;
    node.position = gPosition;
    node.radius = averageRadius(&node.source);
    m_centerSources.insert(node.source);
    m_oldAndNewVertices.push_back(node);
    
    std::map<std::pair<size_t, size_t>, std::vector<size_t>> map;
    for (size_t i = 0; i < edgeSegments.size(); ++i) {
        size_t j = (i + 1) % edgeSegments.size();
        size_t m = (j + 2) % edgeSegments.size();
        const auto &segmentI = edgeSegments[i];
        const auto &segmentJ = edgeSegments[j];
        const auto &segmentM = edgeSegments[m];
        std::vector<size_t> firstSegmentPoints;
        std::vector<size_t> secondSegmentPoints;
        std::vector<size_t> forthSegmentPoints;
        collectEdgePoints(std::get<2>(segmentI), std::get<0>(segmentI), std::get<1>(segmentI),
            &firstSegmentPoints);
        collectEdgePoints(std::get<2>(segmentJ), std::get<0>(segmentJ), std::get<1>(segmentJ),
            &secondSegmentPoints);
        collectEdgePoints(std::get<2>(segmentM), std::get<0>(segmentM), std::get<1>(segmentM),
            &forthSegmentPoints);
        if (cornerVertices.find((*m_sourcePolylines)[std::get<2>(segmentI)][std::get<0>(segmentI)]) != cornerVertices.end() &&
                cornerVertices.find((*m_sourcePolylines)[std::get<2>(segmentI)][std::get<1>(segmentI)]) != cornerVertices.end()) {
            std::vector<std::vector<size_t>> region = {
                firstSegmentPoints,
                createPointsToMapBetween((*m_sourcePolylines)[std::get<2>(segmentI)][std::get<1>(segmentI)],
                    gIndex, forthSegmentPoints.size() - 1, &map),
                createPointsToMapBetween(gIndex,
                    (*m_sourcePolylines)[std::get<2>(segmentI)][std::get<0>(segmentI)], secondSegmentPoints.size() - 1, &map)
            };
            m_newRegions.push_back(region);
            continue;
        }
        if (cornerVertices.find((*m_sourcePolylines)[std::get<2>(segmentI)][std::get<0>(segmentI)]) != cornerVertices.end() &&
                cornerVertices.find((*m_sourcePolylines)[std::get<2>(segmentJ)][std::get<1>(segmentJ)]) != cornerVertices.end()) {
            ++i;
            std::vector<std::vector<size_t>> region = {
                firstSegmentPoints,
                secondSegmentPoints,
                createPointsToMapBetween((*m_sourcePolylines)[std::get<2>(segmentJ)][std::get<1>(segmentJ)],
                    gIndex, firstSegmentPoints.size() - 1, &map),
                createPointsToMapBetween(gIndex,
                    (*m_sourcePolylines)[std::get<2>(segmentI)][std::get<0>(segmentI)], secondSegmentPoints.size() - 1, &map)
            };
            m_newRegions.push_back(region);
        }
    }
    
    return true;
}

bool RegionFiller::resolveEvenSidedEvenSumOfSegmentsAndBothL2L2AreOdd()
{
    return resolveOddSidedOddSumOfSegments(m_sideNum / 2);
}

bool RegionFiller::resolveOddSidedOddSumOfSegments(int siUpperBound)
{
    auto si = [&](int i) {
        int sum = 0;
        for (int j = 1; j <= siUpperBound; ++j) {
            sum += std::pow(-1, j + 1) *
                (m_sideSegmentNums[(i + 2 * j - 1) % m_sideSegmentNums.size()] - 1);
        }
        return sum;
    };
    auto di = [&](int i) {
        return std::round(si(i) * 0.5);
    };
    std::vector<int> gRaySegmentNums(m_sideNum);
    for (int i = 0; i < m_sideNum; ++i) {
        gRaySegmentNums[i] = di(i);
        if (gRaySegmentNums[i] <= 0) {
            qDebug() << "resolveOddSidedOddSumOfSegments failed, di:" << gRaySegmentNums[i];
            return false;
        }
    }
    
    std::vector<int> areas(m_sideNum);
    for (int i = 0; i < m_sideNum; ++i) {
        areas[i] = gRaySegmentNums[i] * gRaySegmentNums[(i + 1) % m_sideNum];
    }
    
    std::vector<std::vector<int>> eIndices(m_sideNum);
    for (int i = 0; i < m_sideNum; ++i) {
        auto length = gRaySegmentNums[(i + 1) % gRaySegmentNums.size()];
        eIndices[i] = std::vector<int>{(int)m_sideSegmentNums[i] - length - 1,
            (int)m_sideSegmentNums[i] - length
        };
        if (eIndices[i][0] < 0 || eIndices[i][1] < 0)
            return false;
    }
    std::vector<float> eEdgeLengths(m_sideNum);
    for (int i = 0; i < m_sideNum; ++i) {
        eEdgeLengths[i] = ((*m_sourceVertices)[(*m_sourcePolylines)[i][eIndices[i][0]]].position -
                (*m_sourceVertices)[(*m_sourcePolylines)[i][eIndices[i][1]]].position).length();
    }
    float averageDislocationEdgeLength = std::accumulate(eEdgeLengths.begin(), eEdgeLengths.end(), 0.0) / eEdgeLengths.size();
    
    auto gi = [&](int i) {
        int j = (i + m_sideNum - 1) % m_sideNum;
        return (*m_sourceVertices)[(*m_sourcePolylines)[i][eIndices[i][0]]].position +
                (*m_sourceVertices)[(*m_sourcePolylines)[j][eIndices[j][1]]].position -
            (*m_sourceVertices)[(*m_sourcePolylines)[i][0]].position;
    };
    auto gOffsetTargets = [&](int i) {
        int h = (i + m_sideNum - 1) % m_sideNum;
        return ((*m_sourceVertices)[(*m_sourcePolylines)[i][eIndices[i][0]]].position +
                (*m_sourceVertices)[(*m_sourcePolylines)[h][eIndices[h][1]]].position) * 0.5;
    };
    
    QVector3D gTop;
    float gBottom = 0;
    for (int i = 0; i < m_sideNum; ++i) {
        gTop += gi(i) / areas[i];
        gBottom += 1.0 / areas[i];
    }
    
    auto gPosition = gTop / gBottom;
    size_t gSource = 0;
    auto gRadius = averageRadius(&gSource);
    m_centerSources.insert(gSource);
    
    std::vector<std::vector<std::vector<size_t>>> segmentsWithG(m_sideNum);
    for (int i = 0; i < m_sideNum; ++i) {
        segmentsWithG[i].resize(2);
    }
    std::vector<size_t> gFace(m_sideNum);
    for (int i = 0; i < m_sideNum; ++i) {
        int j = (i + 1) % m_sideNum;
        auto offsetTarget = gOffsetTargets(j);
        auto gPositionPerEdge = gPosition + (offsetTarget - gPosition).normalized() * averageDislocationEdgeLength * 0.7;
        size_t gIndex = m_oldAndNewVertices.size();
        gFace[j] = gIndex;
        Node node;
        node.position = gPositionPerEdge;
        node.radius = gRadius;
        node.source = gSource;
        m_oldAndNewVertices.push_back(node);
        createPointsBetween(gIndex, (*m_sourcePolylines)[i][eIndices[i][1]], gRaySegmentNums[i], &segmentsWithG[i][1]);
        createPointsBetween(gIndex, (*m_sourcePolylines)[j][eIndices[j][0]], gRaySegmentNums[j], &segmentsWithG[j][0]);
    }
    m_newFaces.push_back(gFace);
    
    std::vector<std::vector<std::vector<size_t>>> reversedSegmentsWithG(m_sideNum);
    reversedSegmentsWithG = segmentsWithG;
    for (int i = 0; i < m_sideNum; ++i) {
        for (int x = 0; x < 2; ++x)
            std::reverse(reversedSegmentsWithG[i][x].begin(), reversedSegmentsWithG[i][x].end());
    }
    
    for (int i = 0; i < m_sideNum; ++i) {
        int j = (i + 1) % m_sideNum;
        std::vector<size_t> e0c1;
        collectEdgePoints(i, eIndices[i][1], (*m_sourcePolylines)[i].size() - 1, &e0c1);
        std::vector<size_t> c1e1;
        collectEdgePoints(j, 0, eIndices[j][0], &c1e1);
        std::vector<std::vector<size_t>> region = {
            e0c1,
            c1e1,
            reversedSegmentsWithG[j][0],
            segmentsWithG[i][1]
        };
        m_newRegions.push_back(region);
    }
    
    // rectangle slices
    for (int i = 0; i < m_sideNum; ++i) {
        int j = (i + 1) % m_sideNum;
        std::vector<size_t> e1e2 = {
            (*m_sourcePolylines)[i][eIndices[i][0]],
            (*m_sourcePolylines)[i][eIndices[i][1]]
        };
        std::vector<size_t> gCoreEdge = {
            gFace[j],
            gFace[i]
        };
        std::vector<std::vector<size_t>> region = {
            e1e2,
            reversedSegmentsWithG[i][1],
            gCoreEdge,
            segmentsWithG[i][0],
        };
        m_newRegions.push_back(region);
    }
    
    return true;
}

bool RegionFiller::resolveEvenSideEvenSumOfSegmentsAndL1IsOddL2IsEven()
{
    std::rotate(m_sourcePolylines->begin(), m_sourcePolylines->begin() + 1, m_sourcePolylines->end());
    prepare();
    return resolveEvenSideEvenSumOfSegmentsAndL1IsEvenL2IsOdd();
}

bool RegionFiller::resolveEvenSideEvenSumOfSegmentsAndL1IsEvenL2IsOdd()
{
    auto modifiedSideSegmentNums = m_sideSegmentNums;
    for (int i = 0; i < m_sideNum; i += 2) {
        modifiedSideSegmentNums[i] -= 1;
    }
    
    auto si = [&](int i) {
        int sum = 0;
        int halfSideNum = m_sideNum / 2;
        for (int j = 1; j <= halfSideNum; ++j) {
            sum += std::pow(-1, j + 1) *
                (modifiedSideSegmentNums[(i + 2 * j - 1) % m_sideSegmentNums.size()]);
        }
        return sum;
    };
    auto di = [&](int i) {
        return std::round(si(i) * 0.5);
    };
    std::vector<int> gRaySegmentNums(m_sideNum);
    for (int i = 0; i < m_sideNum; ++i) {
        gRaySegmentNums[i] = di(i);
        if (gRaySegmentNums[i] <= 0) {
            qDebug() << "resolveEvenSideEvenSumOfSegmentsAndL1IsEvenL2IsOdd failed, di:" << gRaySegmentNums[i];
            return false;
        }
    }
    
    std::vector<int> areas(m_sideNum);
    for (int i = 0; i < m_sideNum; ++i) {
        areas[i] = gRaySegmentNums[i] * gRaySegmentNums[(i + 1) % m_sideNum];
    }
    
    std::vector<std::vector<int>> eIndices(m_sideNum);
    for (int i = 0; i < m_sideNum; ++i) {
        auto length = gRaySegmentNums[(i + gRaySegmentNums.size() - 1) % gRaySegmentNums.size()];
        if (0 == i % 2) {
            eIndices[i] = std::vector<int>{(int)length,
                (int)length + 1
            };
        } else {
            eIndices[i] = std::vector<int>{(int)length,
                (int)length
            };
        }
        if (eIndices[i][0] < 0 || eIndices[i][1] < 0) {
            qDebug() << "resolveEvenSideEvenSumOfSegmentsAndL1IsEvenL2IsOdd failed";
            return false;
        }
    }
    std::vector<float> eEdgeLengths(m_sideNum, (float)0.0);
    int validEdgeNum = 0;
    for (int i = 0; i < m_sideNum; ++i) {
        const auto &firstIndex = eIndices[i][0];
        const auto &secondIndex = eIndices[i][1];
        if (firstIndex == secondIndex)
            continue;
        validEdgeNum++;
        eEdgeLengths[i] = ((*m_sourceVertices)[(*m_sourcePolylines)[i][firstIndex]].position -
                (*m_sourceVertices)[(*m_sourcePolylines)[i][secondIndex]].position).length();
    }
    float averageDislocationEdgeLength = std::accumulate(eEdgeLengths.begin(), eEdgeLengths.end(), 0.0) / validEdgeNum;
    
    auto gi = [&](int i) {
        int h = (i + m_sideNum - 1) % m_sideNum;
        return (*m_sourceVertices)[(*m_sourcePolylines)[i][eIndices[i][0]]].position +
                (*m_sourceVertices)[(*m_sourcePolylines)[h][eIndices[h][1]]].position -
            (*m_sourceVertices)[(*m_sourcePolylines)[i][0]].position;
    };
    auto gOffsetTargets = [&](int i) {
        int h = (i + m_sideNum - 1) % m_sideNum;
        return ((*m_sourceVertices)[(*m_sourcePolylines)[i][eIndices[i][0]]].position +
                (*m_sourceVertices)[(*m_sourcePolylines)[h][eIndices[h][1]]].position) * 0.5;
    };
    
    QVector3D gTop;
    float gBottom = 0;
    for (int i = 0; i < m_sideNum; ++i) {
        gTop += gi(i) / areas[i];
        gBottom += 1.0 / areas[i];
    }
    
    auto gPosition = gTop / gBottom;
    size_t gSource = 0;
    auto gRadius = averageRadius(&gSource);
    
    std::vector<size_t> gFace(m_sideNum / 2);
    for (size_t k = 0; k < gFace.size(); ++k) {
        int i = k * 2;
        int h = (i + m_sideNum - 1) % m_sideNum;
        auto offsetTarget = gOffsetTargets(i) * 0.5 + gOffsetTargets(h) * 0.5;
        size_t gIndex = m_oldAndNewVertices.size();
        auto gPositionPerEdge = gPosition + (offsetTarget - gPosition).normalized() * averageDislocationEdgeLength * 0.7;
        Node node;
        node.position = gPositionPerEdge;
        node.radius = gRadius;
        node.source = gSource;
        m_centerSources.insert(node.source);
        m_oldAndNewVertices.push_back(node);
        gFace[k] = gIndex;
    }
    m_newFaces.push_back(gFace);
    
    std::vector<std::vector<std::vector<size_t>>> segmentsWithG(m_sideNum);
    for (int i = 0; i < m_sideNum; ++i) {
        segmentsWithG[i].resize(2);
    }
    for (int i = 0; i < m_sideNum; ++i) {
        int k = i / 2;
        const auto &g0Index = gFace[k % gFace.size()];
        const auto &g1Index = gFace[(k + 1) % gFace.size()];
        if (eIndices[i][0] == eIndices[i][1]) {
            createPointsBetween(g1Index, (*m_sourcePolylines)[i][eIndices[i][0]], gRaySegmentNums[i],
                &segmentsWithG[i][0]);
            segmentsWithG[i][1] = segmentsWithG[i][0];
        } else {
            createPointsBetween(g0Index, (*m_sourcePolylines)[i][eIndices[i][0]], gRaySegmentNums[i],
                &segmentsWithG[i][0]);
            createPointsBetween(g1Index, (*m_sourcePolylines)[i][eIndices[i][1]], gRaySegmentNums[i],
                &segmentsWithG[i][1]);
        }
    }
    
    std::vector<std::vector<std::vector<size_t>>> reversedSegmentsWithG(m_sideNum);
    reversedSegmentsWithG = segmentsWithG;
    for (int i = 0; i < m_sideNum; ++i) {
        for (int x = 0; x < 2; ++x)
            std::reverse(reversedSegmentsWithG[i][x].begin(), reversedSegmentsWithG[i][x].end());
    }
    
    for (int i = 0; i < m_sideNum; ++i) {
        int j = (i + 1) % m_sideNum;
        std::vector<size_t> e0c1;
        collectEdgePoints(i, eIndices[i][1], (*m_sourcePolylines)[i].size() - 1, &e0c1);
        std::vector<size_t> c1e1;
        collectEdgePoints(j, 0, eIndices[j][0], &c1e1);
        std::vector<std::vector<size_t>> region = {
            e0c1, c1e1, reversedSegmentsWithG[j][0], segmentsWithG[i][1]
        };
        m_newRegions.push_back(region);
    }
    
    // Rectangle slices
    for (size_t k = 0; k < gFace.size(); ++k) {
        int i = k * 2;
        std::vector<size_t> e1e2 = {
            (*m_sourcePolylines)[i][eIndices[i][0]],
            (*m_sourcePolylines)[i][eIndices[i][1]]
        };
        std::vector<size_t> g1g0 = {
            gFace[(k + 1) % gFace.size()],
            gFace[k % gFace.size()]
        };
        std::vector<std::vector<size_t>> region = {
            g1g0, segmentsWithG[i][0], e1e2, reversedSegmentsWithG[i][1]
        };
        m_newRegions.push_back(region);
    }
    
    return true;
}

bool RegionFiller::resolveQuadrilateralRegionMismatchSimilarCase(int m, int n, int p, int q, bool pqSwapped)
{
    return resolveQuadrilateralRegionWithIntegerSolution(m, n, p, q, pqSwapped);
}

bool RegionFiller::resolveQuadrilateralRegionWithNonIntegerSolution(int m, int n, int p, int q, bool pqSwapped)
{
    float aPlusB = m_sideSegmentNums[m] - m_sideSegmentNums[n];
    float aFloat = ((m_sideSegmentNums[m] - m_sideSegmentNums[n]) + (m_sideSegmentNums[p] - m_sideSegmentNums[q])) / 2.0;
    float bFloat = (m_sideSegmentNums[m] - m_sideSegmentNums[n]) - aFloat;
    float a = std::ceil(aFloat);
    float b = std::ceil(bFloat);
    
    int c0EdgeIndex = pqSwapped ? (*m_sourcePolylines)[m].size() - 1 : 0;
    int c1EdgeIndex = pqSwapped ? (*m_sourcePolylines)[p].size() - 1 : 0;
    int c2EdgeIndex = pqSwapped ? (*m_sourcePolylines)[n].size() - 1 : 0;
    int c3EdgeIndex = pqSwapped ? (*m_sourcePolylines)[q].size() - 1 : 0;
    
    int c0AlternativeEdgeIndex = pqSwapped ? 0 : (*m_sourcePolylines)[q].size() - 1;
    int c1AlternativeEdgeIndex = pqSwapped ? 0 : (*m_sourcePolylines)[m].size() - 1;
    int c2AlternativeEdgeIndex = pqSwapped ? 0 : (*m_sourcePolylines)[p].size() - 1;
    int c3AlternativeEdgeIndex = pqSwapped ? 0 : (*m_sourcePolylines)[n].size() - 1;
    
    const auto &c0Index = (*m_sourcePolylines)[m][c0EdgeIndex];
    const auto &c1Index = (*m_sourcePolylines)[p][c1EdgeIndex];
    
    const auto &c0 = (*m_sourceVertices)[c0Index].position;
    const auto &c1 = (*m_sourceVertices)[c1Index].position;

    int f0 = m_sideSegmentNums[n] / 2;
    int f1 = m_sideSegmentNums[n] - f0;
    int f2 = std::ceil((m_sideSegmentNums[p] - a) / 2.0);
    int f3 = m_sideSegmentNums[p] - a - f2;
    
    if (f3 < 0) {
        qDebug() << "resolveQuadrilateralRegionWithNonIntegerSolution failed, f3:" << f3;
        return false;
    }
    
    int e0EdgeIndex = f0;
    if (pqSwapped)
        e0EdgeIndex = (*m_sourcePolylines)[m].size() - 1 - e0EdgeIndex;
    const auto &e0Index = (*m_sourcePolylines)[m][e0EdgeIndex];
    
    int e2EdgeIndex = f0 + aPlusB;
    if (pqSwapped)
        e2EdgeIndex = (*m_sourcePolylines)[m].size() - 1 - e2EdgeIndex;
    const auto &e2Index = (*m_sourcePolylines)[m][e2EdgeIndex];
    
    int e3EdgeIndex = f2;
    if (pqSwapped)
        e3EdgeIndex = (*m_sourcePolylines)[p].size() - 1 - e3EdgeIndex;
    const auto &e3Index = (*m_sourcePolylines)[p][e3EdgeIndex];

    int e4EdgeIndex = (*m_sourcePolylines)[p].size() - 1 - f3;
    if (pqSwapped)
        e4EdgeIndex = (*m_sourcePolylines)[p].size() - 1 - e4EdgeIndex;
    const auto &e4Index = (*m_sourcePolylines)[p][e4EdgeIndex];
    
    int e5EdgeIndex = f3;
    if (pqSwapped)
        e5EdgeIndex = (*m_sourcePolylines)[q].size() - 1 - e5EdgeIndex;
    const auto &e5Index = (*m_sourcePolylines)[q][e5EdgeIndex];

    int e6EdgeIndex = (*m_sourcePolylines)[q].size() - 1 - f2;
    if (pqSwapped)
        e6EdgeIndex = (*m_sourcePolylines)[q].size() - 1 - e6EdgeIndex;
    const auto &e6Index = (*m_sourcePolylines)[q][e6EdgeIndex];

    int e7EdgeIndex = f1;
    if (pqSwapped)
        e7EdgeIndex = (*m_sourcePolylines)[n].size() - 1 - e7EdgeIndex;
    const auto &e7Index = (*m_sourcePolylines)[n][e7EdgeIndex];
    
    const auto &e0 = (*m_sourceVertices)[e0Index].position;
    const auto &e2 = (*m_sourceVertices)[e2Index].position;
    const auto &e3 = (*m_sourceVertices)[e3Index].position;
    const auto &e4 = (*m_sourceVertices)[e4Index].position;
    const auto &e5 = (*m_sourceVertices)[e5Index].position;
    const auto &e6 = (*m_sourceVertices)[e6Index].position;
    const auto &e7 = (*m_sourceVertices)[e7Index].position;
    
    auto g1InitialPosition = e0 + e6 - c0;
    auto g2InitialPosition = e2 + e3 - c1;
    auto gpInitialPosition1 = (e5 + e4) * 0.5;
    auto gpInitialPosition2 = gpInitialPosition1;
    auto g1a = f0 * f2;
    auto g2a = f1 * f2;
    auto gp1a = f0 * f3 + f0 * b;
    auto gp2a = f1 * f3 + f1 * a;
    
    float weightG1 = 1.0 / g1a;
    float weightG2 = 1.0 / g2a;
    float weightGp1 = 1.0 / gp1a;
    float weightGp2 = 1.0 / gp2a;
    
    size_t gSource = 0;
    auto gRadius = averageRadius(&gSource);
    
    auto gPosition = (g1InitialPosition * weightG1 +
            g2InitialPosition * weightG2 +
            gpInitialPosition1 * weightGp1 +
            gpInitialPosition2 * weightGp2) /
        (weightG1 + weightG2 + weightGp1 + weightGp2);
    
    auto averageOffsetDistance = e0.distanceToPoint(e2);
    
    auto g1Position = gPosition + (((e0 + e6) * 0.5) - gPosition).normalized() * averageOffsetDistance * 0.7;
    auto g2Position = gPosition + (((e2 + e3) * 0.5) - gPosition).normalized() * averageOffsetDistance * 0.7;
    auto gpPosition = gPosition + (e7 - gPosition).normalized() * averageOffsetDistance * 0.7;
    
    auto g1Index = m_oldAndNewVertices.size();
    {
        Node node;
        node.position = g1Position;
        node.radius = gRadius;
        node.source = gSource;
        m_centerSources.insert(node.source);
        m_oldAndNewVertices.push_back(node);
    }
    
    auto g2Index = m_oldAndNewVertices.size();
    {
        Node node;
        node.position = g2Position;
        node.radius = gRadius;
        node.source = gSource;
        m_centerSources.insert(node.source);
        m_oldAndNewVertices.push_back(node);
    }
    
    auto gpIndex = m_oldAndNewVertices.size();
    if (0 != f3) {
        Node node;
        node.position = gpPosition;
        node.radius = gRadius;
        node.source = gSource;
        m_centerSources.insert(node.source);
        m_oldAndNewVertices.push_back(node);
    } else {
        gpIndex = e7Index;
    }
    
    std::map<std::pair<size_t, size_t>, std::vector<size_t>> map;
    
    {
        std::vector<std::vector<size_t>> region = {
            collectEdgePoints(m, c0EdgeIndex, e0EdgeIndex),
            createPointsToMapBetween(e0Index, g1Index, f2, &map),
            createPointsToMapBetween(g1Index, e6Index, f0, &map),
            collectEdgePoints(q, e6EdgeIndex, c0AlternativeEdgeIndex)
        };
        m_newRegions.push_back(region);
    }
    
    {
        std::vector<std::vector<size_t>> region = {
            collectEdgePoints(m, e0EdgeIndex, e2EdgeIndex),
            createPointsToMapBetween(e2Index, g2Index, f2, &map),
            createPointsToMapBetween(g2Index, g1Index, aPlusB, &map),
            createPointsToMapBetween(g1Index, e0Index, f2, &map)
        };
        m_newRegions.push_back(region);
    }
    
    {
        std::vector<std::vector<size_t>> region = {
            collectEdgePoints(m, e2EdgeIndex, c1AlternativeEdgeIndex),
            collectEdgePoints(p, c1EdgeIndex, e3EdgeIndex),
            createPointsToMapBetween(e3Index, g2Index, f1, &map),
            createPointsToMapBetween(g2Index, e2Index, f2, &map)
        };
        m_newRegions.push_back(region);
    }
    
    if (gpIndex != e7Index) {
        std::vector<std::vector<size_t>> region = {
            createPointsToMapBetween(g2Index, e3Index, f1, &map),
            collectEdgePoints(p, e3EdgeIndex, e4EdgeIndex),
            createPointsToMapBetween(e4Index, gpIndex, f1, &map),
            createPointsToMapBetween(gpIndex, g2Index, a, &map)
        };
        m_newRegions.push_back(region);
    } else {
        std::vector<std::vector<size_t>> region = {
            createPointsToMapBetween(g2Index, e3Index, f1, &map),
            collectEdgePoints(p, e3EdgeIndex, c2AlternativeEdgeIndex),
            collectEdgePoints(n, c2EdgeIndex, e7EdgeIndex),
            createPointsToMapBetween(gpIndex, g2Index, a, &map)
        };
        m_newRegions.push_back(region);
    }
    
    if (gpIndex != e7Index) {
        std::vector<std::vector<size_t>> region = {
            collectEdgePoints(p, e4EdgeIndex, c2AlternativeEdgeIndex),
            collectEdgePoints(n, c2EdgeIndex, e7EdgeIndex),
            createPointsToMapBetween(e7Index, gpIndex, f3, &map),
            createPointsToMapBetween(gpIndex, e4Index, f1, &map)
        };
        m_newRegions.push_back(region);
    }
    
    if (gpIndex != e7Index) {
        std::vector<std::vector<size_t>> region = {
            createPointsToMapBetween(gpIndex, e7Index, f3, &map),
            collectEdgePoints(n, e7EdgeIndex, c3AlternativeEdgeIndex),
            collectEdgePoints(q, c3EdgeIndex, e5EdgeIndex),
            createPointsToMapBetween(e5Index, gpIndex, f0, &map)
        };
        m_newRegions.push_back(region);
    }
    
    if (gpIndex != e7Index) {
        std::vector<std::vector<size_t>> region = {
            createPointsToMapBetween(g1Index, gpIndex, b, &map),
            createPointsToMapBetween(gpIndex, e5Index, f0, &map),
            collectEdgePoints(q, e5EdgeIndex, e6EdgeIndex),
            createPointsToMapBetween(e6Index, g1Index, f0, &map)
        };
        m_newRegions.push_back(region);
    } else {
        std::vector<std::vector<size_t>> region = {
            createPointsToMapBetween(g1Index, gpIndex, b, &map),
            collectEdgePoints(n, e7EdgeIndex, c3AlternativeEdgeIndex),
            collectEdgePoints(q, c3EdgeIndex, e6EdgeIndex),
            createPointsToMapBetween(e6Index, g1Index, f0, &map)
        };
        m_newRegions.push_back(region);
    }
    {
        std::vector<std::vector<size_t>> region = {
            createPointsToMapBetween(g1Index, g2Index, aPlusB, &map),
            createPointsToMapBetween(g2Index, gpIndex, a, &map),
            createPointsToMapBetween(gpIndex, g1Index, b, &map)
        };
        m_newRegions.push_back(region);
    }
    return true;
}

bool RegionFiller::resolveQuadrilateralRegionWithIntegerSolution(int m, int n, int p, int q, bool pqSwapped)
{
    // a + b = m − n
    // a − b = p − q
    // (a + b) + (a - b) = (m - n) + (p - q)
    // a = ((m - n) + (p - q)) / 2

    int a = ((m_sideSegmentNums[m] - m_sideSegmentNums[n]) + (m_sideSegmentNums[p] - m_sideSegmentNums[q])) / 2;
    int b = (m_sideSegmentNums[m] - m_sideSegmentNums[n]) - a;
    
    bool isMismatchSimilar = 0 == b;

    int f0 = m_sideSegmentNums[n] / 2;
    int f1 = m_sideSegmentNums[n] - f0;
    int f2 = (m_sideSegmentNums[q] - b) / 2;
    int f3 = m_sideSegmentNums[q] - b - f2;
    
    if (f3 < 0) {
        qDebug() << "resolveQuadrilateralRegionWithIntegerSolution failed, f3:" << f3;
        return false;
    }

    float a1 = f0 * f2;
    float a2 = f0 * b;
    float a3 = f0 * f3;
    float a4 = f1 * f2;
    float a5 = f1 * a;
    float a6 = f1 * f3;
    float a7 = a * f2;
    float a8 = b * f2;
    float a9 = a * b;
    
    int c0EdgeIndex = pqSwapped ? (*m_sourcePolylines)[m].size() - 1 : 0;
    int c1EdgeIndex = pqSwapped ? (*m_sourcePolylines)[p].size() - 1 : 0;
    int c2EdgeIndex = pqSwapped ? (*m_sourcePolylines)[n].size() - 1 : 0;
    int c3EdgeIndex = pqSwapped ? (*m_sourcePolylines)[q].size() - 1 : 0;
    
    int c0AlternativeEdgeIndex = pqSwapped ? 0 : (*m_sourcePolylines)[q].size() - 1;
    int c1AlternativeEdgeIndex = pqSwapped ? 0 : (*m_sourcePolylines)[m].size() - 1;
    int c2AlternativeEdgeIndex = pqSwapped ? 0 : (*m_sourcePolylines)[p].size() - 1;
    int c3AlternativeEdgeIndex = pqSwapped ? 0 : (*m_sourcePolylines)[n].size() - 1;

    const auto &c0Index = (*m_sourcePolylines)[m][c0EdgeIndex];
    const auto &c1Index = (*m_sourcePolylines)[p][c1EdgeIndex];
    const auto &c2Index = (*m_sourcePolylines)[n][c2EdgeIndex];
    const auto &c3Index = (*m_sourcePolylines)[q][c3EdgeIndex];

    const auto &c0 = (*m_sourceVertices)[c0Index].position;
    const auto &c1 = (*m_sourceVertices)[c1Index].position;
    const auto &c2 = (*m_sourceVertices)[c2Index].position;
    const auto &c3 = (*m_sourceVertices)[c3Index].position;

    int e0EdgeIndex = f0;
    if (pqSwapped)
        e0EdgeIndex = (*m_sourcePolylines)[m].size() - 1 - e0EdgeIndex;
    const auto &e0Index = (*m_sourcePolylines)[m][e0EdgeIndex];

    int e1EdgeIndex = f0 + a;
    if (pqSwapped)
        e1EdgeIndex = (*m_sourcePolylines)[m].size() - 1 - e1EdgeIndex;
    const auto &e1Index = (*m_sourcePolylines)[m][e1EdgeIndex];

    int e2EdgeIndex = f0 + a + b;
    if (pqSwapped)
        e2EdgeIndex = (*m_sourcePolylines)[m].size() - 1 - e2EdgeIndex;
    const auto &e2Index = (*m_sourcePolylines)[m][e2EdgeIndex];

    int e3EdgeIndex = f2;
    if (pqSwapped)
        e3EdgeIndex = (*m_sourcePolylines)[p].size() - 1 - e3EdgeIndex;
    const auto &e3Index = (*m_sourcePolylines)[p][e3EdgeIndex];

    int e4EdgeIndex = f2 + a;
    if (pqSwapped)
        e4EdgeIndex = (*m_sourcePolylines)[p].size() - 1 - e4EdgeIndex;
    const auto &e4Index = (*m_sourcePolylines)[p][e4EdgeIndex];

    int e5EdgeIndex = f3;
    if (pqSwapped)
        e5EdgeIndex = (*m_sourcePolylines)[q].size() - 1 - e5EdgeIndex;
    const auto &e5Index = (*m_sourcePolylines)[q][e5EdgeIndex];

    int e6EdgeIndex = f3 + b;
    if (pqSwapped)
        e6EdgeIndex = (*m_sourcePolylines)[q].size() - 1 - e6EdgeIndex;
    const auto &e6Index = (*m_sourcePolylines)[q][e6EdgeIndex];

    int e7EdgeIndex = f1;
    if (pqSwapped)
        e7EdgeIndex = (*m_sourcePolylines)[n].size() - 1 - e7EdgeIndex;
    const auto &e7Index = (*m_sourcePolylines)[n][e7EdgeIndex];

    const auto &e0 = (*m_sourceVertices)[e0Index].position;
    const auto &e1 = (*m_sourceVertices)[e1Index].position;
    const auto &e2 = (*m_sourceVertices)[e2Index].position;
    const auto &e3 = (*m_sourceVertices)[e3Index].position;
    const auto &e4 = (*m_sourceVertices)[e4Index].position;
    const auto &e5 = (*m_sourceVertices)[e5Index].position;
    const auto &e6 = (*m_sourceVertices)[e6Index].position;
    const auto &e7 = (*m_sourceVertices)[e7Index].position;

    auto g1 = e1 + e5 - c0;
    auto gn = e1 + e4 - c1;
    auto g2 = e4 + e7 - c2;
    auto gp = e5 + e7 - c3;

    auto weight1 = 1.0 / (a1 + a2 + a7 + a9);
    auto weightn = 1.0 / (a4 + a5 + a8 + a9);
    auto weight2 = 1.0 / (a6);
    auto weightp = 1.0 / (a3);

    auto gPosition = (g1 * weight1 + gn * weightn + g2 * weight2 + gp * weightp) /
        (weight1 + weightn + weight2 + weightp);
    
    size_t gSource = 0;
    auto gRadius = averageRadius(&gSource);

    auto averageOffsetDistance = e0.distanceToPoint(e2);
    if (!isMismatchSimilar)
        averageOffsetDistance *= 0.5;
    auto g1Position = gPosition + (e6 - gPosition).normalized() * averageOffsetDistance * 0.7;
    auto gnPosition = gPosition + (e1 - gPosition).normalized() * averageOffsetDistance * 0.7;
    auto g2Position = gPosition + (e3 - gPosition).normalized() * averageOffsetDistance * 0.7;
    auto gpPosition = gPosition + (e7 - gPosition).normalized() * averageOffsetDistance * 0.7;
    
    if (isMismatchSimilar) {
        gpPosition = ((*m_sourceVertices)[e0Index].position +
            (*m_sourceVertices)[e5Index].position +
            (*m_sourceVertices)[e7Index].position +
            (*m_sourceVertices)[e4Index].position) / 4.0;
        gnPosition = ((*m_sourceVertices)[e0Index].position +
            gpPosition +
            (*m_sourceVertices)[e3Index].position +
            (*m_sourceVertices)[c1Index].position) / 4.0;
    }

    size_t g1Index = m_oldAndNewVertices.size();
    if (!isMismatchSimilar) {
        Node node;
        node.position = g1Position;
        node.radius = gRadius;
        node.source = gSource;
        m_centerSources.insert(node.source);
        m_oldAndNewVertices.push_back(node);
    }

    size_t gnIndex = m_oldAndNewVertices.size();
    {
        Node node;
        node.position = gnPosition;
        node.radius = gRadius;
        node.source = gSource;
        m_centerSources.insert(node.source);
        m_oldAndNewVertices.push_back(node);
    }
    
    size_t g2Index = m_oldAndNewVertices.size();
    if (!isMismatchSimilar) {
        Node node;
        node.position = g2Position;
        node.radius = gRadius;
        node.source = gSource;
        m_centerSources.insert(node.source);
        m_oldAndNewVertices.push_back(node);
    }

    size_t gpIndex = m_oldAndNewVertices.size();
    {
        Node node;
        node.position = gpPosition;
        node.radius = gRadius;
        node.source = gSource;
        m_centerSources.insert(node.source);
        m_oldAndNewVertices.push_back(node);
    }
    
    if (isMismatchSimilar) {
        g1Index = gpIndex;
        g2Index = gnIndex;
    }
    
    std::map<std::pair<size_t, size_t>, std::vector<size_t>> map;
    
    std::vector<std::vector<size_t>> region1 = {
        collectEdgePoints(m, c0EdgeIndex, e0EdgeIndex),
        createPointsToMapBetween(e0Index, g1Index, f2, &map),
        createPointsToMapBetween(g1Index, e6Index, f0, &map),
        collectEdgePoints(q, e6EdgeIndex, c0AlternativeEdgeIndex)
    };
    m_newRegions.push_back(region1);
    
    if (e5Index != e6Index) {
        std::vector<std::vector<size_t>> region2 = {
            createPointsToMapBetween(e6Index, g1Index, f0, &map),
            createPointsToMapBetween(g1Index, gpIndex, b, &map),
            createPointsToMapBetween(gpIndex, e5Index, f0, &map),
            collectEdgePoints(q, e5EdgeIndex, e6EdgeIndex)
        };
        m_newRegions.push_back(region2);
    }
    
    std::vector<std::vector<size_t>> region3 = {
        createPointsToMapBetween(e5Index, gpIndex, f0, &map),
        createPointsToMapBetween(gpIndex, e7Index, f3, &map),
        collectEdgePoints(n, e7EdgeIndex, c3AlternativeEdgeIndex),
        collectEdgePoints(q, c3EdgeIndex, e5EdgeIndex)
    };
    m_newRegions.push_back(region3);
    
    std::vector<std::vector<size_t>> region4 = {
        collectEdgePoints(m, e2EdgeIndex, c1AlternativeEdgeIndex),
        collectEdgePoints(p, c1EdgeIndex, e3EdgeIndex),
        createPointsToMapBetween(e3Index, g2Index, f1, &map),
        createPointsToMapBetween(g2Index, e2Index, f2, &map)
    };
    m_newRegions.push_back(region4);
    
    if (e3Index != e4Index) {
        std::vector<std::vector<size_t>> region5 = {
            createPointsToMapBetween(g2Index, e3Index, f1, &map),
            collectEdgePoints(p, e3EdgeIndex, e4EdgeIndex),
            createPointsToMapBetween(e4Index, gpIndex, f1, &map),
            createPointsToMapBetween(gpIndex, g2Index, a, &map)
        };
        m_newRegions.push_back(region5);
    }
    
    std::vector<std::vector<size_t>> region6 = {
        createPointsToMapBetween(gpIndex, e4Index, f1, &map),
        collectEdgePoints(p, e4EdgeIndex, c2AlternativeEdgeIndex),
        collectEdgePoints(n, c2EdgeIndex, e7EdgeIndex),
        createPointsToMapBetween(e7Index, gpIndex, f3, &map)
    };
    m_newRegions.push_back(region6);
    
    if (g1Index != gnIndex && e0Index != e1Index) {
        std::vector<std::vector<size_t>> region7 = {
            collectEdgePoints(m, e0EdgeIndex, e1EdgeIndex),
            createPointsToMapBetween(e1Index, gnIndex, f2, &map),
            createPointsToMapBetween(gnIndex, g1Index, a, &map),
            createPointsToMapBetween(g1Index, e0Index, f2, &map)
        };
        m_newRegions.push_back(region7);
    }
    
    if (g2Index != gnIndex && e1Index != e2Index) {
        std::vector<std::vector<size_t>> region8 = {
            collectEdgePoints(m, e1EdgeIndex, e2EdgeIndex),
            createPointsToMapBetween(e2Index, g2Index, f2, &map),
            createPointsToMapBetween(g2Index, gnIndex, b, &map),
            createPointsToMapBetween(gnIndex, e1Index, f2, &map)
        };
        m_newRegions.push_back(region8);
    }
    
    if (!isMismatchSimilar) {
        std::vector<std::vector<size_t>> region9 = {
            createPointsToMapBetween(g1Index, gnIndex, a, &map),
            createPointsToMapBetween(gnIndex, g2Index, b, &map),
            createPointsToMapBetween(g2Index, gpIndex, a, &map),
            createPointsToMapBetween(gpIndex, g1Index, b, &map)
        };
        m_newRegions.push_back(region9);
    }
    
    return true;
}

bool RegionFiller::resolveQuadrilateralRegion()
{
    int diff02 = std::abs(m_sideSegmentNums[0] - m_sideSegmentNums[2]);
    int diff13 = std::abs(m_sideSegmentNums[1] - m_sideSegmentNums[3]);
    int m, n, p, q;
    if (diff02 >= diff13) {
        if (m_sideSegmentNums[0] > m_sideSegmentNums[2]) {
            m = 0;
            n = 2;
            p = 1;
            q = 3;
        } else {
            m = 2;
            n = 0;
            p = 3;
            q = 1;
        }
    } else {
        if (m_sideSegmentNums[1] > m_sideSegmentNums[3]) {
            m = 1;
            n = 3;
            p = 2;
            q = 0;
        } else {
            m = 3;
            n = 1;
            p = 0;
            q = 2;
        }
    }
    bool pqSwapped = false;
    if (m_sideSegmentNums[p] < m_sideSegmentNums[q]) {
        std::swap(p, q);
        pqSwapped = true;
    }
    if (diff02 != diff13) {
        if (isEven(m_sumOfSegments)) {
            return resolveQuadrilateralRegionWithIntegerSolution(m, n, p, q, pqSwapped);
        } else {
            return resolveQuadrilateralRegionWithNonIntegerSolution(m, n, p, q, pqSwapped);
        }
    } else {
        return resolveQuadrilateralRegionMismatchSimilarCase(m, n, p, q, pqSwapped);
    }
}

const std::vector<RegionFiller::Node> &RegionFiller::getOldAndNewVertices()
{
    return m_oldAndNewVertices;
}

const std::vector<std::vector<size_t>> &RegionFiller::getNewFaces()
{
    return m_newFaces;
}

void RegionFiller::prepare()
{
    m_sideNum = m_sourcePolylines->size();
    m_sideSegmentNums.clear();
    for (const auto &it: *m_sourcePolylines) {
        auto l = (int)it.size() - 1;
        m_sideSegmentNums.push_back(l);
    }
    m_sumOfSegments = std::accumulate(m_sideSegmentNums.begin(), m_sideSegmentNums.end(), 0);
}

bool RegionFiller::resolveQuadrilateralRegionDirectCase()
{
    std::vector<std::vector<size_t>> region = {
        collectEdgePoints(0, 0, (*m_sourcePolylines)[0].size() - 1),
        collectEdgePoints(1, 0, (*m_sourcePolylines)[1].size() - 1),
        collectEdgePoints(2, 0, (*m_sourcePolylines)[2].size() - 1),
        collectEdgePoints(3, 0, (*m_sourcePolylines)[3].size() - 1)
    };
    m_newRegions.push_back(region);
    return true;
}

void RegionFiller::fillWithoutPartition()
{
    m_oldAndNewVertices = *m_sourceVertices;
    m_newFaces.clear();
    convertPolylinesToFaces();
}

bool RegionFiller::fill()
{
    m_oldAndNewVertices = *m_sourceVertices;
    
    prepare();
    
    if (4 == m_sideNum) {
        if (m_sideSegmentNums[0] == m_sideSegmentNums[2] &&
                m_sideSegmentNums[1] == m_sideSegmentNums[3]) {
            if (!resolveQuadrilateralRegionDirectCase()) {
                qDebug() << "resolveQuadrilateralRegionDirectCase failed";
                return false;
            }
        } else {
            if (!resolveQuadrilateralRegion()) {
                qDebug() << "resolveQuadrilateralRegion failed";
                return false;
            }
        }
    } else {
        if (isOdd(m_sideNum)) {
            if (isEven(m_sumOfSegments)) {
                if (!resolveOddSidedEvenSumOfSegments(m_sideNum)) {
                    qDebug() << "resolveOddSidedEvenSumOfSegments failed, m_sideNum:" << m_sideNum;
                    return false;
                }
            } else {
                if (!resolveOddSidedOddSumOfSegments(m_sideNum)) {
                    qDebug() << "resolveOddSidedOddSumOfSegments failed, m_sideNum:" << m_sideNum;
                    return false;
                }
            }
        } else {
            int l1 = 0;
            int l2 = 0;
            
            int halfSideNum = m_sideNum / 2;
            
            for (int i = 1; i <= halfSideNum; ++i) {
                l1 += m_sideSegmentNums[(i * 2 - 1) % m_sideNum];
                l2 += m_sideSegmentNums[(i * 2 - 1 - 1) % m_sideNum];
            }

            if (isEven(l1) && isEven(l2)) {
                if (!resolveEvenSidedEvenSumOfSegmentsAndBothL2L2AreEven()) {
                    qDebug() << "resolveEvenSidedEvenSumOfSegmentsAndBothL2L2AreEven failed";
                    return false;
                }
            } else if (isOdd(l1) && isOdd(l2)) {
                if (!resolveEvenSidedEvenSumOfSegmentsAndBothL2L2AreOdd()) {
                    qDebug() << "resolveEvenSidedEvenSumOfSegmentsAndBothL2L2AreOdd failed";
                    return false;
                }
            } else if (isEven(l1) && isOdd(l2)) {
                if (!resolveEvenSideEvenSumOfSegmentsAndL1IsEvenL2IsOdd()) {
                    qDebug() << "resolveEvenSideEvenSumOfSegmentsAndL1IsEvenL2IsOdd failed";
                    return false;
                }
            } else {
                if (!resolveEvenSideEvenSumOfSegmentsAndL1IsOddL2IsEven()) {
                    qDebug() << "resolveEvenSideEvenSumOfSegmentsAndL1IsOddL2IsEven failed";
                    return false;
                }
            }
        }
    }
    
    if (!convertRegionsToPatches()) {
        qDebug() << "convertRegionsToPatches failed";
        return false;
    }
    
    return true;
}

void RegionFiller::convertRegionsToFaces()
{
    if (m_newRegions.empty()) {
        qDebug() << "No region, should not happen";
        return;
    }
    
    for (const auto &region: m_newRegions) {
        std::vector<size_t> face;
        for (const auto &line: region) {
            for (size_t i = 1; i < line.size(); ++i) {
                face.push_back(line[i]);
            }
        }
        if (face.size() < 3) {
            qDebug() << "Face length invalid:" << face.size();
            continue;
        }
        m_newFaces.push_back(face);
    }
}

void RegionFiller::convertPolylinesToFaces()
{
    if (m_sourcePolylines->empty())
        return;
    
    std::vector<size_t> face;
    for (const auto &line: *m_sourcePolylines) {
        for (size_t i = 1; i < line.size(); ++i) {
            face.push_back(line[i]);
        }
    }
    if (face.size() < 3) {
        return;
    }
    m_newFaces.push_back(face);
}

bool RegionFiller::createCoonsPatchFrom(const std::vector<size_t> &c0,
        const std::vector<size_t> &c1,
        const std::vector<size_t> &d0,
        const std::vector<size_t> &d1,
        bool fillLastTriangle)
{
    auto Lc_Position = [&](int s, int t) {
        float factor = (float)t / d0.size();
        return (1.0 - factor) * m_oldAndNewVertices[c0[s]].position + factor * m_oldAndNewVertices[c1[s]].position;
    };
    auto Ld_Position = [&](int s, int t) {
        float factor = (float)s / c0.size();
        return (1.0 - factor) * m_oldAndNewVertices[d0[t]].position + factor * m_oldAndNewVertices[d1[t]].position;
    };
    auto B_Position = [&](int s, int t) {
        float tFactor = (float)t / d0.size();
        float sFactor = (float)s / c0.size();
        return m_oldAndNewVertices[c0[0]].position * (1.0 - sFactor) * (1.0 - tFactor) +
            m_oldAndNewVertices[c0[c0.size() - 1]].position * sFactor * (1.0 - tFactor) +
            m_oldAndNewVertices[c1[0]].position * (1.0 - sFactor) * tFactor +
            m_oldAndNewVertices[c1[c1.size() - 1]].position * sFactor * tFactor;
    };
    auto C_Position = [&](int s, int t) {
        return Lc_Position(s, t) + Ld_Position(s, t) - B_Position(s, t);
    };
    
    auto Lc_Radius = [&](int s, int t) {
        float factor = (float)t / d0.size();
        return (1.0 - factor) * m_oldAndNewVertices[c0[s]].radius + factor * m_oldAndNewVertices[c1[s]].radius;
    };
    auto Ld_Radius = [&](int s, int t) {
        float factor = (float)s / c0.size();
        return (1.0 - factor) * m_oldAndNewVertices[d0[t]].radius + factor * m_oldAndNewVertices[d1[t]].radius;
    };
    auto B_Radius = [&](int s, int t) {
        float tFactor = (float)t / d0.size();
        float sFactor = (float)s / c0.size();
        return m_oldAndNewVertices[c0[0]].radius * (1.0 - sFactor) * (1.0 - tFactor) +
            m_oldAndNewVertices[c0[c0.size() - 1]].radius * sFactor * (1.0 - tFactor) +
            m_oldAndNewVertices[c1[0]].radius * (1.0 - sFactor) * tFactor +
            m_oldAndNewVertices[c1[c1.size() - 1]].radius * sFactor * tFactor;
    };
    auto C_Radius = [&](int s, int t) {
        return Lc_Radius(s, t) + Ld_Radius(s, t) - B_Radius(s, t);
    };
    
    auto getSource = [&](size_t i, size_t j, float factor) {
        if (m_centerSources.find(m_oldAndNewVertices[j].source) != m_centerSources.end()) {
            return m_oldAndNewVertices[i].source;
        } else if (m_centerSources.find(m_oldAndNewVertices[i].source) != m_centerSources.end()) {
            return m_oldAndNewVertices[j].source;
        } else {
            if (factor > 0.5) {
                return m_oldAndNewVertices[j].source;
            }
            return m_oldAndNewVertices[i].source;
        }
    };
    
    auto Lc_Source = [&](int s, int t) {
        float factor = (float)t / d0.size();
        return getSource(c0[s], c1[s], factor);
    };
    auto C_Source = [&](int s, int t) {
        return Lc_Source(s, t);
    };
    
    std::vector<std::vector<size_t>> grid(c0.size());
    for (int s = 1; s < (int)c0.size() - 1; ++s) {
        grid[s].resize(d0.size());
        for (int t = 1; t < (int)d0.size() - 1; ++t) {
            Node node;
            node.position = C_Position(s, t);
            node.radius = C_Radius(s, t);
            node.source = C_Source(s, t);
            grid[s][t] = m_oldAndNewVertices.size();
            m_oldAndNewVertices.push_back(node);
        }
    }
    grid[0].resize(d0.size());
    grid[c0.size() - 1].resize(d0.size());
    for (size_t i = 0; i < c0.size(); ++i) {
        grid[i][0] = c0[i];
        grid[i][d0.size() - 1] = c1[i];
    }
    for (size_t i = 0; i < d0.size(); ++i) {
        grid[0][i] = d0[i];
        grid[c0.size() - 1][i] = d1[i];
    }
    for (int s = 1; s < (int)c0.size(); ++s) {
        for (int t = 1; t < (int)d0.size(); ++t) {
            std::vector<size_t> face = {
                grid[s - 1][t - 1],
                grid[s - 1][t],
                grid[s][t],
                grid[s][t - 1]
            };
            m_newFaces.push_back(face);
        }
    }
    if (fillLastTriangle) {
        std::vector<size_t> face = {
            grid[c0.size() - 1][d0.size() - 2],
            grid[c0.size() - 2][d0.size() - 2],
            grid[c0.size() - 2][d0.size() - 1]
        };
        m_newFaces.push_back(face);
    }
    return true;
}

bool RegionFiller::createCoonsPatch(const std::vector<std::vector<size_t>> &region)
{
    // https://en.wikipedia.org/wiki/Coons_patch
    if (region.size() != 4) {
        if (region.size() == 3) {
            createCoonsPatchThreeSidedRegion(region);
            return true;
        }
        qDebug() << "Invalid region edges:" << region.size();
        return false;
    }
    const auto &c0 = region[0];
    auto c1 = region[2];
    std::reverse(c1.begin(), c1.end());
    const auto &d1 = region[1];
    auto d0 = region[3];
    std::reverse(d0.begin(), d0.end());
    if (c0.empty() ||
            c0.size() != c1.size() ||
        d0.empty() ||
            d0.size() != d1.size()) {
        qDebug() << "Invalid region size:" << c0.size() << c1.size() << d0.size() << d1.size();
        return false;
    }
    return createCoonsPatchFrom(c0, c1, d0, d1, false);
}

bool RegionFiller::createCoonsPatchThreeSidedRegion(const std::vector<std::vector<size_t>> &region)
{
    if (region.size() != 3) {
        qDebug() << "Not three sided region";
        return false;
    }

    int longest = 0;
    size_t longestLength = region[longest].size();
    for (int i = 1; i < 3; ++i) {
        if (region[i].size() > longestLength) {
            longest = i;
            longestLength = region[i].size();
        }
    }
    
    int c = longest;
    int a = (c + 1) % 3;
    int b = (a + 1) % 3;
    
    auto c0 = region[a];
    std::reverse(c0.begin(), c0.end());
    auto c1 = std::vector<size_t>(region[c].begin(), region[c].begin() + c0.size());
    
    auto d0 = region[b];
    auto d1 = std::vector<size_t>(region[c].begin() + region[c].size() - d0.size(), region[c].begin() + region[c].size());
    std::reverse(d1.begin(), d1.end());
    
    bool fillTriangle = region[c].size() != region[a].size() &&
        region[a].size() != region[b].size();
    if (c0.size() < d0.size()) {
        if (!createCoonsPatchFrom(d0, d1, c0, c1, fillTriangle))
            return false;
    } else {
        if (!createCoonsPatchFrom(c0, c1, d0, d1, fillTriangle))
            return false;
    }
    
    return true;
}

bool RegionFiller::convertRegionsToPatches()
{
    if (m_newRegions.empty()) {
        qDebug() << "No region, should not happen";
        return false;
    }
    for (const auto &region: m_newRegions) {
        if (!createCoonsPatch(region))
            return false;
    }
    
    return true;
}
