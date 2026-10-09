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

// Procedural bite / head-butt attack animation for the quadruped rig.
//
// A one-shot attack for game use (dogs and cats bite, bulls and rams butt,
// lizards lunge), in four phases:
//
//   Anticipation (0 - anticipationDuration): the weight rocks back onto the
//       hind legs, the body crouches, the head drops and the spine coils.
//       This wind-up telegraphs the attack to the player.
//   Charge (anticipationDuration - strikeMoment): the body surges forward off
//       the planted hind feet while the front feet step forward to catch it;
//       the head stays low and aimed, the jaw opens.
//   Strike (strikeMoment - strikeEnd): the head snaps up and shakes (the damage
//       frame), the jaw bites shut, the tail whips as a counterweight.
//   Recovery (strikeEnd - 1): the body settles back, the front feet step back
//       home and the tail damps out.
//
// The hind feet never slide, the front feet lift as they step, and every bone
// is moved by rotations about its own joint, so the clip works for long-legged
// runners and low, sprawling lizards alike. The clip starts and ends exactly in
// the rest pose, so it blends with the idle and walk loops.
//
// Adjustable animation parameters:
//   - chargeDistanceFactor:   how far the body lunges forward
//   - chargeSpeedFactor:      how explosive the lunge is (> 1 reaches full lunge sooner)
//   - headDropFactor:         how low the head drops while winding up and charging
//   - headStrikeIntensity:    how hard the head snaps up on the strike
//   - jawOpenFactor:          how wide the jaw opens (with a Jaw bone)
//   - spineCompressionFactor: how far the body rocks back and crouches in the wind-up
//   - tailWhipFactor:         how hard the tail whips on the strike
//   - frontLegBraceFactor:    how far the front feet reach while bracing and stepping
//   - backLegPushFactor:      how far the hind legs extend as they push off
//   - anticipationDuration:   end of the wind-up (fraction of the clip)
//   - strikeMoment:           when the strike lands (fraction of the clip)
//   - strikeEnd:              end of the strike (fraction of the clip)
//   - recoverySpeed:          how quickly the body settles back (> 1 = sooner)
//   - bodyMassFactor:         heavier bodies move less and settle more slowly

#include <algorithm>
#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/common.h>
#include <dust3d/animation/quadruped/attack.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>

namespace dust3d {

namespace quadruped {

    bool attack(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = std::max(2, static_cast<int>(parameters.getValue("frameCount", 40)));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 1.2));

        auto boneIdx = buildBoneIndexMap(rigStructure);
        auto bonePos = [&](const std::string& name) -> Vector3 {
            return getBonePos(rigStructure, boneIdx, name);
        };
        auto boneEnd = [&](const std::string& name) -> Vector3 {
            return getBoneEnd(rigStructure, boneIdx, name);
        };

