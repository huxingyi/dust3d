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

// Procedural hurt (hit reaction) animation for the fish rig.
//
// A one-shot clip for game use. On the impact the fish is knocked back and
// sideways away from the blow, its body bends into a C around the hit, it
// rolls to show its flank, the fins flare and the tail thrashes. After a short
// hit-stop it settles back with one small overshoot. The clip starts and ends
// exactly in the rest pose, so it blends in from and back to the swim or idle
// loop.
//
// Adjustable animation parameters:
//   - recoilFactor:     how far the fish is knocked back and sideways
//   - hitDirection:     where the blow comes from (-1 = left, 0 = front, 1 = right)
//   - kinkFactor:       how deep the C-bend is
//   - rollFactor:       how far the body rolls over
//   - tailThrashFactor: how hard the tail thrashes
//   - finFlareFactor:   how far the fins flare
//   - recoverySpeed:    how quickly the fish settles back (> 1 = sooner)

#include <algorithm>
#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/common.h>
#include <dust3d/animation/fish/hurt.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace fish {

    bool hurt(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = std::max(2, static_cast<int>(parameters.getValue("frameCount", 24)));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 0.8));

        auto boneIdx = buildBoneIndexMap(rigStructure);
        auto bonePos = [&](const std::string& name) -> Vector3 {
            return getBonePos(rigStructure, boneIdx, name);
        };
        auto boneEnd = [&](const std::string& name) -> Vector3 {
            return getBoneEnd(rigStructure, boneIdx, name);
        };

        static const char* requiredBones[] = {
            "Root", "Head", "BodyFront", "BodyMid", "BodyRear", "TailStart", "TailEnd"
        };
        if (!validateRequiredBones(boneIdx, requiredBones, sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;

        // The Head bone runs from the nose back to the body; TailEnd ends at the tail tip.
        Vector3 nose = bonePos("Head");
        Vector3 tailTip = boneEnd("TailEnd");
        Vector3 bodyVector = nose - tailTip;
        double bodyLength = bodyVector.length();
        bodyVector.setY(0.0);
        if (bodyVector.lengthSquared() < 1e-10 || bodyLength < 1e-6)
            return false;
        Vector3 forward = bodyVector.normalized();
        Vector3 up(0.0, 1.0, 0.0);
        // The creature's right; a positive rotation about `forward` lowers its right side.
        Vector3 right = Vector3::crossProduct(forward, up).normalized();
        Vector3 centre = (nose + tailTip) * 0.5;

        // 0 at the nose, 1 at the tail tip.
        auto bodyU = [&](const Vector3& p) -> double {
            return std::clamp(Vector3::dotProduct(nose - p, forward) / bodyLength, 0.0, 1.0);
        };

        static const char* spineBones[] = {
            "Root", "Head", "BodyFront", "BodyMid", "BodyRear", "TailStart", "TailEnd"
        };
        static const char* finBones[] = {
            "DorsalFinFront", "DorsalFinMid", "DorsalFinRear",
            "VentralFinFront", "VentralFinMid", "VentralFinRear",
            "LeftPectoralFin", "RightPectoralFin", "LeftPelvicFin", "RightPelvicFin"
        };
        auto isPairedFin = [](const std::string& name) {
            return name.find("Pectoral") != std::string::npos || name.find("Pelvic") != std::string::npos;
        };

        double recoilFactor = parameters.getValue("recoilFactor", 1.0);
        double hitDirection = std::clamp(parameters.getValue("hitDirection", 0.5), -1.0, 1.0);
        double kinkFactor = parameters.getValue("kinkFactor", 1.0);
        double rollFactor = parameters.getValue("rollFactor", 1.0);
        double tailThrashFactor = parameters.getValue("tailThrashFactor", 1.0);
        double finFlareFactor = parameters.getValue("finFlareFactor", 1.0);
        double recoverySpeed = parameters.getValue("recoverySpeed", 1.0);

        // A blow from straight ahead still bends the body a little, as if from the right.
        double side = std::abs(hitDirection) < 0.05 ? 0.35 : hitDirection;
        double recoil = 0.12 * bodyLength * recoilFactor * (1.0 - 0.5 * std::abs(hitDirection));
        double sideShift = 0.12 * bodyLength * recoilFactor * side;
        double kink = 0.1 * bodyLength * kinkFactor * side;
        double rollAngle = 0.45 * rollFactor * side;
        double thrash = 0.08 * bodyLength * tailThrashFactor;

        double snap = hitSnapFraction(durationSeconds);

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            double t = static_cast<double>(frame) / static_cast<double>(frameCount - 1);
            double env = hitReactionEnvelope(t, recoverySpeed, snap);
            double lifted = std::max(0.0, env);
            double shudder = hitShudder(t, 7.0);
            double thrashEnv = smoothstep(t / 0.08) * std::exp(-2.5 * t) * (1.0 - smoothstep((t - 0.55) / 0.45));

            double pairedFinAngle = 0.7 * finFlareFactor * lifted;
            double pairedFinSweep = 0.3 * finFlareFactor * lifted;
            double medianFinSway = 0.3 * finFlareFactor * shudder;

            Matrix4x4 body;
            body.translate(centre + forward * (-recoil * env) - right * (sideShift * env) - up * (0.03 * bodyLength * lifted));
            // Rolls away from the blow (the struck side rises) and the nose dips.
            body.rotate(forward, -rollAngle * env);
            body.rotate(right, -0.1 * recoilFactor * env);
            body.translate(Vector3() - centre);

            auto offsetAt = [&](const Vector3& p) -> Vector3 {
                double u = bodyU(p);
                // C-bend: the middle is pushed away from the blow, the nose and tail lag.
                double c = -kink * std::sin(Math::Pi * u) * env;
                double tail = thrash * u * u * std::sin(2.0 * Math::Pi * 3.0 * t - 4.0 * u) * thrashEnv;
                double tremble = 0.015 * bodyLength * shudder * (0.3 + 0.7 * u);
                return right * (c + tail + tremble);
            };

            std::map<std::string, Matrix4x4> world = restBoneWorldTransforms(rigStructure);
            auto place = [&](const Vector3& p) -> Vector3 {
                return body.transformPoint(p) + offsetAt(p);
            };
            for (const char* name : spineBones) {
                if (!boneIdx.count(name))
                    continue;
                world[name] = buildBoneWorldTransform(place(bonePos(name)), place(boneEnd(name)));
            }
            for (const char* name : finBones) {
                if (!boneIdx.count(name))
                    continue;
                Vector3 p = bonePos(name);
                Vector3 e = boneEnd(name);
                Vector3 start = place(p);
                Vector3 dir = body.transformVector(e - p);
                if (isPairedFin(name)) {
                    // Paired fins flare out and up from the body.
                    double finSide = Vector3::dotProduct(e - p, right) >= 0.0 ? 1.0 : -1.0;
                    Matrix4x4 rot;
                    rot.rotate(body.transformVector(forward), -finSide * pairedFinAngle);
                    rot.rotate(up, finSide * pairedFinSweep);
                    dir = rot.transformVector(dir);
                } else {
                    // Median fins stand up and sway with the body.
                    Matrix4x4 rot;
                    rot.rotate(body.transformVector(forward), medianFinSway);
                    dir = rot.transformVector(dir);
                }
                world[name] = buildBoneWorldTransform(start, start + dir);
            }

            if (boneIdx.count("Jaw")) {
                Vector3 p = bonePos("Jaw");
                Vector3 e = boneEnd("Jaw");
                Vector3 start = place(p);
                Matrix4x4 rot;
                rot.rotate(right, -0.4 * lifted);
                world["Jaw"] = buildBoneWorldTransform(start, start + rot.transformVector(body.transformVector(e - p)));
            }

            auto& animFrame = animationClip.frames[frame];
            animFrame.time = static_cast<float>(frame) / static_cast<float>(frameCount) * durationSeconds;
            animFrame.boneWorldTransforms = world;
            finishFrame(animFrame, inverseBindMatrices);
        }
        return true;
    }

} // namespace fish

} // namespace dust3d
