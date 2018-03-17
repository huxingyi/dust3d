#include "skeletontomesh.h"
#include "meshlite.h"
#include "skeletoneditnodeitem.h"
#include "skeletoneditedgeitem.h"

#if defined(HAVE_CONFIG_H)
#  include <carve_config.h>
#endif
#include <carve/carve.hpp>
#include <carve/csg.hpp>
#include <carve/input.hpp>

// Modified from https://wiki.qt.io/QThreads_general_usage

#define MAX_VERTICES_PER_FACE   100

carve::poly::Polyhedron *makeCarveMeshFromMeshlite(void *meshlite, int meshId)
{
    carve::input::PolyhedronData data;
    int vertexCount = meshlite_get_vertex_count(meshlite, meshId);
    float *vertexPositions = new float[vertexCount * 3];
    meshlite_get_vertex_position_array(meshlite, meshId, vertexPositions, vertexCount * 3);
    int offset = 0;
    for (int i = 0; i < vertexCount; i++) {
        data.addVertex(carve::geom::VECTOR(
            vertexPositions[offset + 0],
            vertexPositions[offset + 1],
            vertexPositions[offset + 2]));
        offset += 3;
    }
    int faceCount = meshlite_get_face_count(meshlite, meshId);
    int *faceVertexNumAndIndices = new int[faceCount * MAX_VERTICES_PER_FACE];
    int filledLength = meshlite_get_face_index_array(meshlite, meshId, faceVertexNumAndIndices, faceCount * MAX_VERTICES_PER_FACE);
    int i = 0;
    while (i < filledLength) {
        int num = faceVertexNumAndIndices[i++];
        std::vector<int> faceIndices;
        for (int j = 0; j < num; j++) {
            int index = faceVertexNumAndIndices[i++];
            faceIndices.push_back(index);
        }
        data.addFace(faceIndices.begin(), faceIndices.end());
    }
    delete[] faceVertexNumAndIndices;
    delete[] vertexPositions;
    return new carve::poly::Polyhedron(data.points, data.getFaceCount(), data.faceIndices);
}

int makeMeshliteMeshFromCarve(void *meshlite, carve::poly::Polyhedron *polyhedron)
{
    int vertexCount = polyhedron->vertices.size();
    float *vertexPositions = new float[vertexCount * 3];
    int offset = 0;
    std::map<const carve::poly::Geometry<3>::vertex_t *, int> vertexIndexMap;
    for (int i = 0; i < vertexCount; i++) {
        auto vertex = &polyhedron->vertices[i];
        vertexPositions[offset++] = vertex->v.x;
        vertexPositions[offset++] = vertex->v.y;
        vertexPositions[offset++] = vertex->v.z;
        vertexIndexMap[vertex] = i;
    }
    int faceCount = polyhedron->faces.size();
    int *faceVertexNumAndIndices = new int[faceCount * MAX_VERTICES_PER_FACE];
    offset = 0;
    for (int i = 0; i < faceCount; i++) {
        auto face  = &polyhedron->faces[i];
        faceVertexNumAndIndices[offset++] = face->vertices.size();
        for (int j = 0; j < (int)face->vertices.size(); j++) {
            faceVertexNumAndIndices[offset++] = vertexIndexMap[face->vertices[j]];
        }
    }
    int meshId = meshlite_build(meshlite, vertexPositions, vertexCount, faceVertexNumAndIndices, offset);
    delete[] vertexPositions;
    delete[] faceVertexNumAndIndices;
    return meshId;
}

carve::poly::Polyhedron *unionCarveMeshs(carve::poly::Polyhedron *first,
    carve::poly::Polyhedron *second)
{
    carve::csg::CSG csg;
    carve::poly::Polyhedron *result = csg.compute(first, second, carve::csg::CSG::UNION, NULL, carve::csg::CSG::CLASSIFY_EDGE);
    return new carve::poly::Polyhedron(*result);
}

struct NodeItemInfo
{
    int index;
    int neighborCount;
};

