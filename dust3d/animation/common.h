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

#include <algorithm>
#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>
#include <functional>
#include <map>
#include <set>
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
        double softness = 0.1, Vector3* previousBend = nullptr, Vector3* previousReach = nullptr)
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
        // Direction must use the original vector length. distTarget may have been
        // softened/clamped; dividing by it stretches the direction and both links.
        Vector3 dirTarget = toTarget.normalized();

        // Within the inner unreachable sphere, projecting a target through
        // the hip can reverse the entire folded limb. Retain a continuous reach
        // direction there, instead of snapping to the opposite side of the sphere.
        if (previousReach && previousReach->lengthSquared() > 1e-12
            && toTarget.length() < 2.0 * minReach) {
            Vector3 previous = previousReach->normalized();
            double angle = std::acos(std::clamp(Vector3::dotProduct(previous, dirTarget), -1.0, 1.0));
            if (angle > 0.35) {
                Vector3 axis = Vector3::crossProduct(previous, dirTarget);
                if (axis.lengthSquared() < 1e-12 && previousBend)
                    axis = Vector3::crossProduct(previous, *previousBend);
                if (axis.lengthSquared() > 1e-12) {
                    Matrix4x4 turn;
                    turn.rotate(axis.normalized(), 0.35);
                    dirTarget = turn.transformVector(previous).normalized();
                } else
                    dirTarget = previous;
            }
        }
        if (previousReach)
            *previousReach = dirTarget;

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

        if (previousBend && previousBend->lengthSquared() > 1e-12) {
            Vector3 previous = *previousBend - dirTarget * Vector3::dotProduct(*previousBend, dirTarget);
            if (previous.lengthSquared() > 1e-12) {
                previous.normalize();
                if (poleLen < 1e-6 * std::max(chainLength, 1e-6))
                    bendDir = previous;
                else if (Vector3::dotProduct(previous, bendDir) < 0.0)
                    bendDir = -bendDir;
            }
        }
        if (previousBend)
            *previousBend = bendDir;

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
    // HIT REACTION HELPERS (shared by the hurt clips)
    // =========================================================================

    // Envelope of a one-shot hit reaction over the normalized clip time t:
    //   0 - snapEnd       snap to the full reaction with a sine ease-out; about 0.1 s
    //                     (3 frames at 30 fps) reads as a hit, a single frame as a glitch,
    //   snapEnd - +0.08   hold at the peak (hit-stop, sells the weight of the blow),
    //   then              settle back with one small overshoot past the rest pose.
    // It is exactly 0 at t <= 0 and t >= 1, with zero slope at the end, so a hurt clip
    // blends in from any loop and hands back to it without a pop.
    // recoverySpeed > 1 settles sooner and livelier, < 1 slower and heavier.
    inline double hitReactionEnvelope(double t, double recoverySpeed = 1.0, double snapEnd = 0.1)
    {
        if (t <= 0.0 || t >= 1.0)
            return 0.0;
        snapEnd = std::clamp(snapEnd, 0.02, 0.3);
        const double holdEnd = snapEnd + 0.08;
        if (t < snapEnd)
            return std::sin(0.5 * Math::Pi * t / snapEnd);
        if (t < holdEnd)
            return 1.0;
        double u = (t - holdEnd) / (1.0 - holdEnd);
        double k = std::max(0.4, recoverySpeed);
        double settle = std::exp(-3.2 * k * u) * std::cos(Math::Pi * 1.35 * k * u);
        double fade = 1.0 - smootherstep((u - 0.55) / 0.45);
        return settle * fade;
    }

    // The snap length for hitReactionEnvelope: about 0.1 s of a clip lasting durationSeconds.
    inline double hitSnapFraction(double durationSeconds)
    {
        return std::clamp(0.1 / std::max(0.1, durationSeconds), 0.04, 0.2);
    }

    // A fast tremble that starts at the impact and dies away within the first third of
    // the clip; multiply it by a small amplitude. Zero at t = 0.
    inline double hitShudder(double t, double cycles = 6.0)
    {
        if (t <= 0.0 || t >= 1.0)
            return 0.0;
        return std::sin(2.0 * Math::Pi * cycles * t) * std::exp(-9.0 * t);
    }

    // Rigid rotation about a pivot point: T(pivot) * R(axis, angle) * T(-pivot).
    inline Matrix4x4 rotationAbout(const Vector3& pivot, const Vector3& axis, double angle)
    {
        Matrix4x4 m;
        m.translate(pivot);
        m.rotate(axis, angle);
        m.translate(Vector3() - pivot);
        return m;
    }

    // A rotation about `pivot` that turns direction `from` toward direction `to` by the
    // fraction w (0 = identity, 1 = all the way).
    inline Matrix4x4 turnAbout(const Vector3& pivot, const Vector3& from, const Vector3& to, double w, Quaternion* previousSwing = nullptr)
    {
        Vector3 a = from.normalized();
        Vector3 b = to.normalized();
        if (previousSwing) {
            Quaternion swing = Quaternion::rotationTo(a, b);
            double dot = swing.w() * previousSwing->w() + swing.x() * previousSwing->x()
                + swing.y() * previousSwing->y() + swing.z() * previousSwing->z();
            if (dot < 0.0)
                swing = Quaternion(-swing.w(), -swing.x(), -swing.y(), -swing.z());
            Vector3 axis(swing.x(), swing.y(), swing.z());
            if (axis.lengthSquared() < 1e-18)
                axis = Vector3(previousSwing->x(), previousSwing->y(), previousSwing->z());
            *previousSwing = swing;
            if (axis.lengthSquared() < 1e-18 || w <= 0.0)
                return Matrix4x4();
            // Keep the continuous quaternion branch before applying a fractional
            // swing. Shortest rotations change branch abruptly at 180 degrees.
            return rotationAbout(pivot, axis.normalized(),
                2.0 * std::acos(std::clamp(swing.w(), -1.0, 1.0)) * std::clamp(w, 0.0, 1.0));
        }
        Vector3 axis = Vector3::crossProduct(a, b);
        if (w <= 0.0)
            return Matrix4x4();
        double dot = std::clamp(Vector3::dotProduct(a, b), -1.0, 1.0);
        if (axis.lengthSquared() < 1e-12) {
            if (dot >= 0.0)
                return Matrix4x4();
            auto swing = Quaternion::rotationTo(a, b);
            axis = Vector3(swing.x(), swing.y(), swing.z());
        }
        double angle = std::acos(dot) * std::min(1.0, w);
        return rotationAbout(pivot, axis.normalized(), angle);
    }

    // Mark the stance boundaries used by the gait equations. Engines can sync
    // phases and trigger footsteps without guessing from mesh or cape velocity.
    inline void addGaitMarkers(RigAnimationClip& clip, double cycles, double duration,
        const std::string& bone, double touchdown, double lift)
    {
        auto phase = [](double value) { return value - std::floor(value); };
        // A clip repeats its gait a handful of times at most; an absurd cycle count
        // must not flood it with events.
        const int count = static_cast<int>(std::clamp(cycles, 1.0, 64.0));
        for (int cycle = 0; cycle < count; ++cycle) {
            clip.events.push_back({ "step", static_cast<float>((cycle + phase(touchdown)) * duration / cycles), bone });
            clip.events.push_back({ "lift", static_cast<float>((cycle + phase(lift)) * duration / cycles), bone });
        }
    }

    // Fill boneSkinMatrices from boneWorldTransforms for one frame.
    inline void finishFrame(BoneAnimationFrame& frame, const std::map<std::string, Matrix4x4>& inverseBindMatrices)
    {
        frame.boneSkinMatrices.clear();
        for (const auto& pair : frame.boneWorldTransforms) {
            auto invIt = inverseBindMatrices.find(pair.first);
            if (invIt == inverseBindMatrices.end())
                continue;
            Matrix4x4 skinMat = pair.second;
            skinMat *= invIt->second;
            frame.boneSkinMatrices[pair.first] = skinMat;
        }
    }

    // Pose a three-bone leg (upper, lower, foot) with two-bone IK. The hip rides on
    // `bodyLayer` (a rigid transform of the rest pose); the ankle (end of the lower bone) goes
    // to `ankleTarget`; the knee is the start of the lower bone (rigs may leave a gap after the
    // upper bone) and bends toward `bendHint` (a world direction; zero keeps the rest bend).
    // Bones are rotated, not rebuilt, so they keep their roll, and a leg whose target is its
    // layered rest ankle is exactly at rest. The foot keeps its rest orientation (flat on the
    // ground) when footFollowsShin is false, or keeps its angle to the shin when true.
    inline void poseTwoBoneLeg(const RigStructure& rigStructure, const std::map<std::string, size_t>& boneIdx,
        const std::string& upper, const std::string& lower, const std::string& foot,
        const Matrix4x4& bodyLayer, const Vector3& ankleTarget, const Vector3& bendHint, bool footFollowsShin,
        std::map<std::string, Matrix4x4>& boneWorldTransforms, Vector3* previousBend = nullptr)
    {
        Vector3 hipRest = getBonePos(rigStructure, boneIdx, upper);
        Vector3 kneeRest = getBonePos(rigStructure, boneIdx, lower);
        Vector3 ankleRest = getBoneEnd(rigStructure, boneIdx, lower);
        Vector3 hip = bodyLayer.transformPoint(hipRest);
        Vector3 knee = bodyLayer.transformPoint(kneeRest);
        Vector3 ankle = bodyLayer.transformPoint(ankleRest);
        std::vector<Vector3> chain = { hip, knee, ankle };
        Vector3 restBend = bodyLayer.transformVector(kneeRest - (hipRest + ankleRest) * 0.5);
        Vector3 bend = bendHint.lengthSquared() > 1e-12 ? bendHint : restBend;
        solveTwoBoneIk(chain, ankleTarget, knee + bend * 4.0, 0.0, previousBend);

        auto rotated = [](const Matrix4x4& layered, const Vector3& from, const Vector3& fromDir,
                           const Vector3& to, const Vector3& toDir) -> Matrix4x4 {
            Matrix4x4 m;
            m.translate(to);
            if (fromDir.lengthSquared() > 1e-18 && toDir.lengthSquared() > 1e-18)
                m.rotate(Quaternion::rotationTo(fromDir.normalized(), toDir.normalized()));
            m.translate(Vector3() - from);
            m *= layered;
            return m;
        };
        auto layeredRest = [&](const std::string& name) {
            Matrix4x4 m = bodyLayer;
            m *= buildBoneWorldTransform(getBonePos(rigStructure, boneIdx, name), getBoneEnd(rigStructure, boneIdx, name));
            return m;
        };
        // The thigh turns from its rest direction to the solved one. The shin is carried by
        // the thigh and then bends at the knee by the change of the knee angle only, like a
        // hinge: a folded leg, whose shin points nearly opposite its rest direction, keeps its
        // twist instead of flipping over.
        Vector3 thighFrom = knee - hip;
        Vector3 thighTo = chain[1] - hip;
        Matrix4x4 thighMotion;
        thighMotion.translate(hip);
        if (thighFrom.lengthSquared() > 1e-18 && thighTo.lengthSquared() > 1e-18)
            thighMotion.rotate(Quaternion::rotationTo(thighFrom.normalized(), thighTo.normalized()));
        thighMotion.translate(Vector3() - hip);
        Matrix4x4 thighWorld = thighMotion;
        thighWorld *= layeredRest(upper);
        boneWorldTransforms[upper] = thighWorld;
        Vector3 shinFrom = ankle - knee;
        Vector3 shinCarried = thighMotion.transformVector(shinFrom);
        Vector3 shinTo = chain[2] - chain[1];
        Matrix4x4 shinMotion = rotated(thighMotion, thighMotion.transformPoint(knee), shinCarried, chain[1], shinTo);
        Matrix4x4 shinWorld = shinMotion;
        shinWorld *= layeredRest(lower);
        boneWorldTransforms[lower] = shinWorld;
        if (boneIdx.count(foot)) {
            Matrix4x4 footRest = buildBoneWorldTransform(getBonePos(rigStructure, boneIdx, foot), getBoneEnd(rigStructure, boneIdx, foot));
            if (footFollowsShin) {
                Matrix4x4 m = shinMotion;
                m *= bodyLayer;
                m *= footRest;
                boneWorldTransforms[foot] = m;
            } else {
                Matrix4x4 m;
                m.translate(chain[2] - ankleRest);
                m *= footRest;
                boneWorldTransforms[foot] = m;
            }
        }
    }

    // A planted leg: the ankle stays at its rest position plus `ankleOffset` while the hip
    // rides on `bodyLayer`; the knee keeps its rest bend and the foot stays flat.
    inline void posePlantedLeg(const RigStructure& rigStructure, const std::map<std::string, size_t>& boneIdx,
        const std::string& upper, const std::string& lower, const std::string& foot,
        const Matrix4x4& bodyLayer, const Vector3& ankleOffset,
        std::map<std::string, Matrix4x4>& boneWorldTransforms)
    {
        Vector3 ankleRest = getBoneEnd(rigStructure, boneIdx, lower);
        poseTwoBoneLeg(rigStructure, boneIdx, upper, lower, foot, bodyLayer, ankleRest + ankleOffset, Vector3(), false,
            boneWorldTransforms);
    }

    // Where a point attached to a bone (given in the rest pose) is after the bone moved from
    // restWorld to world, e.g. a shoulder joint riding on the chest.
    inline Vector3 carryWithBone(const Matrix4x4& restWorld, const Matrix4x4& world, const Vector3& restPoint)
    {
        Matrix4x4 delta = world;
        delta *= restWorld.inverted();
        return delta.transformPoint(restPoint);
    }

    // Keep bones out of the ground in a finished frame. Every selected bone, from the root
    // outwards, is turned about its own start just enough that its end stays at least
    // `clearance` x its capsule radius above groundY; the bones after it follow. The bone roll
    // is kept. With limitToRest, a bone that already rests lower may go down to its rest
    // height (e.g. a tail lying on the ground).
    inline void keepBonesAboveGround(const RigStructure& rigStructure, const std::map<std::string, size_t>& boneIdx,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices, BoneAnimationFrame& frame, double groundY,
        const std::function<bool(const std::string&)>& select, double clearance, bool limitToRest,
        std::map<std::string, Vector3>* groundDirections = nullptr)
    {
        std::vector<std::pair<int, std::string>> order;
        for (const auto& bone : rigStructure.bones) {
            if (!frame.boneWorldTransforms.count(bone.name))
                continue;
            int depth = 0;
            for (std::string p = bone.parent; !p.empty() && depth < 64; ++depth) {
                auto it = boneIdx.find(p);
                p = it == boneIdx.end() ? std::string() : rigStructure.bones[it->second].parent;
            }
            order.push_back({ depth, bone.name });
        }
        std::sort(order.begin(), order.end());
        std::map<std::string, Matrix4x4> correction;
        for (const auto& item : order) {
            const std::string& name = item.second;
            const RigNode& bone = rigStructure.bones[boneIdx.at(name)];
            auto parentCorrection = correction.find(bone.parent);
            bool selected = select(name);
            if (!selected && parentCorrection == correction.end())
                continue;
            Matrix4x4 world = frame.boneWorldTransforms[name];
            Matrix4x4 corr = parentCorrection != correction.end() ? parentCorrection->second : Matrix4x4();
            if (parentCorrection != correction.end()) {
                Matrix4x4 m = corr;
                m *= world;
                world = m;
            }
            if (selected) {
                double length = (Vector3(bone.endX, bone.endY, bone.endZ) - Vector3(bone.posX, bone.posY, bone.posZ)).length();
                Vector3 start = world.transformPoint(Vector3());
                Vector3 end = world.transformPoint(Vector3(0.0, 0.0, length));
                double limit = groundY + clearance * bone.capsuleRadius;
                if (limitToRest)
                    limit = std::min(limit, static_cast<double>(bone.endY));
                if (length > 1e-9 && end.y() < limit) {
                    Vector3 dir = end - start;
                    double rise = limit - start.y();
                    Vector3 horizontal(dir.x(), 0.0, dir.z());
                    // Horizontal projection is singular when a bone points straight down.
                    // Carry its contact direction through that singularity instead of switching
                    // abruptly between ancestors on either side of the vertical pose.
                    Vector3 hint = world.transformVector(Vector3(1.0, 0.0, 0.0));
                    hint = Vector3::crossProduct(hint, Vector3(0.0, 1.0, 0.0));
                    hint.setY(0.0);
                    if (groundDirections && groundDirections->count(name))
                        hint = groundDirections->at(name);
                    if (hint.lengthSquared() < 1e-12)
                        hint = Vector3(0.0, 0.0, 1.0);
                    hint.normalize();
                    double strength = std::clamp(horizontal.length() / (0.3 * length), 0.0, 1.0);
                    strength = strength * strength * (3.0 - 2.0 * strength);
                    if (horizontal.lengthSquared() > 1e-18) {
                        Vector3 target = horizontal.normalized();
                        double angle = std::atan2(Vector3::crossProduct(hint, target).y(),
                            Vector3::dotProduct(hint, target));
                        Matrix4x4 turn;
                        turn.rotate(Vector3(0.0, 1.0, 0.0), angle * strength);
                        horizontal = turn.transformVector(hint);
                    } else
                        horizontal = hint;
                    if (groundDirections)
                        (*groundDirections)[name] = horizontal.normalized();
                    // Tilt up just enough; a bone whose start is itself well below the limit
                    // tilts up at most ~45 degrees rather than standing on end.
                    rise = std::min(rise, 0.7 * length);
                    double h2 = length * length - rise * rise;
                    Vector3 newDir;
                    if (horizontal.lengthSquared() > 1e-12)
                        newDir = horizontal.normalized() * std::sqrt(h2) + Vector3(0.0, rise, 0.0);
                    else
                        newDir = Vector3(0.0, length, 0.0);
                    Matrix4x4 turn;
                    turn.translate(start);
                    turn.rotate(Quaternion::rotationTo(dir.normalized(), newDir.normalized()));
                    turn.translate(Vector3() - start);
                    Matrix4x4 m = turn;
                    m *= world;
                    world = m;
                    Matrix4x4 c = turn;
                    c *= corr;
                    corr = c;
                }
            }
            correction[name] = corr;
            frame.boneWorldTransforms[name] = world;
            auto inv = inverseBindMatrices.find(name);
            if (inv != inverseBindMatrices.end()) {
                Matrix4x4 skin = world;
                skin *= inv->second;
                frame.boneSkinMatrices[name] = skin;
            }
        }
    }

    // Keep tails out of the ground in a finished frame. Any clip that pitches or lowers the
    // body swings a tail that rests near the ground (kangaroo, lizard, crocodile, dinosaur)
    // down through it.
    inline void keepTailsAboveGround(const RigStructure& rigStructure, const std::map<std::string, size_t>& boneIdx,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices, BoneAnimationFrame& frame, double groundY,
        std::map<std::string, Vector3>* groundDirections = nullptr)
    {
        keepBonesAboveGround(
            rigStructure, boneIdx, inverseBindMatrices, frame, groundY, [](const std::string& name) { return name.find("Tail") != std::string::npos; }, 0.0, true, groundDirections);
    }

    // Progress of a body falling to the ground under gravity: 0 at t <= 0, accelerating to 1
    // at `landAt`, then one rebound of height `bounce` (a fraction of the fall) over
    // `bounceTime`, and 1 from then on.
    inline double fallEnvelope(double t, double landAt, double bounce = 0.08, double bounceTime = 0.14)
    {
        if (t <= 0.0)
            return 0.0;
        if (t < landAt) {
            double x = t / landAt;
            return x * x;
        }
        double u = (t - landAt) / std::max(1e-6, bounceTime);
        if (u < 1.0)
            return 1.0 - bounce * std::sin(Math::Pi * u);
        return 1.0;
    }

    // A damped wobble after an impact at u = 0: 0 before it, then a decaying sine of the given
    // period (seconds or normalised time, as long as both match) that has mostly died out
    // after about three decay times.
    inline double impactWobble(double u, double period, double decay)
    {
        if (u <= 0.0)
            return 0.0;
        return std::exp(-u / std::max(1e-6, decay)) * std::sin(2.0 * Math::Pi * u / std::max(1e-6, period));
    }

    // Rate of change of f at x, normalised by `peak` and clamped to [0, 1]: how hard a limb
    // trailing a falling body is pulled behind it (inertia), for lag and follow-through.
    inline double normalisedRate(const std::function<double(double)>& f, double x, double peak)
    {
        // An impact changes velocity instantaneously, but trailing limbs retain
        // their momentum. Average the recent velocity rather than turning that
        // impulse directly into a discontinuous wing/head angle.
        const double h = 0.08;
        return std::clamp((f(x) - f(x - h)) / h / std::max(1e-6, peak), 0.0, 1.0);
    }

    // Roll a limb about its own axis (through `pivot`, along the limb's current direction)
    // by the fraction w, so that `restNormal` (a direction across the limb at rest, carried
    // by `layer`) ends vertical, up or down, whichever is the smaller turn. With the normal
    // of the plane a held weapon lies in, a limb lying on the ground keeps the weapon flat
    // instead of standing it on end. Pass the same `facing` (starting at 0) for every frame
    // of a clip: it keeps the up-or-down choice made on the first frame, so the limb never
    // flips over halfway.
    inline Matrix4x4 rollToFaceUp(const Matrix4x4& layer, const Vector3& pivot, const Vector3& restAxis,
        const Vector3& restNormal, double w, double* facing = nullptr)
    {
        Vector3 up(0.0, 1.0, 0.0);
        Vector3 a0 = restAxis.normalized();
        Vector3 n0 = restNormal - a0 * Vector3::dotProduct(restNormal, a0);
        Vector3 a1 = layer.transformVector(restAxis).normalized();
        Vector3 desired = up - a1 * Vector3::dotProduct(up, a1);
        if (w <= 0.0 || n0.lengthSquared() < 1e-6 || desired.lengthSquared() < 0.04)
            return Matrix4x4();
        Vector3 n1 = layer.transformVector(n0);
        n1 = n1 - a1 * Vector3::dotProduct(n1, a1);
        if (n1.lengthSquared() < 1e-9)
            return Matrix4x4();
        n1.normalize();
        desired.normalize();
        double sign = Vector3::dotProduct(n1, desired) < 0.0 ? -1.0 : 1.0;
        if (facing) {
            if (*facing == 0.0)
                *facing = sign;
            sign = *facing;
        }
        desired = desired * sign;
        double angle = std::atan2(Vector3::dotProduct(Vector3::crossProduct(n1, desired), a1),
            Vector3::dotProduct(n1, desired));
        return rotationAbout(pivot, a1, angle * std::min(1.0, w));
    }

    // Lowest rest point of the feet (Foot bones, or leg tips of six- and eight-legged rigs),
    // or of the body chain for legless rigs: where the ground is.
    inline double restGroundHeight(const RigStructure& rigStructure)
    {
        double lowest = 0.0;
        bool found = false;
        for (const char* keys : { "Foot|Tibia", "Spine|Body|Tail" }) {
            std::string k(keys);
            for (const auto& bone : rigStructure.bones) {
                bool match = false;
                size_t from = 0;
                while (from <= k.size()) {
                    size_t bar = k.find('|', from);
                    std::string key = k.substr(from, bar == std::string::npos ? std::string::npos : bar - from);
                    match = match || bone.name.find(key) != std::string::npos;
                    if (bar == std::string::npos)
                        break;
                    from = bar + 1;
                }
                if (!match)
                    continue;
                double y = std::min(bone.posY, bone.endY);
                if (!found || y < lowest)
                    lowest = y;
                found = true;
            }
            if (found)
                break;
        }
        return lowest;
    }

    // Give every bone transform that was built from the bone's direction alone
    // (buildBoneWorldTransform) the roll of the smallest turn from the bone's rest
    // orientation. The roll buildBoneWorldTransform picks is undefined for bones pointing
    // backwards (-Z: tails, abdomens, fins) and turns wildly as they wobble, twisting or
    // flipping flat parts; turning from the rest orientation instead is exact at rest and
    // only undefined for a bone turned right round from its rest direction. Transforms made
    // by moving the rest pose rigidly (layered clips, deliberate rolls) are left as they are.
    inline void referenceRollsToRest(const RigStructure& rigStructure, const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& clip)
    {
        for (const auto& bone : rigStructure.bones) {
            Vector3 restStart(bone.posX, bone.posY, bone.posZ);
            Vector3 restDir = Vector3(bone.endX, bone.endY, bone.endZ) - restStart;
            if (restDir.lengthSquared() < 1e-18)
                continue;
            restDir.normalize();
            Matrix4x4 restWorld = buildBoneWorldTransform(restStart, restStart + restDir);
            auto inv = inverseBindMatrices.find(bone.name);
            // Direction-only frames have no authored twist. Transport their roll along the
            // motion path: a fresh rest-to-direction rotation is singular at the antipode
            // and can flip an arm or wing as it crosses overhead. Preserve the authored
            // endpoint rolls by distributing the transport's residual twist over the clip.
            std::vector<Matrix4x4> transported;
            bool directionOnly = !clip.frames.empty();
            for (const auto& frame : clip.frames) {
                auto found = frame.boneWorldTransforms.find(bone.name);
                if (found == frame.boneWorldTransforms.end()) {
                    directionOnly = false;
                    break;
                }
                const auto& world = found->second;
                Vector3 start = world.transformPoint(Vector3());
                Vector3 dir = world.transformVector(Vector3(0.0, 0.0, 1.0)).normalized();
                auto raw = buildBoneWorldTransform(start, start + dir);
                for (int k = 0; k < 16; ++k)
                    directionOnly = directionOnly && std::abs(world.constData()[k] - raw.constData()[k]) < 1e-6;
                if (!directionOnly)
                    break;
                Matrix4x4 referenced;
                referenced.translate(start);
                if (transported.empty()) {
                    referenced.rotate(Quaternion::rotationTo(restDir, dir));
                    referenced.translate(Vector3() - restStart);
                    referenced *= restWorld;
                } else {
                    const auto& previous = transported.back();
                    referenced.rotate(Quaternion::rotationTo(previous.transformVector(Vector3(0.0, 0.0, 1.0)), dir));
                    referenced.translate(Vector3() - previous.transformPoint(Vector3()));
                    referenced *= previous;
                }
                transported.push_back(referenced);
            }
            if (directionOnly) {
                const auto& last = transported.back();
                Vector3 dir = last.transformVector(Vector3(0.0, 0.0, 1.0)).normalized();
                Matrix4x4 target;
                target.rotate(Quaternion::rotationTo(restDir, dir));
                target *= restWorld;
                Vector3 from = last.transformVector(Vector3(1.0, 0.0, 0.0)).normalized();
                Vector3 to = target.transformVector(Vector3(1.0, 0.0, 0.0)).normalized();
                double residual = std::atan2(Vector3::dotProduct(dir, Vector3::crossProduct(from, to)),
                    Vector3::dotProduct(from, to));
                for (size_t i = 0; i < transported.size(); ++i) {
                    auto& frame = clip.frames[i];
                    auto& world = transported[i];
                    Vector3 start = world.transformPoint(Vector3());
                    Vector3 axis = world.transformVector(Vector3(0.0, 0.0, 1.0));
                    double t = transported.size() > 1 ? static_cast<double>(i) / (transported.size() - 1) : 0.0;
                    Matrix4x4 correction;
                    correction.translate(start);
                    correction.rotate(axis, residual * t * t * (3.0 - 2.0 * t));
                    correction.translate(Vector3() - start);
                    correction *= world;
                    frame.boneWorldTransforms[bone.name] = correction;
                    if (inv != inverseBindMatrices.end()) {
                        correction *= inv->second;
                        frame.boneSkinMatrices[bone.name] = correction;
                    }
                }
                continue;
            }
            for (auto& frame : clip.frames) {
                auto it = frame.boneWorldTransforms.find(bone.name);
                if (it == frame.boneWorldTransforms.end())
                    continue;
                Matrix4x4& world = it->second;
                Vector3 start = world.transformPoint(Vector3());
                Vector3 dir = world.transformVector(Vector3(0.0, 0.0, 1.0));
                if (dir.lengthSquared() < 1e-18)
                    continue;
                dir.normalize();
                Matrix4x4 fromDirection = buildBoneWorldTransform(start, start + dir);
                const double* a = world.constData();
                const double* b = fromDirection.constData();
                bool builtFromDirection = true;
                for (int k = 0; k < 16 && builtFromDirection; ++k)
                    builtFromDirection = std::abs(a[k] - b[k]) < 1e-6;
                if (!builtFromDirection)
                    continue;
                Matrix4x4 referenced;
                referenced.translate(start);
                referenced.rotate(Quaternion::rotationTo(restDir, dir));
                referenced.translate(Vector3() - restStart);
                referenced *= restWorld;
                world = referenced;
                if (inv != inverseBindMatrices.end()) {
                    Matrix4x4 skin = world;
                    skin *= inv->second;
                    frame.boneSkinMatrices[bone.name] = skin;
                }
            }
        }
    }

    // A bone moved so it runs from newStart to newEnd, turned from its rest direction by the
    // smallest rotation: it keeps its rest twist, unlike buildBoneWorldTransform, whose twist
    // is arbitrary (and flips for bones pointing backwards). Further rolls about the bone's own
    // axis can then be multiplied on (its local Z).
    inline Matrix4x4 boneFromRest(const Matrix4x4& restWorld, const Vector3& restStart, const Vector3& restEnd,
        const Vector3& newStart, const Vector3& newEnd)
    {
        Matrix4x4 m;
        m.translate(newStart);
        Vector3 from = restEnd - restStart;
        Vector3 to = newEnd - newStart;
        if (from.lengthSquared() > 1e-18 && to.lengthSquared() > 1e-18)
            m.rotate(Quaternion::rotationTo(from.normalized(), to.normalized()));
        m.translate(Vector3() - restStart);
        m *= restWorld;
        return m;
    }

    // Preserve a limb's hinge plane as well as its direction. A smallest rotation
    // alone has no defined twist at a fully folded (antipodal) joint.
    inline Matrix4x4 boneFromRestWithPole(const Matrix4x4& restWorld, const Vector3& restStart,
        const Vector3& restEnd, const Vector3& newStart, const Vector3& newEnd,
        const Vector3& restPole, const Vector3& newPole)
    {
        Vector3 from = (restEnd - restStart).normalized(), to = (newEnd - newStart).normalized();
        Vector3 x0 = restPole - from * Vector3::dotProduct(restPole, from);
        Vector3 x1 = newPole - to * Vector3::dotProduct(newPole, to);
        if (x0.lengthSquared() < 1e-12 || x1.lengthSquared() < 1e-12)
            return boneFromRest(restWorld, restStart, restEnd, newStart, newEnd);
        x0.normalize();
        x1.normalize();
        Vector3 source[3] = { x0, Vector3::crossProduct(from, x0), from };
        Vector3 target[3] = { x1, Vector3::crossProduct(to, x1), to };
        Matrix4x4 rotation;
        for (int row = 0; row < 3; ++row)
            for (int col = 0; col < 3; ++col) {
                double value = 0;
                for (int axis = 0; axis < 3; ++axis)
                    value += target[axis].constData()[row] * source[axis].constData()[col];
                rotation.data()[col * 4 + row] = value;
            }
        Matrix4x4 result;
        result.translate(newStart);
        result *= rotation;
        result.translate(Vector3() - restStart);
        result *= restWorld;
        return result;
    }

    // Rest pose world transforms for every bone.
    inline std::map<std::string, Matrix4x4> restBoneWorldTransforms(const RigStructure& rigStructure)
    {
        std::map<std::string, Matrix4x4> transforms;
        for (const auto& bone : rigStructure.bones) {
            transforms[bone.name] = buildBoneWorldTransform(
                Vector3(bone.posX, bone.posY, bone.posZ), Vector3(bone.endX, bone.endY, bone.endZ));
        }
        return transforms;
    }

    // Preserve the local rest transform of bones the clip does not explicitly animate.
    // Resolve parents first, independent of the order of bones in the rig.
    inline void inheritUndrivenBones(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices, BoneAnimationFrame& frame)
    {
        const auto rest = restBoneWorldTransforms(rigStructure);
        const auto index = buildBoneIndexMap(rigStructure);
        std::set<std::string> visiting;
        std::function<void(const RigNode&)> inherit = [&](const RigNode& bone) {
            if (frame.boneWorldTransforms.count(bone.name) || !visiting.insert(bone.name).second)
                return;
            Matrix4x4 world = rest.at(bone.name);
            auto parent = index.find(bone.parent);
            if (parent != index.end()) {
                inherit(rigStructure.bones[parent->second]);
                auto posedParent = frame.boneWorldTransforms.find(bone.parent);
                if (posedParent != frame.boneWorldTransforms.end()) {
                    world = posedParent->second;
                    world *= rest.at(bone.parent).inverted();
                    world *= rest.at(bone.name);
                }
            }
            frame.boneWorldTransforms[bone.name] = world;
            visiting.erase(bone.name);
        };
        for (const auto& bone : rigStructure.bones)
            inherit(bone);
        finishFrame(frame, inverseBindMatrices);
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
    // CAPE GRID SIMULATOR
    // =========================================================================
    //
    // Simulates a cape as a grid of bone chains with horizontal coupling.
    // The grid has 3 columns (Left, Center, Right) each with 3 bones,
    // anchored to the Chest bone. Vertical chains use the same Verlet
    // integration as HairChainSimulator; horizontal distance constraints
    // couple adjacent columns to maintain surface coherence.
    //
    // Bone names: LeftCape1..3, CenterCape1..3, RightCape1..3
    //
    // Usage:
    //   CapeGridSimulator capeSim;
    //   capeSim.initialize(rig, boneIdx,
    //       buildBoneWorldTransform(bonePos("Chest"), boneEnd("Chest")),
    //       stiffness, damping, gravityScale, spreadStiffness);
    //
    //   // Inside frame loop, after Chest world transform is known:
    //   if (capeSim.active)
    //       capeSim.step(boneWorldTransforms["Chest"], dt, boneWorldTransforms);
    struct CapeGridSimulator {
        struct BoneData {
            std::string name;
            Vector3 bindRoot;
            Vector3 bindTip;
            double boneLength;
        };

        static constexpr int kColumns = 3;
        static constexpr int kRows = 3;

        Matrix4x4 parentBindInverse;
        BoneData bones[kColumns][kRows];
        Vector3 tipPos[kColumns][kRows];
        Vector3 prevTipPos[kColumns][kRows];
        double horizontalRestDist[kRows]; // rest distance between adjacent columns at each row
        double stiffness = 0.08;
        double damping = 0.85;
        double gravityScale = 1.2;
        double spreadStiffness = 0.15;
        bool active = false;
        int activeCols = 0;
        int activeRows[kColumns] = {};

        void initialize(const RigStructure& rig,
            const std::map<std::string, size_t>& idx,
            const Matrix4x4& parentBindTransform,
            double stiff = 0.08,
            double damp = 0.85,
            double grav = 1.2,
            double spread = 0.15)
        {
            stiffness = stiff;
            damping = damp;
            gravityScale = grav;
            spreadStiffness = spread;
            parentBindInverse = parentBindTransform.inverted();

            static const char* colNames[kColumns] = { "LeftCape", "CenterCape", "RightCape" };

            activeCols = 0;
            for (int c = 0; c < kColumns; ++c) {
                activeRows[c] = 0;
                for (int r = 0; r < kRows; ++r) {
                    std::string name = std::string(colNames[c]) + std::to_string(r + 1);
                    auto it = idx.find(name);
                    if (it == idx.end())
                        break;
                    const auto& b = rig.bones[it->second];
                    bones[c][r].name = name;
                    bones[c][r].bindRoot = Vector3(b.posX, b.posY, b.posZ);
                    bones[c][r].bindTip = Vector3(b.endX, b.endY, b.endZ);
                    bones[c][r].boneLength = (bones[c][r].bindTip - bones[c][r].bindRoot).length();
                    if (bones[c][r].boneLength < 1e-8)
                        bones[c][r].boneLength = 1e-8;
                    tipPos[c][r] = bones[c][r].bindTip;
                    prevTipPos[c][r] = bones[c][r].bindTip;
                    activeRows[c] = r + 1;
                }
                if (activeRows[c] > 0)
                    ++activeCols;
            }

            // Compute horizontal rest distances between adjacent columns
            for (int r = 0; r < kRows; ++r) {
                horizontalRestDist[r] = 0.0;
                int pairs = 0;
                for (int c = 0; c < kColumns - 1; ++c) {
                    if (r < activeRows[c] && r < activeRows[c + 1]) {
                        horizontalRestDist[r] += (bones[c][r].bindTip - bones[c + 1][r].bindTip).length();
                        ++pairs;
                    }
                }
                if (pairs > 0)
                    horizontalRestDist[r] /= pairs;
            }

            active = (activeCols >= 1);
        }

        void step(const Matrix4x4& parentWorldTransform,
            double dt,
            std::map<std::string, Matrix4x4>& out)
        {
            if (!active)
                return;

            Matrix4x4 delta = parentWorldTransform;
            delta *= parentBindInverse;

            const Vector3 gravityAccel(0.0, -9.8 * gravityScale, 0.0);
            const double dt2 = dt * dt;

            // Verlet integration for each column (same as hair chain)
            for (int c = 0; c < kColumns; ++c) {
                for (int r = 0; r < activeRows[c]; ++r) {
                    Vector3 root = (r == 0)
                        ? delta.transformPoint(bones[c][0].bindRoot)
                        : tipPos[c][r - 1];

                    Vector3 goalTipWorld = delta.transformPoint(bones[c][r].bindTip);
                    Vector3 goalRootWorld = delta.transformPoint(bones[c][r].bindRoot);
                    Vector3 goalDir = goalTipWorld - goalRootWorld;
                    double goalDirLen = goalDir.length();
                    if (goalDirLen > 1e-8)
                        goalDir = goalDir * (1.0 / goalDirLen);
                    else
                        goalDir = Vector3(0.0, -1.0, 0.0);
                    Vector3 goalTip = root + goalDir * bones[c][r].boneLength;

                    Vector3 vel = (tipPos[c][r] - prevTipPos[c][r]) * damping;
                    Vector3 newTip = tipPos[c][r] + vel + gravityAccel * dt2;
                    newTip = newTip + (goalTip - newTip) * stiffness;

                    Vector3 toNew = newTip - root;
                    double toNewLen = toNew.length();
                    if (toNewLen > 1e-8)
                        newTip = root + toNew * (bones[c][r].boneLength / toNewLen);
                    else
                        newTip = root + Vector3(0.0, -bones[c][r].boneLength, 0.0);

                    prevTipPos[c][r] = tipPos[c][r];
                    tipPos[c][r] = newTip;
                }
            }

            // Horizontal distance constraints between adjacent columns
            static const int kConstraintIters = 3;
            for (int iter = 0; iter < kConstraintIters; ++iter) {
                for (int r = 0; r < kRows; ++r) {
                    for (int c = 0; c < kColumns - 1; ++c) {
                        if (r >= activeRows[c] || r >= activeRows[c + 1])
                            continue;
                        Vector3 diff = tipPos[c + 1][r] - tipPos[c][r];
                        double dist = diff.length();
                        if (dist < 1e-8)
                            continue;
                        double restDist = horizontalRestDist[r];
                        double correction = (dist - restDist) / dist * spreadStiffness;
                        Vector3 offset = diff * (correction * 0.5);
                        tipPos[c][r] = tipPos[c][r] + offset;
                        tipPos[c + 1][r] = tipPos[c + 1][r] - offset;
                    }
                }

                // Re-enforce length constraints after horizontal correction
                for (int c = 0; c < kColumns; ++c) {
                    for (int r = 0; r < activeRows[c]; ++r) {
                        Vector3 root = (r == 0)
                            ? delta.transformPoint(bones[c][0].bindRoot)
                            : tipPos[c][r - 1];
                        Vector3 toTip = tipPos[c][r] - root;
                        double len = toTip.length();
                        if (len > 1e-8)
                            tipPos[c][r] = root + toTip * (bones[c][r].boneLength / len);
                        else
                            tipPos[c][r] = root + Vector3(0.0, -bones[c][r].boneLength, 0.0);
                    }
                }
            }

            // Write output transforms
            for (int c = 0; c < kColumns; ++c) {
                for (int r = 0; r < activeRows[c]; ++r) {
                    Vector3 root = (r == 0)
                        ? delta.transformPoint(bones[c][0].bindRoot)
                        : tipPos[c][r - 1];
                    out[bones[c][r].name] = buildBoneWorldTransform(root, tipPos[c][r]);
                }
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
