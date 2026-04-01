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
    // IK SOLVERS
    // =========================================================================

    // CCD (Cyclic Coordinate Descent) IK solver
    // Iteratively adjusts each joint angle from end-effector back to root so
    // that the chain tip converges toward the target position.
    // @param joints: ordered positions [root … end-effector]  (root stays fixed)
    // @param target: desired end-effector position
    // @param maxIterations: maximum IK solver iterations
    inline void solveCcdIk(std::vector<Vector3>& joints, const Vector3& target, int maxIterations)
    {
        const double threshold2 = 1e-8;

        for (int iter = 0; iter < maxIterations; ++iter) {
            double dist2 = (joints.back() - target).lengthSquared();
            if (dist2 <= threshold2)
                break;

            // Walk from the joint just before the end-effector back to the root
            for (int i = static_cast<int>(joints.size()) - 2; i >= 0; --i) {
                Vector3 toEnd = joints.back() - joints[i];
                Vector3 toTarget = target - joints[i];

                if (toEnd.lengthSquared() < 1e-12 || toTarget.lengthSquared() < 1e-12)
                    continue;

                Quaternion rotation = Quaternion::rotationTo(toEnd.normalized(), toTarget.normalized());

                // Apply the rotation to every descendant joint
                Matrix4x4 rotMat;
                rotMat.rotate(rotation);
                for (size_t j = i + 1; j < joints.size(); ++j) {
                    Vector3 offset = joints[j] - joints[i];
                    joints[j] = joints[i] + rotMat.transformVector(offset);
                }
            }
        }
    }

    // FABRIK (Forward And Backward Reaching Inverse Kinematics) solver
    // Alternative to CCD for stronger geometric behavior in serial 1D chains.
    // Iteratively adjusts joint positions to preserve bone lengths while matching
    // end-effector to the target. Works with arbitrary 3D joint chains.
    // @param joints: ordered positions [root … end-effector]  (root stays fixed)
    // @param target: desired end-effector position
    // @param maxIterations: maximum IK solver iterations
    // @param preferredPlaneNormal: optional plane normal to bias leg toward
    inline void solveFabrikIk(std::vector<Vector3>& joints, const Vector3& target, int maxIterations,
        const Vector3& preferredPlaneNormal = Vector3())
    {
        const double threshold2 = 1e-8;
        const double planeBias = 0.45; // bias toward preferred leg plane
        size_t n = joints.size();
        if (n < 2)
            return;

        // original segment lengths
        std::vector<double> lengths(n - 1);
        double totalLength = 0.0;
        for (size_t i = 0; i < n - 1; ++i) {
            lengths[i] = (joints[i + 1] - joints[i]).length();
            totalLength += lengths[i];
        }

        Vector3 rootPos = joints[0];
        double distRootTarget = (target - rootPos).length();
        Vector3 planeNorm;
        if (!preferredPlaneNormal.isZero())
            planeNorm = preferredPlaneNormal.normalized();

        if (distRootTarget <= 1e-12) {
            // degenerate target; no strong positioning.
            return;
        }

        if (distRootTarget > totalLength) {
            // target unreachable: stretch fully toward target.
            Vector3 direction = (target - rootPos).normalized();
            joints[0] = rootPos;
            for (size_t i = 1; i < n; ++i) {
                joints[i] = joints[i - 1] + direction * lengths[i - 1];
            }
            return;
        }

        std::vector<Vector3> newPos(joints);

        for (int iter = 0; iter < maxIterations; ++iter) {
            // Backward reaching
            newPos[n - 1] = target;
            for (int i = static_cast<int>(n) - 2; i >= 0; --i) {
                Vector3 dir = newPos[i] - newPos[i + 1];
                double r = dir.length();
                if (r < 1e-12) {
                    continue;
                }
                dir *= (1.0 / r);
                newPos[i] = newPos[i + 1] + dir * lengths[i];
            }

            // Forward reaching
            newPos[0] = rootPos;
            for (size_t i = 0; i < n - 1; ++i) {
                Vector3 dir = newPos[i + 1] - newPos[i];
                double r = dir.length();
                if (r < 1e-12) {
                    continue;
                }
                dir *= (1.0 / r);
                newPos[i + 1] = newPos[i] + dir * lengths[i];
            }

            // Keep chain close to preferred plane, if one exists
            if (!planeNorm.isZero()) {
                for (size_t i = 1; i + 1 < n; ++i) {
                    Vector3 item = newPos[i] - rootPos;
                    double distToPlane = Vector3::dotProduct(item, planeNorm);
                    Vector3 projected = newPos[i] - planeNorm * distToPlane;
                    newPos[i] = newPos[i] * (1.0 - planeBias) + projected * planeBias;
                }

                // Re-apply segment lengths after plane bias: backward then forward
                for (int i = static_cast<int>(n) - 2; i >= 0; --i) {
                    Vector3 dir = newPos[i] - newPos[i + 1];
                    double r = dir.length();
                    if (r < 1e-12)
                        continue;
                    newPos[i] = newPos[i + 1] + dir * (lengths[i] / r);
                }
                newPos[0] = rootPos;
                for (size_t i = 0; i < n - 1; ++i) {
                    Vector3 dir = newPos[i + 1] - newPos[i];
                    double r = dir.length();
                    if (r < 1e-12)
                        continue;
                    newPos[i + 1] = newPos[i] + dir * (lengths[i] / r);
                }
            }

            if ((newPos.back() - target).lengthSquared() <= threshold2) {
                break;
            }
        }

        joints = newPos;
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
