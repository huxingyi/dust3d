#include <CGAL/boost/graph/graph_traits_Surface_mesh.h>
#include <CGAL/Polygon_mesh_processing/remesh.h>
#include <CGAL/Polygon_mesh_processing/border.h>
#include <boost/function_output_iterator.hpp>
#include "booleanmesh.h"
#include "isotropicremesh.h"

typedef boost::graph_traits<CgalMesh>::halfedge_descriptor halfedge_descriptor;
typedef boost::graph_traits<CgalMesh>::edge_descriptor     edge_descriptor;
namespace PMP = CGAL::Polygon_mesh_processing;

struct halfedge2edge
{
    halfedge2edge(const CgalMesh& m, std::vector<edge_descriptor>& edges)
        : m_mesh(m), m_edges(edges)
    {}
    void operator()(const halfedge_descriptor& h) const
    {
        m_edges.push_back(edge(h, m_mesh));
    }
    const CgalMesh& m_mesh;
    std::vector<edge_descriptor>& m_edges;
};

void isotropicRemesh(const std::vector<QVector3D> &inputVertices,
        std::vector<std::vector<size_t>> &inputTriangles,
        std::vector<QVector3D> &outputVertices,
        std::vector<std::vector<size_t>> &outputTriangles,
        float targetEdgeLength,
        unsigned int iterationNum)
{
    CgalMesh *mesh = buildCgalMesh<CgalKernel>(inputVertices, inputTriangles);
    if (nullptr == mesh)
        return;
    
    std::vector<edge_descriptor> border;
    PMP::border_halfedges(faces(*mesh),
        *mesh,
        boost::make_function_output_iterator(halfedge2edge(*mesh, border)));
    PMP::split_long_edges(border, targetEdgeLength, *mesh);
    
    PMP::isotropic_remeshing(faces(*mesh),
        targetEdgeLength,
        *mesh,
        PMP::parameters::number_of_iterations(iterationNum)
        .protect_constraints(true));
    
    outputVertices.clear();
    outputTriangles.clear();
    fetchFromCgalMesh<CgalKernel>(mesh, outputVertices, outputTriangles);
    
    delete mesh;
}
