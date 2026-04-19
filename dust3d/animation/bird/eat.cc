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

// Procedural bird eating animation — pecking food from the ground.
//
// Realistic ground-feeding behaviour observed in chickens and pigeons:
//
//   Peck cycle (repeated multiple times per clip):
//     1. Anticipation: body lowers slightly, neck extends forward-down
//     2. Strike: rapid head thrust downward to ground level (beak contact)
//     3. Grab: brief hold at ground with beak open/close (micro-vibration)
//     4. Retract: head lifts back, neck coils, swallow motion
//
//   Body dynamics:
//     - Forward lean during peck (pitch down)
//     - Subtle crouch to lower center of mass
//     - Weight shift between feet for balance
//     - Tail rises as counterbalance when head goes down
//
//   Legs:
//     - Feet stay planted (grounded)
//     - Subtle knee bend during crouch phases
//
//   Wings:
//     - Held folded against body, micro-sway for balance
//
// Adjustable parameters:
//   - peckSpeedFactor:     pecking cycle speed
//   - peckDepthFactor:     how far down the head reaches
//   - bodyLeanFactor:      forward lean intensity during peck
//   - tailLiftFactor:      tail counterbalance amplitude
//   - headShakeFactor:     lateral head shake during grab phase
//   - crouchFactor:        body crouch depth

