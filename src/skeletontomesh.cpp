#include "skeletontomesh.h"
#include "meshlite.h"
#include "skeletoneditnodeitem.h"
#include "skeletoneditedgeitem.h"
#include <vector>

#define USE_CARVE 1

#if USE_CARVE == 1
#if defined(HAVE_CONFIG_H)
#  include <carve_config.h>
#endif
#include <carve/carve.hpp>
#include <carve/csg.hpp>
#include <carve/input.hpp>
#endif

#define MAX_VERTICES_PER_FACE   100

#if USE_CARVE == 0
// Polygon_mesh_processing/corefinement_mesh_union.cpp
// https://doc.cgal.org/latest/Polygon_mesh_processing/Polygon_mesh_processing_2corefinement_mesh_union_8cpp-example.html#a2
// https://doc.cgal.org/latest/Polygon_mesh_processing/Polygon_mesh_processing_2triangulate_faces_example_8cpp-example.html
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <CGAL/boost/graph/iterator.h>

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Surface_mesh<K::Point_3>             CgalMesh;
namespace PMP = CGAL::Polygon_mesh_processing;

CgalMesh *makeCgalMeshFromMeshlite(void *meshlite, int meshId)
{
    CgalMesh *mesh = new CgalMesh;
    int vertexCount = meshlite_get_vertex_count(meshlite, meshId);
    float *vertexPositions = new float[vertexCount * 3];
    int vertexArrayLen = meshlite_get_vertex_position_array(meshlite, meshId, vertexPositions, vertexCount * 3);
    int offset = 0;
    assert(vertexArrayLen == vertexCount * 3);
    std::vector<CgalMesh::Vertex_index> vertices;
    for (int i = 0; i < vertexCount; i++) {
        vertices.push_back(mesh->add_vertex(K::Point_3(vertexPositions[offset + 0],
            vertexPositions[offset + 1],
            vertexPositions[offset + 2])));
        offset += 3;
    }
    int faceCount = meshlite_get_face_count(meshlite, meshId);
    int *faceVertexNumAndIndices = new int[faceCount * MAX_VERTICES_PER_FACE];
    int filledLength = meshlite_get_face_index_array(meshlite, meshId, faceVertexNumAndIndices, faceCount * MAX_VERTICES_PER_FACE);
    int i = 0;
    while (i < filledLength) {
        int num = faceVertexNumAndIndices[i++];
        assert(num > 0 && num <= MAX_VERTICES_PER_FACE);
        std::vector<CgalMesh::Vertex_index> faceIndices;
        for (int j = 0; j < num; j++) {
            int index = faceVertexNumAndIndices[i++];
            assert(index >= 0 && index < vertexCount);
            faceIndices.push_back(vertices[index]);
        }
        mesh->add_face(faceIndices);
    }
    delete[] faceVertexNumAndIndices;
    delete[] vertexPositions;
    return mesh;
}

// https://doc.cgal.org/latest/Surface_mesh/index.html#circulators_example

int makeMeshliteMeshFromCgal(void *meshlite, CgalMesh *mesh)
{
    CgalMesh::Vertex_range vertexRange = mesh->vertices();
    int vertexCount = vertexRange.size();
    float *vertexPositions = new float[vertexCount * 3];
    int offset = 0;
    CgalMesh::Vertex_range::iterator vertexIt;
    std::map<CgalMesh::Vertex_index, int> vertexIndexMap;
    int i = 0;
    for (vertexIt = vertexRange.begin(); vertexIt != vertexRange.end(); vertexIt++) {
        auto point = mesh->point(*vertexIt);
        vertexPositions[offset++] = point.x();
        vertexPositions[offset++] = point.y();
        vertexPositions[offset++] = point.z();
        vertexIndexMap[*vertexIt] = i;
        i++;
    }
    CgalMesh::Face_range faceRage = mesh->faces();
    int faceCount = faceRage.size();
    int *faceVertexNumAndIndices = new int[faceCount * MAX_VERTICES_PER_FACE];
    CgalMesh::Face_range::iterator faceIt;
    offset = 0;
    for (faceIt = faceRage.begin(); faceIt != faceRage.end(); faceIt++) {
        CGAL::Vertex_around_face_iterator<CgalMesh> vbegin, vend;
        std::vector<int> indices;
        for (boost::tie(vbegin, vend) = CGAL::vertices_around_face(mesh->halfedge(*faceIt), *mesh);
                vbegin != vend;
                ++vbegin){
            indices.push_back(vertexIndexMap[*vbegin]);
        }
        faceVertexNumAndIndices[offset++] = indices.size();
        for (int j = 0; j < (int)indices.size(); j++) {
            faceVertexNumAndIndices[offset++] = indices[j];
        }
    }
    int meshId = meshlite_build(meshlite, vertexPositions, vertexCount, faceVertexNumAndIndices, offset);
    delete[] vertexPositions;
    delete[] faceVertexNumAndIndices;
    return meshId;
}

