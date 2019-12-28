#include <config.hpp>
#include <field-math.hpp>
#include <optimizer.hpp>
#include <parametrizer.hpp>
#ifdef WITH_CUDA
#include <cuda_runtime.h>
#endif
#include "remesher.h"

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

const std::vector<QVector3D> &Remesher::getRemeshedVertices()
{
    return m_remeshedVertices;
}
const std::vector<std::vector<size_t>> &Remesher::getRemeshedFaces()
{
    return m_remeshedFaces;
}

void Remesher::remesh()
{
    Parametrizer field;
    
#ifdef WITH_CUDA
    cudaFree(0);
#endif
    
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
    
    printf("m_remeshedVertices.size:%lu\r\n", m_remeshedVertices.size());
    printf("m_remeshedFaces.size:%lu\r\n", m_remeshedFaces.size());
}
