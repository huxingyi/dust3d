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

#ifndef DUST3D_UV_SURFACE_PAINT_H_
#define DUST3D_UV_SURFACE_PAINT_H_

#include <cstdint>
#include <dust3d/base/color.h>
#include <dust3d/base/object.h>
#include <dust3d/base/vector3.h>
#include <vector>

namespace dust3d {

// SurfacePaint: one paint part (see ObjectPaintStroke) as something the texture baker can ask
// for a colour.
//
// A low poly game character wears its clothes in its texture. Its mesh is one plain body of
// a few hundred triangles; the shirt, the apron over it, the belt, the gloves and boots, the
// pale chest of a lizard and its eyes are colour. In Dust3D that colour is drawn the way the
// body is: with nodes. A paint part is a chain of nodes that is not meshed. It is a brush
// dragged through space, as wide at each node as the node's radius, and every bit of the
// model's surface inside it takes its colour.
//
// The brush is round along the stroke and, unless the part is rounded, cut square at its two
// ends: two nodes a finger apart with a radius wider than the hips are a belt, two along the
// forearm a sleeve with a straight cuff. One node is a round dab.
//
// A stroke is painted the way a low poly texture is painted by hand: a flat colour, airbrushed.
// It is a little darker towards its edge and lighter in its middle, which is what makes a
// flat patch read as a sleeve or a trouser leg with some roundness to it; a thin soft line
// runs just inside the edge; and outside it the stroke drops a soft shadow on what is under
// it, so the layers read as layers. All of it fades with the contrast between the two
// colours: a stroke that carries on a colour already there joins it without a mark. A soft
// stroke only fades out: a marking, a blush, the shadow of a fold.
class SurfacePaint {
public:
    SurfacePaint(const ObjectPaintStroke& stroke);
    const Uuid& componentId() const { return m_stroke.componentId; }
    // The stroke, with its shadow, reaches no further than this box.
    bool reaches(const Vector3& lower, const Vector3& upper) const;
    // How far a point is outside the brush (negative: inside it), and how wide the brush is at
    // the nearest point of the stroke.
    double distance(const Vector3& position, double* radius = nullptr) const;
    // The colour at a point of the surface whose colour was `under`. `footprint` is the size
    // of a texel there, in world units: edges are filtered to it.
    Color colorAt(const Color& under, const Vector3& position, double footprint) const;

private:
    struct Nearest {
        double distance = 0.0;
        double radius = 0.0;
    };
    Nearest nearest(const Vector3& position) const;
    ObjectPaintStroke m_stroke;
    Vector3 m_lower;
    Vector3 m_upper;
};

// Light painted into the colour texture, the way a hand painted low poly texture carries its
// own: creases, the undersides of forms and whatever another form stands close to are darker
// and cooler, what faces up is lighter and warmer.
//
// OcclusionCaster says how open a point of the model's surface is to the sky around it (1:
// fully open, towards 0: shut in), by casting rays over the hemisphere of its normal against
// the model's triangles. Asked texel by texel it gives the soft shadow under an arm, between
// the legs and under a chin; computeVertexOcclusion asks it at the vertices only, for models
// too large to do every texel.
class OcclusionCaster {
public:
    // `triangles` index `vertices`
    OcclusionCaster(const std::vector<Vector3>& vertices, const std::vector<std::vector<size_t>>& triangles);
    double occlusionAt(const Vector3& position, const Vector3& normal) const;
    size_t triangleCount() const { return m_triangles.size(); }

private:
    struct Triangle {
        Vector3 a, edge1, edge2;
    };
    // A bounding volume hierarchy over the triangles: a ray visits only the boxes it passes
    // through within reach, a few dozen triangles instead of every one near the point.
    struct Node {
        float lower[3];
        float upper[3];
        // a leaf: `count` triangles from `first`; otherwise its children are this node + 1
        // and `first`
        uint32_t first = 0;
        uint32_t count = 0;
    };
    std::vector<Triangle> m_triangles;
    std::vector<Node> m_nodes;
    double m_reach = 0.0;
    double m_lift = 0.0;
    std::vector<Vector3> m_hemisphere;
    uint32_t build(std::vector<uint32_t>& order, std::vector<Vector3>& centres, std::vector<std::pair<Vector3, Vector3>>& bounds,
        uint32_t begin, uint32_t end);
    double nearestHit(const Vector3& origin, const Vector3& direction, double reach) const;
};

std::vector<double> computeVertexOcclusion(const std::vector<Vector3>& vertices,
    const std::vector<std::vector<size_t>>& triangles);

// A colour with that light on it: `occlusion` from the above, `normal` the
// surface normal (y up, z front), `amount` 0..1.
Color bakedShade(const Color& color, double occlusion, const Vector3& normal, double amount);

}

#endif
