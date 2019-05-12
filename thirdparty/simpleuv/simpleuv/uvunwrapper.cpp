#include <map>
#include <unordered_set>
#include <set>
#include <queue>
#include <cmath>
#include <simpleuv/uvunwrapper.h>
#include <simpleuv/parametrize.h>
#include <simpleuv/chartpacker.h>
#include <simpleuv/triangulate.h>
#include <QDebug>

namespace simpleuv 
{

void UvUnwrapper::setMesh(const Mesh &mesh)
{
    m_mesh = mesh;
}

const std::vector<FaceTextureCoords> &UvUnwrapper::getFaceUvs() const
{
    return m_faceUvs;
}

const std::vector<QRectF> &UvUnwrapper::getChartRects() const
{
    return m_chartRects;
}

const std::vector<int> &UvUnwrapper::getChartSourcePartitions() const
{
    return m_chartSourcePartitions;
}

void UvUnwrapper::buildEdgeToFaceMap(const std::vector<Face> &faces, std::map<std::pair<size_t, size_t>, size_t> &edgeToFaceMap)
{
    edgeToFaceMap.clear();
    for (decltype(faces.size()) index = 0; index < faces.size(); ++index) {
        const auto &face = faces[index];
        for (size_t i = 0; i < 3; i++) {
            size_t j = (i + 1) % 3;
            edgeToFaceMap[{face.indices[i], face.indices[j]}] = index;
        }
    }
}

void UvUnwrapper::buildEdgeToFaceMap(const std::vector<size_t> &group, std::map<std::pair<size_t, size_t>, size_t> &edgeToFaceMap)
{
    edgeToFaceMap.clear();
    for (const auto &index: group) {
        const auto &face = m_mesh.faces[index];
        for (size_t i = 0; i < 3; i++) {
            size_t j = (i + 1) % 3;
            edgeToFaceMap[{face.indices[i], face.indices[j]}] = index;
        }
    }
}

void UvUnwrapper::splitPartitionToIslands(const std::vector<size_t> &group, std::vector<std::vector<size_t>> &islands)
{
    std::map<std::pair<size_t, size_t>, size_t> edgeToFaceMap;
    buildEdgeToFaceMap(group, edgeToFaceMap);
    
    std::unordered_set<size_t> processedFaces;
    std::queue<size_t> waitFaces;
    for (const auto &indexInGroup: group) {
        if (processedFaces.find(indexInGroup) != processedFaces.end())
            continue;
        waitFaces.push(indexInGroup);
        std::vector<size_t> island;
        while (!waitFaces.empty()) {
            size_t index = waitFaces.front();
            waitFaces.pop();
            if (processedFaces.find(index) != processedFaces.end())
                continue;
            const auto &face = m_mesh.faces[index];
            for (size_t i = 0; i < 3; i++) {
                size_t j = (i + 1) % 3;
                auto findOppositeFaceResult = edgeToFaceMap.find({face.indices[j], face.indices[i]});
                if (findOppositeFaceResult == edgeToFaceMap.end())
                    continue;
                waitFaces.push(findOppositeFaceResult->second);
            }
            island.push_back(index);
            processedFaces.insert(index);
        }
        if (island.empty())
            continue;
        islands.push_back(island);
    }
}

double UvUnwrapper::distanceBetweenVertices(const Vertex &first, const Vertex &second)
{
    float x = first.xyz[0] - second.xyz[0];
    float y = first.xyz[1] - second.xyz[1];
    float z = first.xyz[2] - second.xyz[2];
    return std::sqrt(x*x + y*y + z*z);
}

void UvUnwrapper::calculateFaceTextureBoundingBox(const std::vector<FaceTextureCoords> &faceTextureCoords,
        float &left, float &top, float &right, float &bottom)
{
    bool leftFirstTime = true;
    bool topFirstTime = true;
    bool rightFirstTime = true;
    bool bottomFirstTime = true;
    for (const auto &item: faceTextureCoords) {
        for (int i = 0; i < 3; ++i) {
            const auto &x = item.coords[i].uv[0];
            const auto &y = item.coords[i].uv[1];
            if (leftFirstTime || x < left) {
                left = x;
                leftFirstTime = false;
            }
            if (rightFirstTime || x > right) {
                right = x;
                rightFirstTime = false;
            }
            if (topFirstTime || y < top) {
                top = y;
                topFirstTime = false;
            }
            if (bottomFirstTime || y > bottom) {
                bottom = y;
                bottomFirstTime = false;
            }
        }
    }
}

void UvUnwrapper::triangulateRing(const std::vector<Vertex> &verticies,
        std::vector<Face> &faces, const std::vector<size_t> &ring)
{
    triangulate(verticies, faces, ring);
}

// The hole filling faces should be put in the back of faces vector, so these uv coords of appended faces will be disgarded.
bool UvUnwrapper::fixHolesExceptTheLongestRing(const std::vector<Vertex> &verticies, std::vector<Face> &faces, size_t *remainingHoleNum)
{
    std::map<std::pair<size_t, size_t>, size_t> edgeToFaceMap;
    buildEdgeToFaceMap(faces, edgeToFaceMap);
    
    std::map<size_t, std::vector<size_t>> holeVertexLink;
    for (const auto &face: faces) {
        for (size_t i = 0; i < 3; i++) {
            size_t j = (i + 1) % 3;
            auto findOppositeFaceResult = edgeToFaceMap.find({face.indices[j], face.indices[i]});
            if (findOppositeFaceResult != edgeToFaceMap.end())
                continue;
            holeVertexLink[face.indices[j]].push_back(face.indices[i]);
        }
    }
    
    std::vector<std::pair<std::vector<size_t>, double>> holeRings;
    while (!holeVertexLink.empty()) {
        bool foundRing = false;
        std::vector<size_t> ring;
        std::unordered_set<size_t> visited;
        std::set<std::pair<size_t, size_t>> visitedPath;
        double ringLength = 0;
        while (!foundRing) {
            ring.clear();
            visited.clear();
            ringLength = 0;
            auto first = holeVertexLink.begin()->first;
            auto index = first;
            auto prev = first;
            ring.push_back(first);
            visited.insert(first);
            while (true) {
                auto findLinkResult = holeVertexLink.find(index);
                if (findLinkResult == holeVertexLink.end()) {
                    qDebug() << "Search ring failed";
                    return false;
                }
                for (const auto &item: findLinkResult->second) {
                    if (item == first) {
                        foundRing = true;
                        break;
                    }
                }
                if (foundRing)
                    break;
                if (findLinkResult->second.size() > 1) {
                    bool foundNewPath = false;
                    for (const auto &item: findLinkResult->second) {
                        if (visitedPath.find({prev, item}) == visitedPath.end()) {
                            index = item;
                            foundNewPath = true;
                            break;
                        }
                    }
                    if (!foundNewPath) {
                        qDebug() << "No new path to try";
                        return false;
                    }
                    visitedPath.insert({prev, index});
                } else {
                    index = *findLinkResult->second.begin();
                }
                if (visited.find(index) != visited.end()) {
                    while (index != *ring.begin())
                        ring.erase(ring.begin());
                    foundRing = true;
                    break;
                }
                ring.push_back(index);
                visited.insert(index);
                ringLength += distanceBetweenVertices(verticies[index], verticies[prev]);
                prev = index;
            }
        }
        if (ring.size() < 3) {
            qDebug() << "Ring too short, size:" << ring.size();
            return false;
        }
        holeRings.push_back({ring, ringLength});
        for (size_t i = 0; i < ring.size(); ++i) {
            size_t j = (i + 1) % ring.size();
            auto findLinkResult = holeVertexLink.find(ring[i]);
            for (auto it = findLinkResult->second.begin(); it != findLinkResult->second.end(); ++it) {
                if (*it == ring[j]) {
                    findLinkResult->second.erase(it);
                    if (findLinkResult->second.empty())
                        holeVertexLink.erase(ring[i]);
                    break;
                }
            }
        }
    }
    
    if (holeRings.size() > 1) {
        // Sort by ring length, the longer ring sit in the lower array indices
        std::sort(holeRings.begin(), holeRings.end(), [](const std::pair<std::vector<size_t>, double> &first, const std::pair<std::vector<size_t>, double> &second) {
            return first.second > second.second;
        });
        for (size_t i = 1; i < holeRings.size(); ++i) {
            triangulateRing(verticies, faces, holeRings[i].first);
        }
        holeRings.resize(1);
    }
    
    if (remainingHoleNum)
        *remainingHoleNum = holeRings.size();
    return true;
}

void UvUnwrapper::makeSeamAndCut(const std::vector<Vertex> &verticies,
        const std::vector<Face> &faces,
        std::map<size_t, size_t> &localToGlobalFacesMap,
        std::vector<size_t> &firstGroup, std::vector<size_t> &secondGroup)
{
    // We group the chart by first pick the top(max y) triangle, then join the adjecent traigles until the joint count reach to half of total
    
    double maxY = 0;
    bool firstTime = true;
    int choosenIndex = -1;
    for (decltype(faces.size()) i = 0; i < faces.size(); ++i) {
        const auto &face = faces[i];
        for (int j = 0; j < 3; ++j) {
            const auto &vertex = verticies[face.indices[j]];
            if (firstTime || vertex.xyz[2] > maxY) {
                maxY = vertex.xyz[2];
                firstTime = false;
                choosenIndex = i;
            }
        }
    }
    if (-1 == choosenIndex)
        return;
    
    std::map<std::pair<size_t, size_t>, size_t> edgeToFaceMap;
    buildEdgeToFaceMap(faces, edgeToFaceMap);
    
    std::unordered_set<size_t> processedFaces;
    std::queue<size_t> waitFaces;
    waitFaces.push(choosenIndex);
    while (!waitFaces.empty()) {
        auto index = waitFaces.front();
        waitFaces.pop();
        if (processedFaces.find(index) != processedFaces.end())
            continue;
        const auto &face = faces[index];
        for (size_t i = 0; i < 3; i++) {
            size_t j = (i + 1) % 3;
            auto findOppositeFaceResult = edgeToFaceMap.find({face.indices[j], face.indices[i]});
            if (findOppositeFaceResult == edgeToFaceMap.end())
                continue;
            waitFaces.push(findOppositeFaceResult->second);
        }
        processedFaces.insert(index);
        firstGroup.push_back(localToGlobalFacesMap[index]);
        if (firstGroup.size() * 2 >= faces.size())
            break;
    }
    for (decltype(faces.size()) index = 0; index < faces.size(); ++index) {
        if (processedFaces.find(index) != processedFaces.end())
            continue;
        secondGroup.push_back(localToGlobalFacesMap[index]);
    }
}

void UvUnwrapper::calculateSizeAndRemoveInvalidCharts()
{
    auto charts = m_charts;
    auto chartSourcePartitions = m_chartSourcePartitions;
    m_charts.clear();
    chartSourcePartitions.clear();
    for (size_t chartIndex = 0; chartIndex < charts.size(); ++chartIndex) {
        auto &chart = charts[chartIndex];
        float left, top, right, bottom;
        left = top = right = bottom = 0;
        calculateFaceTextureBoundingBox(chart.second, left, top, right, bottom);
        std::pair<float, float> size = {right - left, bottom - top};
        if (size.first <= 0 || std::isnan(size.first) || std::isinf(size.first) ||
                size.second <= 0 || std::isnan(size.second) || std::isinf(size.second)) {
            qDebug() << "Found invalid chart size:" << size.first << "x" << size.second;
            continue;
        }
        for (auto &item: chart.second) {
            for (int i = 0; i < 3; ++i) {
                item.coords[i].uv[0] -= left;
                item.coords[i].uv[1] -= top;
            }
        }
        //qDebug() << "left:" << left << "top:" << top << "right:" << right << "bottom:" << bottom;
        //qDebug() << "width:" << size.first << "height:" << size.second;
        m_chartSizes.push_back(size);
        m_charts.push_back(chart);
        m_chartSourcePartitions.push_back(chartSourcePartitions[chartIndex]);
    }
}

void UvUnwrapper::packCharts()
{
    ChartPacker chartPacker;
    chartPacker.setCharts(m_chartSizes);
    chartPacker.pack();
    m_chartRects.resize(m_chartSizes.size());
    const std::vector<std::tuple<float, float, float, float, bool>> &packedResult = chartPacker.getResult();
    for (decltype(m_charts.size()) i = 0; i < m_charts.size(); ++i) {
        const auto &chartSize = m_chartSizes[i];
        auto &chart = m_charts[i];
        if (i >= packedResult.size()) {
            for (auto &item: chart.second) {
                for (int i = 0; i < 3; ++i) {
                    item.coords[i].uv[0] = 0;
                    item.coords[i].uv[1] = 0;
                }
            }
            continue;
        }
        const auto &result = packedResult[i];
        auto &left = std::get<0>(result);
        auto &top = std::get<1>(result);
        auto &width = std::get<2>(result);
        auto &height = std::get<3>(result);
        auto &flipped = std::get<4>(result);
        if (flipped)
            m_chartRects[i] = {left, top, height, width};
        else
            m_chartRects[i] = {left, top, width, height};
        if (flipped) {
            for (auto &item: chart.second) {
                for (int i = 0; i < 3; ++i) {
                    std::swap(item.coords[i].uv[0], item.coords[i].uv[1]);
                }
            }
        }
        for (auto &item: chart.second) {
            for (int i = 0; i < 3; ++i) {
                item.coords[i].uv[0] /= chartSize.first;
                item.coords[i].uv[1] /= chartSize.second;
                item.coords[i].uv[0] *= width;
                item.coords[i].uv[1] *= height;
                item.coords[i].uv[0] += left;
                item.coords[i].uv[1] += top;
            }
        }
    }
}

void UvUnwrapper::finalizeUv()
{
    m_faceUvs.resize(m_mesh.faces.size());
    for (const auto &chart: m_charts) {
        for (decltype(chart.second.size()) i = 0; i < chart.second.size(); ++i) {
            auto &faceUv = m_faceUvs[chart.first[i]];
            faceUv = chart.second[i];
        }
    }
}

void UvUnwrapper::partition()
{
    m_partitions.clear();
    if (m_mesh.facePartitions.empty()) {
        for (decltype(m_mesh.faces.size()) i = 0; i < m_mesh.faces.size(); i++) {
            m_partitions[0].push_back(i);
        }
    } else {
        for (decltype(m_mesh.faces.size()) i = 0; i < m_mesh.faces.size(); i++) {
            int partition = m_mesh.facePartitions[i];
            m_partitions[partition].push_back(i);
        }
    }
}

void UvUnwrapper::unwrapSingleIsland(const std::vector<size_t> &group, int sourcePartition, bool skipCheckHoles)
{
    if (group.empty())
        return;
    
    std::vector<Vertex> localVertices;
    std::vector<Face> localFaces;
    std::map<size_t, size_t> globalToLocalVerticesMap;
    std::map<size_t, size_t> localToGlobalFacesMap;
    for (decltype(group.size()) i = 0; i < group.size(); ++i) {
        const auto &globalFace = m_mesh.faces[group[i]];
        Face localFace;
        for (size_t j = 0; j < 3; j++) {
            int globalVertexIndex = globalFace.indices[j];
            if (globalToLocalVerticesMap.find(globalVertexIndex) == globalToLocalVerticesMap.end()) {
                localVertices.push_back(m_mesh.vertices[globalVertexIndex]);
                globalToLocalVerticesMap[globalVertexIndex] = (int)localVertices.size() - 1;
            }
            localFace.indices[j] = globalToLocalVerticesMap[globalVertexIndex];
        }
        localFaces.push_back(localFace);
        localToGlobalFacesMap[localFaces.size() - 1] = group[i];
    }

    //if (skipCheckHoles) {
    //    parametrizeSingleGroup(localVertices, localFaces, localToGlobalFacesMap, localFaces.size());
    //    return;
    //}

    decltype(localFaces.size()) faceNumBeforeFix = localFaces.size();
    size_t remainingHoleNumAfterFix = 0;
    if (!fixHolesExceptTheLongestRing(localVertices, localFaces, &remainingHoleNumAfterFix)) {
        qDebug() << "fixHolesExceptTheLongestRing failed";
        return;
    }
    if (1 == remainingHoleNumAfterFix) {
        parametrizeSingleGroup(localVertices, localFaces, localToGlobalFacesMap, faceNumBeforeFix, sourcePartition);
        return;
    }
    
    if (0 == remainingHoleNumAfterFix) {
        std::vector<size_t> firstGroup;
        std::vector<size_t> secondGroup;
        makeSeamAndCut(localVertices, localFaces, localToGlobalFacesMap, firstGroup, secondGroup);
        if (firstGroup.empty() || secondGroup.empty()) {
            qDebug() << "Cut mesh failed";
            return;
        }
        unwrapSingleIsland(firstGroup, sourcePartition, true);
        unwrapSingleIsland(secondGroup, sourcePartition, true);
        return;
    }
}

void UvUnwrapper::parametrizeSingleGroup(const std::vector<Vertex> &verticies,
        const std::vector<Face> &faces,
        std::map<size_t, size_t> &localToGlobalFacesMap,
        size_t faceNumToChart,
        int sourcePartition)
{
    std::vector<TextureCoord> localVertexUvs;
    if (!parametrize(verticies, faces, localVertexUvs))
        return;
    std::pair<std::vector<size_t>, std::vector<FaceTextureCoords>> chart;
    for (size_t i = 0; i < faceNumToChart; ++i) {
        const auto &localFace = faces[i];
        auto globalFaceIndex = localToGlobalFacesMap[i];
        FaceTextureCoords faceUv;
        for (size_t j = 0; j < 3; j++) {
            const auto &localVertexIndex = localFace.indices[j];
            const auto &vertexUv = localVertexUvs[localVertexIndex];
            faceUv.coords[j].uv[0] = vertexUv.uv[0];
            faceUv.coords[j].uv[1] = vertexUv.uv[1];
        }
        chart.first.push_back(globalFaceIndex);
        chart.second.push_back(faceUv);
    }
    if (chart.first.empty())
        return;
    m_charts.push_back(chart);
    m_chartSourcePartitions.push_back(sourcePartition);
}

void UvUnwrapper::unwrap()
{
    partition();

    m_faceUvs.resize(m_mesh.faces.size());
    for (const auto &group: m_partitions) {
        std::vector<std::vector<size_t>> islands;
        splitPartitionToIslands(group.second, islands);
        for (const auto &island: islands)
            unwrapSingleIsland(island, group.first);
    }
    
    calculateSizeAndRemoveInvalidCharts();
    packCharts();
    finalizeUv();
}

}
