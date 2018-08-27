#include <map>
#include <vector>
#include <QDebug>
#include <set>
#include <cmath>
#include "theme.h"
#include "meshresultcontext.h"
#include "thekla_atlas.h"
#include "positionmap.h"

struct HalfColorEdge
{
    int cornVertexIndex;
    std::pair<QUuid, QUuid> source;
};

struct CandidateEdge
{
    std::pair<QUuid, QUuid> source;
    int fromVertexIndex;
    int toVertexIndex;
    float dot;
    float length;
};

MeshResultContext::MeshResultContext() :
    m_triangleSourceResolved(false),
    m_triangleColorResolved(false),
    m_triangleEdgeSourceMapResolved(false),
    m_bmeshNodeMapResolved(false),
    m_resultPartsResolved(false),
    m_resultTriangleUvsResolved(false),
    m_resultRearrangedVerticesResolved(false)
{
}

const std::vector<std::pair<QUuid, QUuid>> &MeshResultContext::triangleSourceNodes()
{
    if (!m_triangleSourceResolved) {
        m_triangleSourceResolved = true;
        calculateTriangleSourceNodes(m_triangleSourceNodes, m_vertexSourceMap);
        calculateRemainingVertexSourceNodesAfterTriangleSourceNodesSolved(m_vertexSourceMap);
    }
    return m_triangleSourceNodes;
}

const std::map<int, std::pair<QUuid, QUuid>> &MeshResultContext::vertexSourceMap()
{
    if (!m_triangleSourceResolved) {
        m_triangleSourceResolved = true;
        calculateTriangleSourceNodes(m_triangleSourceNodes, m_vertexSourceMap);
        calculateRemainingVertexSourceNodesAfterTriangleSourceNodesSolved(m_vertexSourceMap);
    }
    return m_vertexSourceMap;
}

const std::vector<QColor> &MeshResultContext::triangleColors()
{
    if (!m_triangleColorResolved) {
        calculateTriangleColors(m_triangleColors);
        m_triangleColorResolved = true;
    }
    return m_triangleColors;
}

const std::map<std::pair<int, int>, std::pair<QUuid, QUuid>> &MeshResultContext::triangleEdgeSourceMap()
{
    if (!m_triangleEdgeSourceMapResolved) {
        calculateTriangleEdgeSourceMap(m_triangleEdgeSourceMap);
        m_triangleEdgeSourceMapResolved = true;
    }
    return m_triangleEdgeSourceMap;
}

const std::map<std::pair<QUuid, QUuid>, BmeshNode *> &MeshResultContext::bmeshNodeMap()
{
    if (!m_bmeshNodeMapResolved) {
        calculateBmeshNodeMap(m_bmeshNodeMap);
        m_bmeshNodeMapResolved = true;
    }
    return m_bmeshNodeMap;
}

