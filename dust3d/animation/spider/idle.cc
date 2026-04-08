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

// Procedural idle animation for spider rig.
//
// Designed for game development idle/rest state. The spider stands in
// place with subtle body bobbing, pedipalp movement, and minor leg
// adjustments. Works for spiders, crabs, scorpions, and any
// eight-legged creature using the spider bone structure.
//
// Adjustable animation parameters:
//   - breathingAmplitudeFactor:   body bob intensity
//   - breathingSpeedFactor:       breathing cycle speed multiplier
//   - pedipalpSwayFactor:   pedipalp movement amplitude
//   - legTwitchFactor:      subtle leg micro-movement
//   - abdomenPulseFactor:   abdomen expansion/contraction
//   - bodySwayFactor:       subtle lateral body sway

#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/common.h>
#include <dust3d/animation/spider/idle.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace spider {

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
            "Root", "Cephalothorax", "Head", "Abdomen",
            "FrontLeftCoxa", "FrontLeftFemur", "FrontLeftTibia",
            "FrontRightCoxa", "FrontRightFemur", "FrontRightTibia",
            "MidFrontLeftCoxa", "MidFrontLeftFemur", "MidFrontLeftTibia",
            "MidFrontRightCoxa", "MidFrontRightFemur", "MidFrontRightTibia",
            "MidBackLeftCoxa", "MidBackLeftFemur", "MidBackLeftTibia",
            "MidBackRightCoxa", "MidBackRightFemur", "MidBackRightTibia",
            "BackLeftCoxa", "BackLeftFemur", "BackLeftTibia",
            "BackRightCoxa", "BackRightFemur", "BackRightTibia"
        };
        if (!validateRequiredBones(boneIdx, requiredBones, sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;

        double breathingAmplitudeFactor = parameters.getValue("breathingAmplitudeFactor", 1.0);
        double breathingSpeedFactor = parameters.getValue("breathingSpeedFactor", 1.0);
        double pedipalpSwayFactor = parameters.getValue("pedipalpSwayFactor", 1.0);
        double legTwitchFactor = parameters.getValue("legTwitchFactor", 1.0);
        double abdomenPulseFactor = parameters.getValue("abdomenPulseFactor", 1.0);
        double bodySwayFactor = parameters.getValue("bodySwayFactor", 1.0);

        Vector3 upDir(0.0, 1.0, 0.0);
        Vector3 cephPos = bonePos("Cephalothorax");
        Vector3 headEnd = boneEnd("Head");
        Vector3 forward(headEnd.x() - cephPos.x(), 0.0, headEnd.z() - cephPos.z());
        if (forward.lengthSquared() < 1e-8)
            forward = Vector3(0.0, 0.0, 1.0);
        forward.normalize();
        Vector3 right = Vector3::crossProduct(upDir, forward);
        if (right.lengthSquared() < 1e-8)
            right = Vector3(1.0, 0.0, 0.0);
        right.normalize();

        double bodySize = (headEnd - boneEnd("Abdomen")).length();
        if (bodySize < 1e-6)
            bodySize = 0.5;
        double breathAmp = bodySize * 0.006 * breathingAmplitudeFactor;
        double swayAmp = bodySize * 0.005 * bodySwayFactor;

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            double tNormalized = static_cast<double>(frame) / static_cast<double>(frameCount);

            double breathPhase = tNormalized * 2.0 * Math::Pi * breathingSpeedFactor;
            double breathOffset = breathAmp * std::sin(breathPhase);
            double lateralSway = swayAmp * std::sin(tNormalized * 2.0 * Math::Pi * 0.4);

            Matrix4x4 bodyTransform;
            bodyTransform.translate(upDir * breathOffset + right * lateralSway);

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

            computeBone("Root");
            computeBone("Cephalothorax");
            computeBone("Head", 0.02 * std::sin(tNormalized * 2.0 * Math::Pi * 0.8));

            // Abdomen pulse
            double abdomenPulse = 0.015 * abdomenPulseFactor * std::sin(breathPhase);
            computeBone("Abdomen", 0.0, abdomenPulse);

            // Pedipalps
            if (boneIdx.count("LeftPedipalp")) {
                double palpAngle = 0.05 * pedipalpSwayFactor * std::sin(tNormalized * 2.0 * Math::Pi * 1.2);
                computeBone("LeftPedipalp", palpAngle, palpAngle * 0.3);
            }
            if (boneIdx.count("RightPedipalp")) {
                double palpAngle = 0.05 * pedipalpSwayFactor * std::sin(tNormalized * 2.0 * Math::Pi * 1.2 + 1.0);
                computeBone("RightPedipalp", palpAngle, palpAngle * 0.3);
            }

            // Legs: subtle twitching
            struct LegTriplet {
                const char* coxa;
                const char* femur;
                const char* tibia;
                double phaseOffset;
            };
            static const LegTriplet legs[] = {
                { "FrontLeftCoxa", "FrontLeftFemur", "FrontLeftTibia", 0.0 },
                { "FrontRightCoxa", "FrontRightFemur", "FrontRightTibia", 0.5 },
                { "MidFrontLeftCoxa", "MidFrontLeftFemur", "MidFrontLeftTibia", 0.25 },
                { "MidFrontRightCoxa", "MidFrontRightFemur", "MidFrontRightTibia", 0.75 },
                { "MidBackLeftCoxa", "MidBackLeftFemur", "MidBackLeftTibia", 0.33 },
                { "MidBackRightCoxa", "MidBackRightFemur", "MidBackRightTibia", 0.83 },
                { "BackLeftCoxa", "BackLeftFemur", "BackLeftTibia", 0.5 },
                { "BackRightCoxa", "BackRightFemur", "BackRightTibia", 1.0 },
            };

            for (const auto& leg : legs) {
                double legPhase = tNormalized * 2.0 * Math::Pi * 0.3 + leg.phaseOffset * Math::Pi;
                double twitch = 0.006 * legTwitchFactor * std::sin(legPhase);
                computeBone(leg.coxa, twitch);
                computeBone(leg.femur, twitch * 0.4);
                computeBone(leg.tibia);
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

} // namespace spider

} // namespace dust3d
