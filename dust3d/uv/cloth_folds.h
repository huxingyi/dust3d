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

#ifndef DUST3D_UV_CLOTH_FOLDS_H_
#define DUST3D_UV_CLOTH_FOLDS_H_

#include <cstdint>
#include <dust3d/base/vector3.h>
#include <string>
#include <utility>
#include <vector>

namespace dust3d {

// ClothFolds: folds and wrinkles of a low poly garment, baked into its normal map, placed
// for the reasons cloth folds:
//
//   Support      Cloth rests on the body where it touches it (the bust, the shoulder
//                blades, the hips, the buttocks, a thigh pushing a skirt out) and is free
//                where it stands off it. place() measures how far each point of the garment
//                stands off the body it is worn over: the touching places are the supports.
//   Tension      Free cloth spanning from one support to another is pulled taut between
//                them and folds along the pull: from the bust down to the waist.
//   Sag          Free cloth between two supports side by side sags between them in U and V
//                folds: across the front of a short skirt between the thighs, under the
//                buttocks.
//   Hang         Free cloth resting only on a support above falls from it in long soft
//                folds down to its hem: a loose skirt from the hips, a puff sleeve from the
//                shoulder.
//   Compression  Cloth wrapping a limb snugly bunches on the inside of a bent joint (elbow,
//                knee), above a snug cuff and where a limb meets the body (armpit, crotch),
//                and gathers under a tight waistband.
//
// Each fold follows the direction the cloth is pulled in (away from its support, leaning
// with gravity), so it is a curve on the surface, with the profile of a sculpted fold: a
// crease line with a soft ridge beside it. Where the cloth rests on the body it stays
// smooth. The folds are the rest pose's: they do not move with the joints.
//
// The folds are evaluated on the 3D surface, so the baked normal map runs across UV seams
// without a break.
class ClothFolds {
public:
    enum class Kind {
        Hang = 0, // from a support down to a free end: pinched at the top, full below
        Tension, // from support to support
        Sag, // between two supports side by side: deepest at its lowest point
        Crease // a short compression crease
    };

    struct Fold {
        Kind kind = Kind::Crease;
        std::vector<Vector3> points; // along the fold, on the surface
        std::vector<Vector3> normals; // the surface normal at each point
        double width = 0.0;
        double depth = 0.0;
        double crease = 0.5; // 0: a soft round pipe, 1: a sharp crease
        double peak = 0.5; // where along it (0..1) the fold is deepest
        double endDepth = 0.0; // how deep it still is at its end (of the deepest), for a fold running into a hem
        double ridgeSide = 1.0; // which side of the crease line the ridge rises on
    };

    struct SkeletonNode {
        Vector3 position;
        double radius = 0.0;
        std::string group; // the part the node belongs to (its edges link it)
    };

    struct PlacementInput {
        // the garment
        std::vector<Vector3> vertices;
        std::vector<std::vector<size_t>> triangles;
        // the skeleton of the body it is worn over (the nodes its skin weights come from)
        std::vector<SkeletonNode> nodes;
        std::vector<std::pair<size_t, size_t>> links; // indices into nodes
        // the surface of the body it is worn over (where the cloth rests on it); without it
        // only the compression folds and the hems get folds
        std::vector<Vector3> bodyVertices;
        std::vector<std::vector<size_t>> bodyTriangles;
        bool cloth = true; // false: a creature skin (only skin creases at the joints)
        double drape = 0.5; // 0..1: how loosely the cloth hangs
        double sizeScale = 1.0; // the width of the folds: 1 is a natural fold width for the size of the figure
        uint32_t seed = 0;
    };

    static std::vector<Fold> place(const PlacementInput& input);
    static std::string serialize(const std::vector<Fold>& folds);
    static std::vector<Fold> deserialize(const std::string& text);

    // `strength` 0..1 scales the depth of every fold.
    ClothFolds(const std::vector<Fold>& folds, double strength);

    // The fold height (world units) at a surface point with unit normal `normal`; `crease`
    // (optional) gets how deep in a crease the point is, 0..1.
    double heightAt(const Vector3& position, const Vector3& normal, double* crease = nullptr) const;

    // Tangent-space normal (x along `tangent`, y along `bitangent`) and cavity (0..1) at a
    // surface point; `footprint` is the texel size in world units.
    void evaluate(const Vector3& position, const Vector3& normal, const Vector3& tangent, const Vector3& bitangent,
        double footprint, Vector3* tangentSpaceNormal, double* cavity) const;

    bool empty() const { return m_folds.empty(); }
    size_t foldCount() const { return m_folds.size(); }

private:
    struct FoldData {
        Fold fold;
        std::vector<double> arc; // arc length at each point
        std::vector<Vector3> tangents; // the direction along it at each point
        double length = 0.0;
        double reach = 0.0; // how far from its line the profile reaches
    };
    struct Segment {
        size_t fold;
        size_t index; // points[index] .. points[index + 1]
    };

    double foldHeight(const FoldData& data, double s, double t, double* crease) const;

    double m_strength = 1.0;
    std::vector<FoldData> m_folds;
    std::vector<Segment> m_segments;
    // segments by grid cell (spatial hash), for looking up the ones near a point
    double m_cellSize = 1.0;
    std::vector<std::pair<uint64_t, size_t>> m_cellSegments;
};

}

#endif
