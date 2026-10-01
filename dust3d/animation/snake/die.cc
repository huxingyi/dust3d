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

// Procedural death for the snake rig.
//
// One-shot, ends lying still. The body thrashes in a few decaying waves, then
// rolls over belly-up, starting at the head and running down to the tail,
// while staying flat on the ground; the head turns aside and the jaw falls
// open. It ends in a loose S-curve.
//
// Adjustable animation parameters:
//   - flipSpeedFactor:  how fast it thrashes and rolls over (> 1 = sooner)
//   - flipAngle:        how far it rolls over, in degrees (180 = belly-up)
//   - thrashFactor:     how hard it thrashes
//   - jawOpen:          how wide the jaw falls open, in degrees

#include <algorithm>
#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/common.h>
#include <dust3d/animation/snake/die.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace snake {

    bool die(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = std::max(2, static_cast<int>(parameters.getValue("frameCount", 42)));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 1.4));

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

        double speed = std::max(0.3, parameters.getValue("flipSpeedFactor", 1.0));
        double rollMax = std::clamp(parameters.getValue("flipAngle", 180.0), 0.0, 180.0) * Math::Pi / 180.0;
        double thrashFactor = parameters.getValue("thrashFactor", 1.0);
        double jawOpen = parameters.getValue("jawOpen", 35.0) * Math::Pi / 180.0;

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

        static const std::vector<std::string> chain = {
            "TailTip", "Tail4", "Tail3", "Tail2", "Tail1",
            "Spine1", "Spine2", "Spine3", "Spine4", "Spine5", "Spine6", "Head"
        };

        std::map<std::string, Matrix4x4> rest = restBoneWorldTransforms(rigStructure);
        double groundY = restGroundHeight(rigStructure);
        // Rolling a bone about its own axis turns the body's cross-section with it. A snake is
        // wider (its capsule radius) than it is tall (its height above the ground): rolled onto
        // its edge it stands taller, so it is lifted by as much to stay on the ground.
        auto rollLift = [&](const std::string& name, double angle) -> double {
            double halfWidth = rigStructure.bones[boneIdx[name]].capsuleRadius;
            double halfHeight = std::max(1e-6, bonePos(name).y() - groundY);
            if (halfWidth <= halfHeight)
                return 0.0;
            double s = std::sin(angle), c = std::cos(angle);
            return std::sqrt(halfWidth * halfWidth * s * s + halfHeight * halfHeight * c * c) - halfHeight;
        };

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            double t = static_cast<double>(frame) / static_cast<double>(frameCount - 1);
            double ts = t * speed;
            // Thrashing: a few fast waves that die away; it settles into a loose S-curve.
            double thrashEnv = smoothstep(ts / 0.06) * std::exp(-3.5 * ts);
            double settle = smoothstep(ts / 0.6);

            auto lateralAt = [&](const Vector3& p) -> double {
                double u = std::clamp(Vector3::dotProduct(p - tailPos, forward) / bodyLength, 0.0, 1.0);
                double thrash = 0.12 * bodyLength * thrashFactor * thrashEnv * std::sin(2.0 * Math::Pi * (4.0 * ts - 1.5 * u));
                double rest = 0.06 * bodyLength * settle * std::sin(2.0 * Math::Pi * (u - 0.2));
                return thrash + rest;
            };
            // Belly-up roll, from the head (u = 1) down to the tail.
            auto rollAt = [&](const Vector3& p) -> double {
                double u = std::clamp(Vector3::dotProduct(p - tailPos, forward) / bodyLength, 0.0, 1.0);
                return rollMax * smoothstep((ts - 0.12 - 0.35 * (1.0 - u)) / 0.35);
            };

            std::map<std::string, Matrix4x4> world = rest;
            for (const auto& name : chain) {
                Vector3 p = bonePos(name);
                Vector3 e = boneEnd(name);
                double lift = rollLift(name, rollAt(p));
                Vector3 np = p + right * lateralAt(p) + up * lift;
                Vector3 ne = e + right * lateralAt(e) + up * lift;
                Matrix4x4 m = boneFromRest(rest[name], p, e, np, ne);
                // Roll the bone about its own axis (its local Z): the body turns over in place.
                Matrix4x4 roll;
                roll.rotate(Vector3(0.0, 0.0, 1.0), rollAt(p));
                m *= roll;
                world[name] = m;
            }
            if (boneIdx.count("Jaw")) {
                Vector3 jawPos = bonePos("Jaw");
                Matrix4x4 headRest = rest["Head"];
                Matrix4x4 delta = world["Head"];
                delta *= headRest.inverted();
                Vector3 jawStart = delta.transformPoint(jawPos);
                Matrix4x4 jaw = rotationAbout(jawStart, delta.transformVector(right), -jawOpen * settle);
                jaw *= delta;
                jaw *= rest["Jaw"];
                world["Jaw"] = jaw;
            }

            auto& animFrame = animationClip.frames[frame];
            animFrame.time = static_cast<float>(frame) / static_cast<float>(frameCount) * durationSeconds;
            animFrame.boneWorldTransforms = world;
            finishFrame(animFrame, inverseBindMatrices);
        }
        return true;
    }

} // namespace snake

} // namespace dust3d
