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
//   - breathingAmplitudeFactor:    gill/body pulse intensity
//   - breathingSpeedFactor:        breathing cycle speed multiplier
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
        const std::map<std::string, Matrix4x4>& /* inverseBindMatrices */,
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

        double breathingSpeedFactor = parameters.getValue("breathingSpeedFactor", 1.0);
        double finScullFactor = parameters.getValue("finScullFactor", 1.0);
        double tailSwayFactor = parameters.getValue("tailSwayFactor", 1.0);
        double bodyUndulationFactor = parameters.getValue("bodyUndulationFactor", 1.0);
        double dorsalSwayFactor = parameters.getValue("dorsalSwayFactor", 1.0);
        double driftFactor = parameters.getValue("driftFactor", 1.0);

        Vector3 upDir(0.0, 1.0, 0.0);
        Vector3 headPos = bonePos("Head");
        Vector3 tailEndPos = boneEnd("TailEnd");
        Vector3 bodyAxis = tailEndPos - headPos;
        if (bodyAxis.isZero())
            return false;

        Vector3 forwardDir = bodyAxis.normalized();
        Vector3 right = Vector3::crossProduct(forwardDir, upDir);
        if (right.lengthSquared() < 1e-8)
            right = Vector3::crossProduct(forwardDir, Vector3(0.0, 0.0, 1.0));
        right.normalize();

        double bodyLength = bodyAxis.length();
        if (bodyLength < 1e-6)
            bodyLength = 1.0;

        double driftAmp = bodyLength * 0.005 * driftFactor;

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            double tNormalized = static_cast<double>(frame) / static_cast<double>(frameCount);

            double breathPhase = tNormalized * 2.0 * Math::Pi * breathingSpeedFactor;
            double vertDrift = driftAmp * std::sin(tNormalized * 2.0 * Math::Pi);

            // Body undulation: subtle S-curve that propagates backward
            double undulationPhase = tNormalized * 2.0 * Math::Pi * breathingSpeedFactor;

            std::map<std::string, Matrix4x4> boneWorldTransforms;

            // Build skin matrix directly as a delta transform (no inverseBindMatrices).
            // skinMat = T(vertDrift) * T(pivot) * R(yaw) * T(-pivot)
            // where pivot is the bone position, and yaw rotates around upDir.
            auto computeBoneSkinMat = [&](const std::string& name,
                                          double yawAngle = 0.0,
                                          double pitchAngle = 0.0) {
                if (boneIdx.count(name) == 0)
                    return;
                Vector3 pos = bonePos(name);
                Vector3 end = boneEnd(name);

                Matrix4x4 skinMat;
                skinMat.translate(upDir * vertDrift);
                if (std::abs(yawAngle) > 1e-6 || std::abs(pitchAngle) > 1e-6) {
                    skinMat.translate(pos);
                    if (std::abs(yawAngle) > 1e-6)
                        skinMat.rotate(upDir, yawAngle);
                    if (std::abs(pitchAngle) > 1e-6)
                        skinMat.rotate(right, pitchAngle);
                    skinMat.translate(-pos);
                }

                animationClip.frames[frame].boneSkinMatrices[name] = skinMat;

                Matrix4x4 animBoneTransform = skinMat;
                animBoneTransform *= buildBoneWorldTransform(pos, end);
                boneWorldTransforms[name] = animBoneTransform;
            };

            double headSway = 0.01 * bodyUndulationFactor * std::sin(undulationPhase);
            double frontSway = 0.015 * bodyUndulationFactor * std::sin(undulationPhase - 0.4);
            double midSway = 0.02 * bodyUndulationFactor * std::sin(undulationPhase - 0.8);
            double rearSway = 0.03 * bodyUndulationFactor * std::sin(undulationPhase - 1.2);

            computeBoneSkinMat("Root");
            computeBoneSkinMat("Head", headSway);
            computeBoneSkinMat("BodyFront", frontSway);
            computeBoneSkinMat("BodyMid", midSway);
            computeBoneSkinMat("BodyRear", rearSway);

            // Tail: gentle sway
            double tailSway1 = 0.04 * tailSwayFactor * std::sin(undulationPhase - 1.6);
            double tailSway2 = 0.06 * tailSwayFactor * std::sin(undulationPhase - 2.0);
            computeBoneSkinMat("TailStart", tailSway1);
            computeBoneSkinMat("TailEnd", tailSway2);

            // Pectoral fins: sculling motion
            if (boneIdx.count("LeftPectoralFin")) {
                double finAngle = 0.15 * finScullFactor * std::sin(breathPhase * 2.0);
                computeBoneSkinMat("LeftPectoralFin", 0.0, finAngle);
            }
            if (boneIdx.count("RightPectoralFin")) {
                double finAngle = 0.15 * finScullFactor * std::sin(breathPhase * 2.0 + Math::Pi);
                computeBoneSkinMat("RightPectoralFin", 0.0, finAngle);
            }

            // Pelvic fins
            if (boneIdx.count("LeftPelvicFin")) {
                double finAngle = 0.08 * finScullFactor * std::sin(breathPhase + 0.5);
                computeBoneSkinMat("LeftPelvicFin", 0.0, finAngle);
            }
            if (boneIdx.count("RightPelvicFin")) {
                double finAngle = 0.08 * finScullFactor * std::sin(breathPhase + 0.5 + Math::Pi);
                computeBoneSkinMat("RightPelvicFin", 0.0, finAngle);
            }

            // Dorsal/ventral fins
            static const char* dorsalBones[] = { "DorsalFinFront", "DorsalFinMid", "DorsalFinRear" };
            static const char* ventralBones[] = { "VentralFinFront", "VentralFinMid", "VentralFinRear" };
            for (int fi = 0; fi < 3; ++fi) {
                if (boneIdx.count(dorsalBones[fi])) {
                    double sway = 0.03 * dorsalSwayFactor * std::sin(breathPhase - fi * 0.5);
                    computeBoneSkinMat(dorsalBones[fi], sway);
                }
                if (boneIdx.count(ventralBones[fi])) {
                    double sway = 0.03 * dorsalSwayFactor * std::sin(breathPhase - fi * 0.5);
                    computeBoneSkinMat(ventralBones[fi], sway);
                }
            }

            auto& animFrame = animationClip.frames[frame];
            animFrame.time = static_cast<float>(tNormalized) * durationSeconds;
            animFrame.boneWorldTransforms = boneWorldTransforms;
        }

        return true;
    }

} // namespace fish

} // namespace dust3d
