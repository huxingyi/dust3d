#include <simpleuv/uvunwrapper.h>
#include <QDebug>
#include <QRectF>
#include "uvunwrap.h"

void uvUnwrap(const Object &object,
    std::vector<std::vector<QVector2D>> &triangleVertexUvs,
    std::set<int> &seamVertices,
    std::map<QUuid, std::vector<QRectF>> &uvRects)
{
    const auto &choosenVertices = object.vertices;
    const auto &choosenTriangles = object.triangles;
    const auto &choosenTriangleNormals = object.triangleNormals;
    triangleVertexUvs.resize(choosenTriangles.size(), {
        QVector2D(), QVector2D(), QVector2D()
    });
    
    if (nullptr == object.triangleSourceNodes())
        return;
    
    const std::vector<std::pair<QUuid, QUuid>> &triangleSourceNodes = *object.triangleSourceNodes();
    
    simpleuv::Mesh inputMesh;
    for (const auto &vertex: choosenVertices) {
        simpleuv::Vertex v;
        v.xyz[0] = vertex.x();
        v.xyz[1] = vertex.y();
        v.xyz[2] = vertex.z();
        inputMesh.vertices.push_back(v);
    }
    std::map<QUuid, int> partIdToPartitionMap;
    std::vector<QUuid> partitionPartUuids;
    for (decltype(choosenTriangles.size()) i = 0; i < choosenTriangles.size(); ++i) {
        const auto &triangle = choosenTriangles[i];
        const auto &sourceNode = triangleSourceNodes[i];
        const auto &normal = choosenTriangleNormals[i];
        simpleuv::Face f;
        f.indices[0] = triangle[0];
        f.indices[1] = triangle[1];
        f.indices[2] = triangle[2];
        inputMesh.faces.push_back(f);
        simpleuv::Vector3 n;
        n.xyz[0] = normal.x();
        n.xyz[1] = normal.y();
        n.xyz[2] = normal.z();
        inputMesh.faceNormals.push_back(n);
        auto findPartitionResult = partIdToPartitionMap.find(sourceNode.first);
        if (findPartitionResult == partIdToPartitionMap.end()) {
            partitionPartUuids.push_back(sourceNode.first);
            partIdToPartitionMap.insert({sourceNode.first, (int)partitionPartUuids.size()});
            inputMesh.facePartitions.push_back((int)partitionPartUuids.size());
        } else {
            inputMesh.facePartitions.push_back(findPartitionResult->second);
        }
    }
    
    simpleuv::UvUnwrapper uvUnwrapper;
    uvUnwrapper.setMesh(inputMesh);
    uvUnwrapper.unwrap();
    qDebug() << "Texture size:" << uvUnwrapper.getTextureSize();
    const std::vector<simpleuv::FaceTextureCoords> &resultFaceUvs = uvUnwrapper.getFaceUvs();
    const std::vector<simpleuv::Rect> &resultChartRects = uvUnwrapper.getChartRects();
    const std::vector<int> &resultChartSourcePartitions = uvUnwrapper.getChartSourcePartitions();
    std::map<int, QVector2D> vertexUvMap;
    for (decltype(choosenTriangles.size()) i = 0; i < choosenTriangles.size(); ++i) {
        const auto &triangle = choosenTriangles[i];
        const auto &src = resultFaceUvs[i];
        auto &dest = triangleVertexUvs[i];
        for (size_t j = 0; j < 3; ++j) {
            QVector2D uvCoord = QVector2D(src.coords[j].uv[0], src.coords[j].uv[1]);
            dest[j][0] = uvCoord.x();
            dest[j][1] = uvCoord.y();
            int vertexIndex = triangle[j];
            auto findVertexUvResult = vertexUvMap.find(vertexIndex);
            if (findVertexUvResult == vertexUvMap.end()) {
                vertexUvMap.insert({vertexIndex, uvCoord});
            } else {
                if (!qFuzzyCompare(findVertexUvResult->second, uvCoord)) {
                    seamVertices.insert(vertexIndex);
                }
            }
        }
    }
    for (size_t i = 0; i < resultChartRects.size(); ++i) {
        const auto &rect = resultChartRects[i];
        const auto &source = resultChartSourcePartitions[i];
        if (0 == source || source > (int)partitionPartUuids.size()) {
            qDebug() << "Invalid UV chart source partition:" << source;
            continue;
        }
        uvRects[partitionPartUuids[source - 1]].push_back({rect.left, rect.top, rect.width, rect.height});
    }
}
