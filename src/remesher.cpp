#include <config.hpp>
#include <field-math.hpp>
#include <optimizer.hpp>
#include <parametrizer.hpp>
#include <cmath>
#ifdef WITH_CUDA
#include <cuda_runtime.h>
#endif
#include <CGAL/boost/graph/graph_traits_Surface_mesh.h>
#include <CGAL/Polygon_mesh_processing/remesh.h>
#include <CGAL/Polygon_mesh_processing/border.h>
#include <boost/function_output_iterator.hpp>
#include <fstream>
#include <vector>
#include "remesher.h"
#include "booleanmesh.h"

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

using namespace qflow;

Remesher::~Remesher()
{
}

void Remesher::setMesh(const std::vector<QVector3D> &vertices,
        const std::vector<std::vector<size_t>> &triangles)
{
    m_vertices = vertices;
    m_triangles = triangles;
}

const std::vector<QVector3D> &Remesher::getRemeshedVertices() const
{
    return m_remeshedVertices;
}

const std::vector<std::vector<size_t>> &Remesher::getRemeshedFaces() const
{
    return m_remeshedFaces;
}

const std::vector<std::pair<QUuid, QUuid>> &Remesher::getRemeshedVertexSources() const
{
    return m_remeshedVertexSources;
}

void Remesher::isotropicRemesh(float targetEdgeLength, unsigned int iterationNum)
{
    CgalMesh *mesh = buildCgalMesh<CgalKernel>(m_vertices, m_triangles);
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
    
    m_vertices.clear();
    m_triangles.clear();
    fetchFromCgalMesh<CgalKernel>(mesh, m_vertices, m_triangles);
    
    delete mesh;
}

void Remesher::remesh()
{
    Parametrizer field;
    
#ifdef WITH_CUDA
    cudaFree(0);
#endif
    
    isotropicRemesh();
    
    field.V.resize(3, m_vertices.size());
    field.F.resize(3, m_triangles.size());
    for (decltype(m_vertices.size()) i = 0; i < m_vertices.size(); i++) {
        const auto &vertex = m_vertices[i];
        field.V.col(i) << (double)vertex.x(), (double)vertex.y(), (double)vertex.z();
    }
    for (decltype(m_triangles.size()) i = 0; i < m_triangles.size(); i++) {
        const auto &face = m_triangles[i];
        field.F.col(i) << (uint32_t)face[0], (uint32_t)face[1], (uint32_t)face[2];
    }
    field.NormalizeMesh();
    
    int faces = -1;
    field.Initialize(faces);
    
    if (field.flag_preserve_boundary) {
        Hierarchy& hierarchy = field.hierarchy;
        hierarchy.clearConstraints();
        for (uint32_t i = 0; i < 3 * hierarchy.mF.cols(); ++i) {
            if (hierarchy.mE2E[i] == -1) {
                uint32_t i0 = hierarchy.mF(i % 3, i / 3);
                uint32_t i1 = hierarchy.mF((i + 1) % 3, i / 3);
                Vector3d p0 = hierarchy.mV[0].col(i0), p1 = hierarchy.mV[0].col(i1);
                Vector3d edge = p1 - p0;
                if (edge.squaredNorm() > 0) {
                    edge.normalize();
                    hierarchy.mCO[0].col(i0) = p0;
                    hierarchy.mCO[0].col(i1) = p1;
                    hierarchy.mCQ[0].col(i0) = hierarchy.mCQ[0].col(i1) = edge;
                    hierarchy.mCQw[0][i0] = hierarchy.mCQw[0][i1] = hierarchy.mCOw[0][i0] = hierarchy.mCOw[0][i1] =
                        1.0;
                }
            }
        }
        hierarchy.propagateConstraints();
    }
    
    Optimizer::optimize_orientations(field.hierarchy);
    field.ComputeOrientationSingularities();
    
    if (field.flag_adaptive_scale == 1) {
        field.EstimateSlope();
    }
    
    Optimizer::optimize_scale(field.hierarchy, field.rho, field.flag_adaptive_scale);
    field.flag_adaptive_scale = 1;
    
    Optimizer::optimize_positions(field.hierarchy, field.flag_adaptive_scale);
    field.ComputePositionSingularities();
    
    field.ComputeIndexMap();
    
    m_remeshedVertices.reserve(field.O_compact.size());
    for (size_t i = 0; i < field.O_compact.size(); ++i) {
        auto t = field.O_compact[i] * field.normalize_scale + field.normalize_offset;
        m_remeshedVertices.push_back(QVector3D(t[0], t[1], t[2]));
    }
    m_remeshedFaces.reserve(field.F_compact.size());
    for (size_t i = 0; i < field.F_compact.size(); ++i) {
        m_remeshedFaces.push_back(std::vector<size_t> {
            (size_t)field.F_compact[i][0],
            (size_t)field.F_compact[i][1],
            (size_t)field.F_compact[i][2],
            (size_t)field.F_compact[i][3]
        });
    }
    
    resolveSources();
}

void Remesher::setNodes(const std::vector<std::pair<QVector3D, float>> &nodes,
        const std::vector<std::pair<QUuid, QUuid>> &sourceIds)
{
    m_nodes = nodes;
    m_sourceIds = sourceIds;
}

void Remesher::resolveSources()
{
    m_remeshedVertexSources.resize(m_remeshedVertices.size());
    std::vector<float> nodeRadius2(m_nodes.size());
    for (size_t nodeIndex = 0; nodeIndex < m_nodes.size(); ++nodeIndex) {
        nodeRadius2[nodeIndex] = std::pow(m_nodes[nodeIndex].second, 2);
    }
    for (size_t vertexIndex = 0; vertexIndex < m_remeshedVertices.size(); ++vertexIndex) {
        std::vector<std::pair<float, size_t>> matches;
        const auto &vertexPosition = m_remeshedVertices[vertexIndex];
        for (size_t nodeIndex = 0; nodeIndex < m_nodes.size(); ++nodeIndex) {
            const auto &nodePosition = m_nodes[nodeIndex].first;
            auto length2 = (vertexPosition - nodePosition).lengthSquared();
            if (length2 > nodeRadius2[nodeIndex])
                continue;
            matches.push_back(std::make_pair(length2, nodeIndex));
        }
        std::sort(matches.begin(), matches.end(), [](const std::pair<float, size_t> &first,
                const std::pair<float, size_t> &second) {
            return first.first < second.first;
        });
        if (matches.empty())
            continue;
        m_remeshedVertexSources[vertexIndex] = m_sourceIds[matches[0].second];
    }
}