void MeshResultContext::calculateTriangleSourceNodes(std::vector<std::pair<QUuid, QUuid>> &triangleSourceNodes, std::map<int, std::pair<QUuid, QUuid>> &vertexSourceMap)
{
    PositionMap<std::pair<QUuid, QUuid>> positionMap;
    std::map<std::pair<int, int>, HalfColorEdge> halfColorEdgeMap;
    std::set<int> brokenTriangleSet;
    for (const auto &it: bmeshVertices) {
        positionMap.addPosition(it.position.x(), it.position.y(), it.position.z(),
            std::make_pair(it.partId, it.nodeId));
    }
    for (auto x = 0u; x < vertices.size(); x++) {
        ResultVertex *resultVertex = &vertices[x];
        std::pair<QUuid, QUuid> source;
        if (positionMap.findPosition(resultVertex->position.x(), resultVertex->position.y(), resultVertex->position.z(), &source)) {
            vertexSourceMap[x] = source;
        }
    }
    for (auto x = 0u; x < triangles.size(); x++) {
        const auto triangle = &triangles[x];
        std::vector<std::pair<std::pair<QUuid, QUuid>, int>> colorTypes;
        for (int i = 0; i < 3; i++) {
            int index = triangle->indicies[i];
            const auto &findResult = vertexSourceMap.find(index);
            if (findResult != vertexSourceMap.end()) {
                std::pair<QUuid, QUuid> source = findResult->second;
                bool colorExisted = false;
                for (auto j = 0u; j < colorTypes.size(); j++) {
                    if (colorTypes[j].first == source) {
                        colorTypes[j].second++;
                        colorExisted = true;
                        break;
                    }
                }
                if (!colorExisted) {
                    colorTypes.push_back(std::make_pair(source, 1));
                }
            }
        }
        if (colorTypes.empty()) {
            //qDebug() << "All vertices of a triangle can't find a color";
            triangleSourceNodes.push_back(std::make_pair(QUuid(), QUuid()));
            brokenTriangleSet.insert(x);
            continue;
        }
        if (colorTypes.size() != 1 || 3 == colorTypes[0].second) {
            std::sort(colorTypes.begin(), colorTypes.end(), [](const std::pair<std::pair<QUuid, QUuid>, int> &a, const std::pair<std::pair<QUuid, QUuid>, int> &b) -> bool {
                return a.second > b.second;
            });
        }
        std::pair<QUuid, QUuid> choosenColor = colorTypes[0].first;
        triangleSourceNodes.push_back(choosenColor);
        for (int i = 0; i < 3; i++) {
            int oppositeStartIndex = triangle->indicies[(i + 1) % 3];
            int oppositeStopIndex = triangle->indicies[i];
            auto oppositePair = std::make_pair(oppositeStartIndex, oppositeStopIndex);
            if (halfColorEdgeMap.find(oppositePair) != halfColorEdgeMap.end()) {
                halfColorEdgeMap.erase(oppositePair);
                continue;
            }
            auto selfPair = std::make_pair(oppositeStopIndex, oppositeStartIndex);
            HalfColorEdge edge;
            edge.cornVertexIndex = triangle->indicies[(i + 2) % 3];
            edge.source = choosenColor;
            halfColorEdgeMap[selfPair] = edge;
        }
    }
    std::map<std::pair<int, int>, int> brokenTriangleMapByEdge;
    std::vector<CandidateEdge> candidateEdges;
    for (const auto &x: brokenTriangleSet) {
        const auto triangle = &triangles[x];
        for (int i = 0; i < 3; i++) {
            int oppositeStartIndex = triangle->indicies[(i + 1) % 3];
            int oppositeStopIndex = triangle->indicies[i];
            auto selfPair = std::make_pair(oppositeStopIndex, oppositeStartIndex);
            brokenTriangleMapByEdge[selfPair] = x;
            auto oppositePair = std::make_pair(oppositeStartIndex, oppositeStopIndex);
            const auto &findOpposite = halfColorEdgeMap.find(oppositePair);
            if (findOpposite == halfColorEdgeMap.end())
                continue;
            QVector3D selfPositions[3] = {
                vertices[triangle->indicies[i]].position, // A
                vertices[triangle->indicies[(i + 1) % 3]].position, // B
                vertices[triangle->indicies[(i + 2) % 3]].position // C
            };
            QVector3D oppositeCornPosition = vertices[findOpposite->second.cornVertexIndex].position; // D
            QVector3D AB = selfPositions[1] - selfPositions[0];
            float length = AB.length();
            QVector3D AC = selfPositions[2] - selfPositions[0];
            QVector3D AD = oppositeCornPosition - selfPositions[0];
            AB.normalize();
            AC.normalize();
            AD.normalize();
            QVector3D ABxAC = QVector3D::crossProduct(AB, AC);
            QVector3D ADxAB = QVector3D::crossProduct(AD, AB);
            ABxAC.normalize();
            ADxAB.normalize();
            float dot = QVector3D::dotProduct(ABxAC, ADxAB);
            //qDebug() << "dot:" << dot;
            CandidateEdge candidate;
            candidate.dot = dot;
            candidate.length = length;
            candidate.fromVertexIndex = triangle->indicies[i];
            candidate.toVertexIndex = triangle->indicies[(i + 1) % 3];
            candidate.source = findOpposite->second.source;
            candidateEdges.push_back(candidate);
        }
    }
    if (candidateEdges.empty())
        return;
    std::sort(candidateEdges.begin(), candidateEdges.end(), [](const CandidateEdge &a, const CandidateEdge &b) -> bool {
        if (a.dot > b.dot)
            return true;
        else if (a.dot < b.dot)
            return false;
        return a.length > b.length;
    });
    for (auto cand = 0u; cand < candidateEdges.size(); cand++) {
        const auto &candidate = candidateEdges[cand];
        if (brokenTriangleSet.empty())
            break;
        //qDebug() << "candidate dot[" << cand << "]:" << candidate.dot;
        std::vector<std::pair<int, int>> toResolvePairs;
        toResolvePairs.push_back(std::make_pair(candidate.fromVertexIndex, candidate.toVertexIndex));
        for (auto order = 0u; order < toResolvePairs.size(); order++) {
            const auto &findTriangle = brokenTriangleMapByEdge.find(toResolvePairs[order]);
            if (findTriangle == brokenTriangleMapByEdge.end())
                continue;
            int x = findTriangle->second;
            if (brokenTriangleSet.find(x) == brokenTriangleSet.end())
                continue;
            brokenTriangleSet.erase(x);
            triangleSourceNodes[x] = candidate.source;
            //qDebug() << "resolved triangle:" << x;
            const auto triangle = &triangles[x];
            for (int i = 0; i < 3; i++) {
                int oppositeStartIndex = triangle->indicies[(i + 1) % 3];
                int oppositeStopIndex = triangle->indicies[i];
                auto oppositePair = std::make_pair(oppositeStartIndex, oppositeStopIndex);
                toResolvePairs.push_back(oppositePair);
            }
        }
    }
}

