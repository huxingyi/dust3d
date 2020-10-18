#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/boost/graph/graph_traits_Surface_mesh.h>
#include <CGAL/Polygon_mesh_processing/remesh.h>
#include <CGAL/Polygon_mesh_processing/border.h>
#include <CGAL/Polygon_mesh_processing/repair.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <boost/function_output_iterator.hpp>
#include "remeshhole.h"

typedef CGAL::Exact_predicates_inexact_constructions_kernel     Kernel;
typedef Kernel::Point_3                                         Point;
typedef CGAL::Surface_mesh<Kernel::Point_3>                     Mesh;
typedef boost::graph_traits<Mesh>::halfedge_descriptor          halfedge_descriptor;
typedef boost::graph_traits<Mesh>::edge_descriptor              edge_descriptor;
typedef boost::graph_traits<Mesh>::vertex_iterator              vertex_iterator;
typedef boost::graph_traits<Mesh>::vertex_descriptor            vertex_descriptor;

struct halfedge2edge
{
  halfedge2edge(const Mesh& m, std::vector<edge_descriptor>& edges)
    : m_mesh(m), m_edges(edges)
  {}
  void operator()(const halfedge_descriptor& h) const
  {
    m_edges.push_back(edge(h, m_mesh));
  }
  const Mesh& m_mesh;
  std::vector<edge_descriptor>& m_edges;
};

void remeshHole(std::vector<QVector3D> &vertices,
    const std::vector<size_t> &hole,
    std::vector<std::vector<size_t>> &newFaces)
{
    if (hole.empty())
        return;
    
    Mesh mesh;
    
    double targetEdgeLength = 0;
    for (size_t i = 1; i < hole.size(); ++i) {
        targetEdgeLength += (vertices[hole[i - 1]] - vertices[hole[i]]).length();
    }
    targetEdgeLength /= hole.size();
    
    std::vector<typename CGAL::Surface_mesh<typename Kernel::Point_3>::Vertex_index> meshFace;
    std::vector<size_t> originalIndices;
    originalIndices.reserve(hole.size());
    for (const auto &v: hole) {
        originalIndices.push_back(v);
        const auto &position = vertices[v];
        meshFace.push_back(mesh.add_vertex(Point(position.x(), position.y(), position.z())));
    }
    mesh.add_face(meshFace);
    
    std::vector<edge_descriptor> border;

    CGAL::Polygon_mesh_processing::triangulate_faces(mesh);
    
    CGAL::Polygon_mesh_processing::border_halfedges(faces(mesh),
        mesh,
        boost::make_function_output_iterator(halfedge2edge(mesh, border)));
        
    auto ecm = mesh.add_property_map<edge_descriptor, bool>("ecm").first;
    for (edge_descriptor e: border)
        ecm[e] = true;
    
    Mesh::Property_map<Mesh::Vertex_index, size_t> meshPropertyMap;
    bool created;
    boost::tie(meshPropertyMap, created) = mesh.add_property_map<Mesh::Vertex_index, size_t>("v:source", 0);
    
    size_t vertexIndex = 0;
    for (auto vertexIt = mesh.vertices_begin(); vertexIt != mesh.vertices_end(); vertexIt++) {
        meshPropertyMap[*vertexIt] = originalIndices[vertexIndex++];
    }
    
    unsigned int nb_iter = 3;
    
    CGAL::Polygon_mesh_processing::isotropic_remeshing(faces(mesh),
        targetEdgeLength,
        mesh,
        CGAL::Polygon_mesh_processing::parameters::number_of_iterations(nb_iter)
        .protect_constraints(true)
        .edge_is_constrained_map(ecm));
    
    for (auto vertexIt = mesh.vertices_begin() + vertexIndex; vertexIt != mesh.vertices_end(); vertexIt++) {
        auto point = mesh.point(*vertexIt);
        originalIndices.push_back(vertices.size());
        vertices.push_back(QVector3D(
            CGAL::to_double(point.x()),
            CGAL::to_double(point.y()),
            CGAL::to_double(point.z())
        ));
        meshPropertyMap[*vertexIt] = originalIndices[vertexIndex++];
    }
    
    for (const auto &faceIt: mesh.faces()) {
        CGAL::Vertex_around_face_iterator<Mesh> vbegin, vend;
        std::vector<size_t> faceIndices;
        for (boost::tie(vbegin, vend) = CGAL::vertices_around_face(mesh.halfedge(faceIt), mesh);
                vbegin != vend;
                ++vbegin) {
            faceIndices.push_back(meshPropertyMap[*vbegin]);
        }
        newFaces.push_back(faceIndices);
    }
}
