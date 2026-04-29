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
// Designed for game development idle/rest state.
// Tail bones (Tail1 through TailTip) remain completely static and grounded.
// Spine bones (Spine1-Spine6) form a subtle reversed S-curve that lifts the
// body upward. The S-curve peaks in the middle of the spine and tapers at
// both ends. Spine6 points between UP and FRONT, and the head bone points
// directly forward.
//
// Adjustable animation parameters:
//   - breathingAmplitudeFactor: body pulse intensity
//   - breathingSpeedFactor:     breathing cycle speed multiplier
//   - headSwayFactor:           head lateral sway amplitude
//   - headLiftHeight:           how high the head is lifted (0.0 - 1.0)
//   - tongueFlickFactor:        jaw open/close simulating tongue flick
//   - bodyUndulationFactor:     subtle body wave at rest

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
        double headLiftHeight = parameters.getValue("headLiftHeight", 0.0);
        double tongueFlickFactor = parameters.getValue("tongueFlickFactor", 1.0);
        double bodyUndulationFactor = parameters.getValue("bodyUndulationFactor", 1.0);

        Vector3 worldUp(0.0, 1.0, 0.0);

        // Compute forward direction from Spine1 to Head (projected to XZ plane)
        Vector3 headPos = bonePos("Head");
        Vector3 spine1Pos = bonePos("Spine1");
        Vector3 bodyDir = headPos - spine1Pos;
        Vector3 forward(bodyDir.x(), 0.0, bodyDir.z());
        if (forward.lengthSquared() < 1e-8)
            forward = Vector3(0.0, 0.0, 1.0);
        forward.normalize();

        Vector3 right = Vector3::crossProduct(worldUp, forward).normalized();
        if (right.lengthSquared() < 1e-8)
            right = Vector3(1.0, 0.0, 0.0);

        // Total length of the spine chain
        double totalSpineLen = 0.0;
        static const char* spineBones[] = { "Spine1", "Spine2", "Spine3", "Spine4", "Spine5", "Spine6" };
        static const char* tailBones[] = { "Tail1", "Tail2", "Tail3", "Tail4", "TailTip" };
        const int spineCount = 6;
        for (int si = 0; si < spineCount; ++si)
            totalSpineLen += (boneEnd(spineBones[si]) - bonePos(spineBones[si])).length();

        if (totalSpineLen < 1e-6)
            totalSpineLen = 1.0;

        // Breathing is expressed as a subtle pitch wiggle (radians)
        double breathAmpAngle = 0.02 * breathingAmplitudeFactor;

        // Maximum cumulative pitch at Spine6's end (0 to ~45°)
        double maxSpine6Pitch = 0.8;
        double targetPitch = maxSpine6Pitch * headLiftHeight;

        // S-curve weight distribution for pitch: peaks in middle, tapers at ends
        double pitchWeights[] = { 0.5, 1.0, 1.5, 1.5, 1.0, 0.5 };
        double pitchWeightSum = 0.0;
        for (double w : pitchWeights)
            pitchWeightSum += w;

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            double tNormalized = static_cast<double>(frame) / static_cast<double>(frameCount);

            double breathPhase = tNormalized * 2.0 * Math::Pi * breathingSpeedFactor;
            double breathPitch = breathAmpAngle * std::sin(breathPhase);

            double undulationPhase = tNormalized * 2.0 * Math::Pi * breathingSpeedFactor;

            std::map<std::string, Matrix4x4> boneWorldTransforms;

            // Root: stays fixed
            boneWorldTransforms["Root"] = buildBoneWorldTransform(bonePos("Root"), boneEnd("Root"));

            // Tail bones: completely static, remain in bind pose (grounded)
            for (int ti = 0; ti < 5; ++ti) {
                const char* name = tailBones[ti];
                boneWorldTransforms[name] = buildBoneWorldTransform(bonePos(name), boneEnd(name));
            }

            // Spine chain: subtle reversed S-curve lift.
            // Pitch follows a bell curve (peaks in middle) so the spine curves
            // up steeply in the middle and flattens toward both ends.
            // Yaw adds a subtle lateral S-curve (reverses direction halfway).
            Vector3 currentPos = bonePos("Spine1");
            Vector3 currentDir = (boneEnd("Spine1") - bonePos("Spine1")).normalized();

            for (int si = 0; si < spineCount; ++si) {
                const char* name = spineBones[si];
                double boneLen = (boneEnd(name) - bonePos(name)).length();

                // Local pitch: negative = upward (toward +Y)
                double localPitch = -targetPitch * pitchWeights[si] / pitchWeightSum;

                // Undulation yaw (existing subtle wave)
                double undulationYaw = 0.015 * bodyUndulationFactor * std::sin(undulationPhase - si * 0.5);

                // Reversed S-curve yaw: subtle lateral curve that reverses
                // direction at the midpoint of the spine
                double sCurveT = (spineCount > 1) ? static_cast<double>(si) / static_cast<double>(spineCount - 1) : 0.0;
                double sCurveYaw = 0.03 * headLiftHeight * std::sin(2.0 * Math::Pi * sCurveT);

                Matrix4x4 rot;
                rot.rotate(right, localPitch);
                rot.rotate(worldUp, undulationYaw + sCurveYaw);
                currentDir = rot.transformVector(currentDir);
                if (!currentDir.isZero())
                    currentDir.normalize();

                Vector3 newEnd = currentPos + currentDir * boneLen;
                boneWorldTransforms[name] = buildBoneWorldTransform(currentPos, newEnd);
                currentPos = newEnd;
            }

            // Head: sits at Spine6's end and points directly forward.
            // We take the horizontal component of the spine's final direction
            // so the head follows the neck's lateral heading while staying flat.
            {
                double headLen = (boneEnd("Head") - bonePos("Head")).length();

                Vector3 headDir = currentDir;
                headDir = Vector3(headDir.x(), 0.0, headDir.z());
                if (headDir.lengthSquared() < 1e-8)
                    headDir = forward;
                else
                    headDir.normalize();

                double headYaw = 0.05 * headSwayFactor * std::sin(tNormalized * 2.0 * Math::Pi * breathingSpeedFactor);

                Matrix4x4 rot;
                rot.rotate(right, breathPitch);
                rot.rotate(worldUp, headYaw);
                headDir = rot.transformVector(headDir);
                if (!headDir.isZero())
                    headDir.normalize();

                Vector3 headEnd = currentPos + headDir * headLen;
                boneWorldTransforms["Head"] = buildBoneWorldTransform(currentPos, headEnd);

                // Jaw: follows head position and orientation with tongue flick
                if (boneIdx.count("Jaw")) {
                    Vector3 jawOrigPos = bonePos("Jaw");
                    Vector3 jawOrigEnd = boneEnd("Jaw");
                    double jawLen = (jawOrigEnd - jawOrigPos).length();
                    Vector3 jawOffset = jawOrigPos - bonePos("Head");

                    Vector3 jawPos = currentPos + rot.transformVector(jawOffset);

                    double flickPhase = tNormalized * 2.0 * Math::Pi * 2.0 * breathingSpeedFactor;
                    double flickPulse = std::max(0.0, std::sin(flickPhase));
                    flickPulse = flickPulse * flickPulse * flickPulse;
                    double jawAngle = 0.04 * tongueFlickFactor * flickPulse;

                    Matrix4x4 jawRot;
                    jawRot.rotate(right, breathPitch + jawAngle);
                    jawRot.rotate(worldUp, headYaw);
                    Vector3 jawDir = jawRot.transformVector(headDir);
                    if (!jawDir.isZero())
                        jawDir.normalize();

                    Vector3 jawEnd = jawPos + jawDir * jawLen;
                    boneWorldTransforms["Jaw"] = buildBoneWorldTransform(jawPos, jawEnd);
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

} // namespace snake

} // namespace dust3d
