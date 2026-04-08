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

// Procedural idle animation for biped rig.
//
// Designed for game development idle/rest state. The character stands in
// place with subtle breathing motion, weight shifting, and minor
// secondary movements. Works for humanoids, robots, dinosaurs, penguins,
// and any creature using the biped bone structure.
//
// Adjustable animation parameters:
//   - breathingAmplitudeFactor:   chest/torso rise-fall intensity
//   - breathingSpeedFactor:       breathing cycle speed multiplier
//   - weightShiftFactor:    lateral hip sway (weight shift side to side)
//   - weightShiftSpeedFactor:     speed of weight shifting cycle
//   - headLookFactor:       subtle head turn/nod amplitude
//   - armRestFactor:        slight arm sway while resting
//   - spineSwayFactor:      subtle spine lateral sway
//   - tailIdleFactor:       tail gentle sway amplitude (if tail bones exist)

#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/biped/idle.h>
#include <dust3d/animation/common.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace biped {

    bool idle(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = static_cast<int>(parameters.getValue("frameCount", 90));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 4.0));

        // ----- bone lookup -----
        auto boneIdx = buildBoneIndexMap(rigStructure);

        auto bonePos = [&](const std::string& name) -> Vector3 {
            return getBonePos(rigStructure, boneIdx, name);
        };
        auto boneEnd = [&](const std::string& name) -> Vector3 {
            return getBoneEnd(rigStructure, boneIdx, name);
        };

        // ----- verify required bones -----
        static const char* requiredBones[] = {
            "Root", "Hips", "Spine", "Chest", "Neck", "Head",
            "LeftUpperLeg", "LeftLowerLeg", "LeftFoot",
            "RightUpperLeg", "RightLowerLeg", "RightFoot",
            "LeftShoulder", "LeftUpperArm", "LeftLowerArm", "LeftHand",
            "RightShoulder", "RightUpperArm", "RightLowerArm", "RightHand"
        };
        if (!validateRequiredBones(boneIdx, requiredBones, sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;

        // ===================================================================
        // 1. Idle parameters
        // ===================================================================
        double breathingAmplitudeFactor = parameters.getValue("breathingAmplitudeFactor", 1.0);
        double breathingSpeedFactor = parameters.getValue("breathingSpeedFactor", 1.0);
        double weightShiftFactor = parameters.getValue("weightShiftFactor", 1.0);
        double weightShiftSpeedFactor = parameters.getValue("weightShiftSpeedFactor", 1.0);
        double headLookFactor = parameters.getValue("headLookFactor", 1.0);
        double armRestFactor = parameters.getValue("armRestFactor", 1.0);
        double spineSwayFactor = parameters.getValue("spineSwayFactor", 1.0);
        double tailIdleFactor = parameters.getValue("tailIdleFactor", 1.0);

        // Body reference
        Vector3 hipsPos = bonePos("Hips");
        Vector3 chestPos = bonePos("Chest");
        Vector3 upDir(0.0, 1.0, 0.0);

        // Derive forward and right from foot positions
        Vector3 leftFootEnd = boneEnd("LeftFoot");
        Vector3 rightFootEnd = boneEnd("RightFoot");
        Vector3 avgFootEnd = (leftFootEnd + rightFootEnd) * 0.5;
        Vector3 hipsToFoot = avgFootEnd - hipsPos;
        Vector3 forward(hipsToFoot.x(), 0.0, hipsToFoot.z());
        if (forward.lengthSquared() < 1e-8)
            forward = Vector3(0.0, 0.0, 1.0);
        forward.normalize();

        Vector3 right = Vector3::crossProduct(upDir, forward);
        if (right.lengthSquared() < 1e-8)
            right = Vector3(1.0, 0.0, 0.0);
        right.normalize();

        // Compute body scale from leg length
        double leftLegLen = (boneEnd("LeftUpperLeg") - bonePos("LeftUpperLeg")).length()
            + (boneEnd("LeftLowerLeg") - bonePos("LeftLowerLeg")).length()
            + (boneEnd("LeftFoot") - bonePos("LeftFoot")).length();
        double rightLegLen = (boneEnd("RightUpperLeg") - bonePos("RightUpperLeg")).length()
            + (boneEnd("RightLowerLeg") - bonePos("RightLowerLeg")).length()
            + (boneEnd("RightFoot") - bonePos("RightFoot")).length();
        double avgLegLength = (leftLegLen + rightLegLen) * 0.5;
        if (avgLegLength < 1e-6)
            avgLegLength = 1.0;

        // Breathing amplitudes scaled to body
        double breathAmp = avgLegLength * 0.008 * breathingAmplitudeFactor;
        double weightShiftAmp = avgLegLength * 0.012 * weightShiftFactor;

        // ===================================================================
        // 2. Generate frames
        // ===================================================================
        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            double tNormalized = static_cast<double>(frame) / static_cast<double>(frameCount);

            // Breathing: slow sine wave
            double breathPhase = tNormalized * 2.0 * Math::Pi * breathingSpeedFactor;
            double breathOffset = breathAmp * std::sin(breathPhase);

            // Weight shift: slower than breathing, lateral sway
            double shiftPhase = tNormalized * 2.0 * Math::Pi * weightShiftSpeedFactor * 0.5;
            double lateralShift = weightShiftAmp * std::sin(shiftPhase);

            // Subtle spine sway
            double spineSway = 0.015 * spineSwayFactor * std::sin(shiftPhase + 0.3);

            // Head subtle look
            double headYaw = 0.03 * headLookFactor * std::sin(tNormalized * 2.0 * Math::Pi * 0.7);
            double headPitch = 0.01 * headLookFactor * std::sin(tNormalized * 2.0 * Math::Pi * 1.1 + 0.5);

            // Body transform: breathing lift + weight shift
            Matrix4x4 bodyTransform;
            bodyTransform.translate(upDir * breathOffset + right * lateralShift);

            std::map<std::string, Matrix4x4> boneWorldTransforms;

            auto computeBodyBone = [&](const std::string& name,
                                       double extraYaw = 0.0,
                                       double extraPitch = 0.0) {
                Vector3 pos = bonePos(name);
                Vector3 end = boneEnd(name);
                Vector3 newPos = bodyTransform.transformPoint(pos);
                Vector3 newEnd = bodyTransform.transformPoint(end);
                if (std::abs(extraYaw) > 1e-6 || std::abs(extraPitch) > 1e-6) {
                    Matrix4x4 extraRot;
                    if (std::abs(extraYaw) > 1e-6)
                        extraRot.rotate(upDir, extraYaw);
                    if (std::abs(extraPitch) > 1e-6)
                        extraRot.rotate(right, extraPitch);
                    Vector3 offset = newEnd - newPos;
                    newEnd = newPos + extraRot.transformVector(offset);
                }
                boneWorldTransforms[name] = buildBoneWorldTransform(newPos, newEnd);
            };

            computeBodyBone("Root");
            computeBodyBone("Hips", 0.0, 0.0);
            computeBodyBone("Spine", spineSway);
            computeBodyBone("Chest", spineSway * 0.5, breathOffset * 0.3 / (avgLegLength * 0.01 + 1e-6));
            computeBodyBone("Neck", -spineSway * 0.3, headPitch * 0.3);
            computeBodyBone("Head", headYaw, headPitch);

            // Tail idle sway
            static const char* tailBones[] = { "TailBase", "TailMid", "TailTip" };
            double tailPhase = tNormalized * 2.0 * Math::Pi * 0.8;
            for (int ti = 0; ti < 3; ++ti) {
                if (boneIdx.count(tailBones[ti]) == 0)
                    continue;
                double attenuation = 1.0 - ti * 0.28;
                double tailAngle = 0.04 * tailIdleFactor * attenuation * std::sin(tailPhase - ti * 0.6);
                computeBodyBone(tailBones[ti], tailAngle);
            }

            // Legs: stationary, just follow body transform
            static const char* legBones[] = {
                "LeftUpperLeg", "LeftLowerLeg", "LeftFoot",
                "RightUpperLeg", "RightLowerLeg", "RightFoot"
            };
            for (const auto& boneName : legBones) {
                computeBodyBone(boneName);
            }

            // Arms: subtle rest sway
            auto computeArmIdle = [&](const char* shoulderName, const char* upperArmName,
                                      const char* lowerArmName, const char* handName,
                                      double phase) {
                double armSway = 0.02 * armRestFactor * std::sin(breathPhase + phase);

                Vector3 shoulderPos = bodyTransform.transformPoint(bonePos(shoulderName));
                Vector3 shoulderEnd = bodyTransform.transformPoint(boneEnd(shoulderName));
                boneWorldTransforms[shoulderName] = buildBoneWorldTransform(shoulderPos, shoulderEnd);

                Vector3 upperArmStart = shoulderEnd;
                Vector3 upperArmEndRest = bodyTransform.transformPoint(boneEnd(upperArmName));
                Vector3 armDir = upperArmEndRest - upperArmStart;
                Matrix4x4 swayMat;
                swayMat.rotate(right, armSway);
                Vector3 newUpperArmEnd = upperArmStart + swayMat.transformVector(armDir);
                boneWorldTransforms[upperArmName] = buildBoneWorldTransform(upperArmStart, newUpperArmEnd);

                Vector3 lowerArmDir = bodyTransform.transformPoint(boneEnd(lowerArmName)) - bodyTransform.transformPoint(boneEnd(upperArmName));
                Vector3 newLowerArmEnd = newUpperArmEnd + swayMat.transformVector(lowerArmDir);
                boneWorldTransforms[lowerArmName] = buildBoneWorldTransform(newUpperArmEnd, newLowerArmEnd);

                Vector3 handDir = bodyTransform.transformPoint(boneEnd(handName)) - bodyTransform.transformPoint(boneEnd(lowerArmName));
                Vector3 newHandEnd = newLowerArmEnd + swayMat.transformVector(handDir);
                boneWorldTransforms[handName] = buildBoneWorldTransform(newLowerArmEnd, newHandEnd);
            };

            computeArmIdle("LeftShoulder", "LeftUpperArm", "LeftLowerArm", "LeftHand", 0.0);
            computeArmIdle("RightShoulder", "RightUpperArm", "RightLowerArm", "RightHand", Math::Pi);

            // Skin matrices
            auto& animFrame = animationClip.frames[frame];
            animFrame.time = static_cast<float>(tNormalized) * durationSeconds;
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
