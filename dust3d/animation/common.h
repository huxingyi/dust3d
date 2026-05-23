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

    // =========================================================================
    // HAIR CHAIN PHYSICS SIMULATOR
    // =========================================================================
    //
    // Simulates a chain of hair bones using verlet integration with inertia,
    // length constraints, angular stiffness, and gravity. The root of the chain
    // is pinned to the parent bone's animated world transform each frame.
    // Subsequent nodes lag behind due to inertia, are pulled back toward the
    // rigid-follow goal by a spring, and droop under gravity.
    //
    // Physics model:
    //   1. Compute goalTip: where the tip would be if the bone rigidly followed
    //      the parent (animated delta transform applied to bind-space tip).
    //   2. Verlet integrate with damping:
    //        vel   = (tip - prevTip) * damping
    //        tip'  = tip + vel + gravity * dt^2
    //   3. Apply stiffness spring pulling tip' toward goalTip.
    //   4. Project tip' onto a sphere of radius boneLength centered at root
    //      (length constraint -- enforces bone inextensibility).
    //
    // Usage:
    //   HairChainSimulator hairSim;
    //   hairSim.initialize(rig, boneIdx,
    //       {"HairBack1", "HairBack2", "HairBack3"},
    //       buildBoneWorldTransform(bonePos("Head"), boneEnd("Head")),
    //       stiffness, damping, gravityScale);
    //
    //   // Inside frame loop, after parent bone world transform is known:
    //   if (hairSim.active)
    //       hairSim.step(boneWorldTransforms["Head"], dt, boneWorldTransforms);
    struct HairChainSimulator {
        struct BoneData {
            std::string name;
            Vector3 bindRoot; // bind-world-space root position
            Vector3 bindTip; // bind-world-space tip position
            double boneLength;
        };

        Matrix4x4 parentBindInverse; // inverse of parent's bind-world transform
        std::vector<BoneData> bones;
        std::vector<Vector3> tipPos; // current simulated tip positions
        std::vector<Vector3> prevTipPos; // previous frame tip positions (verlet)
        double stiffness = 0.12;
        double damping = 0.88;
        double gravityScale = 1.0;
        bool active = false;

        // Initialize the simulator from rig data.
        // boneNames:           ordered chain of hair bone names, root to tip.
        // parentBindTransform: the parent bone's bind-pose world transform
        //                      (buildBoneWorldTransform of the head bone at rest).
        // stiff:  spring fraction pulling toward rigid-follow pose (0=none, 1=rigid).
        // damp:   velocity retention per frame (0=instant stop, 1=no damping).
        // grav:   gravity multiplier (1=normal earth, 0=weightless).
        void initialize(const RigStructure& rig,
            const std::map<std::string, size_t>& idx,
            const std::vector<std::string>& boneNames,
            const Matrix4x4& parentBindTransform,
            double stiff = 0.12,
            double damp = 0.88,
            double grav = 1.0)
        {
            stiffness = stiff;
            damping = damp;
            gravityScale = grav;
            parentBindInverse = parentBindTransform.inverted();
            bones.clear();
            for (const auto& name : boneNames) {
                auto it = idx.find(name);
                if (it == idx.end())
                    break; // stop at first missing bone in chain
                const auto& b = rig.bones[it->second];
                BoneData bd;
                bd.name = name;
                bd.bindRoot = Vector3(b.posX, b.posY, b.posZ);
                bd.bindTip = Vector3(b.endX, b.endY, b.endZ);
                bd.boneLength = (bd.bindTip - bd.bindRoot).length();
                if (bd.boneLength < 1e-8)
                    bd.boneLength = 1e-8;
                bones.push_back(bd);
            }
            tipPos.resize(bones.size());
            prevTipPos.resize(bones.size());
            for (size_t i = 0; i < bones.size(); ++i) {
                tipPos[i] = bones[i].bindTip;
                prevTipPos[i] = bones[i].bindTip;
            }
            active = !bones.empty();
        }

        // Advance the simulation by one frame.
        // parentWorldTransform: the parent (head) bone's world transform this frame.
        // dt:  frame time step in seconds.
        // out: receives the per-bone world transforms for the hair chain.
        void step(const Matrix4x4& parentWorldTransform,
            double dt,
            std::map<std::string, Matrix4x4>& out)
        {
            if (!active)
                return;

            // Delta transform: maps bind-world positions to current world positions.
            Matrix4x4 delta = parentWorldTransform;
            delta *= parentBindInverse;

            const Vector3 gravityAccel(0.0, -9.8 * gravityScale, 0.0);
            const double dt2 = dt * dt;

            for (size_t i = 0; i < bones.size(); ++i) {
                // Root: first bone tracks animated parent, chain bones use simulated prev tip.
                Vector3 root = (i == 0)
                    ? delta.transformPoint(bones[0].bindRoot)
                    : tipPos[i - 1];

                // Goal tip: where the tip would land if this bone rigidly followed the parent.
                // For chained bones, project the goal direction from the simulated root so
                // the stiffness force stays body-relative even as the chain deviates.
                Vector3 goalTipWorld = delta.transformPoint(bones[i].bindTip);
                Vector3 goalRootWorld = delta.transformPoint(bones[i].bindRoot);
                Vector3 goalDir = goalTipWorld - goalRootWorld;
                double goalDirLen = goalDir.length();
                if (goalDirLen > 1e-8)
                    goalDir = goalDir * (1.0 / goalDirLen);
                else
                    goalDir = Vector3(0.0, -1.0, 0.0);
                Vector3 goalTip = root + goalDir * bones[i].boneLength;

                // Verlet integration with velocity damping.
                Vector3 vel = (tipPos[i] - prevTipPos[i]) * damping;
                Vector3 newTip = tipPos[i] + vel + gravityAccel * dt2;

                // Stiffness spring: blend toward goal (rigid-follow) position.
                newTip = newTip + (goalTip - newTip) * stiffness;

                // Length constraint: project onto sphere of radius boneLength from root.
                Vector3 toNew = newTip - root;
                double toNewLen = toNew.length();
                if (toNewLen > 1e-8)
                    newTip = root + toNew * (bones[i].boneLength / toNewLen);
                else
                    newTip = root + Vector3(0.0, -bones[i].boneLength, 0.0);

                prevTipPos[i] = tipPos[i];
                tipPos[i] = newTip;

                out[bones[i].name] = buildBoneWorldTransform(root, newTip);
            }
        }
    };

    // =========================================================================
    // EYELID BLINK
    // =========================================================================

    // Apply a single blink to eyelid bones during the animation cycle.
    // Call this per-frame after other bone transforms are computed.
    // blinkTime: normalized time in [0,1] when the blink occurs
    // blinkDuration: fraction of cycle the blink takes
    inline void applyEyelidBlink(const RigStructure& rig,
        const std::map<std::string, size_t>& boneIdx,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        std::map<std::string, Matrix4x4>& boneWorldTransforms,
        std::map<std::string, Matrix4x4>& boneSkinMatrices,
        float tNormalized,
        float blinkTime = 0.5f,
        float blinkDuration = 0.1f)
    {
        // Need at least one upper+lower pair
        bool hasLeft = boneIdx.count("LeftUpperEyelid") && boneIdx.count("LeftLowerEyelid");
        bool hasRight = boneIdx.count("RightUpperEyelid") && boneIdx.count("RightLowerEyelid");
        if (!hasLeft && !hasRight)
            return;

        // Compute blink factor: 0 = open, 1 = closed
        float blinkFactor = 0.0f;
        float halfDur = blinkDuration * 0.5f;
        float dt = tNormalized - blinkTime;
        // Wrap around for blinks near cycle boundaries
        if (dt > 0.5f)
            dt -= 1.0f;
        if (dt < -0.5f)
            dt += 1.0f;
        float absDt = std::abs(dt);
        if (absDt < halfDur) {
            float t = 1.0f - (absDt / halfDur);
            blinkFactor = (float)smoothstep(t);
        }

        // Eyelid bones must follow the Head every frame (not just during blink).
        // To avoid any numerical offset, we derive the eyelid's rest-pose world
        // transform from its own inverse bind matrix (guaranteed exact match),
        // then apply the Head's animation delta on top.
        auto headWorldIt = boneWorldTransforms.find("Head");
        if (headWorldIt == boneWorldTransforms.end())
            return;

        // Head's rest-pose world transform from its inverse bind matrix
        auto headInvBindIt = inverseBindMatrices.find("Head");
        if (headInvBindIt == inverseBindMatrices.end())
            return;

        // Delta = animatedHead * inverse(restHead)
        Matrix4x4 headDelta = headWorldIt->second;
        headDelta *= headInvBindIt->second;

        // For blink, rotate each lid around the bone direction (the hinge axis).
        // The bone direction is set by the rig generator to be horizontal and
        // perpendicular to the outward eye normal, so rotating around it sweeps
        // the lid over the eyeball naturally.
        // Each bone stores its own closingAngle computed from the actual geometry,
        // guaranteeing the eye fully closes at blinkFactor=1.
        const char* eyelidNames[] = {
            "LeftUpperEyelid",
            "LeftLowerEyelid",
            "RightUpperEyelid",
            "RightLowerEyelid",
        };

        for (const char* eyelidName : eyelidNames) {
            auto it = boneIdx.find(eyelidName);
            if (it == boneIdx.end())
                continue;

            auto invIt = inverseBindMatrices.find(eyelidName);
            if (invIt == inverseBindMatrices.end())
                continue;

            Matrix4x4 eyelidRestTransform = invIt->second.inverted();

            Matrix4x4 transform = headDelta;
            transform *= eyelidRestTransform;

            if (blinkFactor > 0.001f) {
                const auto& bone = rig.bones[it->second];
                Vector3 boneDir(bone.endX - bone.posX, bone.endY - bone.posY, bone.endZ - bone.posZ);
                if (!boneDir.isZero() && std::abs(bone.closingAngle) > 1e-6f) {
                    Vector3 boneMid(
                        (bone.posX + bone.endX) * 0.5,
                        (bone.posY + bone.endY) * 0.5,
                        (bone.posZ + bone.endZ) * 0.5);

                    Vector3 animatedPivot = headDelta.transformPoint(boneMid);
                    Vector3 animatedAxis = headDelta.transformVector(boneDir.normalized());
                    animatedAxis.normalize();

                    float lidAngle = blinkFactor * bone.closingAngle;

                    Matrix4x4 blinkTransform;
                    blinkTransform.translate(animatedPivot);
                    Quaternion blinkRot = Quaternion::fromAxisAndAngle(animatedAxis, lidAngle);
                    blinkTransform.rotate(blinkRot);
                    blinkTransform.translate(Vector3(-animatedPivot.x(), -animatedPivot.y(), -animatedPivot.z()));

                    transform = blinkTransform;
                    transform *= headDelta;
                    transform *= eyelidRestTransform;
                }
            }

            boneWorldTransforms[eyelidName] = transform;

            Matrix4x4 skin = transform;
            skin *= invIt->second;
            boneSkinMatrices[eyelidName] = skin;
        }
    }

} // namespace animation
} // namespace dust3d

#endif // DUST3D_ANIMATION_COMMON_H