void MeshResultContext::calculateRemainingVertexSourceNodesAfterTriangleSourceNodesSolved(std::map<int, std::pair<QUuid, QUuid>> &vertexSourceMap)
{
    std::map<int, std::set<std::pair<QUuid, QUuid>>> remainings;
    for (auto x = 0u; x < triangles.size(); x++) {
        const auto triangle = &triangles[x];
        for (int i = 0; i < 3; i++) {
            int index = triangle->indicies[i];
            const auto &findResult = vertexSourceMap.find(index);
            if (findResult == vertexSourceMap.end()) {
                remainings[index].insert(triangleSourceNodes()[x]);
            }
        }
    }
    for (const auto &it: remainings) {
        float minDist2 = 100000000;
        std::pair<QUuid, QUuid> minSource = std::make_pair(QUuid(), QUuid());
        for (const auto &source: it.second) {
            const auto &vertex = vertices[it.first];
            const auto &findNode = bmeshNodeMap().find(source);
            if (findNode != bmeshNodeMap().end()) {
                float dist2 = (vertex.position - findNode->second->origin).lengthSquared();
                if (dist2 < minDist2) {
                    minSource = source;
                    minDist2 = dist2;
                }
            }
        }
        vertexSourceMap[it.first] = minSource;
    }
}

void MeshResultContext::calculateTriangleColors(std::vector<QColor> &triangleColors)
{
    std::map<std::pair<QUuid, QUuid>, QColor> nodeColorMap;
    for (const auto &it: bmeshNodes) {
        nodeColorMap[std::make_pair(it.partId, it.nodeId)] = it.color;
    }
    const auto sourceNodes = triangleSourceNodes();
    for (const auto &it: sourceNodes) {
        triangleColors.push_back(nodeColorMap[it]);
    }
}

void MeshResultContext::calculateTriangleEdgeSourceMap(std::map<std::pair<int, int>, std::pair<QUuid, QUuid>> &triangleEdgeSourceMap)
{
    const std::vector<std::pair<QUuid, QUuid>> sourceNodes = triangleSourceNodes();
    for (auto x = 0u; x < triangles.size(); x++) {
        const auto triangle = &triangles[x];
        for (int i = 0; i < 3; i++) {
            int startIndex = triangle->indicies[i];
            int stopIndex = triangle->indicies[(i + 1) % 3];
            triangleEdgeSourceMap[std::make_pair(startIndex, stopIndex)] = sourceNodes[x];
        }
    }
}