        static const char* requiredBones[] = {
            "Root", "Pelvis", "Spine", "Chest", "Neck", "Head",
            "FrontLeftUpperLeg", "FrontLeftLowerLeg", "FrontLeftFoot",
            "FrontRightUpperLeg", "FrontRightLowerLeg", "FrontRightFoot",
            "BackLeftUpperLeg", "BackLeftLowerLeg", "BackLeftFoot",
            "BackRightUpperLeg", "BackRightLowerLeg", "BackRightFoot"
        };
        if (!validateRequiredBones(boneIdx, requiredBones, sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;

        double chargeDistanceFactor = parameters.getValue("chargeDistanceFactor", 1.0);
        double chargeSpeedFactor = std::max(0.25, parameters.getValue("chargeSpeedFactor", 1.0));
        double headDropFactor = parameters.getValue("headDropFactor", 1.0);
        double headStrikeIntensity = parameters.getValue("headStrikeIntensity", 1.0);
        double jawOpenFactor = parameters.getValue("jawOpenFactor", 1.0);
        double spineCompressionFactor = parameters.getValue("spineCompressionFactor", 1.0);
        double tailWhipFactor = parameters.getValue("tailWhipFactor", 1.0);
        double frontLegBraceFactor = parameters.getValue("frontLegBraceFactor", 1.0);
        double backLegPushFactor = parameters.getValue("backLegPushFactor", 1.0);
        double anticipationEnd = std::clamp(parameters.getValue("anticipationDuration", 0.25), 0.05, 0.6);
        double strikeMoment = std::clamp(parameters.getValue("strikeMoment", 0.5), anticipationEnd + 0.05, 0.85);
        animationClip.events.push_back({ "hit", static_cast<float>(strikeMoment * durationSeconds), "Head" });
        double strikeEnd = std::clamp(parameters.getValue("strikeEnd", 0.65), strikeMoment + 0.03, 0.92);
        double recoverySpeed = std::max(0.3, parameters.getValue("recoverySpeed", 1.0));
        double bodyMassFactor = std::max(0.2, parameters.getValue("bodyMassFactor", 1.0));

        Vector3 pelvisPos = bonePos("Pelvis");
        Vector3 chestPos = bonePos("Chest");
        Vector3 spineVec = chestPos - pelvisPos;
        double spineLength = std::max(1e-4, spineVec.length());
        Vector3 up(0.0, 1.0, 0.0);
        Vector3 forward(spineVec.x(), 0.0, spineVec.z());
        if (forward.lengthSquared() < 1e-10)
            forward = Vector3(0.0, 0.0, 1.0);
        forward.normalize();
        // The creature's right: a positive rotation about it lifts the nose.
        Vector3 right = Vector3::crossProduct(forward, up).normalized();

        // Heavy bodies move a little less and settle more slowly.
        double mass = 1.0 / std::sqrt(bodyMassFactor);
        double lungeDist = spineLength * 0.35 * chargeDistanceFactor * mass;
        double rockBack = spineLength * 0.1 * spineCompressionFactor * mass;
        double crouch = spineLength * 0.07 * spineCompressionFactor;
        double coilPitch = 0.1 * spineCompressionFactor;
        double strikePitch = 0.08 * headStrikeIntensity * mass;
        double neckDrop = 0.35 * headDropFactor;
        double headDrop = 0.25 * headDropFactor;
        double neckSnap = 0.5 * headStrikeIntensity;
        double headSnap = 0.35 * headStrikeIntensity;
        double jawOpen = 0.5 * jawOpenFactor;
        double stepLift = spineLength * 0.12 * frontLegBraceFactor;
        double brace = spineLength * 0.05 * frontLegBraceFactor;
        double tailWhip = 0.35 * tailWhipFactor;

        // The body rocks about the ground between the hind feet, which stay planted.
        Vector3 hindPivot = (boneEnd("BackLeftLowerLeg") + boneEnd("BackRightLowerLeg")) * 0.5;

        std::vector<std::string> tailBones;
        for (const char* name : { "TailBase", "TailMid", "TailTip" }) {
            if (boneIdx.count(name))
                tailBones.push_back(name);
        }

        std::map<std::string, Matrix4x4> rest = restBoneWorldTransforms(rigStructure);

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            // One-shot: the last frame is t = 1, back at the rest pose.
            double t = static_cast<double>(frame) / static_cast<double>(frameCount - 1);

            double recoverLen = std::min(1.0 - strikeEnd, (1.0 - strikeEnd) * std::sqrt(bodyMassFactor) / recoverySpeed);
            double settle = smootherstep((t - strikeEnd) / std::max(0.05, recoverLen));
            double wind = smootherstep(t / anticipationEnd) * (1.0 - smoothstep((t - anticipationEnd) / (strikeMoment - anticipationEnd)));
            double lunge = smootherstep(chargeSpeedFactor * (t - anticipationEnd) / (strikeMoment - anticipationEnd)) * (1.0 - settle);
            double strikeX = (t - strikeMoment) / (strikeEnd - strikeMoment);
            double strike = (strikeX > 0.0 && strikeX < 1.0) ? std::sin(Math::Pi * strikeX) : 0.0;
            // The head shakes a little after the snap.
            double shakeX = (t - strikeMoment) / (strikeEnd - strikeMoment + 0.12);
            double shake = (shakeX > 0.0 && shakeX < 1.0) ? std::sin(2.0 * Math::Pi * 1.5 * shakeX) * (1.0 - shakeX) : 0.0;

            // Body: rock back and crouch, then surge forward; pitch nose-down while coiled, up on the strike.
            Matrix4x4 body;
            body.translate(forward * (-rockBack * wind + lungeDist * lunge) + up * (-crouch * wind));
            body *= rotationAbout(hindPivot, right, -coilPitch * wind + strikePitch * strike);

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
            auto jointOf = [&](const Matrix4x4& layer, const std::string& name) -> Vector3 {
                return layer.transformPoint(bonePos(name));
            };

            apply("Pelvis", body);
            apply("Spine", body);
            // The chest arches down a little more than the hips while coiled.
            Matrix4x4 chest = rotationAbout(jointOf(body, "Chest"), right, -0.5 * coilPitch * wind);
            chest *= body;
            apply("Chest", chest);

            // Neck: low while winding up and charging, snaps up on the strike.
            Matrix4x4 neck = rotationAbout(jointOf(chest, "Neck"), right,
                -neckDrop * (wind + 0.6 * lunge * (1.0 - strike)) + neckSnap * strike);
            neck *= chest;
            apply("Neck", neck);

            Vector3 headJoint = jointOf(neck, "Head");
            Matrix4x4 head = rotationAbout(headJoint, up, 0.18 * headStrikeIntensity * shake);
            head *= rotationAbout(headJoint, right, -headDrop * (wind + 0.5 * lunge * (1.0 - strike)) + headSnap * strike);
            head *= neck;
            apply("Head", head);

            if (boneIdx.count("Jaw")) {
                // Opens on the approach, bites shut on the strike.
                double open = jawOpen * smoothstep((t - anticipationEnd) / (strikeMoment - anticipationEnd))
                    * (1.0 - smoothstep((t - strikeMoment) / (0.5 * (strikeEnd - strikeMoment))));
                Matrix4x4 jaw = rotationAbout(jointOf(head, "Jaw"), right, -open);
                jaw *= head;
                apply("Jaw", jaw);
            }

            // Tail: lifts while coiled, whips as a counterweight after the strike, then damps out.
            Matrix4x4 tailLayer = body;
            for (size_t i = 0; i < tailBones.size(); ++i) {
                double delay = 0.05 * static_cast<double>(i);
                double whipT = t - strikeMoment + 0.08 - delay;
                double whip = whipT > 0.0 ? std::sin(2.0 * Math::Pi * 1.6 * whipT) * std::exp(-4.0 * whipT) : 0.0;
                whip *= 1.0 - smoothstep((t - 0.8) / 0.2);
                double lift = 0.18 * tailWhipFactor * (wind + 0.4 * lunge);
                Vector3 joint = jointOf(tailLayer, tailBones[i]);
                Matrix4x4 seg = rotationAbout(joint, up, tailWhip * (1.0 + 0.3 * static_cast<double>(i)) * whip);
                // The tail points backwards: a nose-down rotation lifts it.
                seg *= rotationAbout(joint, right, -lift / static_cast<double>(tailBones.size()));
                seg *= tailLayer;
                apply(tailBones[i], seg);
                tailLayer = seg;
            }

            // Front feet: brace ahead while coiling, step forward with the lunge, step back home.
            double stepOut = smootherstep((t - anticipationEnd) / (strikeMoment - anticipationEnd));
            double outX = (t - anticipationEnd) / (strikeMoment - anticipationEnd);
            double backX = (t - strikeEnd) / std::max(0.05, recoverLen);
            double arc = ((outX > 0.0 && outX < 1.0) ? std::sin(Math::Pi * outX) : 0.0)
                + ((backX > 0.0 && backX < 1.0) ? std::sin(Math::Pi * backX) : 0.0);
            Vector3 frontOffset = forward * (brace * wind + (lungeDist * 1.05) * stepOut * (1.0 - settle))
                + up * (stepLift * arc);
            // On the strike the front legs take the weight.
            frontOffset += up * (-0.02 * spineLength * strike);
            frontOffset.setY(std::max(0.0, frontOffset.y()));
            posePlantedLeg(rigStructure, boneIdx, "FrontLeftUpperLeg", "FrontLeftLowerLeg", "FrontLeftFoot", chest, frontOffset, world);
            posePlantedLeg(rigStructure, boneIdx, "FrontRightUpperLeg", "FrontRightLowerLeg", "FrontRightFoot", chest, frontOffset, world);

            // Hind feet stay planted; as the body surges the heels lift a little (the push-off).
            Vector3 hindOffset = up * (spineLength * 0.03 * backLegPushFactor * lunge * (1.0 - strike));
            posePlantedLeg(rigStructure, boneIdx, "BackLeftUpperLeg", "BackLeftLowerLeg", "BackLeftFoot", body, hindOffset, world);
            posePlantedLeg(rigStructure, boneIdx, "BackRightUpperLeg", "BackRightLowerLeg", "BackRightFoot", body, hindOffset, world);
            for (const char* name : { "FrontLeftUpperLeg", "FrontRightUpperLeg", "BackLeftUpperLeg", "BackRightUpperLeg" })
                layers[name] = Matrix4x4();

            // Any other bone (ears, eyelids, spikes...) follows the nearest moved ancestor.
            for (const auto& bone : rigStructure.bones) {
                if (layers.count(bone.name) || bone.name == "Root")
                    continue;
                if (bone.name.find("Leg") != std::string::npos || bone.name.find("Foot") != std::string::npos)
                    continue;
                std::string parent = bone.parent;
                while (!parent.empty() && !layers.count(parent)) {
                    auto it = boneIdx.find(parent);
                    parent = it == boneIdx.end() ? std::string() : rigStructure.bones[it->second].parent;
                }
                if (!parent.empty())
                    apply(bone.name, layers[parent]);
            }

            auto& frameData = animationClip.frames[frame];
            frameData.time = static_cast<float>(frame) / static_cast<float>(frameCount) * durationSeconds;
            frameData.boneWorldTransforms = world;
            finishFrame(frameData, inverseBindMatrices);
        }

        return true;
    }

} // namespace quadruped

} // namespace dust3d
