#include <QtGlobal>
#include <QDebug>
#include <map>
#include <vector>
#include <QVector3D>
#include <unordered_map>
#include "meshweldseam.h"
#include "meshutil.h"

int meshWeldSeam(void *meshlite, int meshId, float allowedSmallestDistance, const std::unordered_set<int> &seamVerticesIndicies)
{
    int vertexCount = meshlite_get_vertex_count(meshlite, meshId);
    float *vertexPositions = new float[vertexCount * 3];
    int vertexArrayLen = meshlite_get_vertex_position_array(meshlite, meshId, vertexPositions, vertexCount * 3);
    int offset = 0;
    Q_ASSERT(vertexArrayLen == vertexCount * 3);
    std::vector<QVector3D> positions;
    for (int i = 0; i < vertexCount; i++) {
        float x = vertexPositions[offset + 0];
        float y = vertexPositions[offset + 1];
        float z = vertexPositions[offset + 2];
        positions.push_back(QVector3D(x, y, z));
        offset += 3;
    }
    int faceCount = meshlite_get_face_count(meshlite, meshId);
    int *faceVertexNumAndIndices = new int[faceCount * MAX_VERTICES_PER_FACE];
    int filledLength = meshlite_get_face_index_array(meshlite, meshId, faceVertexNumAndIndices, faceCount * MAX_VERTICES_PER_FACE);
    int i = 0;
    std::vector<std::vector<int>> newFaceIndicies;
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
        newFaceIndicies.push_back(indices);
    }
    float squareOfAllowedSmallestDistance = allowedSmallestDistance * allowedSmallestDistance;
    int weldedMesh = 0;
    std::map<int, int> weldVertexToMap;
    std::unordered_set<int> weldTargetVertices;
    std::unordered_set<int> processedFaces;
    std::map<std::pair<int, int>, std::pair<int, int>> triangleEdgeMap;
    std::unordered_map<int, int> vertexAdjFaceCountMap;
    for (int i = 0; i < (int)newFaceIndicies.size(); i++) {
        const auto &faceIndicies = newFaceIndicies[i];
        if (faceIndicies.size() == 3) {
            vertexAdjFaceCountMap[faceIndicies[0]]++;
            vertexAdjFaceCountMap[faceIndicies[1]]++;
            vertexAdjFaceCountMap[faceIndicies[2]]++;
            triangleEdgeMap[std::make_pair(faceIndicies[0], faceIndicies[1])] = std::make_pair(i, faceIndicies[2]);
            triangleEdgeMap[std::make_pair(faceIndicies[1], faceIndicies[2])] = std::make_pair(i, faceIndicies[0]);
            triangleEdgeMap[std::make_pair(faceIndicies[2], faceIndicies[0])] = std::make_pair(i, faceIndicies[1]);
        }
    }
    for (int i = 0; i < (int)newFaceIndicies.size(); i++) {
        if (processedFaces.find(i) != processedFaces.end())
            continue;
        const auto &faceIndicies = newFaceIndicies[i];
        if (faceIndicies.size() == 3) {
            bool indiciesSeamCheck[3] = {
                seamVerticesIndicies.empty() || seamVerticesIndicies.find(faceIndicies[0]) != seamVerticesIndicies.end(),
                seamVerticesIndicies.empty() || seamVerticesIndicies.find(faceIndicies[1]) != seamVerticesIndicies.end(),
                seamVerticesIndicies.empty() || seamVerticesIndicies.find(faceIndicies[2]) != seamVerticesIndicies.end()
            };
            for (int j = 0; j < 3; j++) {
                int next = (j + 1) % 3;
                int nextNext = (j + 2) % 3;
                if (indiciesSeamCheck[j] && indiciesSeamCheck[next]) {
                    std::pair<int, int> edge = std::make_pair(faceIndicies[j], faceIndicies[next]);
                    int thirdVertexIndex = faceIndicies[nextNext];
                    if ((positions[edge.first] - positions[edge.second]).lengthSquared() < squareOfAllowedSmallestDistance) {
                        auto oppositeEdge = std::make_pair(edge.second, edge.first);
                        auto findOppositeFace = triangleEdgeMap.find(oppositeEdge);
                        if (findOppositeFace == triangleEdgeMap.end()) {
                            qDebug() << "Find opposite edge failed";
                            continue;
                        }
                        int oppositeFaceIndex = findOppositeFace->second.first;
                        // Weld on the longer edge vertex
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
    for (int i = 0; i < (int)newFaceIndicies.size(); i++) {
        const auto &faceIndicies = newFaceIndicies[i];
        std::vector<int> mappedFaceIndicies;
        bool errored = false;
        for (const auto &index: faceIndicies) {
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
            mappedFaceIndicies.push_back(finalIndex);
        }
        if (errored || mappedFaceIndicies.size() < 3)
            continue;
        bool welded = false;
        for (decltype(mappedFaceIndicies.size()) j = 0; j < mappedFaceIndicies.size(); j++) {
            int next = (j + 1) % 3;
            if (mappedFaceIndicies[j] == mappedFaceIndicies[next]) {
                welded = true;
                break;
            }
        }
        if (welded) {
            weldedCount++;
            continue;
        }
        faceCountAfterWeld++;
        newFaceVertexNumAndIndices.push_back(mappedFaceIndicies.size());
        for (const auto &index: mappedFaceIndicies) {
            newFaceVertexNumAndIndices.push_back(index);
        }
    }
    qDebug() << "Welded" << weldedCount << "triangles(" << newFaceIndicies.size() << " - " << weldedCount << " = " << faceCountAfterWeld << ")";
    weldedMesh = meshlite_build(meshlite, vertexPositions, vertexCount, newFaceVertexNumAndIndices.data(), newFaceVertexNumAndIndices.size());
    delete[] faceVertexNumAndIndices;
    delete[] vertexPositions;
    return weldedMesh;
}
