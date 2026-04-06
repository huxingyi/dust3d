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
// Designed for game development idle/rest state. The bird stands in
// place with subtle body bobbing, head pecking/looking, wing folding,
// and tail feather movement. Works for chickens, eagles, penguins,
// parrots, and any bird using the bird bone structure.
//
// Adjustable animation parameters:
//   - breathingAmplitude:   body rise-fall intensity
//   - breathingSpeed:       breathing cycle speed multiplier
//   - headLookFactor:       head turn/peek amplitude
//   - headPeckFactor:       head pecking motion intensity
//   - wingFoldFactor:       wing micro-fold/unfold
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

        double breathingAmplitude = parameters.getValue("breathingAmplitude", 1.0);
        double breathingSpeed = parameters.getValue("breathingSpeed", 1.0);
        double headLookFactor = parameters.getValue("headLookFactor", 1.0);
        double headPeckFactor = parameters.getValue("headPeckFactor", 1.0);
        double wingFoldFactor = parameters.getValue("wingFoldFactor", 1.0);
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
        double breathAmp = bodyHeight * 0.008 * breathingAmplitude;
        double shiftAmp = bodyHeight * 0.006 * weightShiftFactor;

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            double tNormalized = static_cast<double>(frame) / static_cast<double>(frameCount);

            double breathPhase = tNormalized * 2.0 * Math::Pi * breathingSpeed;
            double breathOffset = breathAmp * std::sin(breathPhase);
            double lateralShift = shiftAmp * std::sin(tNormalized * 2.0 * Math::Pi * 0.4);

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

            // Head: looking around + subtle peck
            double headYaw = 0.06 * headLookFactor * std::sin(tNormalized * 2.0 * Math::Pi * 0.5);
            double headPeck = 0.04 * headPeckFactor * std::max(0.0, std::sin(tNormalized * 2.0 * Math::Pi * 1.5));

            computeBone("Root");
            computeBone("Pelvis");
            computeBone("Spine");
            computeBone("Chest", 0.0, breathOffset * 0.2 / (bodyHeight * 0.01 + 1e-6));
            computeBone("Neck", headYaw * 0.3, headPeck * 0.5);
            computeBone("Head", headYaw, headPeck);

            // Beak
            if (boneIdx.count("Beak")) {
                computeBone("Beak", 0.0, headPeck * 0.2);
            }

            // Tail feathers
            if (boneIdx.count("TailBase")) {
                double tailYaw = 0.04 * tailFeatherFactor * std::sin(tNormalized * 2.0 * Math::Pi * 0.6);
                computeBone("TailBase", tailYaw);
            }
            if (boneIdx.count("TailFeathers")) {
                double tailYaw = 0.06 * tailFeatherFactor * std::sin(tNormalized * 2.0 * Math::Pi * 0.6 - 0.5);
                computeBone("TailFeathers", tailYaw);
            }

            // Wings: subtle fold
            auto computeWingIdle = [&](const char* shoulder, const char* elbow, const char* hand, double sign) {
                double wingAngle = 0.02 * wingFoldFactor * std::sin(breathPhase * 0.5) * sign;
                computeBone(shoulder, 0.0, wingAngle);
                computeBone(elbow, 0.0, wingAngle * 0.5);
                computeBone(hand, 0.0, wingAngle * 0.3);
            };
            computeWingIdle("LeftWingShoulder", "LeftWingElbow", "LeftWingHand", 1.0);
            computeWingIdle("RightWingShoulder", "RightWingElbow", "RightWingHand", -1.0);

            // Legs: stationary
            static const char* legBones[] = {
                "LeftUpperLeg", "LeftLowerLeg", "LeftFoot",
                "RightUpperLeg", "RightLowerLeg", "RightFoot"
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

} // namespace bird

} // namespace dust3d