void MeshResultContext::calculateBmeshNodeMap(std::map<std::pair<QUuid, QUuid>, BmeshNode *> &bmeshNodeMap) {
    for (auto i = 0u; i < bmeshNodes.size(); i++) {
        BmeshNode *bmeshNode = &bmeshNodes[i];
        bmeshNodeMap[std::make_pair(bmeshNode->partId, bmeshNode->nodeId)] = bmeshNode;
    }
}

struct BmeshNodeDistWithWorldCenter
{
    BmeshNode *bmeshNode;
    float dist2;
};

const std::map<QUuid, ResultPart> &MeshResultContext::parts()
{
    if (!m_resultPartsResolved) {
        calculateResultParts(m_resultParts);
        m_resultPartsResolved = true;
    }
    return m_resultParts;
}

const std::vector<ResultTriangleUv> &MeshResultContext::triangleUvs()
{
    if (!m_resultTriangleUvsResolved) {
        calculateResultTriangleUvs(m_resultTriangleUvs, m_seamVertices);
        m_resultTriangleUvsResolved = true;
    }
    return m_resultTriangleUvs;
}

void MeshResultContext::calculateResultParts(std::map<QUuid, ResultPart> &parts)
{
    std::map<std::pair<QUuid, int>, int> oldVertexToNewMap;
    for (auto x = 0u; x < triangles.size(); x++) {
        const auto &triangle = triangles[x];
        const auto &sourceNode = triangleSourceNodes()[x];
        auto it = parts.find(sourceNode.first);
        if (it == parts.end()) {
            ResultPart newPart;
            newPart.color = triangleColors()[x];
            parts.insert(std::make_pair(sourceNode.first, newPart));
        }
        auto &resultPart = parts[sourceNode.first];
        ResultTriangle newTriangle;
        newTriangle.normal = triangle.normal;
        for (auto i = 0u; i < 3; i++) {
            auto key = std::make_pair(sourceNode.first, triangle.indicies[i]);
            const auto &it = oldVertexToNewMap.find(key);
            bool isNewVertex = it == oldVertexToNewMap.end();
            bool isSeamVertex = m_seamVertices.end() != m_seamVertices.find(triangle.indicies[i]);
            if (isNewVertex || isSeamVertex) {
                int newIndex = resultPart.vertices.size();
                resultPart.interpolatedVertexNormals.push_back(newTriangle.normal);
                resultPart.vertices.push_back(vertices[triangle.indicies[i]]);
                ResultVertexUv vertexUv;
                vertexUv.uv[0] = triangleUvs()[x].uv[i][0];
                vertexUv.uv[1] = triangleUvs()[x].uv[i][1];
                resultPart.vertexUvs.push_back(vertexUv);
                if (isNewVertex && !isSeamVertex)
                    oldVertexToNewMap.insert(std::make_pair(key, newIndex));
                newTriangle.indicies[i] = newIndex;
            } else {
                newTriangle.indicies[i] = it->second;
                resultPart.interpolatedVertexNormals[it->second] += newTriangle.normal;
            }
        }
        resultPart.triangles.push_back(newTriangle);
        resultPart.uvs.push_back(triangleUvs()[x]);
    }
    for (auto &partIt: parts) {
        for (auto &normalIt: partIt.second.interpolatedVertexNormals) {
            normalIt.normalize();
        }
    }
}

