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

#include <algorithm>
#include <cmath>
#include <dust3d/animation/common.h>
#include <dust3d/animation/fish/die.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace fish {

    bool die(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& /* inverseBindMatrices */,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        int frameCount = static_cast<int>(parameters.getValue("frameCount", 30.0));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 2.0));

        std::map<std::string, size_t> boneIdx;
        for (size_t i = 0; i < rigStructure.bones.size(); ++i)
            boneIdx[rigStructure.bones[i].name] = i;

        auto getBonePos = [&](const std::string& name) -> Vector3 {
            auto it = boneIdx.find(name);
            if (it == boneIdx.end())
                return Vector3();
            const auto& b = rigStructure.bones[it->second];
            return Vector3(b.posX, b.posY, b.posZ);
        };

        auto getBoneEnd = [&](const std::string& name) -> Vector3 {
            auto it = boneIdx.find(name);
            if (it == boneIdx.end())
                return Vector3();
            const auto& b = rigStructure.bones[it->second];
            return Vector3(b.endX, b.endY, b.endZ);
        };

        static const char* requiredBones[] = {
            "Root", "Head", "BodyFront", "BodyMid", "BodyRear", "TailStart", "TailEnd"
        };

        for (const char* name : requiredBones) {
            if (boneIdx.find(name) == boneIdx.end())
                return false;
        }

        Vector3 bodyFront = getBonePos("Head");
        Vector3 bodyBack = getBoneEnd("TailEnd");
        Vector3 bodyVector = bodyBack - bodyFront;
        if (bodyVector.isZero())
            return false;

        Vector3 forwardDir = bodyVector.normalized();
        Vector3 worldUp(0.0, 1.0, 0.0);
        Vector3 right = Vector3::crossProduct(forwardDir, worldUp);
        if (right.lengthSquared() < 1e-8)
            right = Vector3::crossProduct(forwardDir, Vector3(0.0, 0.0, 1.0));
        right.normalize();
        Vector3 up = Vector3::crossProduct(right, forwardDir).normalized();

        double bodyLength = bodyVector.length();

        // User-exposed parameters
        double hitIntensityFactor = parameters.getValue("hitIntensityFactor", 1.0);
        double hitFrequency = parameters.getValue("hitFrequency", 8.0);
        double flipSpeedFactor = parameters.getValue("flipSpeedFactor", 1.0);
        // Max roll angle in degrees (0–180). Default 180 = fully belly-up.
        double flipAngleDeg = parameters.getValue("flipAngle", 180.0);
        double flipAngleMax = flipAngleDeg * (Math::Pi / 180.0);
        // Tilt: signed Y-axis height difference between head and tail at rest, as a
        // fraction of body length. Positive = head up, negative = head down.
        double tilt = parameters.getValue("tilt", 0.0);
        double finFlopFactor = parameters.getValue("finFlopFactor", 1.0);
        double spinDecay = parameters.getValue("spinDecay", 4.0);

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        const std::vector<std::string> spineBones = {
            "Root", "Head", "BodyFront", "BodyMid", "BodyRear", "TailStart", "TailEnd"
        };

        // Segment tail amplification – tail thrashes more than the head during impact
        const double tailAmplificationMax = 2.5;

        for (int frame = 0; frame < frameCount; ++frame) {
            // One-shot clip: tNormalized reaches exactly 1.0 at the last frame
            double t = (frameCount > 1)
                ? static_cast<double>(frame) / static_cast<double>(frameCount - 1)
                : 0.0;

            // --- Body roll: smoothly rotate 180° (belly-up) around the spine axis ---
            // Use a logistic-style curve shaped by flipSpeedFactor so the roll accelerates
            // early and eases into the final upside-down position.
            // At t=0: rollAngle=0; at t=1: rollAngle=Math::Pi.
            // We map t through a skewed sigmoid then scale to [0, Pi].
            double rollT = std::min(t * flipSpeedFactor, 1.0);
            // Ease-in (fast start, slow settle): cubic ease-out curve
            double smoothT = 1.0 - (1.0 - rollT) * (1.0 - rollT) * (1.0 - rollT);
            // Clamp hard at exactly flipAngleMax — no overshoot, fish stays dead
            double rollAngle = flipAngleMax * smoothT;

            // --- Impact thrash: decaying oscillation starting immediately ---
            // Amplitude decays after the impact; tail end thrashes more than the head.
            double thrashDecayFactor = std::exp(-spinDecay * t);

            // --- Lateral sink drift: fish slowly sinks / drifts sideways as it dies ---
            // A gentle constant drift in the up direction (belly rising as body rolls)
            double sinkY = -bodyLength * 0.04 * t; // gentle downward drift

            // Tilt: pitch rotation (around the right axis) at the bodyFront pivot.
            // Grows from 0 at t=0 to the full configured angle at t=1.
            // tilt is in [-1, 1]; max pitch = 45 degrees.
            double tiltAngle = tilt * (Math::Pi * 0.25) * t;

            // Body transform: combine the overall roll with impact jerk translation
            // The impact jerk is largest near t=0 and decays away
            double jerkAmp = hitIntensityFactor * bodyLength * 0.12 * thrashDecayFactor;
            double jerkPhase = t * hitFrequency * 2.0 * Math::Pi;

            Matrix4x4 bodyRollMat;
            bodyRollMat.rotate(right, tiltAngle);
            bodyRollMat.rotate(forwardDir, rollAngle);

            std::map<std::string, Matrix4x4> boneWorldTransforms;
            std::map<std::string, Vector3> spineLateralOffset;

            for (size_t i = 0; i < spineBones.size(); ++i) {
                const std::string& boneName = spineBones[i];
                if (boneIdx.find(boneName) == boneIdx.end())
                    continue;

                Vector3 pos = getBonePos(boneName);
                Vector3 end = getBoneEnd(boneName);

                // Segment factor: 0 at head, 1 at tail
                double segFactor = static_cast<double>(i) / static_cast<double>(spineBones.size() - 1);

                // Thrash amplitude grows toward the tail
                double segThrashAmp = jerkAmp * (1.0 + (tailAmplificationMax - 1.0) * segFactor);
                double segPhaseShift = segFactor * Math::Pi; // traveling wave feel
                double segLateral = segThrashAmp * std::sin(jerkPhase - segPhaseShift);
                Vector3 thrash = right * segLateral;

                spineLateralOffset[boneName] = thrash;

                // Skin matrix: T(sink) * T(pivot) * R(tilt) * R(roll) * T(-pivot) * T(thrash)
                // Tilt (pitch around right axis) is applied before roll so it acts in
                // the fish's own body frame regardless of how far it has rolled.
                Matrix4x4 skinMat;
                skinMat.translate(up * sinkY);
                skinMat.translate(bodyFront);
                skinMat.rotate(right, tiltAngle);
                skinMat.rotate(forwardDir, rollAngle);
                skinMat.translate(-bodyFront);
                skinMat.translate(thrash);
                animationClip.frames[frame].boneSkinMatrices[boneName] = skinMat;

                // World transform: skinMat applied to the bind-pose bone transform.
                // Using matrix composition rather than endpoint reconstruction preserves
                // the roll orientation even for bones aligned with the rotation axis.
                Matrix4x4 animBoneTransform = skinMat;
                animBoneTransform *= animation::buildBoneWorldTransform(pos, end);
                boneWorldTransforms[boneName] = animBoneTransform;
            }

            // --- Fin flopping: fins oscillate with decay, as if flung loose ---
            auto getParentLateralOffset = [&](const char* boneName) -> Vector3 {
                auto it = boneIdx.find(boneName);
                if (it == boneIdx.end())
                    return Vector3();
                const std::string& parentName = rigStructure.bones[it->second].parent;
                auto jt = spineLateralOffset.find(parentName);
                if (jt == spineLateralOffset.end())
                    return Vector3();
                return jt->second;
            };

            // Compute fin skin matrix:
            // T(sink) * T(finRootRolled) * FinRot * T(-finRootRolled) * RollPivot * T(parentThrash)
            // where finRootRolled = RollPivot(finPos + parentThrash) (no sink in pivot)
            auto applyFinSkinMat = [&](const char* boneName, double finRotAngle) {
                if (boneIdx.count(boneName) == 0)
                    return;
                Vector3 pos = getBonePos(boneName);
                Vector3 end = getBoneEnd(boneName);
                Vector3 parentThrash = getParentLateralOffset(boneName);

                // Fin root position after thrash+roll (before sink)
                Vector3 finRootRolled = bodyFront + bodyRollMat.transformVector(pos + parentThrash - bodyFront);

                // Skin matrix built innermost-last via right-multiplication:
                Matrix4x4 skinMat;
                skinMat.translate(up * sinkY);
                skinMat.translate(finRootRolled);
                skinMat.rotate(forwardDir, finRotAngle);
                skinMat.translate(-finRootRolled);
                skinMat.translate(bodyFront);
                skinMat.rotate(right, tiltAngle);
                skinMat.rotate(forwardDir, rollAngle);
                skinMat.translate(-bodyFront);
                skinMat.translate(parentThrash);
                animationClip.frames[frame].boneSkinMatrices[boneName] = skinMat;

                // World transform: skinMat * bindPoseTransform preserves orientation
                // for all bone directions including those aligned with the roll axis.
                Matrix4x4 animFinTransform = skinMat;
                animFinTransform *= animation::buildBoneWorldTransform(pos, end);
                boneWorldTransforms[boneName] = animFinTransform;
            };

            double decayFlop = std::exp(-spinDecay * 0.6 * t);
            double decaySway = std::exp(-spinDecay * 0.4 * t);

            applyFinSkinMat("DorsalFinFront", finFlopFactor * 0.2 * decaySway * std::sin(jerkPhase + 0.1));
            applyFinSkinMat("DorsalFinMid", finFlopFactor * 0.2 * decaySway * std::sin(jerkPhase + 0.3));
            applyFinSkinMat("DorsalFinRear", finFlopFactor * 0.2 * decaySway * std::sin(jerkPhase + 0.6));
            applyFinSkinMat("VentralFinFront", finFlopFactor * 0.2 * decaySway * std::sin(jerkPhase + 0.1));
            applyFinSkinMat("VentralFinMid", finFlopFactor * 0.2 * decaySway * std::sin(jerkPhase + 0.3));
            applyFinSkinMat("VentralFinRear", finFlopFactor * 0.2 * decaySway * std::sin(jerkPhase + 0.6));

            applyFinSkinMat("LeftPectoralFin", finFlopFactor * bodyLength * 0.3 * decayFlop * std::sin(jerkPhase + 0.0));
            applyFinSkinMat("RightPectoralFin", finFlopFactor * bodyLength * 0.3 * decayFlop * -std::sin(jerkPhase + 0.0));
            applyFinSkinMat("LeftPelvicFin", finFlopFactor * bodyLength * 0.3 * decayFlop * std::sin(jerkPhase + 0.5));
            applyFinSkinMat("RightPelvicFin", finFlopFactor * bodyLength * 0.3 * decayFlop * -std::sin(jerkPhase + 0.5));

            auto& animFrame = animationClip.frames[frame];
            animFrame.time = static_cast<float>(t) * durationSeconds;
            animFrame.boneWorldTransforms = boneWorldTransforms;
            // boneSkinMatrices were written directly above; skip the endpoint-based recomputation.
        }

        return true;
    }

} // namespace fish

} // namespace dust3d
