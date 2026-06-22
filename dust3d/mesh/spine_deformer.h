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

#ifndef DUST3D_MESH_SPINE_DEFORMER_H_
#define DUST3D_MESH_SPINE_DEFORMER_H_

#include <algorithm>
#include <cmath>
#include <dust3d/base/math.h>
#include <dust3d/base/vector3.h>
#include <dust3d/mesh/base_normal.h>
#include <dust3d/mesh/mesh_node.h>
#include <vector>

namespace dust3d {

// Sweeps a source mesh along a tube spine. The source mesh is treated as a
// straight reference shape whose primary axis is Y: every source vertex's Y
// position selects where along the spine it lands, while its X/Z offset from
// the source centre becomes the cross-section offset perpendicular to the
// spine. Cross-sections are scaled to the interpolated node radius, squashed by
// deformWidth (X) and deformThickness (Z), and spun by the cut rotation.
//
// This is the exact transformation used by the "Imported" part generator,
// factored out so the "Rock" generator can deform along the spine the same way.
class SpineDeformer {
public:
    SpineDeformer(const std::vector<MeshNode>& meshNodes,
        const Vector3& sourceMin, const Vector3& sourceMax,
        float deformWidth, float deformThickness, float cutRotation)
        : m_meshNodes(meshNodes)
        , m_deformWidth(deformWidth)
        , m_deformThickness(deformThickness)
        , m_cutRotation(cutRotation)
    {
        m_sourceMin = sourceMin;
        m_sourceCenter = (sourceMin + sourceMax) * 0.5;
        Vector3 sourceSize = sourceMax - sourceMin;

        // Use Y as the primary axis of the source model (spine direction).
        m_sourceLength = sourceSize.y();
        if (m_sourceLength < 0.0001)
            m_sourceLength = 0.0001;

        m_crossSize = std::max(sourceSize.x(), sourceSize.z());
        if (m_crossSize < 0.0001)
            m_crossSize = 0.0001;

        // Cumulative spine distance at each node.
        m_spineDistances.assign(meshNodes.size(), 0.0);
        for (size_t i = 1; i < meshNodes.size(); ++i)
            m_spineDistances[i] = m_spineDistances[i - 1] + (meshNodes[i].origin - meshNodes[i - 1].origin).length();
        m_totalSpineLength = meshNodes.empty() ? 0.0 : m_spineDistances.back();
        if (m_totalSpineLength < 0.0001)
            m_totalSpineLength = 0.0001;

        // Forward direction at each node.
        m_spineForwards.assign(meshNodes.size(), Vector3(0, 1, 0));
        for (size_t i = 0; i + 1 < meshNodes.size(); ++i)
            m_spineForwards[i] = (meshNodes[i + 1].origin - meshNodes[i].origin).normalized();
        if (meshNodes.size() > 1)
            m_spineForwards.back() = m_spineForwards[meshNodes.size() - 2];
        else if (!meshNodes.empty())
            m_spineForwards[0] = Vector3(0, 1, 0);

        // A single global base normal for the whole spine, computed the same way
        // as TubeMeshBuilder. Deriving every cross-section frame from this
        // consistent reference (instead of a per-segment up-vector guess) keeps
        // the swept cross-sections from twisting or flipping as the spine bends.
        std::vector<Vector3> positions(meshNodes.size());
        for (size_t i = 0; i < meshNodes.size(); ++i)
            positions[i] = meshNodes[i].origin;
        m_baseNormal = BaseNormal::calculateTubeBaseNormal(positions);
        if (m_baseNormal.isZero())
            m_baseNormal = Vector3(1, 0, 0);
    }

