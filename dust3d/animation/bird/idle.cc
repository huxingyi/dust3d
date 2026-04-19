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

// Procedural idle animation for bird rig.
//
// Designed for game development idle/rest state. Uses
// techniques: saccadic (snap-and-hold) head motion, layered breathing
// harmonics, tail pump synced to respiration, procedural micro-fidgets,
// and body roll with weight shift. All frequencies are integer multiples
// for seamless looping. Feet stay planted.
//
// Adjustable animation parameters:
//   - breathingAmplitudeFactor:   body rise-fall intensity
//   - breathingSpeedFactor:       breathing cycle speed multiplier
//   - headLookFactor:       head turn/peek amplitude
//   - headPeckFactor:       head pecking motion intensity
//   - tailFeatherFactor:    tail feather sway
//   - weightShiftFactor:    lateral weight shift

#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/bird/idle.h>
#include <dust3d/animation/common.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace bird {

    bool idle(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = static_cast<int>(parameters.getValue("frameCount", 90));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 4.0));

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

        double breathingAmplitudeFactor = parameters.getValue("breathingAmplitudeFactor", 1.0);
        double breathingSpeedFactor = parameters.getValue("breathingSpeedFactor", 1.0);
        double headLookFactor = parameters.getValue("headLookFactor", 1.0);
        double headPeckFactor = parameters.getValue("headPeckFactor", 1.0);
        double tailFeatherFactor = parameters.getValue("tailFeatherFactor", 1.0);
        double weightShiftFactor = parameters.getValue("weightShiftFactor", 1.0);

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
        double breathAmp = bodyHeight * 0.008 * breathingAmplitudeFactor;
        double shiftAmp = bodyHeight * 0.006 * weightShiftFactor;

        // Saccadic head motion helper: fast snap then hold, like real birds.
        // Uses a smoothstep-shaped curve that spends most time holding and
        // transitions quickly between target angles.
        auto saccade = [](double t, double holdRatio) -> double {
            double cycle = std::fmod(t, 1.0);
            double movePhase = (1.0 - holdRatio);
            if (cycle < movePhase * 0.5) {
                double s = cycle / (movePhase * 0.5);
                return s * s * (3.0 - 2.0 * s);
            } else if (cycle > 1.0 - movePhase * 0.5) {
                double s = (1.0 - cycle) / (movePhase * 0.5);
                return s * s * (3.0 - 2.0 * s);
            }
            return 1.0;
        };

        // Cheap procedural noise from layered sines (no PRNG needed, loopable)
        auto microNoise = [](double t, double freq1, double freq2, double freq3) -> double {
            return 0.5 * std::sin(t * 2.0 * Math::Pi * freq1)
                + 0.3 * std::sin(t * 2.0 * Math::Pi * freq2 + 1.3)
                + 0.2 * std::sin(t * 2.0 * Math::Pi * freq3 + 2.7);
        };

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            double tNormalized = static_cast<double>(frame) / static_cast<double>(frameCount);

            // Layered breathing: primary + secondary harmonic for organic rhythm
            double breathPhase = tNormalized * 2.0 * Math::Pi * breathingSpeedFactor;
            double breathPrimary = std::sin(breathPhase);
            double breathSecondary = 0.3 * std::sin(breathPhase * 2.0);
            double breathOffset = breathAmp * (breathPrimary + breathSecondary) / 1.3;

            // Weight shift with subtle body lean (forward-back rock)
            double lateralShift = shiftAmp * std::sin(tNormalized * 2.0 * Math::Pi * 1.0);
            double forwardRock = bodyHeight * 0.003 * std::sin(tNormalized * 2.0 * Math::Pi * 2.0);

            // Micro-fidget overlay: tiny high-frequency perturbation to break mechanical feel
            double fidgetY = bodyHeight * 0.0008 * microNoise(tNormalized, 3.0, 5.0, 7.0);
            double fidgetX = bodyHeight * 0.0005 * microNoise(tNormalized, 4.0, 6.0, 8.0);

            Matrix4x4 bodyTransform;
            bodyTransform.translate(upDir * (breathOffset + fidgetY)
                + right * (lateralShift + fidgetX)
                + forward * forwardRock);

            // Subtle body roll toward weighted side
            double bodyRoll = 0.008 * weightShiftFactor * std::sin(tNormalized * 2.0 * Math::Pi * 1.0);

            std::map<std::string, Matrix4x4> boneWorldTransforms;
            std::map<std::string, Matrix4x4> boneSkinMatrices;

            // Helper: compute world transform from skin matrix and bind-pose endpoints.
            // skinMatrix * bindPoseWorldTransform preserves bone roll.
            auto worldFromSkin = [&](const std::string& name, const Matrix4x4& skin) -> Matrix4x4 {
                Matrix4x4 bindWorld = buildBoneWorldTransform(bonePos(name), boneEnd(name));
                Matrix4x4 result = skin;
                result *= bindWorld;
                return result;
            };

            // Skin matrix with extra rotation around a bone's bind-pose pivot
            auto makeSkin = [&](const std::string& name, const Matrix4x4& baseSkin,
                                double extraYaw = 0.0, double extraPitch = 0.0, double extraRoll = 0.0) {
                Matrix4x4 skin = baseSkin;
                if (std::abs(extraYaw) > 1e-6 || std::abs(extraPitch) > 1e-6 || std::abs(extraRoll) > 1e-6) {
                    Vector3 pivot = bonePos(name);
                    skin.translate(pivot);
                    if (std::abs(extraRoll) > 1e-6)
                        skin.rotate(forward, extraRoll);
                    if (std::abs(extraYaw) > 1e-6)
                        skin.rotate(upDir, extraYaw);
                    if (std::abs(extraPitch) > 1e-6)
                        skin.rotate(right, extraPitch);
                    skin.translate(-pivot);
                }
                boneSkinMatrices[name] = skin;
                boneWorldTransforms[name] = worldFromSkin(name, skin);
            };

            // Saccadic head motion: snap between look targets then hold
            // Two saccade events per cycle (look left, then look right)
            double saccadeCycle = std::fmod(tNormalized * 2.0, 1.0);
            double saccadeBlend = saccade(saccadeCycle, 0.7);
            double headLookDir = (tNormalized < 0.5) ? 1.0 : -1.0;
            double headYaw = 0.08 * headLookFactor * saccadeBlend * headLookDir;

            // Head peck with quick down-snap and slow return (bird foraging motion)
            double peckRaw = std::sin(tNormalized * 2.0 * Math::Pi * 2.0);
            double headPeck = 0.05 * headPeckFactor * std::max(0.0, peckRaw * peckRaw * peckRaw);

            // Head micro-fidget: tiny fast twitches
            double headFidgetYaw = 0.01 * headLookFactor * microNoise(tNormalized, 5.0, 7.0, 11.0);

            makeSkin("Root", bodyTransform, 0.0, 0.0, bodyRoll);
            makeSkin("Pelvis", bodyTransform, 0.0, 0.0, bodyRoll);
            makeSkin("Spine", bodyTransform, 0.0, 0.0, bodyRoll * 0.7);
            makeSkin("Chest", bodyTransform, 0.0, breathOffset * 0.3 / (bodyHeight * 0.01 + 1e-6), bodyRoll * 0.5);
            makeSkin("Neck", bodyTransform, (headYaw + headFidgetYaw) * 0.3, headPeck * 0.5);
            makeSkin("Head", bodyTransform, headYaw + headFidgetYaw, headPeck);

            // Beak: same skin as head so it follows head exactly
            if (boneIdx.count("Beak")) {
                // Build a skin that includes head rotation around head pivot
                Vector3 headPivot = bonePos("Head");
                Matrix4x4 beakSkin = bodyTransform;
                beakSkin.translate(headPivot);
                beakSkin.rotate(upDir, headYaw + headFidgetYaw);
                beakSkin.rotate(right, headPeck);
                beakSkin.translate(-headPivot);
                boneSkinMatrices["Beak"] = beakSkin;
                boneWorldTransforms["Beak"] = worldFromSkin("Beak", beakSkin);
            }

            // Tail: subtle yaw sway synced to breathing.
            // Uses skin-matrix approach (translate to pivot, rotate, translate back)
            // to preserve bone roll, matching glide.cc's tail handling.
            // Only yaw is applied — pitch on tail bones causes mesh distortion.
            {
                double tailYaw = 0.03 * tailFeatherFactor * std::sin(tNormalized * 2.0 * Math::Pi * 1.0);
                bool hasTailBase = boneIdx.count("TailBase") > 0;
                bool hasTailFeathers = boneIdx.count("TailFeathers") > 0;

                if (hasTailBase) {
                    Vector3 tailBindPos = bonePos("TailBase");
                    Matrix4x4 tailSkin = bodyTransform;
                    tailSkin.translate(tailBindPos);
                    tailSkin.rotate(upDir, tailYaw);
                    tailSkin.translate(-tailBindPos);
                    boneSkinMatrices["TailBase"] = tailSkin;
                    boneWorldTransforms["TailBase"] = worldFromSkin("TailBase", tailSkin);

                    if (hasTailFeathers) {
                        double featherYaw = 0.05 * tailFeatherFactor * std::sin(tNormalized * 2.0 * Math::Pi * 1.0 - 0.5);
                        boneSkinMatrices["TailFeathers"] = tailSkin;
                        boneWorldTransforms["TailFeathers"] = worldFromSkin("TailFeathers", tailSkin);
                    }
                }
            }

            // Wings: follow body transform only (no fold)
            static const char* wingBones[] = {
                "LeftWingShoulder", "LeftWingElbow", "LeftWingHand",
                "RightWingShoulder", "RightWingElbow", "RightWingHand"
            };
            for (const auto& boneName : wingBones) {
                boneSkinMatrices[boneName] = bodyTransform;
                boneWorldTransforms[boneName] = worldFromSkin(boneName, bodyTransform);
            }

            // Legs: grounded (identity skin matrix, no transform)
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
