#ifndef NODEMESH_CGAL_MESH_H
#define NODEMESH_CGAL_MESH_H
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>
#include <QVector3D>
#include <vector>
#include <cmath>
#include <nodemesh/misc.h>

typedef CGAL::Exact_predicates_exact_constructions_kernel ExactKernel;
typedef CGAL::Surface_mesh<ExactKernel::Point_3> ExactMesh;

template <class Kernel>
typename CGAL::Surface_mesh<typename Kernel::Point_3> *buildCgalMesh(const std::vector<QVector3D> &positions, const std::vector<std::vector<size_t>> &indices)
{
    typename CGAL::Surface_mesh<typename Kernel::Point_3> *mesh = new typename CGAL::Surface_mesh<typename Kernel::Point_3>;
    std::map<size_t, typename CGAL::Surface_mesh<typename Kernel::Point_3>::Vertex_index> vertexIndices;
    for (const auto &face: indices) {
        std::vector<typename CGAL::Surface_mesh<typename Kernel::Point_3>::Vertex_index> faceVertexIndices;
        bool faceValid = true;
        for (const auto &index: face) {
            const auto &pos = positions[index];
            if (!nodemesh::validatePosition(pos)) {
                faceValid = false;
                break;
            }
        }
        if (!faceValid)
            continue;
        for (const auto &index: face) {
            auto findIndex = vertexIndices.find(index);
            if (findIndex != vertexIndices.end()) {
                faceVertexIndices.push_back(findIndex->second);
            } else {
                const auto &pos = positions[index];
                auto newIndex = mesh->add_vertex(typename Kernel::Point_3(pos.x(), pos.y(), pos.z()));
                vertexIndices.insert({index, newIndex});
                faceVertexIndices.push_back(newIndex);
            }
        }
        mesh->add_face(faceVertexIndices);
    }
    return mesh;
}

template <class Kernel>
void fetchFromCgalMesh(typename CGAL::Surface_mesh<typename Kernel::Point_3> *mesh, std::vector<QVector3D> &vertices, std::vector<std::vector<size_t>> &faces)
{
    std::map<typename CGAL::Surface_mesh<typename Kernel::Point_3>::Vertex_index, size_t> vertexIndicesMap;
    for (auto vertexIt = mesh->vertices_begin(); vertexIt != mesh->vertices_end(); vertexIt++) {
        auto point = mesh->point(*vertexIt);
        float x = (float)CGAL::to_double(point.x());
        float y = (float)CGAL::to_double(point.y());
        float z = (float)CGAL::to_double(point.z());
        vertexIndicesMap[*vertexIt] = vertices.size();
        vertices.push_back(QVector3D(x, y, z));
    }
    
    typename CGAL::Surface_mesh<typename Kernel::Point_3>::Face_range faceRage = mesh->faces();
    typename CGAL::Surface_mesh<typename Kernel::Point_3>::Face_range::iterator faceIt;
    for (faceIt = faceRage.begin(); faceIt != faceRage.end(); faceIt++) {
        CGAL::Vertex_around_face_iterator<typename CGAL::Surface_mesh<typename Kernel::Point_3>> vbegin, vend;
        std::vector<size_t> faceIndices;
        for (boost::tie(vbegin, vend) = CGAL::vertices_around_face(mesh->halfedge(*faceIt), *mesh);
                vbegin != vend;
                ++vbegin){
            faceIndices.push_back(vertexIndicesMap[*vbegin]);
        }
        faces.push_back(faceIndices);
    }
}

#endif

