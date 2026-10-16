/*
 *  Copyright (c) 2016-2026 Jeremy HU <jeremy-at-dust3d dot org>. All rights reserved.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:

 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.

 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#ifndef DUST3D_MESH_HAIR_BUILDER_H_
#define DUST3D_MESH_HAIR_BUILDER_H_

#include <cstdint>
#include <dust3d/base/uuid.h>
#include <dust3d/base/vector2.h>
#include <dust3d/base/vector3.h>
#include <dust3d/mesh/mesh_node.h>
#include <memory>
#include <vector>

namespace dust3d {

// A lock of hair grown along a guide stroke: the part's nodes, from the root on the scalp to
// where the lock ends, each node's radius the width of the lock there. The lock is a clump of
// strands side by side across that width, each a smooth flattened tube that tapers to a point
// and lies on the scalp: wherever a strand would go into the surface under it (the head it
// grows on) it is lifted out, so a guide drawn roughly over the head gives hair that follows
// the head's form.
class HairBuilder {
public:
    struct Parameters {
        // strands side by side in the lock (1-16)
        size_t strands = 5;
        // how thick a strand is for its width (0.2 flat ribbon - 1.5 round)
        double thickness = 0.45;
        // the width of the tip, as a fraction of the root's (0 - 1)
        double tip = 0.15;
        // how closely the lock is drawn down onto the scalp where it is near it (0: it goes
        // where its nodes are; 1: it lies on the scalp all along), less towards the tip
        double hug = 0.5;
        // different strands are a little different (in length, spacing), the same each time
        uint32_t seed = 1;
    };

    // the surface the hair grows on and lies over: triangles indexing vertices
    void setScalp(const std::vector<Vector3>& vertices, const std::vector<std::vector<size_t>>& triangles);
    void build(const std::vector<MeshNode>& guide, const Parameters& parameters);

    const std::vector<Vector3>& vertices() const { return m_vertices; }
    // quads and triangles, facing out
    const std::vector<std::vector<size_t>>& faces() const { return m_faces; }
    // per face, the UV of each corner: each strand a strip of the unit square
    const std::vector<std::vector<Vector2>>& faceUvs() const { return m_faceUvs; }
    // the guide node each vertex belongs to (for the rig)
    const std::vector<Uuid>& vertexSources() const { return m_vertexSources; }

    HairBuilder();
    ~HairBuilder();

private:
    struct Scalp;
    std::unique_ptr<Scalp> m_scalp;
    std::vector<Vector3> m_vertices;
    std::vector<std::vector<size_t>> m_faces;
    std::vector<std::vector<Vector2>> m_faceUvs;
    std::vector<Uuid> m_vertexSources;
};

}

#endif
