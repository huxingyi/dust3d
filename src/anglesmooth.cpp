#include <cmath>
#include <map>
#include "dust3dutil.h"
#include "anglesmooth.h"

void angleSmooth(const std::vector<QVector3D> &vertices,
    const std::vector<std::tuple<size_t, size_t, size_t>> &triangles,
    const std::vector<QVector3D> &triangleNormals,
    float thresholdAngleDegrees, std::vector<QVector3D> &triangleVertexNormals)
{
    std::vector<std::vector<std::pair<size_t, size_t>>> triangleVertexNormalsMapByIndicies(vertices.size());
    std::vector<QVector3D> angleAreaWeightedNormals;
    for (size_t triangleIndex = 0; triangleIndex < triangles.size(); ++triangleIndex) {
        const auto &sourceTriangle = triangles[triangleIndex];
        size_t indicies[] = {std::get<0>(sourceTriangle),
            std::get<1>(sourceTriangle),
            std::get<2>(sourceTriangle)};
        const auto &v1 = vertices[indicies[0]];
        const auto &v2 = vertices[indicies[1]];
        const auto &v3 = vertices[indicies[2]];
        float area = areaOfTriangle(v1, v2, v3);
        float angles[] = {radianBetweenVectors(v2-v1, v3-v1),
            radianBetweenVectors(v1-v2, v3-v2),
            radianBetweenVectors(v1-v3, v2-v3)};
        for (int i = 0; i < 3; ++i) {
            triangleVertexNormalsMapByIndicies[indicies[i]].push_back({triangleIndex, angleAreaWeightedNormals.size()});
            angleAreaWeightedNormals.push_back(triangleNormals[triangleIndex] * area * angles[i]);
        }
    }
    triangleVertexNormals = angleAreaWeightedNormals;
    std::map<std::pair<size_t, size_t>, float> degreesBetweenFacesMap;
    for (size_t vertexIndex = 0; vertexIndex < vertices.size(); ++vertexIndex) {
        const auto &triangleVertices = triangleVertexNormalsMapByIndicies[vertexIndex];
        for (const auto &triangleVertex: triangleVertices) {
            for (const auto &otherTriangleVertex: triangleVertices) {
                if (triangleVertex.first == otherTriangleVertex.first)
                    continue;
                float degrees = 0;
                auto findDegreesResult = degreesBetweenFacesMap.find({triangleVertex.first, otherTriangleVertex.first});
                if (findDegreesResult == degreesBetweenFacesMap.end()) {
                    degrees = angleBetweenVectors(triangleNormals[triangleVertex.first], triangleNormals[otherTriangleVertex.first]);
                    degreesBetweenFacesMap.insert({{triangleVertex.first, otherTriangleVertex.first}, degrees});
                    degreesBetweenFacesMap.insert({{otherTriangleVertex.first, triangleVertex.first}, degrees});
                } else {
                    degrees = findDegreesResult->second;
                }
                if (degrees > thresholdAngleDegrees) {
                    continue;
                }
                triangleVertexNormals[triangleVertex.second] += angleAreaWeightedNormals[otherTriangleVertex.second];
            }
        }
    }
    for (auto &item: triangleVertexNormals)
        item.normalize();
}
