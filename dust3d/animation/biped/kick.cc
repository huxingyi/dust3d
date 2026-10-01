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

// Procedural kick animation for the biped rig.
//
// A one-shot melee attack for game use. The kicking knee comes up (the
// chamber) while the body leans back to balance, the leg snaps out forward
// with the foot flexed at the target, pulls back into the chamber and steps
// down where it started. The standing foot stays planted, the arms swing out
// for balance and the head keeps looking at the target.
//
// With bothLegs = 1 it is the kangaroo's kick: the body rocks back onto the
// tail (the tail stays on the ground) and both feet kick out together. Rigs
// without a tail rock back about the heels instead.
//
// The clip starts and ends exactly in the rest pose, so it blends with the
// idle and walk loops.
//
// Adjustable animation parameters:
//   - kickHeightFactor:    how high the kick lands (1 = about hip height)
//   - kickReachFactor:     how far forward the foot reaches
//   - chamberFactor:       how high the knee comes up before the kick
//   - leanBackFactor:      how far the body leans back to balance
//   - armBalanceFactor:    how far the arms swing out for balance
//   - bothLegs:            0 = one leg, 1 = both legs (kangaroo, balanced on the tail)
//   - kickLeg:             0 = right leg, 1 = left leg (one-leg kick)
//   - strikeTimingFactor:  when the kick lands (1 = 40% into the clip)

