#include <QDebug>
#include "meshutil.h"
#include "meshlite.h"

#define USE_CGAL    1

#define MAX_VERTICES_PER_FACE   100

#if USE_CGAL == 1
// Polygon_mesh_processing/corefinement_mesh_union.cpp
// https://doc.cgal.org/latest/Polygon_mesh_processing/Polygon_mesh_processing_2corefinement_mesh_union_8cpp-example.html#a2
// https://doc.cgal.org/latest/Polygon_mesh_processing/Polygon_mesh_processing_2triangulate_faces_example_8cpp-example.html
// https://github.com/CGAL/cgal/issues/2875
// https://doc.cgal.org/latest/Polygon_mesh_processing/Polygon_mesh_processing_2hausdorff_distance_remeshing_example_8cpp-example.html#a3
// https://github.com/CGAL/cgal/blob/master/Subdivision_method_3/examples/Subdivision_method_3/CatmullClark_subdivision.cpp
// https://doc.cgal.org/latest/Polygon_mesh_processing/index.html#title25
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <CGAL/boost/graph/iterator.h>
#include <CGAL/assertions_behaviour.h>
#include <CGAL/Polygon_mesh_processing/remesh.h>
#include <CGAL/subdivision_method_3.h>
#include <CGAL/Simple_cartesian.h>
#include <CGAL/convex_hull_3.h>

typedef CGAL::Exact_predicates_exact_constructions_kernel ExactKernel;
typedef CGAL::Simple_cartesian<double> SimpleKernel;
typedef CGAL::Surface_mesh<ExactKernel::Point_3> ExactMesh;
typedef CGAL::Surface_mesh<SimpleKernel::Point_3> SimpleMesh;
namespace PMP = CGAL::Polygon_mesh_processing;

void loadMeshVerticesPositions(void *meshliteContext, int meshId, std::vector<QVector3D> &positions)
{
    int vertexCount = meshlite_get_vertex_count(meshliteContext, meshId);
    float *vertexPositions = new float[vertexCount * 3];
    int vertexArrayLen = meshlite_get_vertex_position_array(meshliteContext, meshId, vertexPositions, vertexCount * 3);
    int offset = 0;
    assert(vertexArrayLen == vertexCount * 3);
    for (int i = 0; i < vertexCount; i++) {
        float x = vertexPositions[offset + 0];
        float y = vertexPositions[offset + 1];
        float z = vertexPositions[offset + 2];
        if (std::isnan(x) || std::isinf(x))
            x = 0;
        if (std::isnan(y) || std::isinf(y))
            y = 0;
        if (std::isnan(z) || std::isinf(z))
            z = 0;
        positions.push_back(QVector3D(x, y, z));
        offset += 3;
    }
    delete[] vertexPositions;
}

template <class Kernel>
typename CGAL::Surface_mesh<typename Kernel::Point_3> *makeCgalMeshFromMeshlite(void *meshlite, int meshId)
{
    typename CGAL::Surface_mesh<typename Kernel::Point_3> *mesh = new typename CGAL::Surface_mesh<typename Kernel::Point_3>;
    int vertexCount = meshlite_get_vertex_count(meshlite, meshId);
    float *vertexPositions = new float[vertexCount * 3];
    int vertexArrayLen = meshlite_get_vertex_position_array(meshlite, meshId, vertexPositions, vertexCount * 3);
    int offset = 0;
    assert(vertexArrayLen == vertexCount * 3);
    std::vector<QVector3D> oldPositions;
    std::map<int, typename CGAL::Surface_mesh<typename Kernel::Point_3>::Vertex_index> oldToNewMap;
    for (int i = 0; i < vertexCount; i++) {
        float x = vertexPositions[offset + 0];
        float y = vertexPositions[offset + 1];
        float z = vertexPositions[offset + 2];
        if (std::isnan(x) || std::isinf(x))
            x = 0;
        if (std::isnan(y) || std::isinf(y))
            y = 0;
        if (std::isnan(z) || std::isinf(z))
            z = 0;
        oldPositions.push_back(QVector3D(x, y, z));
        offset += 3;
    }
    int faceCount = meshlite_get_face_count(meshlite, meshId);
    int *faceVertexNumAndIndices = new int[faceCount * MAX_VERTICES_PER_FACE];
    int filledLength = meshlite_get_face_index_array(meshlite, meshId, faceVertexNumAndIndices, faceCount * MAX_VERTICES_PER_FACE);
    int i = 0;
    int addedFaceCount = 0;
    while (i < filledLength) {
        int num = faceVertexNumAndIndices[i++];
        assert(num > 0 && num <= MAX_VERTICES_PER_FACE);
        if (num < 3)
            continue;
        std::vector<typename CGAL::Surface_mesh<typename Kernel::Point_3>::Vertex_index> faceVertexIndices;
        for (int j = 0; j < num; j++) {
            int index = faceVertexNumAndIndices[i++];
            assert(index >= 0 && index < vertexCount);
            if (oldToNewMap.find(index) == oldToNewMap.end()) {
                const auto &pos = oldPositions[index];
                oldToNewMap[index] = mesh->add_vertex(typename Kernel::Point_3(pos.x(), pos.y(), pos.z()));
            }
            faceVertexIndices.push_back(oldToNewMap[index]);
        }
        mesh->add_face(faceVertexIndices);
        addedFaceCount++;
    }
    delete[] faceVertexNumAndIndices;
    delete[] vertexPositions;
    if (addedFaceCount < 3) {
        delete mesh;
        mesh = nullptr;
    }
    return mesh;
}