void MeshResultContext::calculateResultTriangleUvs(std::vector<ResultTriangleUv> &uvs, std::set<int> &seamVertices)
{
    using namespace Thekla;
    
    const std::vector<ResultRearrangedVertex> &choosenVertices = rearrangedVertices();
    const std::vector<ResultRearrangedTriangle> &choosenTriangles = rearrangedTriangles();
    
    Atlas_Input_Mesh inputMesh;
 
    inputMesh.vertex_count = choosenVertices.size();
    inputMesh.vertex_array = new Atlas_Input_Vertex[inputMesh.vertex_count];
    inputMesh.face_count = choosenTriangles.size();
    inputMesh.face_array = new Atlas_Input_Face[inputMesh.face_count];
    for (auto i = 0; i < inputMesh.vertex_count; i++) {
        const ResultRearrangedVertex *src = &choosenVertices[i];
        Atlas_Input_Vertex *dest = &inputMesh.vertex_array[i];
        dest->position[0] = src->position.x();
        dest->position[1] = src->position.y();
        dest->position[2] = src->position.z();
        dest->normal[0] = 0;
        dest->normal[1] = 0;
        dest->normal[2] = 0;
        dest->uv[0] = 0;
        dest->uv[1] = 0;
        dest->first_colocal = i;
    }
    std::map<std::pair<int, int>, int> edgeToFaceIndexMap;
    std::map<QUuid, int> materialIndexMap;
    for (auto i = 0; i < inputMesh.face_count; i++) {
        const ResultRearrangedTriangle *src = &choosenTriangles[i];
        Atlas_Input_Face *dest = &inputMesh.face_array[i];
        auto &materialKey = triangleSourceNodes()[src->originalIndex].first;
        auto findMaterialResult = materialIndexMap.find(materialKey);
        int materialIndex = 0;
        if (findMaterialResult == materialIndexMap.end()) {
            materialIndex = materialIndexMap.size() + 1;
            materialIndexMap[materialKey] = materialIndex;
        } else {
            materialIndex = findMaterialResult->second;
        }
        dest->material_index = materialIndex;
        dest->vertex_index[0] = src->indicies[0];
        dest->vertex_index[1] = src->indicies[1];
        dest->vertex_index[2] = src->indicies[2];
        edgeToFaceIndexMap[std::make_pair(src->indicies[0], src->indicies[1])] = src->originalIndex;
        edgeToFaceIndexMap[std::make_pair(src->indicies[1], src->indicies[2])] = src->originalIndex;
        edgeToFaceIndexMap[std::make_pair(src->indicies[2], src->indicies[0])] = src->originalIndex;
        for (auto j = 0; j < 3; j++) {
            Atlas_Input_Vertex *vertex = &inputMesh.vertex_array[src->indicies[j]];
            vertex->normal[0] += src->normal.x();
            vertex->normal[1] += src->normal.y();
            vertex->normal[2] += src->normal.z();
        }
    }
    for (auto i = 0; i < inputMesh.vertex_count; i++) {
        Atlas_Input_Vertex *dest = &inputMesh.vertex_array[i];
        QVector3D normal(dest->normal[0], dest->normal[1], dest->normal[2]);
        normal.normalize();
        dest->normal[0] = normal.x();
        dest->normal[1] = normal.y();
        dest->normal[2] = normal.z();
    }
    
    Atlas_Options atlasOptions;
    atlas_set_default_options(&atlasOptions);
    
    atlasOptions.packer_options.witness.packing_quality = 4;
    
    Atlas_Error error = Atlas_Error_Success;
    Atlas_Output_Mesh *outputMesh = atlas_generate(&inputMesh, &atlasOptions, &error);
    
    PositionMap<int> uvPositionAndIndexMap;
    
    uvs.resize(triangles.size());
    std::set<int> refs;
    for (auto i = 0; i < outputMesh->index_count; i += 3) {
        Atlas_Output_Vertex *outputVertices[] = {
            &outputMesh->vertex_array[outputMesh->index_array[i + 0]],
            &outputMesh->vertex_array[outputMesh->index_array[i + 1]],
            &outputMesh->vertex_array[outputMesh->index_array[i + 2]]
        };
        int faceIndex = edgeToFaceIndexMap[std::make_pair(outputVertices[0]->xref, outputVertices[1]->xref)];
        //Q_ASSERT(faceIndex == edgeToFaceIndexMap[std::make_pair(outputVertices[1]->xref, outputVertices[2]->xref)]);
        //Q_ASSERT(faceIndex == edgeToFaceIndexMap[std::make_pair(outputVertices[2]->xref, outputVertices[0]->xref)]);
        int firstIndex = 0;
        for (auto j = 0; j < 3; j++) {
            if (choosenVertices[outputVertices[0]->xref].originalIndex == triangles[faceIndex].indicies[j]) {
                firstIndex = j;
                break;
            }
        }
        for (auto j = 0; j < 3; j++) {
            Atlas_Output_Vertex *from = outputVertices[j];
            ResultTriangleUv *to = &uvs[faceIndex];
            to->resolved = true;
            int toIndex = (firstIndex + j) % 3;
            to->uv[toIndex][0] = (float)from->uv[0] / outputMesh->atlas_width;
            to->uv[toIndex][1] = (float)from->uv[1] / outputMesh->atlas_height;
            int originalRef = choosenVertices[from->xref].originalIndex;
            if (refs.find(originalRef) == refs.end()) {
                refs.insert(originalRef);
                uvPositionAndIndexMap.addPosition(from->uv[0], from->uv[1], (float)originalRef, 0);
            } else {
                if (!uvPositionAndIndexMap.findPosition(from->uv[0], from->uv[1], (float)originalRef, nullptr)) {
                    seamVertices.insert(originalRef);
                }
            }
        }
    }
    int unresolvedUvFaceCount = 0;
    for (auto i = 0u; i < uvs.size(); i++) {
        ResultTriangleUv *uv = &uvs[i];
        if (!uv->resolved) {
            unresolvedUvFaceCount++;
        }
    }
    qDebug() << "unresolvedUvFaceCount:" << unresolvedUvFaceCount;
    
    atlas_free(outputMesh);
}

