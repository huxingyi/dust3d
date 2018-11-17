#include <QtGlobal>
#include <QDebug>
#include <map>
#include <vector>
#include <QVector3D>
#include <unordered_map>
#include "meshweldseam.h"
#include "meshutil.h"

int meshWeldSeam(void *meshlite, int meshId, float allowedSmallestDistance, const PositionMap<bool> &excludePositions, int *affectedNum)
{
    int vertexCount = meshlite_get_vertex_count(meshlite, meshId);
    float *vertexPositions = new float[vertexCount * 3];
    int vertexArrayLen = meshlite_get_vertex_position_array(meshlite, meshId, vertexPositions, vertexCount * 3);
    int offset = 0;
    Q_ASSERT(vertexArrayLen == vertexCount * 3);
    std::vector<QVector3D> positions;
    std::unordered_set<int> excludeVertices;
    for (int i = 0; i < vertexCount; i++) {
        float x = vertexPositions[offset + 0];
        float y = vertexPositions[offset + 1];
        float z = vertexPositions[offset + 2];
        auto position = QVector3D(x, y, z);
        if (excludePositions.findPosition(x, y, z))
            excludeVertices.insert(i);
        positions.push_back(position);
        offset += 3;
    }
    int faceCount = meshlite_get_face_count(meshlite, meshId);
    int *faceVertexNumAndIndices = new int[faceCount * MAX_VERTICES_PER_FACE];
    int filledLength = meshlite_get_face_index_array(meshlite, meshId, faceVertexNumAndIndices, faceCount * MAX_VERTICES_PER_FACE);
    int i = 0;
    std::vector<std::vector<int>> newFaceIndices;
    while (i < filledLength) {
        int num = faceVertexNumAndIndices[i++];
        Q_ASSERT(num > 0 && num <= MAX_VERTICES_PER_FACE);
        if (num < 3) {
            i += num;
            continue;
        }
        std::vector<int> indices;
        for (int j = 0; j < num; j++) {
            int index = faceVertexNumAndIndices[i++];
            Q_ASSERT(index >= 0 && index < vertexCount);
            indices.push_back(index);
        }
        newFaceIndices.push_back(indices);
    }
    float squareOfAllowedSmallestDistance = allowedSmallestDistance * allowedSmallestDistance;
    int weldedMesh = 0;
    std::map<int, int> weldVertexToMap;
    std::unordered_set<int> weldTargetVertices;
    std::unordered_set<int> processedFaces;
    std::map<std::pair<int, int>, std::pair<int, int>> triangleEdgeMap;
    std::unordered_map<int, int> vertexAdjFaceCountMap;
    for (int i = 0; i < (int)newFaceIndices.size(); i++) {
        const auto &faceIndices = newFaceIndices[i];
        if (faceIndices.size() == 3) {
            vertexAdjFaceCountMap[faceIndices[0]]++;
            vertexAdjFaceCountMap[faceIndices[1]]++;
            vertexAdjFaceCountMap[faceIndices[2]]++;
            triangleEdgeMap[std::make_pair(faceIndices[0], faceIndices[1])] = std::make_pair(i, faceIndices[2]);
            triangleEdgeMap[std::make_pair(faceIndices[1], faceIndices[2])] = std::make_pair(i, faceIndices[0]);
            triangleEdgeMap[std::make_pair(faceIndices[2], faceIndices[0])] = std::make_pair(i, faceIndices[1]);
        }
    }
    for (int i = 0; i < (int)newFaceIndices.size(); i++) {
        if (processedFaces.find(i) != processedFaces.end())
            continue;
        const auto &faceIndices = newFaceIndices[i];
        if (faceIndices.size() == 3) {
            bool indicesSeamCheck[3] = {
                excludeVertices.find(faceIndices[0]) == excludeVertices.end(),
                excludeVertices.find(faceIndices[1]) == excludeVertices.end(),
                excludeVertices.find(faceIndices[2]) == excludeVertices.end()
            };
            for (int j = 0; j < 3; j++) {
                int next = (j + 1) % 3;
                int nextNext = (j + 2) % 3;
                if (indicesSeamCheck[j] && indicesSeamCheck[next]) {
                    std::pair<int, int> edge = std::make_pair(faceIndices[j], faceIndices[next]);
                    int thirdVertexIndex = faceIndices[nextNext];
                    if ((positions[edge.first] - positions[edge.second]).lengthSquared() < squareOfAllowedSmallestDistance) {
                        auto oppositeEdge = std::make_pair(edge.second, edge.first);
                        auto findOppositeFace = triangleEdgeMap.find(oppositeEdge);
                        if (findOppositeFace == triangleEdgeMap.end()) {
                            qDebug() << "Find opposite edge failed";
                            continue;
                        }
                        int oppositeFaceIndex = findOppositeFace->second.first;
                        if (((positions[edge.first] - positions[thirdVertexIndex]).lengthSquared() <
                                    (positions[edge.second] - positions[thirdVertexIndex]).lengthSquared()) &&
                                vertexAdjFaceCountMap[edge.second] <= 4 &&
                                weldVertexToMap.find(edge.second) == weldVertexToMap.end()) {
                            weldVertexToMap[edge.second] = edge.first;
                            weldTargetVertices.insert(edge.first);
                            processedFaces.insert(i);
                            processedFaces.insert(oppositeFaceIndex);
                            break;
                        } else if (vertexAdjFaceCountMap[edge.first] <= 4 &&
                                weldVertexToMap.find(edge.first) == weldVertexToMap.end()) {
                            weldVertexToMap[edge.first] = edge.second;
                            weldTargetVertices.insert(edge.second);
                            processedFaces.insert(i);
                            processedFaces.insert(oppositeFaceIndex);
                            break;
                        }
                    }
                }
            }
        }
    }
    std::vector<int> newFaceVertexNumAndIndices;
    int weldedCount = 0;
    int faceCountAfterWeld = 0;
    for (int i = 0; i < (int)newFaceIndices.size(); i++) {
        const auto &faceIndices = newFaceIndices[i];
        std::vector<int> mappedFaceIndices;
        bool errored = false;
        for (const auto &index: faceIndices) {
            int finalIndex = index;
            int mapTimes = 0;
            while (mapTimes < 500) {
                auto findMapResult = weldVertexToMap.find(finalIndex);
                if (findMapResult == weldVertexToMap.end())
                    break;
                finalIndex = findMapResult->second;
                mapTimes++;
            }
            if (mapTimes >= 500) {
                qDebug() << "Map too much times";
                errored = true;
                break;
            }
            mappedFaceIndices.push_back(finalIndex);
        }
        if (errored || mappedFaceIndices.size() < 3)
            continue;
        bool welded = false;
        for (decltype(mappedFaceIndices.size()) j = 0; j < mappedFaceIndices.size(); j++) {
            int next = (j + 1) % 3;
            if (mappedFaceIndices[j] == mappedFaceIndices[next]) {
                welded = true;
                break;
            }
        }
        if (welded) {
            weldedCount++;
            continue;
        }
        faceCountAfterWeld++;
        newFaceVertexNumAndIndices.push_back(mappedFaceIndices.size());
        for (const auto &index: mappedFaceIndices) {
            newFaceVertexNumAndIndices.push_back(index);
        }
    }
    if (affectedNum)
        *affectedNum = weldedCount;
    qDebug() << "Welded" << weldedCount << "triangles(" << newFaceIndices.size() << " - " << weldedCount << " = " << faceCountAfterWeld << ")";
    weldedMesh = meshlite_build(meshlite, vertexPositions, vertexCount, newFaceVertexNumAndIndices.data(), newFaceVertexNumAndIndices.size());
    delete[] faceVertexNumAndIndices;
    delete[] vertexPositions;
    return weldedMesh;
}