    // Map a source vertex onto the spine.
    Vector3 deformVertex(const Vector3& sourceVertex) const
    {
        // Normalize the source position along the primary axis to [0, 1].
        double t = (sourceVertex.y() - m_sourceMin.y()) / m_sourceLength;
        t = std::max(0.0, std::min(1.0, t));

        // Local offset in the cross-section (relative to the source centre).
        double localU = sourceVertex.x() - m_sourceCenter.x();
        double localV = sourceVertex.z() - m_sourceCenter.z();

        double targetDist = t * m_totalSpineLength;
        size_t segIdx = segmentForDistance(targetDist);

        double segLen = (m_meshNodes.size() >= 2) ? (m_spineDistances[segIdx + 1] - m_spineDistances[segIdx]) : 0.0;
        double segT = (segLen > 0.0001) ? (targetDist - m_spineDistances[segIdx]) / segLen : 0.0;

        Vector3 spinePos = (m_meshNodes.size() >= 2)
            ? m_meshNodes[segIdx].origin * (1.0 - segT) + m_meshNodes[segIdx + 1].origin * segT
            : m_meshNodes[0].origin;
        double radius = (m_meshNodes.size() >= 2)
            ? m_meshNodes[segIdx].radius * (1.0 - segT) + m_meshNodes[segIdx + 1].radius * segT
            : m_meshNodes[0].radius;

        Vector3 right;
        Vector3 realUp;
        crossSectionBasis(segIdx, &right, &realUp);

        // Apply the cut rotation to the cross-section basis.
        double rotationAngle = m_cutRotation * Math::Pi;
        double cosR = std::cos(rotationAngle);
        double sinR = std::sin(rotationAngle);
        Vector3 rotatedRight = right * cosR + realUp * sinR;
        Vector3 rotatedUp = realUp * cosR - right * sinR;

        // Scale the cross-section by the node radius relative to the source size.
        double scale = (radius * 2.0) / m_crossSize;
        scale *= m_deformWidth;

        return spinePos + rotatedRight * (localU * scale) + rotatedUp * (localV * scale * m_deformThickness / m_deformWidth);
    }

    // Map a source normal onto the spine frame (cut rotation is intentionally
    // not applied to normals, matching the Imported generator).
    Vector3 deformNormal(const Vector3& sourceNormal, double sourceY) const
    {
        double t = (sourceY - m_sourceMin.y()) / m_sourceLength;
        t = std::max(0.0, std::min(1.0, t));
        size_t segIdx = segmentForDistance(t * m_totalSpineLength);

        Vector3 right;
        Vector3 realUp;
        crossSectionBasis(segIdx, &right, &realUp);
        Vector3 forward = m_spineForwards[segIdx];

        return (right * sourceNormal.x() + forward * sourceNormal.y() + realUp * sourceNormal.z()).normalized();
    }

private:
    size_t segmentForDistance(double targetDist) const
    {
        size_t segIdx = 0;
        for (size_t i = 1; i < m_meshNodes.size(); ++i) {
            if (m_spineDistances[i] >= targetDist) {
                segIdx = i - 1;
                break;
            }
            segIdx = i - 1;
        }
        if (m_meshNodes.size() < 2)
            segIdx = 0;
        else if (segIdx + 1 >= m_meshNodes.size())
            segIdx = m_meshNodes.size() - 2;
        return segIdx;
    }

    void crossSectionBasis(size_t segIdx, Vector3* right, Vector3* realUp) const
    {
        Vector3 forward = m_spineForwards[segIdx];
        // Re-orthogonalise the global base normal against the local forward
        // direction to obtain the cross-section frame. m_baseNormal plays the
        // role of the width axis (right); realUp is the thickness axis. The
        // cross-product order is chosen so (right, forward, realUp) keeps the
        // same handedness as the source mesh: for a straight +Y spine this is
        // right=+X, realUp=+Z (an identity frame), so an imported model is swept
        // without being mirrored. (TubeMeshBuilder uses the opposite order, but
        // it builds fresh geometry and has no source chirality to preserve.)
        Vector3 v = Vector3::crossProduct(m_baseNormal, forward).normalized();
        Vector3 u = Vector3::crossProduct(forward, v).normalized();
        *right = u;
        *realUp = v;
    }

    const std::vector<MeshNode>& m_meshNodes;
    float m_deformWidth;
    float m_deformThickness;
    float m_cutRotation;
    Vector3 m_sourceMin;
    Vector3 m_sourceCenter;
    double m_sourceLength = 0.0001;
    double m_crossSize = 0.0001;
    std::vector<double> m_spineDistances;
    double m_totalSpineLength = 0.0001;
    std::vector<Vector3> m_spineForwards;
    Vector3 m_baseNormal;
};

}

#endif