#include <cmath>
#include <dust3d/animation/bird/eat.h>
#include <dust3d/animation/common.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace bird {

    bool eat(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& /* inverseBindMatrices */,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = static_cast<int>(parameters.getValue("frameCount", 60));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 3.0));

        auto boneIdx = buildBoneIndexMap(rigStructure);

        auto bonePos = [&](const std::string& name) -> Vector3 {
            return getBonePos(rigStructure, boneIdx, name);
        };
        auto boneEnd = [&](const std::string& name) -> Vector3 {
            return getBoneEnd(rigStructure, boneIdx, name);
        };

        static const char* requiredBones[] = {
            "Root", "Pelvis", "Spine", "Chest", "Neck", "Head",
            "LeftUpperLeg", "LeftLowerLeg", "LeftFoot",
            "RightUpperLeg", "RightLowerLeg", "RightFoot",
            "LeftWingShoulder", "LeftWingElbow", "LeftWingHand",
            "RightWingShoulder", "RightWingElbow", "RightWingHand"
        };
        if (!validateRequiredBones(boneIdx, requiredBones, sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;

        double peckSpeedFactor = parameters.getValue("peckSpeedFactor", 1.0);
        double peckDepthFactor = parameters.getValue("peckDepthFactor", 1.0);
        double bodyLeanFactor = parameters.getValue("bodyLeanFactor", 1.0);
        double tailLiftFactor = parameters.getValue("tailLiftFactor", 1.0);
        double headShakeFactor = parameters.getValue("headShakeFactor", 1.0);
        double crouchFactor = parameters.getValue("crouchFactor", 1.0);

        Vector3 upDir(0.0, 1.0, 0.0);
        Vector3 pelvisPos = bonePos("Pelvis");
        Vector3 chestPos = bonePos("Chest");
        Vector3 spineDir = chestPos - pelvisPos;
        Vector3 forward(spineDir.x(), 0.0, spineDir.z());
        if (forward.lengthSquared() < 1e-8)
            forward = Vector3(0.0, 0.0, 1.0);
        forward.normalize();
        Vector3 right = Vector3::crossProduct(upDir, forward);
        if (right.lengthSquared() < 1e-8)
            right = Vector3(1.0, 0.0, 0.0);
        right.normalize();

        double bodyHeight = (boneEnd("Head") - pelvisPos).length();
        if (bodyHeight < 1e-6)
            bodyHeight = 0.5;

        // Chicken pecking rice from the ground — continuous eating clip.
        //
        // The base pose is already the eating posture: chest tilted forward,
        // neck folded down, head aimed at the ground, beak near ground level.
        // Every frame starts from this lowered position. The peck motion is
        // a small, fast, sharp snap of the beak downward from this base pose
        // and back — like a chicken rapidly picking grains off the ground.
        // The clip is seamlessly loopable.
        //
        // Base eating pose (constant, applied every frame):
        double spineBasePitch = 0.20 * bodyLeanFactor;
        double chestBasePitch = 0.70 * bodyLeanFactor;
        double neckBasePitch = 1.0 * peckDepthFactor;
        double headBasePitch = 0.6 * peckDepthFactor;
        // Peck snap amplitude (oscillates on top of base pose):
        double peckNeckAmp = 0.15 * peckDepthFactor; // small neck snap
        double peckHeadAmp = 0.10 * peckDepthFactor; // small head snap
        double peckBeakAmp = 0.25 * peckDepthFactor; // beak does the sharp jab
        double tailLiftAmp = 0.10 * tailLiftFactor;
        double headShakeAmp = 0.04 * headShakeFactor;
        double crouchAmp = bodyHeight * 0.03 * crouchFactor;

        // Continuous rapid pecking
        double pecksPerCycle = std::max(2.0, std::round(5.0 * peckSpeedFactor));

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            double tNormalized = static_cast<double>(frame) / static_cast<double>(frameCount);

            double peckPhaseRaw = std::fmod(tNormalized * pecksPerCycle, 1.0);

            // Peck snap envelope: quick jab down from eating pose and back.
            // 0.00-0.10: snap beak down (strike)
            // 0.10-0.20: hold at ground (pick grain)
            // 0.20-0.50: lift back to eating pose
            // 0.50-1.00: rest at eating pose, tiny head bob
            double peckSnap = 0.0; // 0 = eating base pose, 1 = full jab down
            double grabPhase = 0.0;

            if (peckPhaseRaw < 0.10) {
                double s = peckPhaseRaw / 0.10;
                peckSnap = s * s;
            } else if (peckPhaseRaw < 0.20) {
                peckSnap = 1.0;
                grabPhase = (peckPhaseRaw - 0.10) / 0.10;
            } else if (peckPhaseRaw < 0.50) {
                double s = (peckPhaseRaw - 0.20) / 0.30;
                peckSnap = 1.0 - smoothstep(s);
            } else {
                peckSnap = 0.0;
            }

            // Constant eating-posture body crouch
            double crouchOffset = -crouchAmp;
            double lateralShift = bodyHeight * 0.002 * std::sin(tNormalized * 2.0 * Math::Pi * 0.5);

            Matrix4x4 bodyTransform;
            bodyTransform.translate(upDir * crouchOffset + right * lateralShift);

            std::map<std::string, Matrix4x4> boneWorldTransforms;
            std::map<std::string, Matrix4x4> boneSkinMatrices;

            auto worldFromSkin = [&](const std::string& name, const Matrix4x4& skin) -> Matrix4x4 {
                Matrix4x4 bindWorld = buildBoneWorldTransform(bonePos(name), boneEnd(name));
                Matrix4x4 result = skin;
                result *= bindWorld;
                return result;
            };

            // Tiny lateral jitter while picking
            double headShake = 0.0;
            if (grabPhase > 0.0) {
                headShake = headShakeAmp * std::sin(grabPhase * 6.0 * Math::Pi);
            }

            // Bone pitches = constant base + small peck oscillation
            double chestPitch = chestBasePitch;
            double spinePitch = spineBasePitch;
            double neckPitch = neckBasePitch + peckNeckAmp * peckSnap;
            double headPitch = headBasePitch + peckHeadAmp * peckSnap;
            double beakPitch = peckBeakAmp * peckSnap;

            // Chained skin matrices: Chest → Neck → Head → Beak
            // Each child inherits all parent rotations.

            // Root, Pelvis: body translation only
            for (const auto& name : { "Root", "Pelvis" }) {
                boneSkinMatrices[name] = bodyTransform;
                boneWorldTransforms[name] = worldFromSkin(name, bodyTransform);
            }

            // Spine: slight forward tilt to help bring head down
            Matrix4x4 spineSkin = bodyTransform;
            {
                Vector3 pivot = bonePos("Spine");
                spineSkin.translate(pivot);
                spineSkin.rotate(right, spineBasePitch);
                spineSkin.translate(-pivot);
            }
            boneSkinMatrices["Spine"] = spineSkin;
            boneWorldTransforms["Spine"] = worldFromSkin("Spine", spineSkin);

            // Chest: constant forward tilt (eating posture base), chained on spine
            Matrix4x4 chestSkin = spineSkin;
            {
                Vector3 pivot = bonePos("Chest");
                chestSkin.translate(pivot);
                chestSkin.rotate(right, chestPitch);
                chestSkin.translate(-pivot);
            }
            boneSkinMatrices["Chest"] = chestSkin;
            boneWorldTransforms["Chest"] = worldFromSkin("Chest", chestSkin);

            // Neck: base fold + peck snap, chained on chest
            Matrix4x4 neckSkin = chestSkin;
            {
                Vector3 pivot = bonePos("Neck");
                neckSkin.translate(pivot);
                if (std::abs(headShake) > 1e-6)
                    neckSkin.rotate(upDir, headShake * 0.3);
                neckSkin.rotate(right, neckPitch);
                neckSkin.translate(-pivot);
            }
            boneSkinMatrices["Neck"] = neckSkin;
            boneWorldTransforms["Neck"] = worldFromSkin("Neck", neckSkin);

            // Head: base aim + peck snap, chained on neck
            Matrix4x4 headSkin = neckSkin;
            {
                Vector3 pivot = bonePos("Head");
                headSkin.translate(pivot);
                if (std::abs(headShake) > 1e-6)
                    headSkin.rotate(upDir, headShake);
                headSkin.rotate(right, headPitch);
                headSkin.translate(-pivot);
            }
            boneSkinMatrices["Head"] = headSkin;
            boneWorldTransforms["Head"] = worldFromSkin("Head", headSkin);

            // Beak: the sharp snap — rotates on top of head's full chain
            if (boneIdx.count("Beak")) {
                Matrix4x4 beakSkin = headSkin;
                Vector3 pivot = bonePos("Beak");
                beakSkin.translate(pivot);
                beakSkin.rotate(right, beakPitch);
                beakSkin.translate(-pivot);
                boneSkinMatrices["Beak"] = beakSkin;
                boneWorldTransforms["Beak"] = worldFromSkin("Beak", beakSkin);
            }

            // Tail: constant lift for eating posture + small extra during peck
            {
                double tailPitch = -tailLiftAmp - tailLiftAmp * 0.3 * peckSnap;
                double tailYaw = 0.015 * std::sin(tNormalized * 2.0 * Math::Pi * 1.0);
                bool hasTailBase = boneIdx.count("TailBase") > 0;
                bool hasTailFeathers = boneIdx.count("TailFeathers") > 0;

                if (hasTailBase) {
                    Vector3 tailPivot = bonePos("TailBase");
                    Matrix4x4 tailSkin = bodyTransform;
                    tailSkin.translate(tailPivot);
                    tailSkin.rotate(upDir, tailYaw);
                    tailSkin.rotate(right, tailPitch);
                    tailSkin.translate(-tailPivot);
                    boneSkinMatrices["TailBase"] = tailSkin;
                    boneWorldTransforms["TailBase"] = worldFromSkin("TailBase", tailSkin);

                    if (hasTailFeathers) {
                        Matrix4x4 featherSkin = tailSkin;
                        boneSkinMatrices["TailFeathers"] = featherSkin;
                        boneWorldTransforms["TailFeathers"] = worldFromSkin("TailFeathers", featherSkin);
                    }
                }
            }

            // Wings: folded, follow body with micro-sway
            {
                double wingSway = 0.01 * std::sin(tNormalized * 2.0 * Math::Pi * 2.0);
                static const char* wingBones[] = {
                    "LeftWingShoulder", "LeftWingElbow", "LeftWingHand",
                    "RightWingShoulder", "RightWingElbow", "RightWingHand"
                };
                for (const auto& boneName : wingBones) {
                    Matrix4x4 wingSkin = bodyTransform;
                    Vector3 wingPivot = bonePos(boneName);
                    wingSkin.translate(wingPivot);
                    wingSkin.rotate(forward, wingSway);
                    wingSkin.translate(-wingPivot);
                    boneSkinMatrices[boneName] = wingSkin;
                    boneWorldTransforms[boneName] = worldFromSkin(boneName, wingSkin);
                }
            }

            // Legs: grounded (identity skin)
            static const char* legBones[] = {
                "LeftUpperLeg", "LeftLowerLeg", "LeftFoot",
                "RightUpperLeg", "RightLowerLeg", "RightFoot"
            };
            for (const auto& boneName : legBones) {
                Matrix4x4 identity;
                boneSkinMatrices[boneName] = identity;
                boneWorldTransforms[boneName] = worldFromSkin(boneName, identity);
            }

            // Write frame
            auto& animFrame = animationClip.frames[frame];
            animFrame.time = static_cast<float>(tNormalized) * durationSeconds;
            animFrame.boneWorldTransforms = boneWorldTransforms;
            animFrame.boneSkinMatrices = boneSkinMatrices;
        }

        return true;
    }

} // namespace bird

} // namespace dust3d
