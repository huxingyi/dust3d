#include <simpleuv/uvunwrapper.h>
#include "uvunwrap.h"

void uvUnwrap(const Outcome &outcome, std::vector<std::vector<QVector2D>> &triangleVertexUvs, std::set<int> &seamVertices)
{
    const auto &choosenVertices = outcome.vertices;
    const auto &choosenTriangles = outcome.triangles;
    triangleVertexUvs.resize(choosenTriangles.size(), {
        QVector2D(), QVector2D(), QVector2D()
    });
    
    if (nullptr == outcome.triangleSourceNodes())
        return;
    
    const std::vector<std::pair<QUuid, QUuid>> &triangleSourceNodes = *outcome.triangleSourceNodes();
    
    simpleuv::Mesh inputMesh;
    for (const auto &vertex: choosenVertices) {
        simpleuv::Vertex v;
        v.xyz[0] = vertex.x();
        v.xyz[1] = vertex.y();
        v.xyz[2] = vertex.z();
        inputMesh.vertices.push_back(v);
    }
    std::map<QUuid, int> partIdToPartitionMap;
    int partitions = 0;
    for (decltype(choosenTriangles.size()) i = 0; i < choosenTriangles.size(); ++i) {
        const auto &triangle = choosenTriangles[i];
        const auto &sourceNode = triangleSourceNodes[i];
        simpleuv::Face f;
        f.indices[0] = triangle[0];
        f.indices[1] = triangle[1];
        f.indices[2] = triangle[2];
        inputMesh.faces.push_back(f);
        auto findPartitionResult = partIdToPartitionMap.find(sourceNode.first);
        if (findPartitionResult == partIdToPartitionMap.end()) {
            ++partitions;
            partIdToPartitionMap.insert({sourceNode.first, partitions});
            inputMesh.facePartitions.push_back(partitions);
        } else {
            inputMesh.facePartitions.push_back(findPartitionResult->second);
        }
    }
    
    simpleuv::UvUnwrapper uvUnwrapper;
    uvUnwrapper.setMesh(inputMesh);
    uvUnwrapper.unwrap();
    const std::vector<simpleuv::FaceTextureCoords> &resultFaceUvs = uvUnwrapper.getFaceUvs();
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
}
