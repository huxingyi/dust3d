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

// Procedural idle animation for snake rig.
//
// Designed for game development idle/rest state. The snake rests coiled
// or loosely positioned with subtle breathing (body diameter pulse),
// gentle head sway, tongue flick simulation, and minor body undulation.
// Works for snakes, worms, eels, and any legless serpentine creature.
//
// Adjustable animation parameters:
//   - breathingAmplitudeFactor:   body pulse intensity
//   - breathingSpeedFactor:       breathing cycle speed multiplier
//   - headSwayFactor:       head lateral sway amplitude
//   - headRaiseFactor:      head raise/lower amplitude
//   - tongueFlickFactor:    jaw open/close simulating tongue flick
//   - bodyUndulationFactor: subtle body wave at rest
//   - tailTwitchFactor:     tail tip twitch amplitude

#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/common.h>
#include <dust3d/animation/snake/idle.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace snake {

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
            "Root", "Head",
            "Spine1", "Spine2", "Spine3", "Spine4", "Spine5", "Spine6",
            "Tail1", "Tail2", "Tail3", "Tail4", "TailTip"
        };
        if (!validateRequiredBones(boneIdx, requiredBones, sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;

        double breathingAmplitudeFactor = parameters.getValue("breathingAmplitudeFactor", 1.0);
        double breathingSpeedFactor = parameters.getValue("breathingSpeedFactor", 1.0);
        double headSwayFactor = parameters.getValue("headSwayFactor", 1.0);
        double headRaiseFactor = parameters.getValue("headRaiseFactor", 1.0);
        double tongueFlickFactor = parameters.getValue("tongueFlickFactor", 1.0);
        double bodyUndulationFactor = parameters.getValue("bodyUndulationFactor", 1.0);
        double tailTwitchFactor = parameters.getValue("tailTwitchFactor", 1.0);

        Vector3 upDir(0.0, 1.0, 0.0);
        Vector3 headPos = bonePos("Head");
        Vector3 spine1Pos = bonePos("Spine1");
        Vector3 bodyDir = headPos - spine1Pos;
        Vector3 forward(bodyDir.x(), 0.0, bodyDir.z());
        if (forward.lengthSquared() < 1e-8)
            forward = Vector3(0.0, 0.0, 1.0);
        forward.normalize();

        double bodyLength = (headPos - boneEnd("TailTip")).length();
        if (bodyLength < 1e-6)
            bodyLength = 1.0;
        double breathAmp = bodyLength * 0.003 * breathingAmplitudeFactor;

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            double tNormalized = static_cast<double>(frame) / static_cast<double>(frameCount);

            double breathPhase = tNormalized * 2.0 * Math::Pi * breathingSpeedFactor;
            double breathOffset = breathAmp * std::sin(breathPhase);

            Matrix4x4 bodyTransform;
            bodyTransform.translate(upDir * breathOffset);

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
                    if (std::abs(extraPitch) > 1e-6) {
                        Vector3 right = Vector3::crossProduct(upDir, forward);
                        if (right.lengthSquared() > 1e-8)
                            extraRot.rotate(right.normalized(), extraPitch);
                    }
                    Vector3 offset = newEnd - newPos;
                    newEnd = newPos + extraRot.transformVector(offset);
                }
                boneWorldTransforms[name] = buildBoneWorldTransform(newPos, newEnd);
            };

            computeBone("Root");

            // Head: sway and raise
            double headYaw = 0.05 * headSwayFactor * std::sin(tNormalized * 2.0 * Math::Pi * 0.4);
            double headPitch = 0.03 * headRaiseFactor * std::sin(tNormalized * 2.0 * Math::Pi * 0.6 + 0.5);
            computeBone("Head", headYaw, headPitch);

            // Jaw: tongue flick (periodic quick open/close)
            if (boneIdx.count("Jaw")) {
                // Quick flick: a sharp pulse every ~2 seconds
                double flickPhase = tNormalized * 2.0 * Math::Pi * 2.0 * breathingSpeedFactor;
                double flickPulse = std::max(0.0, std::sin(flickPhase));
                flickPulse = flickPulse * flickPulse * flickPulse; // sharpen the pulse
                double jawAngle = 0.04 * tongueFlickFactor * flickPulse;
                computeBone("Jaw", 0.0, jawAngle);
            }

            // Spine: subtle undulation wave
            static const char* spineBones[] = { "Spine1", "Spine2", "Spine3", "Spine4", "Spine5", "Spine6" };
            double undulationPhase = tNormalized * 2.0 * Math::Pi * breathingSpeedFactor * 0.5;
            for (int si = 0; si < 6; ++si) {
                double spineYaw = 0.015 * bodyUndulationFactor * std::sin(undulationPhase - si * 0.5);
                computeBone(spineBones[si], spineYaw);
            }

            // Tail: gentle twitch, propagating wave
            static const char* tailBones[] = { "Tail1", "Tail2", "Tail3", "Tail4", "TailTip" };
            for (int ti = 0; ti < 5; ++ti) {
                double tailYaw = 0.02 * tailTwitchFactor * std::sin(undulationPhase - (6 + ti) * 0.5);
                computeBone(tailBones[ti], tailYaw);
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

} // namespace snake

} // namespace dust3d
