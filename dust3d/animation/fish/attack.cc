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

// Procedural attack (bite) animation for the fish rig.
//
// A one-shot clip for game use. The fish draws back into an S-shaped coil
// with its paired fins flared to brake, then darts forward with one hard tail
// stroke and bites (the mouth opens on the approach and snaps shut on the hit
// when the rig has a Jaw bone), shakes its head to tear, and glides back to
// rest. The clip starts and ends exactly in the rest pose, so it blends in
// from and back to the swim or idle loop.
//
// Adjustable animation parameters:
//   - lungeDistanceFactor: how far the fish darts forward
//   - coilFactor:          how far it draws back and how deep the S-coil is
//   - tailBeatFactor:      how hard the tail strokes on the dart
//   - biteShakeFactor:     how hard the head shakes after the bite
//   - finFlareFactor:      how far the paired fins flare while coiling
//   - strikeTimingFactor:  when the bite lands (1 = 45% into the clip)

#include <algorithm>
#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/common.h>
#include <dust3d/animation/fish/attack.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace fish {

    bool attack(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = std::max(2, static_cast<int>(parameters.getValue("frameCount", 27)));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 0.9));

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

        double lungeDistanceFactor = parameters.getValue("lungeDistanceFactor", 1.0);
        double coilFactor = parameters.getValue("coilFactor", 1.0);
        double tailBeatFactor = parameters.getValue("tailBeatFactor", 1.0);
        double biteShakeFactor = parameters.getValue("biteShakeFactor", 1.0);
        double finFlareFactor = parameters.getValue("finFlareFactor", 1.0);
        double tStrike = std::clamp(0.45 * parameters.getValue("strikeTimingFactor", 1.0), 0.25, 0.75);
        animationClip.events.push_back({ "hit", static_cast<float>(tStrike * durationSeconds), "Head" });

        double pullBack = 0.12 * bodyLength * coilFactor;
        double coilDepth = 0.07 * bodyLength * coilFactor;
        double lunge = 0.35 * bodyLength * lungeDistanceFactor;
        double tailStroke = 0.12 * bodyLength * tailBeatFactor;
        double shake = 0.3 * biteShakeFactor;

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            double t = static_cast<double>(frame) / static_cast<double>(frameCount - 1);

            // Envelopes: coil back, dart and bite, then glide back to rest.
            // The dart takes about 0.15 s (4-5 frames at 30 fps): fast, but it reads.
            double dartTime = std::clamp(0.15 / std::max(0.1, static_cast<double>(durationSeconds)), 0.08, 0.3);
            double coil = smoothstep(t / (tStrike - dartTime)) * (1.0 - smoothstep((t - (tStrike - dartTime)) / dartTime));
            double dart = smoothstep((t - (tStrike - dartTime)) / dartTime) * (1.0 - smoothstep((t - (tStrike + 0.12)) / std::max(0.1, 0.95 - (tStrike + 0.12))));
            // One tail stroke through the dart.
            double strokeX = (t - (tStrike - dartTime - 0.02)) / (dartTime + 0.14);
            double stroke = (strokeX > 0.0 && strokeX < 1.0) ? std::sin(2.0 * Math::Pi * strokeX) * std::sin(Math::Pi * strokeX) : 0.0;
            // Head shake after the bite: two quick decaying swings.
            double shakeX = (t - tStrike) / 0.3;
            double shakeEnv = (shakeX > 0.0 && shakeX < 1.0) ? std::sin(2.0 * Math::Pi * 2.0 * shakeX) * (1.0 - shakeX) : 0.0;

            double pairedFinAngle = 0.6 * finFlareFactor * coil - 0.25 * dart;
            double pairedFinSweep = 0.4 * finFlareFactor * coil - 0.3 * dart;
            double medianFinSway = 0.15 * stroke;

            Matrix4x4 body;
            body.translate(forward * (-pullBack * coil + lunge * dart));
            // The head end pitches up a little while coiling and drops on the bite.
            body.translate(centre);
            body.rotate(right, 0.08 * coilFactor * coil - 0.05 * dart);
            body.translate(Vector3() - centre);

            auto offsetAt = [&](const Vector3& p) -> Vector3 {
                double u = bodyU(p);
                // S-coil: the head end and the tail bow to opposite sides, the nose stays aimed.
                double s = coilDepth * std::sin(2.0 * Math::Pi * u) * coil * smoothstep(u / 0.2);
                double tail = tailStroke * u * u * stroke;
                // The head shakes about the middle of the body.
                double shakeSide = shake * bodyLength * 0.12 * (1.0 - u) * (1.0 - u) * shakeEnv;
                return right * (s + tail + shakeSide);
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

            // Optional jaw: opens on the approach, snaps shut on the bite.
            if (boneIdx.count("Jaw")) {
                Vector3 p = bonePos("Jaw");
                Vector3 e = boneEnd("Jaw");
                Vector3 start = place(p);
                double open = 0.6 * (smoothstep((t - (tStrike - 0.2)) / 0.15) * (1.0 - smoothstep((t - (tStrike - 0.02)) / 0.04)));
                Matrix4x4 rot;
                rot.rotate(right, -open);
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
