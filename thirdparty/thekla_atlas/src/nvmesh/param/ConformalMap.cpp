// This code is in the public domain -- castano@gmail.com

#include "ConformalMap.h"
#include "Util.h"

#include <nvmath/Sparse.h>
#include <nvmath/Vector.inl>
#include <nvmath/Solver.h>

#include <nvmesh/halfedge/Mesh.h>
#include <nvmesh/halfedge/Vertex.h>
#include <nvmesh/halfedge/Face.h>
#include <nvmesh/MeshTopology.h>

using namespace nv;

namespace {

    static float triangleArea(Vector3::Arg a, Vector3::Arg b)
    {
        return 0.5f * sqrtf(lengthSquared(a) * lengthSquared(b) - square(dot(a, b)));
    }


    static float computeHarmonicCoordinate(const HalfEdge::Edge * edge)
    {
        // Sum of the cotangents of the opposite angles.
        // w_ij = cot theta_ij + cot theta_ji

        //    R 
        //   / \
        //  /   \
        // P --- Q
        //  \   /
        //   \ /
        //    S 

        const Vector3 P = edge->from()->pos;
        const Vector3 Q = edge->to()->pos;
        const Vector3 R = edge->next->to()->pos;
        const Vector3 S = edge->pair->next->to()->pos;

        const Vector3 RP = P - R;
        const Vector3 RQ = Q - R;
        const Vector3 SP = P - S;
        const Vector3 SQ = Q - S;

        float cotR = dot(RP, RQ) / triangleArea(RP, RQ);
        float cotS = dot(SP, SQ) / triangleArea(SP, SQ);

        //float cotR = 0.5f * (lengthSquared(RP) + lengthSquared(RQ) - lengthSquared(PQ)) / triangleArea(RP, RQ);
        //float cotS = 0.5f * (lengthSquared(SP) + lengthSquared(SQ) - lengthSquared(SQ)) / triangleArea(SP, SQ);

        return cotR + cotS;
    }

    static Vector2 computeMeanValueCoordinate(const HalfEdge::Edge * edge)
    {
        const Vector3 P = edge->from()->pos;
        const Vector3 Q = edge->to()->pos;
        const Vector3 R = edge->next->to()->pos;
        const Vector3 S = edge->pair->next->to()->pos;

        const Vector3 QR = R - Q;
        const Vector3 QS = S - Q;
        const Vector3 QP = P - Q;

        const Vector3 PR = R - P;
        const Vector3 PS = S - P;
        const Vector3 PQ = Q - P;

        float tanS0 = (length(PQ) * length(PS) - dot(PQ, PS)) / triangleArea(PQ, PS);
        float tanR0 = (length(PQ) * length(PR) - dot(PQ, PR)) / triangleArea(PQ, PR);

        float tanS1 = (length(QP) * length(QS) - dot(QP, QS)) / triangleArea(QP, QS);
        float tanR1 = (length(QP) * length(QR) - dot(QP, QR)) / triangleArea(QP, QR);

        float mv1 = (tanS0 + tanR0) / length(PQ);
        float mv0 = (tanS1 + tanR1) / length(PQ);

        return Vector2(mv0, mv1);
    }

    static Vector2 computeWachpressCoordinate(const HalfEdge::Edge * edge)
    {
        const Vector3 P = edge->from()->pos;
        const Vector3 Q = edge->to()->pos;
        const Vector3 R = edge->next->to()->pos;
        const Vector3 S = edge->pair->next->to()->pos;

        const Vector3 QR = R - Q;
        const Vector3 QS = S - Q;
        const Vector3 QP = P - Q;

        const Vector3 PR = R - P;
        const Vector3 PS = S - P;
        const Vector3 PQ = Q - P;

        float PRQ = triangleArea(PQ, PR);
        float PSQ = triangleArea(PQ, PS);

        float QRS = triangleArea(QS, QR);
        float PSR = triangleArea(PR, PS);

        float chi0 = PSR / (PSQ * PRQ);
        float chi1 = QRS / (PSQ * PRQ);

        return Vector2(chi0, chi1);
    }

    enum Coordinate {
        Coordinate_Harmonic,
        Coordinate_MeanValue,
        Coordinate_Wachpress,
    };

} // namespace




bool nv::computeConformalMap(HalfEdge::Mesh * mesh)
{
    // @@ computeBarycentricMap(mesh, Coordinate_Harmonic);

    nvDebugCheck(mesh != NULL);
    nvDebugCheck(mesh->faceCount() > 1);

#if _DEBUG
    nvDebugCheck(isTriangularMesh(mesh));

    MeshTopology topology(mesh);
    nvDebugCheck(topology.isDisk());
#endif

    const uint vertexCount = mesh->vertexCount();

    // Two equations: Ab=x, Ac=y
    SparseMatrix A(vertexCount, vertexCount);
    FullVector b(vertexCount);
    FullVector c(vertexCount);
    FullVector x(vertexCount);
    FullVector y(vertexCount);

    // Set right hand side to boundary parameterization or zero.
    for (uint v = 0; v < vertexCount; v++)
    {
        HalfEdge::Vertex * vertex = mesh->vertexAt(v);

        if (vertex->isBoundary()) {
            x[v] = vertex->tex.x;
            y[v] = vertex->tex.y;
        }
        else {
            x[v] = 0;
            y[v] = 0;
        }
    }

    // Set initial solution. @@ It may make more sense to set this to the input parameterization.
    b = x;
    c = y;

    // Fill matrix.
    for (uint v = 0; v < vertexCount; v++)
    {
        HalfEdge::Vertex * vertex = mesh->vertexAt(v);

        if (vertex->isBoundary()) {
            A.setCoefficient(v, v, 1.0f);
        }
        else {
            float sum = 0.0f;

            // Traverse edges around vertex.
            for (HalfEdge::Vertex::ConstEdgeIterator it(vertex->edges()); !it.isDone(); it.advance())
            {
                const HalfEdge::Edge * edge = it.current();

                const float energy = computeHarmonicCoordinate(edge);

                A.setCoefficient(v, edge->to()->id, energy);
                sum += energy;
            }

            // Set diagonal.
            A.setCoefficient(v, v, -sum);
        }
    }

    // Solve equations.
    SymmetricSolver(A, b, x);
    SymmetricSolver(A, c, y);

    // Set texture coordinates.
    for (uint v = 0; v < vertexCount; v++)
    {
        HalfEdge::Vertex * vertex = mesh->vertexAt(v);
        vertex->tex.x = x[v];
        vertex->tex.y = y[v];
    }

    return true;
}