#include <algorithm>
#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/biped/kick.h>
#include <dust3d/animation/common.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace biped {

    bool kick(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = std::max(2, static_cast<int>(parameters.getValue("frameCount", 24)));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 0.8));

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

        double kickHeightFactor = parameters.getValue("kickHeightFactor", 1.0);
        double kickReachFactor = parameters.getValue("kickReachFactor", 1.0);
        double chamberFactor = parameters.getValue("chamberFactor", 1.0);
        double leanBackFactor = parameters.getValue("leanBackFactor", 1.0);
        double armBalanceFactor = parameters.getValue("armBalanceFactor", 1.0);
        bool bothLegs = parameters.getValue("bothLegs", 0.0) > 0.5;
        bool leftLeg = parameters.getValue("kickLeg", 0.0) > 0.5;
        double tStrike = std::clamp(0.4 * parameters.getValue("strikeTimingFactor", 1.0), 0.25, 0.6);

        Vector3 up(0.0, 1.0, 0.0);
        Vector3 toes = (boneEnd("LeftFoot") + boneEnd("RightFoot")) * 0.5 - (boneEnd("LeftLowerLeg") + boneEnd("RightLowerLeg")) * 0.5;
        Vector3 forward(toes.x(), 0.0, toes.z());
        if (forward.lengthSquared() < 1e-10)
            forward = Vector3(0.0, 0.0, 1.0);
        forward.normalize();
        // The creature's right: a positive rotation about it lifts the front (leans back).
        Vector3 right = Vector3::crossProduct(forward, up).normalized();

        double legLength = (boneEnd("RightUpperLeg") - bonePos("RightUpperLeg")).length()
            + (boneEnd("RightLowerLeg") - bonePos("RightLowerLeg")).length();
        if (legLength < 1e-6)
            return false;

        std::string tailEndName;
        for (const char* name : { "TailTip", "TailMid", "TailBase" }) {
            if (boneIdx.count(name)) {
                tailEndName = name;
                break;
            }
        }
        double groundY = std::min(boneEnd("LeftLowerLeg").y(), boneEnd("RightLowerLeg").y());
        Vector3 heels = (boneEnd("LeftLowerLeg") + boneEnd("RightLowerLeg")) * 0.5;
        Vector3 pivot = heels;
        if (bothLegs && !tailEndName.empty()) {
            // Balanced on the tail: the body rocks back about where the tail meets the ground.
            pivot = boneEnd(tailEndName);
            pivot.setY(groundY);
        }

        double lean = (bothLegs ? 0.55 : 0.2) * leanBackFactor;
        double lift = bothLegs ? legLength * 0.25 : 0.0;

        struct Leg {
            const char* upper;
            const char* lower;
            const char* foot;
            bool kicks;
        };
        Leg legs[2] = {
            { "LeftUpperLeg", "LeftLowerLeg", "LeftFoot", bothLegs || leftLeg },
            { "RightUpperLeg", "RightLowerLeg", "RightFoot", bothLegs || !leftLeg },
        };

        std::map<std::string, Matrix4x4> rest = restBoneWorldTransforms(rigStructure);

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            double t = static_cast<double>(frame) / static_cast<double>(frameCount - 1);

            double chamberStart = std::max(0.08, tStrike - 0.14);
            double chamber = smoothstep(t / chamberStart) * (1.0 - smoothstep((t - (tStrike + 0.26)) / std::max(0.1, 0.95 - (tStrike + 0.26))));
            double extend = smoothstep((t - chamberStart) / (tStrike - chamberStart)) * (1.0 - smoothstep((t - (tStrike + 0.08)) / 0.16));
            double leanEnv = smoothstep(t / tStrike) * (1.0 - smoothstep((t - (tStrike + 0.2)) / std::max(0.1, 0.95 - (tStrike + 0.2))));

            Matrix4x4 body;
            body.translate(up * (lift * leanEnv));
            body *= rotationAbout(pivot, right, lean * leanEnv);

            std::map<std::string, Matrix4x4> world = rest;
            std::map<std::string, Matrix4x4> layers;
            auto apply = [&](const std::string& name, const Matrix4x4& layer) {
                if (!boneIdx.count(name))
                    return;
                layers[name] = layer;
                Matrix4x4 m = layer;
                m *= rest[name];
                world[name] = m;
            };

            for (const char* name : { "Hips", "Spine", "Chest" })
                apply(name, body);
            // The head keeps looking at the target: it counter-rotates most of the lean.
            Vector3 neckJoint = body.transformPoint(bonePos("Neck"));
            Matrix4x4 neck = rotationAbout(neckJoint, right, -0.6 * lean * leanEnv);
            neck *= body;
            apply("Neck", neck);
            apply("Head", neck);
            for (const auto& bone : rigStructure.bones) {
                if (bone.parent == "Head" && !layers.count(bone.name))
                    apply(bone.name, neck);
            }
            for (const char* name : { "TailBase", "TailMid", "TailTip" })
                apply(name, body);

            // Arms: out and forward for balance (directions, so A-pose and T-pose rigs both work).
            for (const char* side : { "Left", "Right" }) {
                std::string shoulder = std::string(side) + "Shoulder";
                std::string upper = std::string(side) + "UpperArm";
                std::string lower = std::string(side) + "LowerArm";
                std::string hand = std::string(side) + "Hand";
                if (!boneIdx.count(upper))
                    continue;
                apply(shoulder, body);
                Vector3 restDir = (boneEnd(upper) - bonePos(upper)).normalized();
                Vector3 outward = right * (Vector3::dotProduct(boneEnd(upper) - bonePos("Chest"), right) >= 0.0 ? 1.0 : -1.0);
                Vector3 balanceDir = (forward * (bothLegs ? 0.7 : 0.35) - up * 0.55 + outward * (bothLegs ? 0.15 : 0.6)).normalized();
                double w = std::min(1.0, armBalanceFactor * leanEnv);
                Vector3 bodyRestDir = body.transformVector(restDir);
                double c = std::clamp(Vector3::dotProduct(bodyRestDir, balanceDir), -1.0, 1.0);
                Vector3 axis = Vector3::crossProduct(bodyRestDir, balanceDir);
                Matrix4x4 armLayer;
                if (axis.lengthSquared() > 1e-12) {
                    Vector3 joint = body.transformPoint(bonePos(upper));
                    armLayer = rotationAbout(joint, axis.normalized(), std::acos(c) * w);
                }
                armLayer *= body;
                apply(upper, armLayer);
                // The forearm bends a little into a guard.
                Vector3 elbow = armLayer.transformPoint(bonePos(lower));
                Vector3 elbowAxis = Vector3::crossProduct(armLayer.transformVector(restDir), up);
                Matrix4x4 forearm;
                if (elbowAxis.lengthSquared() > 1e-12)
                    forearm = rotationAbout(elbow, elbowAxis.normalized(), 0.6 * w);
                forearm *= armLayer;
                apply(lower, forearm);
                apply(hand, forearm);
            }

            for (const auto& leg : legs) {
                if (!leg.kicks) {
                    posePlantedLeg(rigStructure, boneIdx, leg.upper, leg.lower, leg.foot, body, Vector3(), world);
                    continue;
                }
                Vector3 hip = body.transformPoint(bonePos(leg.upper));
                Vector3 ankleRest = boneEnd(leg.lower);
                // Chamber: knee up, foot tucked under it. Strike: foot out forward at hip height.
                Vector3 chamberAt = hip + forward * (legLength * 0.3 * chamberFactor) - up * (legLength * (0.75 - 0.3 * chamberFactor));
                Vector3 strikeAt = hip + forward * (legLength * 0.95 * kickReachFactor) + up * (legLength * 0.3 * (kickHeightFactor - 1.0));
                Vector3 target = ankleRest + (chamberAt - ankleRest) * chamber;
                target = target + (strikeAt - target) * extend;
                // The knee points forward and up while chambered; the foot keeps its angle to the
                // shin, so a flexed foot hits with the sole and toes.
                Vector3 restBend = body.transformVector(bonePos(leg.lower) - (bonePos(leg.upper) + ankleRest) * 0.5);
                restBend = restBend.lengthSquared() > 1e-12 ? restBend.normalized() : forward;
                Vector3 bend = restBend * (1.0 - chamber) + (forward + up * 0.5).normalized() * chamber;
                poseTwoBoneLeg(rigStructure, boneIdx, leg.upper, leg.lower, leg.foot, body, target, bend, true, world);
            }

            auto& animFrame = animationClip.frames[frame];
            animFrame.time = static_cast<float>(frame) / static_cast<float>(frameCount) * durationSeconds;
            animFrame.boneWorldTransforms = world;
            finishFrame(animFrame, inverseBindMatrices);
        }
        return true;
    }

} // namespace biped

} // namespace dust3d