const std::vector<ResultRearrangedVertex> &MeshResultContext::rearrangedVertices()
{
    if (!m_resultRearrangedVerticesResolved) {
        calculateResultRearrangedVertices(m_rearrangedVertices, m_rearrangedTriangles);
        m_resultRearrangedVerticesResolved = true;
    }
    return m_rearrangedVertices;
}

const std::vector<ResultRearrangedTriangle> &MeshResultContext::rearrangedTriangles()
{
    if (!m_resultRearrangedVerticesResolved) {
        calculateResultRearrangedVertices(m_rearrangedVertices, m_rearrangedTriangles);
        m_resultRearrangedVerticesResolved = true;
    }
    return m_rearrangedTriangles;
}

void MeshResultContext::calculateResultRearrangedVertices(std::vector<ResultRearrangedVertex> &rearrangedVertices, std::vector<ResultRearrangedTriangle> &rearrangedTriangles)
{
    std::map<std::pair<QUuid, int>, int> oldVertexToNewMap;
    rearrangedVertices.clear();
    rearrangedTriangles.clear();
    for (auto x = 0u; x < triangles.size(); x++) {
        const auto &triangle = triangles[x];
        const auto &sourceNode = triangleSourceNodes()[x];
        ResultRearrangedTriangle newTriangle;
        newTriangle.normal = triangle.normal;
        newTriangle.originalIndex = x;
        for (auto i = 0u; i < 3; i++) {
            auto key = std::make_pair(sourceNode.first, triangle.indicies[i]);
            const auto &it = oldVertexToNewMap.find(key);
            if (it == oldVertexToNewMap.end()) {
                ResultRearrangedVertex rearrangedVertex;
                rearrangedVertex.originalIndex = triangle.indicies[i];
                rearrangedVertex.position = vertices[triangle.indicies[i]].position;
                int newIndex = rearrangedVertices.size();
                rearrangedVertices.push_back(rearrangedVertex);
                oldVertexToNewMap.insert(std::make_pair(key, newIndex));
                newTriangle.indicies[i] = newIndex;
            } else {
                newTriangle.indicies[i] = it->second;
            }
        }
        rearrangedTriangles.push_back(newTriangle);
    }
}
