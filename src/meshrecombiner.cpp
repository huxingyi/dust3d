#include <set>
#include <QDebug>
#include <cmath>
#include <queue>
#include "meshrecombiner.h"
#include "positionkey.h"
#include "meshwrapper.h"
#include "util.h"

#define MAX_EDGE_LOOP_LENGTH            1000

void MeshRecombiner::setVertices(const std::vector<QVector3D> *vertices,
    const std::vector<std::pair<MeshCombiner::Source, size_t>> *verticesSourceIndices)
{
    m_vertices = vertices;
    m_verticesSourceIndices = verticesSourceIndices;
}

void MeshRecombiner::setFaces(const std::vector<std::vector<size_t>> *faces)
{
    m_faces = faces;
}

bool MeshRecombiner::convertHalfEdgesToEdgeLoops(const std::vector<std::pair<size_t, size_t>> &halfEdges,
    std::vector<std::vector<size_t>> *edgeLoops)
{
    std::map<size_t, size_t> vertexLinkMap;
    for (const auto &halfEdge: halfEdges) {
        auto inserResult = vertexLinkMap.insert(halfEdge);
        if (!inserResult.second) {
            //qDebug() << "Create edge loop from half edge failed, found repeated vertex link" << halfEdge.first << "->" << halfEdge.second << "exist:" << inserResult.first->first << "->" << inserResult.first->second;
            return false;
        }
    }
    while (!vertexLinkMap.empty()) {
        std::vector<size_t> edgeLoop;
        size_t vertex = vertexLinkMap.begin()->first;
        size_t head = vertex;
        bool loopBack = false;
        size_t limitLoop = MAX_EDGE_LOOP_LENGTH;
        while ((limitLoop--) > 0) {
            edgeLoop.push_back(vertex);
            auto findNext = vertexLinkMap.find(vertex);
            if (findNext == vertexLinkMap.end())
                break;
            vertex = findNext->second;
            if (vertex == head) {
                loopBack = true;
                break;
            }
        }
        if (!loopBack) {
            //qDebug() << "Create edge loop from half edge failed, edge doesn't loop back";
            return false;
        }
        if (edgeLoop.size() < 3) {
            //qDebug() << "Create edge loop from half edge failed, edge loop size invalid:" << edgeLoop.size();
            return false;
        }
        for (const auto &vertex: edgeLoop) {
            vertexLinkMap.erase(vertex);
        }
        edgeLoops->push_back(edgeLoop);
    }
    return true;
}

size_t MeshRecombiner::splitSeamVerticesToIslands(const std::map<size_t, std::vector<size_t>> &seamEdges,
        std::map<size_t, size_t> *vertexToIslandMap)
{
    std::set<size_t> visited;
    size_t nextIslandId = 0;
    for (const auto &it: seamEdges) {
        std::queue<size_t> vertices;
        vertices.push(it.first);
        bool hasVertexJoin = false;
        while (!vertices.empty()) {
            auto v = vertices.front();
            vertices.pop();
            if (visited.find(v) != visited.end())
                continue;
            visited.insert(v);
            vertexToIslandMap->insert({v, nextIslandId});
            hasVertexJoin = true;
            const auto findNeighbors = seamEdges.find(v);
            if (findNeighbors != seamEdges.end()) {
                for (const auto &neighbor: findNeighbors->second) {
                    vertices.push(neighbor);
                }
            }
        }
        if (hasVertexJoin)
            ++nextIslandId;
    }
    return nextIslandId;
}

bool MeshRecombiner::buildHalfEdgeToFaceMap(std::map<std::pair<size_t, size_t>, size_t> &halfEdgeToFaceMap)
{
    bool succeed = true;
    for (size_t faceIndex = 0; faceIndex < m_faces->size(); ++faceIndex) {
        const auto &face = (*m_faces)[faceIndex];
        for (size_t i = 0; i < face.size(); ++i) {
            size_t j = (i + 1) % face.size();
            const auto insertResult = halfEdgeToFaceMap.insert({{face[i], face[j]}, faceIndex});
            if (!insertResult.second) {
                //qDebug() << "Non manifold edge found:" << face[i] << face[j];
                succeed = false;
            }
        }
    }
    return succeed;
}

