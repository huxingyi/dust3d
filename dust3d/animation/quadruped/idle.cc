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

// Procedural idle animation for quadruped rig.
//
// Designed for game development idle/rest state. The animal stands in
// place with subtle breathing, tail sway, head movement, and minor
// weight shifts. Works for horses, dogs, cats, deer, and any
// four-legged creature using the quadruped bone structure.
//
// Adjustable animation parameters:
//   - breathingAmplitude:   chest rise-fall intensity
//   - breathingSpeed:       breathing cycle speed multiplier
//   - tailIdleFactor:       tail gentle sway amplitude
//   - headLookFactor:       subtle head turn/nod amplitude
//   - earTwitchFactor:      ear twitch intensity (if ear bones exist)
//   - weightShiftFactor:    subtle lateral body sway
//   - jawFactor:            jaw open/close (chewing, panting)
//   - spineSwayFactor:      subtle spine undulation

#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/common.h>
#include <dust3d/animation/quadruped/idle.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace quadruped {

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
            "FrontLeftUpperLeg", "FrontLeftLowerLeg", "FrontLeftFoot",
            "FrontRightUpperLeg", "FrontRightLowerLeg", "FrontRightFoot",
            "BackLeftUpperLeg", "BackLeftLowerLeg", "BackLeftFoot",
            "BackRightUpperLeg", "BackRightLowerLeg", "BackRightFoot"
        };
        if (!validateRequiredBones(boneIdx, requiredBones, sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;

        double breathingAmplitude = parameters.getValue("breathingAmplitude", 1.0);
        double breathingSpeed = parameters.getValue("breathingSpeed", 1.0);
        double tailIdleFactor = parameters.getValue("tailIdleFactor", 1.0);
        double headLookFactor = parameters.getValue("headLookFactor", 1.0);
        double weightShiftFactor = parameters.getValue("weightShiftFactor", 1.0);
        double jawFactor = parameters.getValue("jawFactor", 1.0);
        double spineSwayFactor = parameters.getValue("spineSwayFactor", 1.0);

        Vector3 upDir(0.0, 1.0, 0.0);
        Vector3 pelvisPos = bonePos("Pelvis");
        Vector3 chestPos = bonePos("Chest");
        Vector3 spineDir = (chestPos - pelvisPos);
        Vector3 forward(spineDir.x(), 0.0, spineDir.z());
        if (forward.lengthSquared() < 1e-8)
            forward = Vector3(0.0, 0.0, 1.0);
        forward.normalize();
        Vector3 right = Vector3::crossProduct(upDir, forward);
        if (right.lengthSquared() < 1e-8)
            right = Vector3(1.0, 0.0, 0.0);
        right.normalize();

        // Body scale from spine length
        double spineLength = (chestPos - pelvisPos).length();
        if (spineLength < 1e-6)
            spineLength = 1.0;
        double breathAmp = spineLength * 0.01 * breathingAmplitude;
        double shiftAmp = spineLength * 0.008 * weightShiftFactor;

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            double tNormalized = static_cast<double>(frame) / static_cast<double>(frameCount);

            double breathPhase = tNormalized * 2.0 * Math::Pi * breathingSpeed;
            double breathOffset = breathAmp * std::sin(breathPhase);
            double lateralShift = shiftAmp * std::sin(tNormalized * 2.0 * Math::Pi * 0.5 * breathingSpeed);

            Matrix4x4 bodyTransform;
            bodyTransform.translate(upDir * breathOffset + right * lateralShift);

            std::map<std::string, Matrix4x4> boneWorldTransforms;

            auto computeBone = [&](const std::string& name,
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

            double spineSway = 0.012 * spineSwayFactor * std::sin(tNormalized * 2.0 * Math::Pi * 0.4);
            double headYaw = 0.04 * headLookFactor * std::sin(tNormalized * 2.0 * Math::Pi * 0.3);
            double headPitch = 0.02 * headLookFactor * std::sin(tNormalized * 2.0 * Math::Pi * 0.7 + 0.5);

            computeBone("Root");
            computeBone("Pelvis");
            computeBone("Spine", spineSway);
            computeBone("Chest", spineSway * 0.5);
            computeBone("Neck", -spineSway * 0.3, headPitch * 0.5);
            computeBone("Head", headYaw, headPitch);

            // Jaw subtle movement
            if (boneIdx.count("Jaw")) {
                double jawAngle = 0.03 * jawFactor * std::max(0.0, std::sin(tNormalized * 2.0 * Math::Pi * 1.5));
                computeBone("Jaw", 0.0, jawAngle);
            }

            // Tail idle sway
            static const char* tailBones[] = { "TailBase", "TailMid", "TailTip" };
            double tailPhase = tNormalized * 2.0 * Math::Pi * 0.9;
            for (int ti = 0; ti < 3; ++ti) {
                if (boneIdx.count(tailBones[ti]) == 0)
                    continue;
                double attenuation = 1.0 - ti * 0.25;
                double tailAngle = 0.06 * tailIdleFactor * attenuation * std::sin(tailPhase - ti * 0.7);
                computeBone(tailBones[ti], tailAngle);
            }

            // Legs: stationary, follow body
            static const char* legBones[] = {
                "FrontLeftUpperLeg", "FrontLeftLowerLeg", "FrontLeftFoot",
                "FrontRightUpperLeg", "FrontRightLowerLeg", "FrontRightFoot",
                "BackLeftUpperLeg", "BackLeftLowerLeg", "BackLeftFoot",
                "BackRightUpperLeg", "BackRightLowerLeg", "BackRightFoot"
            };
            for (const auto& boneName : legBones) {
                computeBone(boneName);
            }

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

} // namespace quadruped

} // namespace dust3d
