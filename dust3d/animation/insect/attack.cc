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

// Procedural flying attack (dive) for the insect rig: wasps, flies, locusts.
//
// A one-shot attack for a flying insect, layered on the flying cycle
// (InsectFly, which also takes its own parameters: wingBeatFactor,
// wingFlapFactor, gaitSpeedFactor...), so it starts and ends in the flying
// pose and the wings never stop beating. The insect pulls up and back, then
// dives forward and down nose-first with its front legs reaching out to grab,
// and climbs back to where it started. For an insect on the ground use
// InsectBite.
//
// Adjustable animation parameters:
//   - diveIntensityFactor:  how far and steep the dive is
//   - attackSpeedFactor:    how early the dive lands (> 1 = sooner)
//   - legReachFactor:       how far the front legs reach out on the strike

#include <algorithm>
#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/common.h>
#include <dust3d/animation/insect/attack.h>
#include <dust3d/animation/insect/fly.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace insect {

    bool attack(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = std::max(2, static_cast<int>(parameters.getValue("frameCount", 30.0)));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 1.0));

        auto boneIdx = buildBoneIndexMap(rigStructure);
        auto bonePos = [&](const std::string& name) -> Vector3 {
            return getBonePos(rigStructure, boneIdx, name);
        };
        auto boneEnd = [&](const std::string& name) -> Vector3 {
            return getBoneEnd(rigStructure, boneIdx, name);
        };
        static const char* requiredBones[] = { "Head", "Thorax", "Abdomen" };
        if (!validateRequiredBones(boneIdx, requiredBones, sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;

        double diveIntensity = parameters.getValue("diveIntensityFactor", 1.0);
        double attackSpeed = std::clamp(parameters.getValue("attackSpeedFactor", 1.0), 0.5, 2.0);
        double legReach = parameters.getValue("legReachFactor", 1.0);

        RigAnimationClip baseClip;
        AnimationParams flyParams = parameters;
        flyParams.setValue("frameCount", frameCount);
        flyParams.setValue("durationSeconds", durationSeconds);
        if (!fly(rigStructure, inverseBindMatrices, baseClip, flyParams) || static_cast<int>(baseClip.frames.size()) != frameCount)
            return false;
        // The flying clip on its own gets its bones' twist from the rest pose afterwards (bones
        // it builds from a direction alone); give the base that twist now, as layered on top it
        // would no longer be recognised.
        animation::referenceRollsToRest(rigStructure, inverseBindMatrices, baseClip);

        Vector3 bodyVector = bonePos("Thorax") - boneEnd("Abdomen");
        double bodyLength = std::max(1e-4, bodyVector.length());
        Vector3 forward(bodyVector.x(), 0.0, bodyVector.z());
        if (forward.lengthSquared() < 1e-10)
            forward = Vector3(0.0, 0.0, 1.0);
        forward.normalize();
        Vector3 up(0.0, 1.0, 0.0);
        // A positive rotation about `right` lifts the head end.
        Vector3 right = Vector3::crossProduct(forward, up).normalized();
        Vector3 centre = (bonePos("Thorax") + boneEnd("Abdomen")) * 0.5;

        std::map<std::string, Matrix4x4> rest = restBoneWorldTransforms(rigStructure);

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            double t = static_cast<double>(frame) / static_cast<double>(frameCount - 1);
            double strikeAt = std::clamp(0.5 / attackSpeed, 0.3, 0.7);
            // Pull up and back, then dive through the strike, then climb back.
            double windup = smoothstep(t / (strikeAt * 0.5)) * (1.0 - smoothstep((t - strikeAt * 0.5) / (strikeAt * 0.5)));
            double dive = smoothstep((t - strikeAt * 0.4) / (strikeAt * 0.6)) * (1.0 - smoothstep((t - strikeAt) / (0.95 - strikeAt)));

            Matrix4x4 layer;
            layer.translate(centre
                + forward * (bodyLength * (0.6 * dive - 0.15 * windup) * diveIntensity)
                + up * (bodyLength * (0.15 * windup - 0.4 * dive) * diveIntensity));
            layer.rotate(right, (0.25 * windup - 0.7 * dive) * diveIntensity);
            layer.translate(Vector3() - centre);

            std::map<std::string, Matrix4x4> world = rest;
            for (const auto& pair : baseClip.frames[frame].boneWorldTransforms) {
                Matrix4x4 m = layer;
                m *= pair.second;
                world[pair.first] = m;
            }
            // The front legs reach forward and down to grab on the strike.
            for (const char* side : { "Left", "Right" }) {
                std::string coxa = std::string("Front") + side + "Coxa";
                if (!boneIdx.count(coxa))
                    continue;
                Vector3 hip = world[coxa].transformPoint(Vector3());
                Matrix4x4 reach = rotationAbout(hip, layer.transformVector(right), -0.9 * legReach * dive);
                for (const char* part : { "Coxa", "Femur", "Tibia" }) {
                    std::string name = std::string("Front") + side + part;
                    if (!world.count(name))
                        continue;
                    Matrix4x4 m = reach;
                    m *= world[name];
                    world[name] = m;
                }
            }

            auto& animFrame = animationClip.frames[frame];
            animFrame.time = static_cast<float>(frame) / static_cast<float>(frameCount) * durationSeconds;
            animFrame.boneWorldTransforms = world;
            finishFrame(animFrame, inverseBindMatrices);
        }
        return true;
    }

} // namespace insect

} // namespace dust3d
