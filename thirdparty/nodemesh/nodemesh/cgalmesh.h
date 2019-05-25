#ifndef NODEMESH_CGAL_MESH_H
#define NODEMESH_CGAL_MESH_H
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>
#include <QVector3D>
#include <vector>
#include <cmath>
#include <nodemesh/misc.h>
#include <nodemesh/positionkey.h>

typedef CGAL::Exact_predicates_inexact_constructions_kernel CgalKernel;
typedef CGAL::Surface_mesh<CgalKernel::Point_3> CgalMesh;

template <class Kernel>
typename CGAL::Surface_mesh<typename Kernel::Point_3> *buildCgalMesh(const std::vector<QVector3D> &positions, const std::vector<std::vector<size_t>> &indices)
{
    typename CGAL::Surface_mesh<typename Kernel::Point_3> *mesh = new typename CGAL::Surface_mesh<typename Kernel::Point_3>;
    std::map<nodemesh::PositionKey, typename CGAL::Surface_mesh<typename Kernel::Point_3>::Vertex_index> vertexIndices;
    for (const auto &face: indices) {
        std::vector<typename CGAL::Surface_mesh<typename Kernel::Point_3>::Vertex_index> faceVertexIndices;
        bool faceValid = true;
        std::vector<nodemesh::PositionKey> positionKeys;
        std::vector<QVector3D> positionsInKeys;
        std::set<nodemesh::PositionKey> existedKeys;
        for (const auto &index: face) {
            const auto &position = positions[index];
            if (!nodemesh::validatePosition(position)) {
                faceValid = false;
                break;
            }
            auto positionKey = nodemesh::PositionKey(position);
            if (existedKeys.find(positionKey) != existedKeys.end()) {
                continue;
            }
            existedKeys.insert(positionKey);
            positionKeys.push_back(positionKey);
            positionsInKeys.push_back(position);
        }
        if (!faceValid)
            continue;
        if (positionKeys.size() < 3)
            continue;
        for (size_t index = 0; index < positionKeys.size(); ++index) {
            const auto &position = positionsInKeys[index];
            const auto &positionKey = positionKeys[index];
            auto findIndex = vertexIndices.find(positionKey);
            if (findIndex != vertexIndices.end()) {
                faceVertexIndices.push_back(findIndex->second);
            } else {
                auto newIndex = mesh->add_vertex(typename Kernel::Point_3(position.x(), position.y(), position.z()));
                vertexIndices.insert({positionKey, newIndex});
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

template <class Kernel>
bool isNullCgalMesh(typename CGAL::Surface_mesh<typename Kernel::Point_3> *mesh)
{
    typename CGAL::Surface_mesh<typename Kernel::Point_3>::Face_range faceRage = mesh->faces();
    return faceRage.begin() == faceRage.end();
}

#endif