SkeletonToMesh::SkeletonToMesh(SkeletonEditGraphicsView *graphicsView) :
    m_mesh(NULL)
{
    QList<QGraphicsItem *>::iterator it;
    QList<QGraphicsItem *> list = graphicsView->scene()->items();
    std::map<SkeletonEditNodeItem *, NodeItemInfo> nodeItemsMap;
    std::map<QGraphicsItemGroup *, int> groupIdsMap;
    int maxNeighborCount = 0;
    for (it = list.begin(); it != list.end(); ++it) {
        if ((*it)->data(0).toString() == "edge") {
            SkeletonEditEdgeItem *edgeItem = static_cast<SkeletonEditEdgeItem *>(*it);
            SkeletonEditNodeItem *nodeItems[] = {edgeItem->firstNode(), edgeItem->secondNode()};
            int nodeIndices[] = {0, 0};
            SkeletonGroup *skeletonGroup = NULL;
            for (int i = 0; i < 2; i++) {
                SkeletonEditNodeItem *nodeItem = nodeItems[i];
                std::map<QGraphicsItemGroup *, int>::iterator findGroupId = groupIdsMap.find(nodeItem->group());
                if (findGroupId == groupIdsMap.end()) {
                    SkeletonGroup group;
                    group.maxNeighborCount = 0;
                    group.rootNode = 0;
                    group.bmeshId = -1;
                    group.meshId = -1;
                    groupIdsMap[nodeItem->group()] = m_groups.size();
                    m_groups.push_back(group);
                }
                skeletonGroup = &m_groups[groupIdsMap[nodeItem->group()]];
                std::map<SkeletonEditNodeItem *, NodeItemInfo>::iterator findNode = nodeItemsMap.find(nodeItem);
                if (findNode == nodeItemsMap.end()) {
                    SkeletonNode node;
                    NodeItemInfo info;
                    QPointF origin = nodeItem->origin();
                    
                    node.originX = origin.x();
                    node.originY = origin.y();
                    node.originZ = nodeItem->slave()->origin().x();
                    node.bmeshNodeId = -1;
                    node.radius = nodeItem->radius();
                    
                    info.index = skeletonGroup->nodes.size();
                    info.neighborCount = 1;
                    
                    nodeIndices[i] = info.index;
                    nodeItemsMap[nodeItem] = info;
                    skeletonGroup->nodes.push_back(node);
                } else {
                    nodeIndices[i] = findNode->second.index;
                    findNode->second.neighborCount++;
                    if (findNode->second.neighborCount > skeletonGroup->maxNeighborCount) {
                        skeletonGroup->rootNode = findNode->second.index;
                        maxNeighborCount = findNode->second.neighborCount;
                    }
                }
            }
            SkeletonEdge edge;
            edge.firstNode = nodeIndices[0];
            edge.secondNode = nodeIndices[1];
            skeletonGroup->edges.push_back(edge);
        }
    }
}

SkeletonToMesh::~SkeletonToMesh()
{
    delete m_mesh;
}

Mesh *SkeletonToMesh::takeResultMesh()
{
    Mesh *mesh = m_mesh;
    m_mesh = NULL;
    return mesh;
}

void SkeletonToMesh::process()
{
    if (m_groups.size() <= 0) {
        emit finished();
        return;
    }
    float left = -1;
    float right = -1;
    float top = -1;
    float bottom = -1;
    float zLeft = -1;
    float zRight = -1;
    for (size_t i = 0; i < m_groups.size(); i++) {
        SkeletonGroup *group = &m_groups[i];
        for (size_t j = 0; j < group->nodes.size(); j++) {
            SkeletonNode *node = &group->nodes[j];
            if (left < 0 || node->originX < left) {
                left = node->originX;
            }
            if (top < 0 || node->originY < top) {
                top = node->originY;
            }
            if (node->originX > right) {
                right = node->originX;
            }
            if (node->originY > bottom) {
                bottom = node->originY;
            }
            if (zLeft < 0 || node->originZ < zLeft) {
                zLeft = node->originZ;
            }
            if (node->originZ > zRight) {
                zRight = node->originZ;
            }
        }
    }
    float height = bottom - top;
    if (height <= 0) {
        emit finished();
        return;
    }
    float zWidth = right - left;
    void *context = meshlite_create_context();
    
    for (size_t i = 0; i < m_groups.size(); i++) {
        SkeletonGroup *group = &m_groups[i];
        group->bmeshId = meshlite_bmesh_create(context);
        for (size_t j = 0; j < group->nodes.size(); j++) {
            SkeletonNode *node = &group->nodes[j];
            float x = (node->originX - left - height / 2) / height;
            float y = (node->originY - top - height / 2) / height;
            float z = (node->originZ - zLeft - zWidth / 2) / height;
            float r = node->radius / height;
            node->bmeshNodeId = meshlite_bmesh_add_node(context, group->bmeshId, x, y, z, r);
        }
        for (size_t j = 0; j < group->edges.size(); j++) {
            SkeletonNode *firstNode = &group->nodes[group->edges[j].firstNode];
            SkeletonNode *secondNode = &group->nodes[group->edges[j].secondNode];
            meshlite_bmesh_add_edge(context, group->bmeshId, firstNode->bmeshNodeId, secondNode->bmeshNodeId);
        }
        group->meshId = meshlite_bmesh_generate_mesh(context, group->bmeshId, group->nodes[group->rootNode].bmeshNodeId);
    }
    
    carve::poly::Polyhedron *unionPolyhedron = makeCarveMeshFromMeshlite(context, m_groups[0].meshId);
    for (size_t i = 1; i < m_groups.size(); i++) {
        carve::poly::Polyhedron *polyhedron = makeCarveMeshFromMeshlite(context, m_groups[i].meshId);
        carve::poly::Polyhedron *newUnionPolyhedron = unionCarveMeshs(unionPolyhedron, polyhedron);
        delete polyhedron;
        delete unionPolyhedron;
        unionPolyhedron = newUnionPolyhedron;
        if (NULL == unionPolyhedron) {
            break;
        }
    }
    
    for (size_t i = 1; i < m_groups.size(); i++) {
        meshlite_bmesh_destroy(context, m_groups[i].bmeshId);
    }
    
    if (unionPolyhedron) {
        int meshIdGeneratedFromCarve = makeMeshliteMeshFromCarve(context, unionPolyhedron);
        delete unionPolyhedron;
        
        int triangulate = meshlite_triangulate(context, meshIdGeneratedFromCarve);
        meshlite_export(context, triangulate, "/Users/jeremy/testlib.obj");
        m_mesh = new Mesh(context, triangulate);
    }
    meshlite_destroy_context(context);
    emit finished();
}
