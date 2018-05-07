// This code is in the public domain -- castano@gmail.com

#include "BoundaryMap.h"
#include "Util.h"

#include <nvmath/Sparse.h>
#include <nvmath/Solver.h>

#include <nvmesh/halfedge/Edge.h>
#include <nvmesh/halfedge/Mesh.h>
#include <nvmesh/halfedge/Vertex.h>
#include <nvmesh/halfedge/Face.h>

using namespace nv;



bool nv::computeCircularBoundaryMap(HalfEdge::Mesh * mesh)
{
    HalfEdge::Vertex * vertex = findBoundaryVertex(mesh);

    if (vertex == NULL)
    {
        return false;
    }

    // Compute boundary length.
    float boundaryLength = 0.0f;

    HalfEdge::Edge * const firstEdge = vertex->edge();
    HalfEdge::Edge * edge = firstEdge;
    do {
        boundaryLength += edge->length();
        edge = edge->next();
    } while (edge != firstEdge);

    float length = 0.0f;

    edge = firstEdge;
    do {
        float angle = length * 2.0f * PI / boundaryLength;
        edge->vertex()->tex.set(cos(angle), sin(angle));

		length += edge->length();
        edge = edge->next();
    } while (edge != firstEdge);

    return true;
}

