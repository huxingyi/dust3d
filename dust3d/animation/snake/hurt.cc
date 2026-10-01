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

// Procedural hurt (hit reaction) animation for the snake rig.
//
// A one-shot clip for game use. On the impact the front of the body flinches
// back and up away from the blow, the middle of the body is knocked sideways
// into a C-bend, the jaw gapes in a hiss and the tail thrashes. After a short
// hit-stop the body settles back to rest with one small overshoot. The clip
// starts and ends exactly in the rest pose, so it blends in from any loop
// (idle, slither) and hands back to it without a pop.
//
// Adjustable animation parameters:
//   - recoilFactor:       how far the head and neck flinch back and up
//   - kinkFactor:         how far the middle of the body is knocked sideways
//   - hitDirection:       where the blow comes from (-1 = left, 0 = front, 1 = right)
//   - jawOpenFactor:      how wide the jaw gapes
//   - tailThrashFactor:   how hard the tail thrashes
//   - recoverySpeed:      how quickly the body settles back (> 1 = sooner)

#include <algorithm>
#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/common.h>
#include <dust3d/animation/snake/hurt.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace snake {

    bool hurt(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = static_cast<int>(parameters.getValue("frameCount", 24));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 0.8));

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

        double recoilFactor = parameters.getValue("recoilFactor", 1.0);
        double kinkFactor = parameters.getValue("kinkFactor", 1.0);
        double hitDirection = std::clamp(parameters.getValue("hitDirection", 0.5), -1.0, 1.0);
        double jawOpenFactor = parameters.getValue("jawOpenFactor", 1.0);
        double tailThrashFactor = parameters.getValue("tailThrashFactor", 1.0);
        double recoverySpeed = parameters.getValue("recoverySpeed", 1.0);

        double recoil = 0.09 * bodyLength * recoilFactor * (1.0 - 0.4 * std::abs(hitDirection));
        double lift = 0.08 * bodyLength * recoilFactor;
        // A blow from the front still kinks the body a little; the side is the right by default.
        double side = std::abs(hitDirection) < 0.05 ? 0.35 : hitDirection;
        double kink = 0.07 * bodyLength * kinkFactor * side;
        double thrash = 0.035 * bodyLength * tailThrashFactor;
        double jawOpen = 0.5 * jawOpenFactor;
        double headPitch = 0.35 * recoilFactor;

        static const std::vector<std::string> chain = {
            "TailTip", "Tail4", "Tail3", "Tail2", "Tail1",
            "Spine1", "Spine2", "Spine3", "Spine4", "Spine5", "Spine6"
        };

        double snap = hitSnapFraction(durationSeconds);

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            // Reaches t = 1 on the last frame, which is the rest pose.
            double t = static_cast<double>(frame) / static_cast<double>(std::max(1, frameCount - 1));
            double env = hitReactionEnvelope(t, recoverySpeed, snap);
            double lifted = std::max(0.0, env);
            double shudder = hitShudder(t);
            double thrashEnv = smoothstep(t / 0.08) * std::exp(-2.5 * t) * (1.0 - smoothstep((t - 0.55) / 0.45));

            auto offsetAt = [&](const Vector3& p) -> Vector3 {
                double u = std::clamp(Vector3::dotProduct(p - tailPos, forward) / bodyLength, 0.0, 1.0);
                double front = smoothstep((u - 0.5) / 0.5);
                Vector3 offset = forward * (-recoil * front * env)
                    + up * (lift * std::pow(front, 1.5) * lifted)
                    - right * (kink * std::sin(Math::Pi * u) * env)
                    + right * (thrash * (1.0 - u) * (1.0 - u) * std::sin(2.0 * Math::Pi * 3.0 * t - 5.0 * u) * thrashEnv)
                    + right * (0.012 * bodyLength * shudder * (0.3 + 0.7 * u));
                return offset;
            };

            std::map<std::string, Matrix4x4> boneWorldTransforms = restBoneWorldTransforms(rigStructure);
            for (const auto& name : chain) {
                Vector3 p = bonePos(name);
                Vector3 e = boneEnd(name);
                boneWorldTransforms[name] = buildBoneWorldTransform(p + offsetAt(p), e + offsetAt(e));
            }

            // The head flinches up and away and turns from the blow.
            Vector3 headStart = headPos + offsetAt(headPos);
            Vector3 headDir = boneEnd("Head") - headPos;
            Matrix4x4 headRot;
            headRot.rotate(up, 0.25 * side * env);
            headRot.rotate(right, headPitch * env);
            boneWorldTransforms["Head"] = buildBoneWorldTransform(headStart, headStart + headRot.transformVector(headDir));

            if (boneIdx.count("Jaw")) {
                Vector3 jawPos = bonePos("Jaw");
                Vector3 jawDir = boneEnd("Jaw") - jawPos;
                Vector3 jawStart = headStart + headRot.transformVector(jawPos - headPos);
                // The gape opens with the flinch and closes a little later than the body settles.
                double open = jawOpen * lifted * (1.0 - smoothstep((t - 0.5) / 0.4));
                Matrix4x4 jawRot = headRot;
                jawRot.rotate(right, -open);
                boneWorldTransforms["Jaw"] = buildBoneWorldTransform(jawStart, jawStart + jawRot.transformVector(jawDir));
            }

            auto& animFrame = animationClip.frames[frame];
            animFrame.time = static_cast<float>(frame) / static_cast<float>(frameCount) * durationSeconds;
            animFrame.boneWorldTransforms = boneWorldTransforms;
            finishFrame(animFrame, inverseBindMatrices);
        }
        return true;
    }

} // namespace snake

} // namespace dust3d