bool MeshRecombiner::recombine()
{
    buildHalfEdgeToFaceMap(m_halfEdgeToFaceMap);
    
    std::map<size_t, std::vector<size_t>> seamLink;
    for (const auto &face: *m_faces) {
        for (size_t i = 0; i < face.size(); ++i) {
            const auto &index = face[i];
            auto source = (*m_verticesSourceIndices)[index];
            if (MeshCombiner::Source::None == source.first) {
                auto next = face[(i + 1) % face.size()];
                auto nextSource = (*m_verticesSourceIndices)[next];
                if (MeshCombiner::Source::None == nextSource.first) {
                    seamLink[index].push_back(next);
                }
            }
        }
    }
    std::map<size_t, size_t> seamVertexToIslandMap;
    size_t islands = splitSeamVerticesToIslands(seamLink, &seamVertexToIslandMap);
    //qDebug() << "Seam islands:" << islands;
    
    std::map<std::pair<size_t, size_t>, std::pair<size_t, bool>> edgesInSeamArea;
    for (size_t faceIndex = 0; faceIndex < (*m_faces).size(); ++faceIndex) {
        const auto &face = (*m_faces)[faceIndex];
        bool containsSeamVertex = false;
        bool inFirstGroup = false;
        size_t island = 0;
        for (size_t i = 0; i < face.size(); ++i) {
            const auto &index = face[i];
            auto source = (*m_verticesSourceIndices)[index];
            if (MeshCombiner::Source::None == source.first) {
                const auto &findIsland = seamVertexToIslandMap.find(index);
                if (findIsland != seamVertexToIslandMap.end()) {
                    containsSeamVertex = true;
                    island = findIsland->second;
                }
            } else if (MeshCombiner::Source::First == source.first) {
                inFirstGroup = true;
            }
        }
        if (containsSeamVertex) {
            m_facesInSeamArea.insert({faceIndex, island});
            for (size_t i = 0; i < face.size(); ++i) {
                const auto &index = face[i];
                const auto &next = face[(i + 1) % face.size()];
                std::pair<size_t, size_t> edge = {index, next};
                edgesInSeamArea.insert({edge, {island, inFirstGroup}});
            }
        }
    }
    
    struct IslandData
    {
        std::vector<std::pair<size_t, size_t>> halfedges[2];
        std::vector<std::vector<size_t>> edgeLoops[2];
    };
    std::map<size_t, IslandData> islandsMap;
    
    for (const auto &edge: edgesInSeamArea) {
        if (edgesInSeamArea.find({edge.first.second, edge.first.first}) == edgesInSeamArea.end()) {
            islandsMap[edge.second.first].halfedges[edge.second.second ? 0 : 1].push_back(edge.first);
        }
    }
    for (auto &it: islandsMap) {
        for (size_t side = 0; side < 2; ++side) {
            if (!convertHalfEdgesToEdgeLoops(it.second.halfedges[side], &it.second.edgeLoops[side])) {
                //qDebug() << "Convert half edges to edge loops for island" << it.first << "side" << side << "failed";
                it.second.edgeLoops[side].clear();
            }
        }
    }
    
    for (auto &it: islandsMap) {
        for (size_t side = 0; side < 2; ++side) {
            for (size_t i = 0; i < it.second.edgeLoops[side].size(); ++i) {
                auto &edgeLoop = it.second.edgeLoops[side][i];
                size_t totalAdjustedTriangles = 0;
                size_t adjustedTriangles = 0;
                while ((adjustedTriangles=adjustTrianglesFromSeam(edgeLoop, it.first)) > 0) {
                    totalAdjustedTriangles += adjustedTriangles;
                }
                //qDebug() << "Island" << it.first << "side" << side << "edge loop" << i << "adjusted" << totalAdjustedTriangles << "triangles";
            }
        }
    }
    
    for (auto &it: islandsMap) {
        if (1 == it.second.edgeLoops[0].size() &&
                it.second.edgeLoops[0].size() == it.second.edgeLoops[1].size()) {
            if (bridge(it.second.edgeLoops[0][0], it.second.edgeLoops[1][0])) {
                m_goodSeams.insert(it.first);
            }
        }
    }
    //for (auto &it: islandsMap) {
    //    m_goodSeams.insert(it.first);
    //}
    
    copyNonSeamFacesAsRegenerated();
    removeReluctantVertices();
    
    //qDebug() << "Optimized" << m_goodSeams.size() << "seams";
    
    return true;
}

