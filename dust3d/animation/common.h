/*
 *  Copyright (c) 2026 Jeremy HU <jeremy-at-dust3d dot org>. All rights reserved.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

// Shared utilities for animation generation across all rig types.
// Contains IK solvers, bone transforms, bone lookup helpers, and interpolation helpers.

#ifndef DUST3D_ANIMATION_COMMON_H
#define DUST3D_ANIMATION_COMMON_H

#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>
#include <map>
#include <string>
#include <vector>

namespace dust3d {
namespace animation {

    // =========================================================================
    // ANALYTIC TWO-BONE IK SOLVER
    // =========================================================================

    // Analytic two-bone IK solver using the law of cosines.
    // Computes the exact knee/elbow position for a two-bone chain (3 joints)
    // given a pole vector that controls the bend direction.
    // This produces smooth, deterministic results without iteration.
    // @param joints: exactly 3 positions [root, mid, end]  (root stays fixed)
    // @param target: desired end-effector position
    // @param poleVector: world-space point that the mid joint should bend toward
    inline void solveTwoBoneIk(std::vector<Vector3>& joints, const Vector3& target, const Vector3& poleVector,
        double softness = 0.1)
    {
        if (joints.size() != 3)
            return;

        Vector3 root = joints[0];
        double lenA = (joints[1] - joints[0]).length(); // upper bone
        double lenB = (joints[2] - joints[1]).length(); // lower bone

        Vector3 toTarget = target - root;
        double distTarget = toTarget.length();

        if (distTarget < 1e-12) {
            // Target at root; just keep current pose
            return;
        }

        double chainLength = lenA + lenB;
        double minReach = std::abs(lenA - lenB) + 1e-6;

        // Soft IK: exponential falloff near full extension to prevent
        // the chain from ever fully straightening (eliminates knee pop).
        // softDist is the distance at which softening begins.
        double softDist = chainLength * softness;
        double hardLimit = chainLength - softDist;

        if (softDist > 1e-8 && distTarget > hardLimit) {
            // Soft zone: asymptotically approach chainLength
            // da = distTarget - hardLimit (how far into the soft zone)
            double da = distTarget - hardLimit;
            // Exponential ease: softDist * (1 - e^(-da/softDist))
            // This maps [hardLimit, inf) -> [hardLimit, chainLength)
            double softened = softDist * (1.0 - std::exp(-da / softDist));
            distTarget = hardLimit + softened;
        }

        // Clamp to valid range
        if (distTarget > chainLength - 1e-6)
            distTarget = chainLength - 1e-6;
        if (distTarget < minReach)
            distTarget = minReach;

        // Law of cosines: angle at root joint
        double cosAngleA = (lenA * lenA + distTarget * distTarget - lenB * lenB) / (2.0 * lenA * distTarget);
        cosAngleA = std::max(-1.0, std::min(1.0, cosAngleA));
        double angleA = std::acos(cosAngleA);

        // Build a coordinate frame along root->target with pole vector determining bend plane
        Vector3 dirTarget = toTarget * (1.0 / distTarget);

        // Project pole vector onto the plane perpendicular to dirTarget
        Vector3 toPole = poleVector - root;
        Vector3 poleOnPlane = toPole - dirTarget * Vector3::dotProduct(toPole, dirTarget);
        double poleLen = poleOnPlane.length();

        Vector3 bendDir;
        if (poleLen < 1e-10) {
            // Pole vector is aligned with target direction; pick an arbitrary perpendicular
            Vector3 arbitrary = (std::abs(dirTarget.x()) < 0.9) ? Vector3(1, 0, 0) : Vector3(0, 1, 0);
            bendDir = Vector3::crossProduct(dirTarget, arbitrary).normalized();
        } else {
            bendDir = poleOnPlane * (1.0 / poleLen);
        }

        // Mid joint position
        joints[1] = root + dirTarget * (lenA * cosAngleA) + bendDir * (lenA * std::sin(angleA));

        // End joint: place at target (clamped distance along root->target direction)
        Vector3 clampedTarget = root + dirTarget * distTarget;
        Vector3 dirB = clampedTarget - joints[1];
        double dirBLen = dirB.length();
        if (dirBLen > 1e-12)
            joints[2] = joints[1] + dirB * (lenB / dirBLen);
        else
            joints[2] = clampedTarget;
    }

    // =========================================================================
    // BONE TRANSFORMS
    // =========================================================================

    // Build a world-space bone transform matrix from start and end positions
    // Translate to boneStart, then rotate so local +Z aligns with bone direction.
    inline Matrix4x4 buildBoneWorldTransform(const Vector3& boneStart, const Vector3& boneEnd)
    {
        Vector3 dir = boneEnd - boneStart;
        Matrix4x4 transform;
        transform.translate(boneStart);
        if (!dir.isZero()) {
            Quaternion orient = Quaternion::rotationTo(Vector3(0.0, 0.0, 1.0), dir.normalized());
            transform.rotate(orient);
        }
        return transform;
    }

    // =========================================================================
    // BONE LOOKUP HELPERS
    // =========================================================================

    // Build a bone index map from rig structure for fast name -> index lookups
    inline std::map<std::string, size_t> buildBoneIndexMap(const RigStructure& rigStructure)
    {
        std::map<std::string, size_t> boneIdx;
        for (size_t i = 0; i < rigStructure.bones.size(); ++i) {
            boneIdx[rigStructure.bones[i].name] = i;
        }
        return boneIdx;
    }

    // Get bone start position by name. Returns zero vector if not found.
    inline Vector3 getBonePos(const RigStructure& rigStructure, const std::map<std::string, size_t>& boneIdx,
        const std::string& name)
    {
        auto it = boneIdx.find(name);
        if (it == boneIdx.end())
            return Vector3();
        const auto& b = rigStructure.bones[it->second];
        return Vector3(b.posX, b.posY, b.posZ);
    }

    // Get bone end position by name. Returns zero vector if not found.
    inline Vector3 getBoneEnd(const RigStructure& rigStructure, const std::map<std::string, size_t>& boneIdx,
        const std::string& name)
    {
        auto it = boneIdx.find(name);
        if (it == boneIdx.end())
            return Vector3();
        const auto& b = rigStructure.bones[it->second];
        return Vector3(b.endX, b.endY, b.endZ);
    }

    // Validate that all required bones exist in the rig
    inline bool validateRequiredBones(const std::map<std::string, size_t>& boneIdx,
        const char* const* requiredBones, size_t count)
    {
        for (size_t i = 0; i < count; ++i) {
            if (boneIdx.find(requiredBones[i]) == boneIdx.end())
                return false;
        }
        return true;
    }

    // =========================================================================
    // INTERPOLATION HELPERS
    // =========================================================================

    // Smooth Hermite interpolation (Ken Perlin's smoothstep: 3t^2-2t^3).
    // Maps [0,1] to [0,1] with zero first-derivative at both endpoints, removing
    // the constant-velocity snap that a plain lerp produces at phase boundaries.
    inline double smoothstep(double t)
    {
        // clamp to [0,1] defensively
        if (t <= 0.0)
            return 0.0;
        if (t >= 1.0)
            return 1.0;
        return t * t * (3.0 - 2.0 * t);
    }

    // Smoother-step (Ken Perlin's improved version: 6t^5-15t^4+10t^3).
    // Zero first *and* second derivative at both endpoints -- better for the foot
    // lift arc where a sudden acceleration from rest looks unnatural.
    inline double smootherstep(double t)
    {
        if (t <= 0.0)
            return 0.0;
        if (t >= 1.0)
            return 1.0;
        return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
    }

} // namespace animation
} // namespace dust3d

#endif // DUST3D_ANIMATION_COMMON_H