void loadCombinableMeshVerticesPositions(void *mesh, std::vector<QVector3D> &positions)
{
    ExactMesh *exactMesh = (ExactMesh *)mesh;
    if (nullptr == exactMesh)
        return;
    
    for (auto vertexIt = exactMesh->vertices_begin(); vertexIt != exactMesh->vertices_end(); vertexIt++) {
        auto point = exactMesh->point(*vertexIt);
        float x = (float)CGAL::to_double(point.x());
        float y = (float)CGAL::to_double(point.y());
        float z = (float)CGAL::to_double(point.z());
        if (std::isnan(x) || std::isinf(x))
            x = 0;
        if (std::isnan(y) || std::isinf(y))
            y = 0;
        if (std::isnan(z) || std::isinf(z))
            z = 0;
        positions.push_back(QVector3D(x, y, z));
    }
}

// https://doc.cgal.org/latest/Surface_mesh/index.html#circulators_example
// https://www.cgal.org/FAQ.html
template <class Kernel>
int makeMeshliteMeshFromCgal(void *meshlite, typename CGAL::Surface_mesh<typename Kernel::Point_3> *mesh)
{
    typename CGAL::Surface_mesh<typename Kernel::Point_3>::Vertex_range vertexRange = mesh->vertices();
    int vertexCount = vertexRange.size();
    float *vertexPositions = new float[vertexCount * 3];
    int offset = 0;
    typename CGAL::Surface_mesh<typename Kernel::Point_3>::Vertex_range::iterator vertexIt;
    std::map<typename CGAL::Surface_mesh<typename Kernel::Point_3>::Vertex_index, int> vertexIndexMap;
    int i = 0;
    for (vertexIt = vertexRange.begin(); vertexIt != vertexRange.end(); vertexIt++) {
        typename Kernel::Point_3 point = mesh->point(*vertexIt);
        float x = (float)CGAL::to_double(point.x());
        float y = (float)CGAL::to_double(point.y());
        float z = (float)CGAL::to_double(point.z());
        if (std::isnan(x) || std::isinf(x))
            x = 0;
        if (std::isnan(y) || std::isinf(y))
            y = 0;
        if (std::isnan(z) || std::isinf(z))
            z = 0;
        vertexPositions[offset++] = x;
        vertexPositions[offset++] = y;
        vertexPositions[offset++] = z;
        //printf("vertexPositions[%d]: %f,%f,%f\n", i, vertexPositions[offset - 3], vertexPositions[offset - 2], vertexPositions[offset - 1]);
        vertexIndexMap[*vertexIt] = i;
        i++;
    }
    typename CGAL::Surface_mesh<typename Kernel::Point_3>::Face_range faceRage = mesh->faces();
    int faceCount = faceRage.size();
    int *faceVertexNumAndIndices = new int[faceCount * MAX_VERTICES_PER_FACE];
    typename CGAL::Surface_mesh<typename Kernel::Point_3>::Face_range::iterator faceIt;
    offset = 0;
    for (faceIt = faceRage.begin(); faceIt != faceRage.end(); faceIt++) {
        CGAL::Vertex_around_face_iterator<typename CGAL::Surface_mesh<typename Kernel::Point_3>> vbegin, vend;
        std::vector<int> indices;
        //printf("face begin\n");
        for (boost::tie(vbegin, vend) = CGAL::vertices_around_face(mesh->halfedge(*faceIt), *mesh);
                vbegin != vend;
                ++vbegin){
            //printf(" %d", vertexIndexMap[*vbegin]);
            indices.push_back(vertexIndexMap[*vbegin]);
        }
        //printf("\nface end\n");
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

template <class InputIterator, class Kernel>
bool precheckForConvexHull(InputIterator first, InputIterator beyond)
{
  typedef typename CGAL::internal::Convex_hull_3::Default_traits_for_Chull_3<typename Kernel::Point_3, typename CGAL::Surface_mesh<typename Kernel::Point_3>>::type Traits;
  typedef typename Traits::Point_3                Point_3;
  typedef std::list<Point_3>                      Point_3_list;
  typedef typename Point_3_list::iterator         P3_iterator;
  Traits traits;

  Point_3_list points(first, beyond);
  if (!(points.size() > 3))
    return false;

  // at least 4 points
  typename Traits::Collinear_3 collinear = traits.collinear_3_object();
  typename Traits::Equal_3 equal = traits.equal_3_object();

  P3_iterator point1_it = points.begin();
  P3_iterator point2_it = points.begin();
  point2_it++;

  // find three that are not collinear
  while (point2_it != points.end() && equal(*point1_it,*point2_it))
    ++point2_it;

  if (!(point2_it != points.end()))
    return false;
  
  P3_iterator point3_it = point2_it;
  ++point3_it;
  
  if (!(point3_it != points.end()))
    return false;
  
  while (point3_it != points.end() && collinear(*point1_it,*point2_it,*point3_it))
    ++point3_it;
  
  if (!(point3_it != points.end()))
    return false;
  
  return true;
}

template <class Kernel>
ExactMesh *makeCgalConvexHullMeshFromMeshlite(void *meshlite, int meshId)
{
    typename CGAL::Surface_mesh<typename Kernel::Point_3> *mesh = new typename CGAL::Surface_mesh<typename Kernel::Point_3>;
    int vertexCount = meshlite_get_vertex_count(meshlite, meshId);
    float *vertexPositions = new float[vertexCount * 3];
    int vertexArrayLen = meshlite_get_vertex_position_array(meshlite, meshId, vertexPositions, vertexCount * 3);
    int offset = 0;
    assert(vertexArrayLen == vertexCount * 3);
    std::vector<typename Kernel::Point_3> points;
    for (int i = 0; i < vertexCount; i++) {
        float x = vertexPositions[offset + 0];
        float y = vertexPositions[offset + 1];
        float z = vertexPositions[offset + 2];
        if (std::isnan(x) || std::isinf(x))
            x = 0;
        if (std::isnan(y) || std::isinf(y))
            y = 0;
        if (std::isnan(z) || std::isinf(z))
            z = 0;
        points.push_back(typename Kernel::Point_3(x, y, z));
        offset += 3;
    }
    delete[] vertexPositions;
    try {
        if (precheckForConvexHull<typename std::vector<typename Kernel::Point_3>::iterator, Kernel>(points.begin(), points.end())) {
            CGAL::convex_hull_3(points.begin(), points.end(), *mesh);
        } else {
            delete mesh;
            return nullptr;
        }
    } catch (...) {
        delete mesh;
        return nullptr;
    }
    return mesh;
}

ExactMesh *unionCgalMeshs(ExactMesh *first, ExactMesh *second)
{
    ExactMesh *mesh = new ExactMesh;
    try {
        if (!PMP::corefine_and_compute_union(*first, *second, *mesh)) {
            delete mesh;
            return nullptr;
        }
    } catch (...) {
        delete mesh;
        return nullptr;
    }
    return mesh;
}

ExactMesh *diffCgalMeshs(ExactMesh *first, ExactMesh *second)
{
    ExactMesh *mesh = new ExactMesh;
    try {
        if (!PMP::corefine_and_compute_difference(*first, *second, *mesh)) {
            delete mesh;
            return nullptr;
        }
    } catch (...) {
        delete mesh;
        return nullptr;
    }
    return mesh;
}

#endif

// http://cgal-discuss.949826.n4.nabble.com/Polygon-mesh-processing-corefine-and-compute-difference-td4663104.html
int unionMeshs(void *meshliteContext, const std::vector<int> &meshIds, const std::set<int> &inverseIds, int *errorCount)
{
#if USE_CGAL == 1
    std::vector<ExactMesh *> externalMeshs;
    for (size_t i = 0; i < meshIds.size(); i++) {
        int triangledMeshId = meshlite_triangulate(meshliteContext, meshIds[i]);
        ExactMesh *mesh = makeCgalMeshFromMeshlite<ExactKernel>(meshliteContext, triangledMeshId);
        if (CGAL::Polygon_mesh_processing::does_self_intersect(*mesh)) {
            qDebug() << "CGAL::Polygon_mesh_processing::does_self_intersect:" << i;
            if (errorCount)
                (*errorCount)++;
            delete mesh;
            continue;
        }
        externalMeshs.push_back(mesh);
    }
    if (externalMeshs.size() > 0) {
        ExactMesh *mergedExternalMesh = externalMeshs[0];
        for (size_t i = 1; i < externalMeshs.size(); i++) {
            if (!mergedExternalMesh) {
                qDebug() << "Last union failed, break with remains unprocessed:" << (externalMeshs.size() - i);
                break;
            }
            ExactMesh *unionedExternalMesh = NULL;
            try {
                if (inverseIds.find(meshIds[i]) == inverseIds.end())
                    unionedExternalMesh = unionCgalMeshs(mergedExternalMesh, externalMeshs[i]);
                else
                    unionedExternalMesh = diffCgalMeshs(mergedExternalMesh, externalMeshs[i]);
            } catch (...) {
                qDebug() << "unionCgalMeshs throw exception";
                if (errorCount)
                    (*errorCount)++;
            }
            delete externalMeshs[i];
            if (unionedExternalMesh) {
                delete mergedExternalMesh;
                mergedExternalMesh = unionedExternalMesh;
            } else {
                if (errorCount)
                    (*errorCount)++;
                qDebug() << "unionCgalMeshs failed";
            }
        }
        if (mergedExternalMesh) {
            int mergedMeshId = makeMeshliteMeshFromCgal<ExactKernel>(meshliteContext, mergedExternalMesh);
            if (mergedMeshId)
                mergedMeshId = fixMeshHoles(meshliteContext, mergedMeshId);
            else {
                if (errorCount)
                    (*errorCount)++;
                qDebug() << "makeMeshliteMeshFromCgal failed";
            }
            delete mergedExternalMesh;
            return mergedMeshId;
        }
    }
#else
    return mergeMeshs(meshliteContext, meshIds);
#endif
    return 0;
}

int mergeMeshs(void *meshliteContext, const std::vector<int> &meshIds)
{
    int mergedMeshId = meshIds[0];
    for (size_t i = 1; i < meshIds.size(); i++) {
        mergedMeshId = meshlite_merge(meshliteContext, mergedMeshId, meshIds[i]);
    }
    return mergedMeshId;
}

int subdivMesh(void *meshliteContext, int meshId, int *errorCount)
{
    int triangulatedMeshId = meshlite_triangulate(meshliteContext, meshId);
    if (0 == meshlite_is_triangulated_manifold(meshliteContext, triangulatedMeshId)) {
#if USE_CGAL == 1
        int subdiviedMeshId = 0;
        SimpleMesh *simpleMesh = nullptr;
        try {
            simpleMesh = makeCgalMeshFromMeshlite<SimpleKernel>(meshliteContext, triangulatedMeshId);
            CGAL::Subdivision_method_3::CatmullClark_subdivision(*simpleMesh, CGAL::Polygon_mesh_processing::parameters::number_of_iterations(1));
            subdiviedMeshId = makeMeshliteMeshFromCgal<SimpleKernel>(meshliteContext, simpleMesh);
        } catch (...) {
            if (errorCount)
                (*errorCount)++;
        }
        if (simpleMesh)
            delete simpleMesh;
        if (subdiviedMeshId)
            subdiviedMeshId = fixMeshHoles(meshliteContext, subdiviedMeshId);
        return subdiviedMeshId;
#endif
    }
    return meshlite_subdivide(meshliteContext, meshId);
}

int fixMeshHoles(void *meshliteContext, int meshId)
{
    return meshlite_fix_hole(meshliteContext, meshId);
}

void initMeshUtils()
{
    CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
}

void *convertToCombinableMesh(void *meshliteContext, int meshId)
{
    ExactMesh *mesh = nullptr;
    if (0 == meshId)
        return nullptr;
    mesh = makeCgalMeshFromMeshlite<ExactKernel>(meshliteContext, meshId);
    if (nullptr == mesh) {
        return mesh;
    }
    if (CGAL::Polygon_mesh_processing::does_self_intersect(*mesh)) {
        qDebug() << "CGAL::Polygon_mesh_processing::does_self_intersect meshId:" << meshId;
        delete mesh;
        return nullptr;
    }
    return (void *)mesh;
}

void *unionCombinableMeshs(void *first, void *second)
{
    return (void *)unionCgalMeshs((ExactMesh *)first, (ExactMesh *)second);
}

void *diffCombinableMeshs(void *first, void *second)
{
    return (void *)diffCgalMeshs((ExactMesh *)first, (ExactMesh *)second);
}

int convertFromCombinableMesh(void *meshliteContext, void *mesh)
{
    return makeMeshliteMeshFromCgal<ExactKernel>(meshliteContext, (ExactMesh *)mesh);
}

void deleteCombinableMesh(void *mesh)
{
    delete (ExactMesh *)mesh;
}

void *cloneCombinableMesh(void *mesh)
{
    if (nullptr == mesh)
        return nullptr;
    return (void *)new ExactMesh(*(ExactMesh *)mesh);
}

void *convertToCombinableConvexHullMesh(void *meshliteContext, int meshId)
{
    ExactMesh *mesh = makeCgalConvexHullMeshFromMeshlite<ExactKernel>(meshliteContext, meshId);
    return (void *)mesh;
}