size_t MeshRecombiner::adjustTrianglesFromSeam(std::vector<size_t> &edgeLoop, size_t seamIndex)
{
    if (edgeLoop.size() <= 3)
        return 0;

    std::vector<size_t> halfEdgeToFaces;
    for (size_t i = 0; i < edgeLoop.size(); ++i) {
        size_t j = (i + 1) % edgeLoop.size();
        auto findFace = m_halfEdgeToFaceMap.find({edgeLoop[j], edgeLoop[i]});
        if (findFace == m_halfEdgeToFaceMap.end()) {
            qDebug() << "Find face for half edge failed:" << edgeLoop[j] << edgeLoop[i];
            return 0;
        }
        halfEdgeToFaces.push_back(findFace->second);
    }
    
    std::vector<size_t> removedFaceIndices;
    std::set<size_t> ignored;
    for (size_t i = 0; i < edgeLoop.size(); ++i) {
        size_t j = (i + 1) % edgeLoop.size();
        if (halfEdgeToFaces[i] == halfEdgeToFaces[j]) {
            removedFaceIndices.push_back(halfEdgeToFaces[i]);
            ignored.insert(edgeLoop[j]);
            ++i;
            continue;
        }
    }
    
    if (!ignored.empty()) {
        std::vector<size_t> newEdgeLoop;
        for (const auto &v: edgeLoop) {
            if (ignored.find(v) != ignored.end())
                continue;
            newEdgeLoop.push_back(v);
        }
        if (newEdgeLoop.size() < 3)
            return 0;
        edgeLoop = newEdgeLoop;
        for (const auto &faceIndex: removedFaceIndices)
            m_facesInSeamArea.insert({faceIndex, seamIndex});
    }
    
    return ignored.size();
}

size_t MeshRecombiner::otherVertexOfTriangle(const std::vector<size_t> &face, const std::vector<size_t> &indices)
{
    for (const auto &v: face) {
        for (const auto &u: indices) {
            if (u != v)
                return u;
        }
    }
    return face[0];
}

void MeshRecombiner::copyNonSeamFacesAsRegenerated()
{
    for (size_t faceIndex = 0; faceIndex < m_faces->size(); ++faceIndex) {
        const auto &findFaceInSeam = m_facesInSeamArea.find(faceIndex);
        if (findFaceInSeam != m_facesInSeamArea.end() &&
                m_goodSeams.find(findFaceInSeam->second) != m_goodSeams.end())
            continue;
        m_regeneratedFaces.push_back((*m_faces)[faceIndex]);
    }
}

const std::vector<QVector3D> &MeshRecombiner::regeneratedVertices()
{
    return m_regeneratedVertices;
}

const std::vector<std::pair<MeshCombiner::Source, size_t>> &MeshRecombiner::regeneratedVerticesSourceIndices()
{
    return m_regeneratedVerticesSourceIndices;
}

const std::vector<std::vector<size_t>> &MeshRecombiner::regeneratedFaces()
{
    return m_regeneratedFaces;
}

size_t MeshRecombiner::nearestIndex(const QVector3D &position, const std::vector<size_t> &edgeLoop)
{
    float minDist2 = std::numeric_limits<float>::max();
    size_t choosenIndex = 0;
    for (size_t i = 0; i < edgeLoop.size(); ++i) {
        float dist2 = ((*m_vertices)[edgeLoop[i]] - position).lengthSquared();
        if (dist2 < minDist2) {
            minDist2 = dist2;
            choosenIndex = i;
        }
    }
    return choosenIndex;
}

