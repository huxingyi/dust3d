#include "skeletontomesh.h"
#include "meshlite.h"
#include "skeletoneditnodeitem.h"
#include "skeletoneditedgeitem.h"
#include "skeletonsnapshot.h"
#include <vector>

#define USE_CARVE   0
#define USE_CGAL    1
#define USE_EXTERNAL    (USE_CARVE == 1 || USE_CGAL == 1)

#if USE_CARVE == 1
#if defined(HAVE_CONFIG_H)
#  include <carve_config.h>
#endif
#include <carve/carve.hpp>
#include <carve/csg.hpp>
#include <carve/input.hpp>
#endif

#define MAX_VERTICES_PER_FACE   100

#if USE_CGAL == 1
// Polygon_mesh_processing/corefinement_mesh_union.cpp
// https://doc.cgal.org/latest/Polygon_mesh_processing/Polygon_mesh_processing_2corefinement_mesh_union_8cpp-example.html#a2
// https://doc.cgal.org/latest/Polygon_mesh_processing/Polygon_mesh_processing_2triangulate_faces_example_8cpp-example.html
// https://github.com/CGAL/cgal/issues/2875
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <CGAL/boost/graph/iterator.h>

typedef CGAL::Exact_predicates_exact_constructions_kernel Kernel;
typedef CGAL::Surface_mesh<Kernel::Point_3>             CgalMesh;
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
        vertices.push_back(mesh->add_vertex(Kernel::Point_3(vertexPositions[offset + 0],
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
// https://www.cgal.org/FAQ.html

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
        Kernel::Point_3 point = mesh->point(*vertexIt);
        vertexPositions[offset++] = CGAL::to_double(point.x());
        vertexPositions[offset++] = CGAL::to_double(point.y());
        vertexPositions[offset++] = CGAL::to_double(point.z());
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

SkeletonToMesh::SkeletonToMesh(SkeletonSnapshot *snapshot) :
    m_mesh(NULL),
    m_snapshot(*snapshot)
{
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

#if USE_CARVE == 1
#define ExternalMesh                    carve::poly::Polyhedron
#define makeExternalMeshFromMeshlite    makeCarveMeshFromMeshlite
#define unionExternalMeshs              unionCarveMeshs
#define makeMeshliteMeshFromExternal    makeMeshliteMeshFromCarve
#endif

#if USE_CGAL == 1
#define ExternalMesh                    CgalMesh
#define makeExternalMeshFromMeshlite    makeCgalMeshFromMeshlite
#define unionExternalMeshs              unionCgalMeshs
#define makeMeshliteMeshFromExternal    makeMeshliteMeshFromCgal
#endif

void SkeletonToMesh::process()
{
    std::vector<SkeletonSnapshot> groups;
    QRectF globalFront = m_snapshot.boundingBoxFront();
    QRectF globalSide = m_snapshot.boundingBoxSide();
    m_snapshot.splitByConnectivity(&groups);
    
    std::vector<int> meshIds;
    void *meshliteContext = meshlite_create_context();
    for (size_t i = 0; i < groups.size(); i++) {
        SkeletonSnapshot *skeleton = &groups[i];
        QRectF front = skeleton->boundingBoxFront();
        QRectF side = skeleton->boundingBoxSide();
        float canvasWidth = skeleton->canvas["width"].toFloat();
        float canvasHeight = skeleton->canvas["height"].toFloat();
        
        std::map<QString, int> bmeshNodeMap;
        
        std::map<QString, std::map<QString, QString>>::iterator nodeIterator;
        int bmeshId = meshlite_bmesh_create(meshliteContext);
        for (nodeIterator = skeleton->nodes.begin(); nodeIterator != skeleton->nodes.end(); nodeIterator++) {
            if ("red" != nodeIterator->second["sideColorName"])
                continue;
            std::map<QString, std::map<QString, QString>>::iterator nextSidePair = skeleton->nodes.find(nodeIterator->second["nextSidePair"]);
            if (nextSidePair == skeleton->nodes.end())
                continue;
            float x = (nodeIterator->second["x"].toFloat() - globalFront.left()) / canvasHeight;
            float y = (nodeIterator->second["y"].toFloat() - globalFront.top()) / canvasHeight;
            float z = (nextSidePair->second["x"].toFloat() - globalSide.left()) / canvasHeight;
            float r = nodeIterator->second["radius"].toFloat() / canvasHeight;
            float t = nextSidePair->second["radius"].toFloat() / canvasHeight;
            int bmeshNodeId = meshlite_bmesh_add_node(meshliteContext, bmeshId, x, y, z, r, t);
            printf("meshlite_bmesh_add_node x:%f y:%f z:%f r:%f t:%f nodeName:%s bmeshNodeId:%d\n", x, y, z, r, t, nodeIterator->first.toUtf8().constData(), bmeshNodeId);
            bmeshNodeMap[nodeIterator->first] = bmeshNodeId;
        }
        
        std::map<QString, std::map<QString, QString>>::iterator edgeIterator;
        for (edgeIterator = skeleton->edges.begin(); edgeIterator != skeleton->edges.end(); edgeIterator++) {
            std::map<QString, int>::iterator from = bmeshNodeMap.find(edgeIterator->second["from"]);
            if (from == bmeshNodeMap.end())
                continue;
            std::map<QString, int>::iterator to = bmeshNodeMap.find(edgeIterator->second["to"]);
            if (to == bmeshNodeMap.end())
                continue;
            printf("meshlite_bmesh_add_edge %d -> %d\n", from->second, to->second);
            meshlite_bmesh_add_edge(meshliteContext, bmeshId, from->second, to->second);
        }
        
        if (bmeshNodeMap.size() > 0 && !skeleton->rootNode().isEmpty()) {
            int meshId = meshlite_bmesh_generate_mesh(meshliteContext, bmeshId, bmeshNodeMap[skeleton->rootNode()]);
            meshIds.push_back(meshId);
        }
        
        meshlite_bmesh_destroy(meshliteContext, bmeshId);
    }

    if (meshIds.size() > 0) {
#if USE_EXTERNAL
        std::vector<ExternalMesh *> externalMeshs;
        for (size_t i = 0; i < meshIds.size(); i++) {
            int triangledMeshId = meshlite_triangulate(meshliteContext, meshIds[i]);
            if (meshlite_is_triangulated_manifold(meshliteContext, triangledMeshId))
                externalMeshs.push_back(makeExternalMeshFromMeshlite(meshliteContext, triangledMeshId));
        }
        if (externalMeshs.size() > 0) {
            ExternalMesh *mergedExternalMesh = externalMeshs[0];
            for (size_t i = 1; i < externalMeshs.size(); i++) {
                if (!mergedExternalMesh)
                    break;
                ExternalMesh *unionedExternalMesh = unionExternalMeshs(mergedExternalMesh, externalMeshs[i]);
                delete mergedExternalMesh;
                delete externalMeshs[i];
                mergedExternalMesh = unionedExternalMesh;
            }
            if (mergedExternalMesh) {
                int mergedMeshId = makeMeshliteMeshFromExternal(meshliteContext, mergedExternalMesh);
                delete mergedExternalMesh;
                m_mesh = new Mesh(meshliteContext, mergedMeshId);
            }
        }
#else
        int mergedMeshId = meshIds[0];
        for (size_t i = 1; i < meshIds.size(); i++) {
            mergedMeshId = meshlite_merge(meshliteContext, mergedMeshId, meshIds[i]);
        }
        m_mesh = new Mesh(meshliteContext, mergedMeshId);
#endif
    }
    
    meshlite_destroy_context(meshliteContext);
    emit finished();
}
