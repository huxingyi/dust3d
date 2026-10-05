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

// Procedural hopping gait for the biped rig: kangaroos, wallabies, hares,
// jerboas and other two-legged hoppers.
//
// A looping locomotion cycle (the game moves the character; the animation stays
// in place). Both feet push off together, the body flies in a ballistic arc
// with the legs swinging forward under it, lands and compresses, and the next
// hop starts. The tail swings up in the air and down on the ground as a
// counterweight, the head stays level, and the small arms stay tucked.
//
// Adjustable animation parameters:
//   - hopHeightFactor:   height of the hop
//   - strideFactor:      how far the feet swing forward and back
//   - crouchDepthFactor: compression on landing and push-off
//   - groundTimeFactor:  share of the cycle with the feet on the ground
//   - leanForwardFactor: how far the body tips forward
//   - tailSwingFactor:   tail counterbalance swing
//   - armTuckFactor:     how much the arms pull in
//   - hopsPerCycle:      hops in one clip (1 or more)

#include <algorithm>
#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/biped/hop.h>
#include <dust3d/animation/common.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace biped {

    bool hop(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = static_cast<int>(parameters.getValue("frameCount", 20));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 0.6));

        auto boneIdx = buildBoneIndexMap(rigStructure);
        auto bonePos = [&](const std::string& name) -> Vector3 {
            return getBonePos(rigStructure, boneIdx, name);
        };
        auto boneEnd = [&](const std::string& name) -> Vector3 {
            return getBoneEnd(rigStructure, boneIdx, name);
        };

        static const char* requiredBones[] = {
            "Root", "Hips", "Spine", "Chest", "Neck", "Head",
            "LeftUpperLeg", "LeftLowerLeg", "LeftFoot",
            "RightUpperLeg", "RightLowerLeg", "RightFoot"
        };
        if (!validateRequiredBones(boneIdx, requiredBones, sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;

        Vector3 upDir(0.0, 1.0, 0.0);
        Vector3 hipsPos = bonePos("Hips");
        Vector3 avgFootEnd = (boneEnd("LeftFoot") + boneEnd("RightFoot")) * 0.5;
        Vector3 forward(avgFootEnd.x() - hipsPos.x(), 0.0, avgFootEnd.z() - hipsPos.z());
        if (forward.lengthSquared() < 1e-8)
            forward = Vector3(0.0, 0.0, 1.0);
        forward.normalize();
        Vector3 right = Vector3::crossProduct(upDir, forward).normalized();

        struct LegRest {
            const char* upper;
            const char* lower;
            const char* foot;
            Vector3 upperPos, upperEnd, lowerEnd, footEnd, restStickDir, restUpperToLower;
            double length;
        };
        auto makeLeg = [&](const char* upper, const char* lower, const char* foot) {
            LegRest r {};
            r.upper = upper;
            r.lower = lower;
            r.foot = foot;
            r.upperPos = bonePos(upper);
            r.upperEnd = boneEnd(upper);
            r.lowerEnd = boneEnd(lower);
            r.footEnd = boneEnd(foot);
            Vector3 chord = r.footEnd - r.upperEnd;
            r.restStickDir = chord.isZero() ? Vector3(0, -1, 0) : chord.normalized();
            r.restUpperToLower = r.lowerEnd - r.upperEnd;
            r.length = (r.upperEnd - r.upperPos).length() + (r.lowerEnd - r.upperEnd).length() + (r.footEnd - r.lowerEnd).length();
            return r;
        };
        LegRest legs[2] = { makeLeg("LeftUpperLeg", "LeftLowerLeg", "LeftFoot"),
            makeLeg("RightUpperLeg", "RightLowerLeg", "RightFoot") };
        double legLength = std::max(1e-3, (legs[0].length + legs[1].length) * 0.5);

        double hopHeight = legLength * 0.35 * parameters.getValue("hopHeightFactor", 1.0);
        double stride = legLength * 0.55 * parameters.getValue("strideFactor", 1.0);
        double crouch = legLength * 0.14 * parameters.getValue("crouchDepthFactor", 1.0);
        double groundShare = std::clamp(0.34 * parameters.getValue("groundTimeFactor", 1.0), 0.15, 0.7);
        double lean = 0.12 * parameters.getValue("leanForwardFactor", 1.0);
        double tailSwing = 0.35 * parameters.getValue("tailSwingFactor", 1.0);
        double armTuck = 0.5 * parameters.getValue("armTuckFactor", 1.0);
        int hops = std::max(1, static_cast<int>(std::round(parameters.getValue("hopsPerCycle", 1.0))));

        double groundLevel = std::min(legs[0].footEnd.y(), legs[1].footEnd.y());

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);
        animationClip.movementSpeed = static_cast<float>(stride * hops / (groundShare * durationSeconds));
        animationClip.movementDirectionX = static_cast<float>(forward.x());
        animationClip.movementDirectionZ = static_cast<float>(forward.z());

        for (int frame = 0; frame < frameCount; ++frame) {
            double tClip = static_cast<double>(frame) / static_cast<double>(frameCount);
            double p = std::fmod(tClip * hops, 1.0);

            // Ground phase [0, groundShare): compress then extend. Air phase: a parabola.
            bool onGround = p < groundShare;
            double g = onGround ? p / groundShare : 0.0;
            double a = onGround ? 0.0 : (p - groundShare) / (1.0 - groundShare);
            double bodyVertical = onGround ? -crouch * std::sin(Math::Pi * g) : hopHeight * 4.0 * a * (1.0 - a);
            // Air: 0..1 (1 at the apex); ground: -1..0 (-1 at the deepest crouch).
            double airness = onGround ? -std::sin(Math::Pi * g) : 4.0 * a * (1.0 - a);
            // Pitch: nose up on push-off, nose down coming in to land.
            double pitchSwing = onGround ? 0.06 * std::sin(Math::Pi * g) : -0.1 * std::sin(2.0 * Math::Pi * a);
            double bodyPitch = lean + pitchSwing;

            Matrix4x4 bodyTransform;
            bodyTransform.translate(upDir * bodyVertical);
            Vector3 pivot = hipsPos;
            bodyTransform.translate(pivot);
            bodyTransform.rotate(right, bodyPitch);
            bodyTransform.translate(pivot * -1.0);

            std::map<std::string, Matrix4x4> boneWorldTransforms;
            auto bodyBone = [&](const std::string& name, double extraPitch) {
                if (!boneIdx.count(name))
                    return;
                Vector3 s = bodyTransform.transformPoint(bonePos(name));
                Vector3 e = bodyTransform.transformPoint(boneEnd(name));
                if (std::abs(extraPitch) > 1e-6) {
                    Matrix4x4 r;
                    r.rotate(right, extraPitch);
                    e = s + r.transformVector(e - s);
                }
                boneWorldTransforms[name] = buildBoneWorldTransform(s, e);
            };
            bodyBone("Root", 0.0);
            bodyBone("Hips", 0.0);
            bodyBone("Spine", 0.0);
            bodyBone("Chest", 0.0);
            // Keep the head level: counter the body pitch.
            bodyBone("Neck", -bodyPitch * 0.5);
            bodyBone("Head", -bodyPitch * 0.8);

            // Tail: a chain, swinging up in the air and down on the ground.
            static const char* tailBones[] = { "TailBase", "TailMid", "TailTip" };
            Vector3 prevEnd;
            bool hasPrev = false;
            double accumulated = 0.0;
            for (int i = 0; i < 3; ++i) {
                if (!boneIdx.count(tailBones[i]))
                    continue;
                Vector3 s = bodyTransform.transformPoint(bonePos(tailBones[i]));
                Vector3 e = bodyTransform.transformPoint(boneEnd(tailBones[i]));
                Vector3 dir = e - s;
                if (hasPrev)
                    s = prevEnd;
                accumulated += tailSwing * airness * (0.5 + 0.25 * i) - bodyPitch * (i == 0 ? 0.6 : 0.0);
                Matrix4x4 r;
                r.rotate(right, accumulated);
                e = s + r.transformVector(dir);
                boneWorldTransforms[tailBones[i]] = buildBoneWorldTransform(s, e);
                prevEnd = e;
                hasPrev = true;
            }

            // Legs: planted and sliding back on the ground, swinging forward in the air.
            for (const auto& leg : legs) {
                Vector3 home = leg.footEnd;
                Vector3 onGroundHome = home - upDir * (home.y() - groundLevel);
                Vector3 target;
                if (onGround) {
                    target = onGroundHome + forward * (stride * (0.5 - g));
                } else {
                    double tuck = legLength * 0.22 * std::sin(Math::Pi * a);
                    // Hermite swing: match the planted foot velocity at both contacts.
                    double m = -stride * (1.0 - groundShare) / groundShare;
                    double swing = -0.5 * stride + stride * smoothstep(a)
                        + m * (a - 3.0 * a * a + 2.0 * a * a * a);
                    target = onGroundHome + upDir * (bodyVertical + tuck) + forward * swing;
                }
                Vector3 hip = bodyTransform.transformPoint(leg.upperPos);
                Vector3 knee = bodyTransform.transformPoint(leg.upperEnd);
                Vector3 foot = bodyTransform.transformPoint(leg.footEnd);
                std::vector<Vector3> joints = { hip, knee, foot };
                solveTwoBoneIk(joints, target, knee + forward * 0.5);
                Vector3 stick = joints[2] - joints[1];
                Vector3 stickDir = stick.isZero() ? leg.restStickDir : stick.normalized();
                Matrix4x4 stickRot;
                stickRot.rotate(Quaternion::rotationTo(leg.restStickDir, stickDir));
                Vector3 ankle = joints[1] + stickRot.transformVector(leg.restUpperToLower);
                boneWorldTransforms[leg.upper] = buildBoneWorldTransform(joints[0], joints[1]);
                boneWorldTransforms[leg.lower] = buildBoneWorldTransform(joints[1], ankle);
                boneWorldTransforms[leg.foot] = buildBoneWorldTransform(ankle, joints[2]);
            }

            // Arms: tucked in to the chest, bobbing a little with the hop.
            static const char* arms[2][4] = {
                { "LeftShoulder", "LeftUpperArm", "LeftLowerArm", "LeftHand" },
                { "RightShoulder", "RightUpperArm", "RightLowerArm", "RightHand" }
            };
            for (const auto& arm : arms) {
                if (!boneIdx.count(arm[0]) || !boneIdx.count(arm[1]))
                    continue;
                bodyBone(arm[0], 0.0);
                Vector3 shoulderEnd = bodyTransform.transformPoint(boneEnd(arm[0]));
                Vector3 prev = shoulderEnd;
                double bend = armTuck * (1.0 + 0.3 * airness);
                for (int i = 1; i < 4; ++i) {
                    if (!boneIdx.count(arm[i]))
                        continue;
                    Vector3 dir = bodyTransform.transformVector(boneEnd(arm[i]) - bonePos(arm[i]));
                    Matrix4x4 r;
                    r.rotate(right, -bend * (i == 1 ? 0.6 : 1.0));
                    Vector3 e = prev + r.transformVector(dir);
                    boneWorldTransforms[arm[i]] = buildBoneWorldTransform(prev, e);
                    prev = e;
                }
            }

            auto& animFrame = animationClip.frames[frame];
            animFrame.time = static_cast<float>(tClip) * durationSeconds;
            animFrame.boneWorldTransforms = boneWorldTransforms;
            for (const auto& pair : boneWorldTransforms) {
                auto invIt = inverseBindMatrices.find(pair.first);
                if (invIt != inverseBindMatrices.end()) {
                    Matrix4x4 skinMat = pair.second;
                    skinMat *= invIt->second;
                    animFrame.boneSkinMatrices[pair.first] = skinMat;
                }
            }
        }
        return true;
    }

} // namespace biped

} // namespace dust3d