bool MeshRecombiner::bridge(const std::vector<size_t> &first, const std::vector<size_t> &second)
{
    const std::vector<size_t> *large = &first;
    const std::vector<size_t> *small = &second;
    if (large->size() < small->size())
        std::swap(large, small);
    std::vector<std::pair<size_t, size_t>> matchedPairs;
    std::map<size_t, size_t> nearestIndicesFromLargeToSmall;
    for (size_t i = 0; i < small->size(); ++i) {
        const auto &positionOnSmall = (*m_vertices)[(*small)[i]];
        size_t nearestIndexOnLarge = nearestIndex(positionOnSmall, *large);
        auto matchResult = nearestIndicesFromLargeToSmall.find(nearestIndexOnLarge);
        size_t nearestIndexOnSmall;
        if (matchResult == nearestIndicesFromLargeToSmall.end()) {
            const auto &positionOnLarge = (*m_vertices)[(*large)[nearestIndexOnLarge]];
            nearestIndexOnSmall = nearestIndex(positionOnLarge, *small);
            nearestIndicesFromLargeToSmall.insert({nearestIndexOnLarge, nearestIndexOnSmall});
        } else {
            nearestIndexOnSmall = matchResult->second;
        }
        if (nearestIndexOnSmall == i) {
            matchedPairs.push_back({nearestIndexOnSmall, nearestIndexOnLarge});
        }
    }
    
    //qDebug() << "small:" << small->size() << "large:" << large->size() << "matches:" << matchedPairs.size();
    
    if (matchedPairs.empty())
        return false;
    
    for (size_t i = 0; i < matchedPairs.size(); ++i) {
        size_t j = (i + 1) % matchedPairs.size();
        std::vector<size_t> smallSide;
        std::vector<size_t> largeSide;
        for (size_t indexOnSmall = matchedPairs[i].first;
                ;
                indexOnSmall = (indexOnSmall + 1) % small->size()) {
            smallSide.push_back((*small)[indexOnSmall]);
            if (indexOnSmall == matchedPairs[j].first)
                break;
        }
        for (size_t indexOnLarge = matchedPairs[j].second;
                ;
                indexOnLarge = (indexOnLarge + 1) % large->size()) {
            largeSide.push_back((*large)[indexOnLarge]);
            if (indexOnLarge == matchedPairs[i].second)
                break;
        }
        std::reverse(largeSide.begin(), largeSide.end());
        fillPairs(smallSide, largeSide);
    }
    
    return true;
}

void MeshRecombiner::fillPairs(const std::vector<size_t> &small, const std::vector<size_t> &large)
{
    size_t smallIndex = 0;
    size_t largeIndex = 0;
    while (smallIndex + 1 < small.size() ||
            largeIndex + 1 < large.size()) {
        if (smallIndex + 1 < small.size() && largeIndex + 1 < large.size()) {
            float angleOnSmallEdgeLoop = radianBetweenVectors((*m_vertices)[large[largeIndex]] - (*m_vertices)[small[smallIndex]],
                (*m_vertices)[small[smallIndex + 1]] - (*m_vertices)[small[smallIndex]]);
            float angleOnLargeEdgeLoop = radianBetweenVectors((*m_vertices)[small[smallIndex]] - (*m_vertices)[large[largeIndex]],
                (*m_vertices)[large[largeIndex + 1]] - (*m_vertices)[large[largeIndex]]);
            if (angleOnSmallEdgeLoop < angleOnLargeEdgeLoop) {
                m_regeneratedFaces.push_back({
                    small[smallIndex],
                    small[smallIndex + 1],
                    large[largeIndex]
                });
                ++smallIndex;
                continue;
            }
            m_regeneratedFaces.push_back({
                large[largeIndex + 1],
                large[largeIndex],
                small[smallIndex]
            });
            ++largeIndex;
            continue;
        }
        if (smallIndex + 1 >= small.size()) {
            m_regeneratedFaces.push_back({
                large[largeIndex + 1],
                large[largeIndex],
                small[smallIndex]
            });
            ++largeIndex;
            continue;
        }
        if (largeIndex + 1 >= large.size()) {
            m_regeneratedFaces.push_back({
                small[smallIndex],
                small[smallIndex + 1],
                large[largeIndex]
            });
            ++smallIndex;
            continue;
        }
        break;
    }
}

void MeshRecombiner::removeReluctantVertices()
{
    std::vector<std::vector<size_t>> rearrangedFaces;
    std::map<size_t, size_t> oldToNewIndexMap;
    for (const auto &face: m_regeneratedFaces) {
        std::vector<size_t> newFace;
        for (const auto &index: face) {
            const auto &findIndex = oldToNewIndexMap.find(index);
            if (findIndex != oldToNewIndexMap.end()) {
                newFace.push_back(findIndex->second);
            } else {
                size_t newIndex = m_regeneratedVertices.size();
                m_regeneratedVertices.push_back((*m_vertices)[index]);
                m_regeneratedVerticesSourceIndices.push_back((*m_verticesSourceIndices)[index]);
                oldToNewIndexMap.insert({index, newIndex});
                newFace.push_back(newIndex);
            }
        }
        rearrangedFaces.push_back(newFace);
    }
    m_regeneratedFaces = rearrangedFaces;
}