CgalMesh *unionCgalMeshs(CgalMesh *first, CgalMesh *second)
{
    CgalMesh *mesh = new CgalMesh;
    if (!PMP::corefine_and_compute_union(*first, *second, *mesh)) {
        return NULL;
    }
    return mesh;
}

#endif

#if USE_CARVE == 1

carve::poly::Polyhedron *makeCarveMeshFromMeshlite(void *meshlite, int meshId)
{
    carve::input::PolyhedronData data;
    int vertexCount = meshlite_get_vertex_count(meshlite, meshId);
    float *vertexPositions = new float[vertexCount * 3];
    int vertexArrayLen = meshlite_get_vertex_position_array(meshlite, meshId, vertexPositions, vertexCount * 3);
    int offset = 0;
    assert(vertexArrayLen == vertexCount * 3);
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
        assert(num > 0 && num <= MAX_VERTICES_PER_FACE);
        std::vector<int> faceIndices;
        for (int j = 0; j < num; j++) {
            int index = faceVertexNumAndIndices[i++];
            assert(index >= 0 && index < vertexCount);
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

#endif

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
                    node.thickness = nodeItem->slave()->radius();
                    
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

#if USE_CARVE
#define ExternalMesh                    carve::poly::Polyhedron
#define makeExternalMeshFromMeshlite    makeCarveMeshFromMeshlite
#define unionExternalMeshs              unionCarveMeshs
#define makeMeshliteMeshFromExternal    makeMeshliteMeshFromCarve
#else
#define ExternalMesh                    CgalMesh
#define makeExternalMeshFromMeshlite    makeCgalMeshFromMeshlite
#define unionExternalMeshs              unionCgalMeshs
#define makeMeshliteMeshFromExternal    makeMeshliteMeshFromCgal
#endif

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
            float t = node->thickness / height;
            node->bmeshNodeId = meshlite_bmesh_add_node(context, group->bmeshId, x, y, z, r, t);
        }
        for (size_t j = 0; j < group->edges.size(); j++) {
            SkeletonNode *firstNode = &group->nodes[group->edges[j].firstNode];
            SkeletonNode *secondNode = &group->nodes[group->edges[j].secondNode];
            meshlite_bmesh_add_edge(context, group->bmeshId, firstNode->bmeshNodeId, secondNode->bmeshNodeId);
        }
        group->meshId = meshlite_bmesh_generate_mesh(context, group->bmeshId, group->nodes[group->rootNode].bmeshNodeId);
    }
    
    int mergedMeshId = m_groups[0].meshId;
    for (size_t i = 1; i < m_groups.size(); i++) {
        mergedMeshId = meshlite_merge(context, mergedMeshId, m_groups[i].meshId);
    }
    
    /*
    ExternalMesh *unionPolyhedron = makeExternalMeshFromMeshlite(context, m_groups[0].meshId);
    for (size_t i = 1; i < m_groups.size(); i++) {
        ExternalMesh *polyhedron = makeExternalMeshFromMeshlite(context, m_groups[i].meshId);
        ExternalMesh *newUnionPolyhedron = unionExternalMeshs(unionPolyhedron, polyhedron);
        delete polyhedron;
        delete unionPolyhedron;
        unionPolyhedron = newUnionPolyhedron;
        if (NULL == unionPolyhedron) {
            break;
        }
    }
    */
    
    for (size_t i = 1; i < m_groups.size(); i++) {
        meshlite_bmesh_destroy(context, m_groups[i].bmeshId);
    }
    
    /*
    if (unionPolyhedron) {
        int meshIdGeneratedFromExternal = makeMeshliteMeshFromExternal(context, unionPolyhedron);
        delete unionPolyhedron;
        
        meshlite_export(context, meshIdGeneratedFromExternal, "/Users/jeremy/testlib.obj");
        m_mesh = new Mesh(context, meshIdGeneratedFromExternal);
    }
    */
    
    m_mesh = new Mesh(context, mergedMeshId);
    
    meshlite_destroy_context(context);
    emit finished();
}
