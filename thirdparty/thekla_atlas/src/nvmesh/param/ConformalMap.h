// This code is in the public domain -- castano@gmail.com

#pragma once
#ifndef NV_MESH_CONFORMALMAP_H
#define NV_MESH_CONFORMALMAP_H

namespace nv
{
    namespace HalfEdge { class Mesh; }

    bool computeConformalMap(HalfEdge::Mesh * mesh);
    bool computeNaturalConformalMap(HalfEdge::Mesh * mesh);

} // nv namespace

#endif // NV_MESH_CONFORMALMAP_H
