/*
This file is an unofficial API exporter for Instant-Meshes on Windows.

Copyright (c) 2015 Wenzel Jakob, Daniele Panozzo, Marco Tarini,
and Olga Sorkine-Hornung. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

You are under no obligation whatsoever to provide any bug fixes, patches, or
upgrades to the features, functionality or performance of the source code
("Enhancements") to anyone; however, if you choose to make your Enhancements
available either publicly, or directly to the authors of this software, without
imposing a separate written license agreement for such Enhancements, then you
hereby grant the following license: a non-exclusive, royalty-free perpetual
license to install, use, modify, prepare derivative works, incorporate into
other computer software, distribute, and sublicense such enhancements or
derivative works thereof, in binary and source code form.
*/
#include <cmath>
#include <batch.h>
#include <meshio.h>
#include <dedge.h>
#include <subdivide.h>
#include <meshstats.h>
#include <hierarchy.h>
#include <field.h>
#include <normal.h>
#include <extract.h>
#include <bvh.h>
#include <simpleuv/triangulate.h>
#include <map>
#include "instant-meshes-api.h"

static bool g_engineInitialized = false;
int nprocs = -1;
static std::vector<Dust3D_InstantMeshesVertex> g_resultVertices;
static std::vector<Dust3D_InstantMeshesTriangle> g_resultTriangles;
static std::vector<Dust3D_InstantMeshesQuad> g_resultQuads;

