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

// Procedural idle animation for fish rig.
//
// Designed for game development idle/rest state. The fish hovers in
// place with subtle body undulation, pectoral fin sculling, and gentle
// tail sway to maintain position. Works for any fish, from goldfish
// to sharks, using the fish bone structure.
//
// Adjustable animation parameters:
//   - breathingAmplitude:    gill/body pulse intensity
//   - breathingSpeed:        breathing cycle speed multiplier
//   - finScullFactor:        pectoral fin sculling amplitude
//   - tailSwayFactor:        tail gentle sway amplitude
//   - bodyUndulationFactor:  subtle body S-curve undulation
//   - dorsalSwayFactor:      dorsal/ventral fin sway
//   - driftFactor:           gentle vertical drift (hovering)

#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/common.h>
#include <dust3d/animation/fish/idle.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace fish {

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
            "Root", "Head", "BodyFront", "BodyMid", "BodyRear",
            "TailStart", "TailEnd"
        };
        if (!validateRequiredBones(boneIdx, requiredBones, sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;

        double breathingAmplitude = parameters.getValue("breathingAmplitude", 1.0);
        double breathingSpeed = parameters.getValue("breathingSpeed", 1.0);
        double finScullFactor = parameters.getValue("finScullFactor", 1.0);
        double tailSwayFactor = parameters.getValue("tailSwayFactor", 1.0);
        double bodyUndulationFactor = parameters.getValue("bodyUndulationFactor", 1.0);
        double dorsalSwayFactor = parameters.getValue("dorsalSwayFactor", 1.0);
        double driftFactor = parameters.getValue("driftFactor", 1.0);

        // Fish: forward is typically +Z (head toward +Z), up is +Y
        // lateral is X axis
        Vector3 upDir(0.0, 1.0, 0.0);
        Vector3 headPos = bonePos("Head");
        Vector3 tailEnd = boneEnd("TailEnd");
        Vector3 bodyAxis = headPos - tailEnd;
        Vector3 forward(bodyAxis.x(), 0.0, bodyAxis.z());
        if (forward.lengthSquared() < 1e-8)
            forward = Vector3(0.0, 0.0, 1.0);
        forward.normalize();
        Vector3 lateral(1.0, 0.0, 0.0); // fish sways laterally on X

        double bodyLength = bodyAxis.length();
        if (bodyLength < 1e-6)
            bodyLength = 1.0;

        double driftAmp = bodyLength * 0.005 * driftFactor;

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            double tNormalized = static_cast<double>(frame) / static_cast<double>(frameCount);

            double breathPhase = tNormalized * 2.0 * Math::Pi * breathingSpeed;
            double vertDrift = driftAmp * std::sin(tNormalized * 2.0 * Math::Pi * 0.3);

            Matrix4x4 bodyTransform;
            bodyTransform.translate(upDir * vertDrift);

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
                        extraRot.rotate(lateral, extraPitch);
                    Vector3 offset = newEnd - newPos;
                    newEnd = newPos + extraRot.transformVector(offset);
                }
                boneWorldTransforms[name] = buildBoneWorldTransform(newPos, newEnd);
            };

            // Body undulation: subtle S-curve that propagates backward
            double undulationPhase = tNormalized * 2.0 * Math::Pi * breathingSpeed * 0.8;
            double headSway = 0.01 * bodyUndulationFactor * std::sin(undulationPhase);
            double frontSway = 0.015 * bodyUndulationFactor * std::sin(undulationPhase - 0.4);
            double midSway = 0.02 * bodyUndulationFactor * std::sin(undulationPhase - 0.8);
            double rearSway = 0.03 * bodyUndulationFactor * std::sin(undulationPhase - 1.2);

            computeBone("Root");
            computeBone("Head", headSway);
            computeBone("BodyFront", frontSway);
            computeBone("BodyMid", midSway);
            computeBone("BodyRear", rearSway);

            // Tail: gentle sway
            double tailSway1 = 0.04 * tailSwayFactor * std::sin(undulationPhase - 1.6);
            double tailSway2 = 0.06 * tailSwayFactor * std::sin(undulationPhase - 2.0);
            computeBone("TailStart", tailSway1);
            computeBone("TailEnd", tailSway2);

            // Pectoral fins: sculling motion
            if (boneIdx.count("LeftPectoralFin")) {
                double finAngle = 0.15 * finScullFactor * std::sin(breathPhase * 1.5);
                computeBone("LeftPectoralFin", 0.0, finAngle);
            }
            if (boneIdx.count("RightPectoralFin")) {
                double finAngle = 0.15 * finScullFactor * std::sin(breathPhase * 1.5 + Math::Pi);
                computeBone("RightPectoralFin", 0.0, finAngle);
            }

            // Pelvic fins
            if (boneIdx.count("LeftPelvicFin")) {
                double finAngle = 0.08 * finScullFactor * std::sin(breathPhase + 0.5);
                computeBone("LeftPelvicFin", 0.0, finAngle);
            }
            if (boneIdx.count("RightPelvicFin")) {
                double finAngle = 0.08 * finScullFactor * std::sin(breathPhase + 0.5 + Math::Pi);
                computeBone("RightPelvicFin", 0.0, finAngle);
            }

            // Dorsal/ventral fins
            static const char* dorsalBones[] = { "DorsalFinFront", "DorsalFinMid", "DorsalFinRear" };
            static const char* ventralBones[] = { "VentralFinFront", "VentralFinMid", "VentralFinRear" };
            for (int fi = 0; fi < 3; ++fi) {
                if (boneIdx.count(dorsalBones[fi])) {
                    double sway = 0.03 * dorsalSwayFactor * std::sin(breathPhase * 0.7 - fi * 0.5);
                    computeBone(dorsalBones[fi], sway);
                }
                if (boneIdx.count(ventralBones[fi])) {
                    double sway = 0.03 * dorsalSwayFactor * std::sin(breathPhase * 0.7 - fi * 0.5);
                    computeBone(ventralBones[fi], sway);
                }
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

} // namespace fish

} // namespace dust3d
