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

// Procedural ground attack for the bird rig: a peck and/or a kick.
//
// A one-shot attack for birds on the ground (emus, cassowaries, ostriches,
// chickens). BirdAttack is the flying dive; this is what a walking bird does.
// The bird rears up with its neck cocked back and its wings flared, then
// lunges: the neck shoots forward and down (the peck) and, with kickFactor,
// one leg snaps out forward with the claws first (the kick). The standing
// foot stays planted, and everything returns to the rest pose, so the clip
// blends with the idle and walk loops.
//
// Adjustable animation parameters:
//   - peckFactor:           how far the neck and head strike forward and down
//   - kickFactor:           0 = no kick, 1 = a full forward kick
//   - lungeDistanceFactor:  how far the body lunges forward
//   - rearUpFactor:         how far the body rears up in the windup
//   - wingFlareFactor:      how far the wings flare out
//   - kickLeg:              0 = right leg, 1 = left leg
//   - strikeTimingFactor:   when the strike lands (1 = 40% into the clip)

#include <algorithm>
#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/bird/strike.h>
#include <dust3d/animation/common.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace bird {

    bool strike(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = std::max(2, static_cast<int>(parameters.getValue("frameCount", 27)));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 0.9));

        auto boneIdx = buildBoneIndexMap(rigStructure);
        auto bonePos = [&](const std::string& name) -> Vector3 {
            return getBonePos(rigStructure, boneIdx, name);
        };
        auto boneEnd = [&](const std::string& name) -> Vector3 {
            return getBoneEnd(rigStructure, boneIdx, name);
        };

        static const char* requiredBones[] = {
            "Root", "Pelvis", "Spine", "Chest", "Neck", "Head",
            "LeftWingShoulder", "LeftWingElbow", "LeftWingHand",
            "RightWingShoulder", "RightWingElbow", "RightWingHand",
            "LeftUpperLeg", "LeftLowerLeg", "LeftFoot",
            "RightUpperLeg", "RightLowerLeg", "RightFoot"
        };
        if (!validateRequiredBones(boneIdx, requiredBones, sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;

        double peckFactor = parameters.getValue("peckFactor", 1.0);
        double kickFactor = std::clamp(parameters.getValue("kickFactor", 0.0), 0.0, 2.0);
        double lungeDistanceFactor = parameters.getValue("lungeDistanceFactor", 1.0);
        double rearUpFactor = parameters.getValue("rearUpFactor", 1.0);
        double wingFlareFactor = parameters.getValue("wingFlareFactor", 1.0);
        bool leftLeg = parameters.getValue("kickLeg", 0.0) > 0.5;
        double tStrike = std::clamp(0.4 * parameters.getValue("strikeTimingFactor", 1.0), 0.25, 0.65);
        // The peck and the kick land together.
        if (peckFactor > 0.0 || kickFactor <= 0.0)
            animationClip.events.push_back({ "hit", static_cast<float>(tStrike * durationSeconds), "Beak" });
        if (kickFactor > 0.0)
            animationClip.events.push_back({ "hit", static_cast<float>(tStrike * durationSeconds), leftLeg ? "LeftFoot" : "RightFoot" });

        Vector3 bodyVector = boneEnd("Chest") - bonePos("Pelvis");
        bodyVector.setY(0.0);
        if (bodyVector.lengthSquared() < 1e-10)
            return false;
        Vector3 forward = bodyVector.normalized();
        Vector3 up(0.0, 1.0, 0.0);
        // The creature's right: a positive rotation about it lifts the front.
        Vector3 right = Vector3::crossProduct(forward, up).normalized();
        double bodyLength = (boneEnd("Chest") - bonePos("Pelvis")).length();
        double legLength = (boneEnd("RightUpperLeg") - bonePos("RightUpperLeg")).length()
            + (boneEnd("RightLowerLeg") - bonePos("RightLowerLeg")).length();
        if (legLength < 1e-6 || bodyLength < 1e-6)
            return false;

        const char* kickUpper = leftLeg ? "LeftUpperLeg" : "RightUpperLeg";
        const char* kickLower = leftLeg ? "LeftLowerLeg" : "RightLowerLeg";
        const char* kickFoot = leftLeg ? "LeftFoot" : "RightFoot";
        const char* standUpper = leftLeg ? "RightUpperLeg" : "LeftUpperLeg";
        const char* standLower = leftLeg ? "RightLowerLeg" : "LeftLowerLeg";
        const char* standFoot = leftLeg ? "RightFoot" : "LeftFoot";

        // The body rocks about the standing foot when kicking, else between the feet.
        Vector3 pivot = kickFactor > 0.0 ? boneEnd(standLower)
                                         : (boneEnd("LeftLowerLeg") + boneEnd("RightLowerLeg")) * 0.5;

        double rearAngle = 0.25 * rearUpFactor;
        double lungeAngle = 0.3 * peckFactor;
        double lunge = bodyLength * 0.35 * lungeDistanceFactor;
        double neckBack = 0.45 * peckFactor;
        double neckStrike = 0.8 * peckFactor;
        double headStrike = 0.35 * peckFactor;
        double wingFlare = 0.8 * wingFlareFactor;

        std::map<std::string, Matrix4x4> rest = restBoneWorldTransforms(rigStructure);

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            double t = static_cast<double>(frame) / static_cast<double>(frameCount - 1);
            double windupEnd = tStrike - 0.12;
            double windup = smoothstep(t / windupEnd) * (1.0 - smoothstep((t - windupEnd) / (tStrike - windupEnd)));
            double hit = smoothstep((t - windupEnd) / (tStrike - windupEnd))
                * (1.0 - smoothstep((t - (tStrike + 0.1)) / std::max(0.1, 0.95 - (tStrike + 0.1))));
            double chamber = smoothstep(t / windupEnd) * (1.0 - smoothstep((t - (tStrike + 0.2)) / std::max(0.1, 0.95 - (tStrike + 0.2))));
            double extendX = (t - windupEnd) / (tStrike + 0.16 - windupEnd);
            double extend = (extendX > 0.0 && extendX < 1.0) ? std::sin(Math::Pi * std::min(1.0, extendX * 1.3)) : 0.0;
            double flare = std::max(windup, 0.7 * hit);

            Matrix4x4 body;
            body.translate(forward * (lunge * hit));
            body *= rotationAbout(pivot, right, rearAngle * windup - lungeAngle * hit);

            std::map<std::string, Matrix4x4> world = rest;
            auto apply = [&](const std::string& name, const Matrix4x4& layer) {
                if (!boneIdx.count(name))
                    return;
                Matrix4x4 m = layer;
                m *= rest[name];
                world[name] = m;
            };
            for (const char* name : { "Pelvis", "Spine", "Chest", "TailBase", "TailFeathers" })
                apply(name, body);

            // Neck cocks back in the windup and shoots forward and down on the strike.
            Vector3 neckBase = body.transformPoint(bonePos("Neck"));
            Matrix4x4 neck = rotationAbout(neckBase, right, neckBack * windup - neckStrike * hit);
            neck *= body;
            apply("Neck", neck);
            Vector3 headJoint = neck.transformPoint(bonePos("Head"));
            Matrix4x4 head = rotationAbout(headJoint, right, 0.2 * neckBack * windup - headStrike * hit);
            head *= neck;
            apply("Head", head);
            apply("Beak", head);

            // Wings flare up and out from the shoulders.
            for (const char* side : { "Left", "Right" }) {
                std::string shoulderName = std::string(side) + "WingShoulder";
                Vector3 shoulder = body.transformPoint(bonePos(shoulderName));
                double wingSide = Vector3::dotProduct(boneEnd(std::string(side) + "WingHand") - bonePos(shoulderName), right) >= 0.0 ? 1.0 : -1.0;
                // A positive rotation about `forward` lowers the +right side.
                Matrix4x4 wing = rotationAbout(shoulder, forward, -wingSide * wingFlare * flare);
                wing *= body;
                for (const char* part : { "WingShoulder", "WingElbow", "WingHand" })
                    apply(std::string(side) + part, wing);
            }

            posePlantedLeg(rigStructure, boneIdx, standUpper, standLower, standFoot, body, Vector3(), world);
            if (kickFactor <= 0.0) {
                posePlantedLeg(rigStructure, boneIdx, kickUpper, kickLower, kickFoot, body, Vector3(), world);
            } else {
                Vector3 hip = body.transformPoint(bonePos(kickUpper));
                Vector3 ankleRest = boneEnd(kickLower);
                Vector3 chamberAt = hip + forward * (legLength * 0.35) - up * (legLength * 0.55);
                Vector3 strikeAt = hip + forward * (legLength * 0.85) - up * (legLength * 0.35);
                double c = std::min(1.0, kickFactor) * chamber;
                Vector3 target = ankleRest + (chamberAt - ankleRest) * c;
                target = target + (strikeAt - target) * (std::min(1.0, kickFactor) * extend);
                Vector3 restBend = body.transformVector(bonePos(kickLower) - (bonePos(kickUpper) + ankleRest) * 0.5);
                restBend = restBend.lengthSquared() > 1e-12 ? restBend.normalized() : forward;
                Vector3 bend = restBend * (1.0 - c) + (forward + up * 0.4).normalized() * c;
                poseTwoBoneLeg(rigStructure, boneIdx, kickUpper, kickLower, kickFoot, body, target, bend, true, world);
            }

            auto& animFrame = animationClip.frames[frame];
            animFrame.time = static_cast<float>(frame) / static_cast<float>(frameCount) * durationSeconds;
            animFrame.boneWorldTransforms = world;
            finishFrame(animFrame, inverseBindMatrices);
        }
        return true;
    }

} // namespace bird

} // namespace dust3d
