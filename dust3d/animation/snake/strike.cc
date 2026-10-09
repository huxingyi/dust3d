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

// Procedural strike animation for the snake rig.
//
// A one-shot attack for game use: the front of the body rises and draws back
// into an S-shaped coil, the head lifts to look at the target, then the head
// and neck shoot forward with the jaw wide open, and the body settles back to
// its rest pose. The tail stays on the ground. The clip starts and ends in the
// rest pose, so it can be played back to back.
//
// Adjustable animation parameters:
//   - liftHeightFactor:    how high the front of the body rises
//   - coilFactor:          how far the neck draws back before the strike
//   - lungeDistanceFactor: how far the head shoots forward
//   - jawOpenFactor:       how wide the jaw opens on the strike
//   - strikeTimingFactor:  when the strike lands (1 = 45% into the clip)

#include <algorithm>
#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/common.h>
#include <dust3d/animation/snake/strike.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace snake {

    bool strike(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = static_cast<int>(parameters.getValue("frameCount", 30));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 0.9));

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

        // Tail tip to head is the body axis.
        Vector3 tailPos = bonePos("TailTip");
        Vector3 headPos = bonePos("Head");
        Vector3 bodyVector = headPos - tailPos;
        bodyVector.setY(0.0);
        if (bodyVector.lengthSquared() < 1e-10)
            return false;
        double bodyLength = bodyVector.length();
        Vector3 forward = bodyVector.normalized();
        Vector3 up(0.0, 1.0, 0.0);
        Vector3 right = Vector3::crossProduct(forward, up).normalized();

        double liftHeightFactor = parameters.getValue("liftHeightFactor", 1.0);
        double coilFactor = parameters.getValue("coilFactor", 1.0);
        double lungeDistanceFactor = parameters.getValue("lungeDistanceFactor", 1.0);
        double jawOpenFactor = parameters.getValue("jawOpenFactor", 1.0);
        double strikeTimingFactor = std::clamp(parameters.getValue("strikeTimingFactor", 1.0), 0.5, 1.7);

        double liftHeight = 0.26 * bodyLength * liftHeightFactor;
        double coilBack = 0.07 * bodyLength * coilFactor;
        double lunge = 0.28 * bodyLength * lungeDistanceFactor;
        double jawOpen = 0.65 * jawOpenFactor;
        double tStrike = std::clamp(0.45 * strikeTimingFactor, 0.25, 0.75);
        animationClip.events.push_back({ "hit", static_cast<float>(tStrike * durationSeconds), "Head" });

        // How much of the body takes part: nothing behind 35% of the length, all of the head.
        auto weightAt = [&](const Vector3& p) -> double {
            double u = Vector3::dotProduct(p - tailPos, forward) / bodyLength;
            return smoothstep((u - 0.35) / 0.65);
        };

        static const std::vector<std::string> chain = {
            "TailTip", "Tail4", "Tail3", "Tail2", "Tail1",
            "Spine1", "Spine2", "Spine3", "Spine4", "Spine5", "Spine6"
        };

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            double t = static_cast<double>(frame) / static_cast<double>(std::max(1, frameCount - 1));

            // Envelopes: raise, coil back, strike forward, recover.
            double raise = smoothstep(t / 0.3) * (1.0 - smoothstep((t - 0.62) / 0.36));
            double coil = smoothstep(t / std::max(0.05, tStrike - 0.08))
                * (1.0 - smoothstep((t - (tStrike - 0.06)) / 0.08));
            double hit = smoothstep((t - (tStrike - 0.07)) / 0.08)
                * (1.0 - smoothstep((t - (tStrike + 0.1)) / std::max(0.05, 0.9 - (tStrike + 0.1))));

            auto offsetAt = [&](const Vector3& p) -> Vector3 {
                double w = weightAt(p);
                double lift = liftHeight * std::pow(w, 1.5) * raise * (1.0 - 0.35 * hit);
                double thrust = -coilBack * (0.5 * w + std::sin(Math::Pi * w)) * coil
                    + lunge * w * w * hit;
                double side = 0.05 * bodyLength * std::sin(2.0 * Math::Pi * w) * coil;
                return up * lift + forward * thrust + right * side;
            };

            std::map<std::string, Matrix4x4> boneWorldTransforms;
            for (const auto& name : chain) {
                Vector3 p = bonePos(name);
                Vector3 e = boneEnd(name);
                boneWorldTransforms[name] = buildBoneWorldTransform(p + offsetAt(p), e + offsetAt(e));
            }

            // Head: follows the neck, looks up during the coil and bites down on the strike.
            Vector3 headStart = headPos + offsetAt(headPos);
            Vector3 headDir = boneEnd("Head") - headPos;
            double pitch = 0.3 * coil - 0.12 * hit;
            Matrix4x4 headRot;
            headRot.rotate(right, pitch);
            Vector3 headEnd = headStart + headRot.transformVector(headDir);
            boneWorldTransforms["Head"] = buildBoneWorldTransform(headStart, headEnd);

            if (boneIdx.count("Jaw")) {
                Vector3 jawPos = bonePos("Jaw");
                Vector3 jawDir = boneEnd("Jaw") - jawPos;
                Vector3 jawStart = headStart + headRot.transformVector(jawPos - headPos);
                double open = jawOpen * (0.25 * coil + hit);
                Matrix4x4 jawRot;
                jawRot.rotate(right, pitch - open);
                boneWorldTransforms["Jaw"] = buildBoneWorldTransform(jawStart, jawStart + jawRot.transformVector(jawDir));
            }

            for (const auto& bone : rigStructure.bones) {
                if (boneWorldTransforms.count(bone.name))
                    continue;
                boneWorldTransforms[bone.name] = buildBoneWorldTransform(
                    Vector3(bone.posX, bone.posY, bone.posZ), Vector3(bone.endX, bone.endY, bone.endZ));
            }

            auto& animFrame = animationClip.frames[frame];
            animFrame.time = static_cast<float>(frame) / static_cast<float>(frameCount) * durationSeconds;
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