void DUST3D_INSTANT_MESHES_FUNCTION_CONVENTION Dust3D_instantMeshesRemesh(const Dust3D_InstantMeshesVertex *vertices, size_t nVertices,
    const Dust3D_InstantMeshesTriangle *triangles, size_t nTriangles,
    size_t nTargetVertex,
    const Dust3D_InstantMeshesVertex **resultVertices,
    size_t *nResultVertices,
    const Dust3D_InstantMeshesTriangle **resultTriangles,
    size_t *nResultTriangles,
    const Dust3D_InstantMeshesQuad **resultQuads,
    size_t *nResultQuads)
{
    if (!g_engineInitialized) {
        g_engineInitialized = true;
        tbb::task_scheduler_init init(nprocs == -1 ? tbb::task_scheduler_init::automatic : nprocs);
    }

    int rosy = 4;
    int posy = 4;
    Float scale = -1;
    int face_count = -1;
    int vertex_count = nTargetVertex;
    Float creaseAngle = -1;
    bool extrinsic = true;
    bool align_to_boundaries = false;
    int smooth_iter = 2;
    int knn_points = 10;
    bool pure_quad = false;
    bool deterministic = false;
    
    MatrixXu F;
    MatrixXf V, N;
    VectorXf A;
    std::set<uint32_t> crease_in, crease_out;
    BVH *bvh = nullptr;
    AdjacencyMatrix adj = nullptr;
    
    V.resize(3, nVertices);
    F.resize(3, nTriangles);
    for (decltype(nVertices) i = 0; i < nVertices; i++) {
        const auto &vertex = vertices[i];
        V.col(i) << (double)vertex.x, (double)vertex.y, (double)vertex.z;
    }
    for (decltype(nTriangles) i = 0; i < nTriangles; i++) {
        const auto &face = triangles[i];
        F.col(i) << (uint32_t)face.indices[0], (uint32_t)face.indices[1], (uint32_t)face.indices[2];
    }
    
    bool pointcloud = F.size() == 0;

    Timer<> timer;
    MeshStats stats = compute_mesh_stats(F, V, deterministic);

    if (pointcloud) {
        bvh = new BVH(&F, &V, &N, stats.mAABB);
        bvh->build();
        adj = generate_adjacency_matrix_pointcloud(V, N, bvh, stats, knn_points, deterministic);
        A.resize(V.cols());
        A.setConstant(1.0f);
    }

    if (scale < 0 && vertex_count < 0 && face_count < 0) {
        vertex_count = V.cols() / 16;
    }

    if (scale > 0) {
        Float face_area = posy == 4 ? (scale*scale) : (std::sqrt(3.f)/4.f*scale*scale);
        face_count = stats.mSurfaceArea / face_area;
        vertex_count = posy == 4 ? face_count : (face_count / 2);
    } else if (face_count > 0) {
        Float face_area = stats.mSurfaceArea / face_count;
        vertex_count = posy == 4 ? face_count : (face_count / 2);
        scale = posy == 4 ? std::sqrt(face_area) : (2*std::sqrt(face_area * std::sqrt(1.f/3.f)));
    } else if (vertex_count > 0) {
        face_count = posy == 4 ? vertex_count : (vertex_count * 2);
        Float face_area = stats.mSurfaceArea / face_count;
        scale = posy == 4 ? std::sqrt(face_area) : (2*std::sqrt(face_area * std::sqrt(1.f/3.f)));
    }

    MultiResolutionHierarchy mRes;

    if (!pointcloud) {
        /* Subdivide the mesh if necessary */
        VectorXu V2E, E2E;
        VectorXb boundary, nonManifold;
        if (stats.mMaximumEdgeLength*2 > scale || stats.mMaximumEdgeLength > stats.mAverageEdgeLength * 2) {
            build_dedge(F, V, V2E, E2E, boundary, nonManifold);
            subdivide(F, V, V2E, E2E, boundary, nonManifold, std::min(scale/2, (Float) stats.mAverageEdgeLength*2), deterministic);
        }

        /* Compute a directed edge data structure */
        build_dedge(F, V, V2E, E2E, boundary, nonManifold);

        /* Compute adjacency matrix */
        adj = generate_adjacency_matrix_uniform(F, V2E, E2E, nonManifold);

        /* Compute vertex/crease normals */
        if (creaseAngle >= 0)
            generate_crease_normals(F, V, V2E, E2E, boundary, nonManifold, creaseAngle, N, crease_in);
        else
            generate_smooth_normals(F, V, V2E, E2E, nonManifold, N);

        /* Compute dual vertex areas */
        compute_dual_vertex_areas(F, V, V2E, E2E, nonManifold, A);

        mRes.setE2E(std::move(E2E));
    }

    /* Build multi-resolution hierarrchy */
    mRes.setAdj(std::move(adj));
    mRes.setF(std::move(F));
    mRes.setV(std::move(V));
    mRes.setA(std::move(A));
    mRes.setN(std::move(N));
    mRes.setScale(scale);
    mRes.build(deterministic);
    mRes.resetSolution();

    if (align_to_boundaries && !pointcloud) {
        mRes.clearConstraints();
        for (uint32_t i=0; i<3*mRes.F().cols(); ++i) {
            if (mRes.E2E()[i] == INVALID) {
                uint32_t i0 = mRes.F()(i%3, i/3);
                uint32_t i1 = mRes.F()((i+1)%3, i/3);
                Vector3f p0 = mRes.V().col(i0), p1 = mRes.V().col(i1);
                Vector3f edge = p1-p0;
                if (edge.squaredNorm() > 0) {
                    edge.normalize();
                    mRes.CO().col(i0) = p0;
                    mRes.CO().col(i1) = p1;
                    mRes.CQ().col(i0) = mRes.CQ().col(i1) = edge;
                    mRes.CQw()[i0] = mRes.CQw()[i1] = mRes.COw()[i0] =
                        mRes.COw()[i1] = 1.0f;
                }
            }
        }
        mRes.propagateConstraints(rosy, posy);
    }

    if (bvh) {
        bvh->setData(&mRes.F(), &mRes.V(), &mRes.N());
    } else if (smooth_iter > 0) {
        bvh = new BVH(&mRes.F(), &mRes.V(), &mRes.N(), stats.mAABB);
        bvh->build();
    }

    Optimizer optimizer(mRes, false);
    optimizer.setRoSy(rosy);
    optimizer.setPoSy(posy);
    optimizer.setExtrinsic(extrinsic);

    optimizer.optimizeOrientations(-1);
    optimizer.notify();
    optimizer.wait();

    std::map<uint32_t, uint32_t> sing;
    compute_orientation_singularities(mRes, sing, extrinsic, rosy);
    timer.reset();

    optimizer.optimizePositions(-1);
    optimizer.notify();
    optimizer.wait();

    optimizer.shutdown();
    
    MatrixXf O_extr, N_extr, Nf_extr;
    std::vector<std::vector<TaggedLink>> adj_extr;
    extract_graph(mRes, extrinsic, rosy, posy, adj_extr, O_extr, N_extr,
                  crease_in, crease_out, deterministic);

    MatrixXu F_extr;
    extract_faces(adj_extr, O_extr, N_extr, Nf_extr, F_extr, posy,
            mRes.scale(), crease_out, true, pure_quad, bvh, smooth_iter);

    auto outputFace = [&](const std::vector<size_t> &newFace) {
        if (newFace.size() > 4) {
            std::vector<simpleuv::Vertex> verticesForTriangulation;
            verticesForTriangulation.reserve(newFace.size());
            std::vector<simpleuv::Face> facesFromTriangulation;
            std::vector<size_t> ringForTriangulation;
            ringForTriangulation.reserve(newFace.size());
            for (const auto &it: newFace) {
                const auto &position = g_resultVertices[it];
                ringForTriangulation.push_back(verticesForTriangulation.size());
                verticesForTriangulation.push_back(simpleuv::Vertex {{position.x, position.y, position.z}});
            }
            simpleuv::triangulate(verticesForTriangulation, facesFromTriangulation, ringForTriangulation);
            for (const auto &it: facesFromTriangulation) {
                g_resultTriangles.push_back(Dust3D_InstantMeshesTriangle {{
                    newFace[it.indices[0]],
                    newFace[it.indices[1]],
                    newFace[it.indices[2]]
                }});
            }
            return;
        } else if (newFace.size() == 4) {
            g_resultQuads.push_back(Dust3D_InstantMeshesQuad {{
                newFace[0],
                newFace[1],
                newFace[2],
                newFace[3]
            }});
            return;
        } else if (newFace.size() == 3) {
            g_resultTriangles.push_back(Dust3D_InstantMeshesTriangle {{
                newFace[0],
                newFace[1],
                newFace[2]
            }});
            return;
        }
    };
    
    auto outputMesh = [&](const MatrixXf &V, const MatrixXu &F) {
        g_resultVertices.resize(V.cols());
        for (uint32_t i = 0; i < V.cols(); ++i) {
            g_resultVertices[i] = Dust3D_InstantMeshesVertex {V(0, i), V(1, i), V(2, i)};
        }
        
        g_resultTriangles.clear();
        g_resultQuads.clear();
        
        std::map<uint32_t, std::pair<uint32_t, std::map<uint32_t, uint32_t>>> irregularMap;
        size_t nIrregular = 0;
        for (uint32_t i = 0; i < (uint32_t)F.cols(); ++i) {
            std::vector<size_t> newFace;
            bool irregular = posy == 4 && F(2, i) == F(3, i);
            if (irregular) {
                nIrregular++;
                auto &value = irregularMap[F(2, i)];
                value.first = i;
                value.second[F(0, i)] = F(1, i);
                continue;
            }
            for (int j = 0; j < posy; ++j) {
                newFace.push_back((size_t)F(j, i));
            }
            outputFace(newFace);
        }
        for (auto item : irregularMap) {
            auto face = item.second;
            uint32_t v = face.second.begin()->first, first = v, i = 0;
            std::vector<size_t> newFace;
            while (true) {
                newFace.push_back(v);
                v = face.second[v];
                if (v == first || ++i == face.second.size())
                    break;
            }
            outputFace(newFace);
        }
    };
    outputMesh(O_extr, F_extr);

    *nResultVertices = g_resultVertices.size();
    *resultVertices = g_resultVertices.data();
    *nResultTriangles = g_resultTriangles.size();
    *resultTriangles = g_resultTriangles.data();
    *nResultQuads = g_resultQuads.size();
    *resultQuads = g_resultQuads.data();
}
