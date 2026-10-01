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

// Procedural death for the fish rig.
//
// One-shot, ends lying still. In water (onGround = 0) the killing blow sets off
// a thrashing wave down the body that dies away while the fish rolls over slowly
// belly-up and drifts up a little, fins going limp. On land or on the bottom
// (onGround = 1, use flipAngle 90) it drops onto its side, then flops: the body
// arches, lifting head and tail off the ground, and slaps down, a few times,
// weaker each time, before it lies still with its fins folded back.
//
// Adjustable animation parameters:
//   - hitIntensityFactor: how hard it thrashes or flops
//   - hitFrequency:       how fast (thrash cycles; on the ground about a third as many flops)
//   - flipSpeedFactor:    how fast it rolls over (> 1 = sooner)
//   - flipAngle:          how far it rolls, in degrees (180 = belly-up, 90 = onto its side)
//   - tilt:               pitch at the end (-1..1 of 45 degrees, + = head up)
//   - finFlopFactor:      how much the fins flap before folding back
//   - spinDecay:          how quickly the thrashing dies away
//   - onGround:           0 = in water, 1 = on land / the bottom

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
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = std::max(2, static_cast<int>(parameters.getValue("frameCount", 54.0)));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 1.8));

        auto boneIdx = buildBoneIndexMap(rigStructure);
        static const char* requiredBones[] = {
            "Root", "Head", "BodyFront", "BodyMid", "BodyRear", "TailStart", "TailEnd"
        };
        if (!validateRequiredBones(boneIdx, requiredBones, sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;
        auto bonePos = [&](const std::string& name) -> Vector3 {
            return getBonePos(rigStructure, boneIdx, name);
        };
        auto boneEnd = [&](const std::string& name) -> Vector3 {
            return getBoneEnd(rigStructure, boneIdx, name);
        };

        double intensity = parameters.getValue("hitIntensityFactor", 1.0);
        double frequency = std::max(0.5, parameters.getValue("hitFrequency", 8.0));
        double rollSpeed = std::max(0.2, parameters.getValue("flipSpeedFactor", 1.0));
        double rollMax = std::clamp(parameters.getValue("flipAngle", 180.0), 0.0, 180.0) * Math::Pi / 180.0;
        double tilt = std::clamp(parameters.getValue("tilt", 0.0), -1.0, 1.0);
        double finFlop = parameters.getValue("finFlopFactor", 1.0);
        double decay = std::max(0.0, parameters.getValue("spinDecay", 4.0));
        bool onGround = parameters.getValue("onGround", 0.0) > 0.5;

        // The spine, head to tail tip.
        static const std::vector<std::string> spine = { "Head", "BodyFront", "BodyMid", "BodyRear", "TailStart", "TailEnd" };
        std::vector<Vector3> restPoints;
        for (const auto& name : spine)
            restPoints.push_back(bonePos(name));
        restPoints.push_back(boneEnd("TailEnd"));
        Vector3 bodyVector = restPoints.front() - restPoints.back();
        if (bodyVector.lengthSquared() < 1e-12)
            return false;
        double bodyLength = bodyVector.length();
        Vector3 up(0.0, 1.0, 0.0);
        Vector3 forward = Vector3(bodyVector.x(), 0.0, bodyVector.z());
        forward = forward.lengthSquared() > 1e-12 ? forward.normalized() : Vector3(0.0, 0.0, 1.0);
        Vector3 right = Vector3::crossProduct(forward, up).normalized();
        // Arc position along the body, 0 at the head and 1 at the tail tip.
        std::vector<double> along(restPoints.size(), 0.0);
        {
            double total = 0.0;
            for (size_t i = 1; i < restPoints.size(); ++i) {
                total += (restPoints[i] - restPoints[i - 1]).length();
                along[i] = total;
            }
            for (auto& a : along)
                a /= std::max(1e-9, total);
        }
        const size_t pivot = 2; // BodyMid: the body bends about its middle

        double bodyRadius = 0.0;
        for (const auto& name : spine)
            bodyRadius = std::max(bodyRadius, static_cast<double>(rigStructure.bones[boneIdx[name]].capsuleRadius));
        if (bodyRadius < 1e-6)
            bodyRadius = 0.12 * bodyLength;
        Vector3 centre = restPoints[pivot];
        // Lying on its side a fish rests on its (narrower) flank.
        double groundY = centre.y() - bodyRadius;
        double flankHalfWidth = 0.55 * bodyRadius;

        std::map<std::string, Matrix4x4> rest = restBoneWorldTransforms(rigStructure);

        // On the ground: flops, weaker each time; in water: a thrashing wave.
        int flops = std::max(1, static_cast<int>(std::round(frequency / 2.7)));
        auto flopPulse = [&](double t) {
            // Arching pulses between 0.2 and 0.85 of the clip, decaying.
            double u = (t - 0.2) / 0.65;
            if (u <= 0.0 || u >= 1.0)
                return 0.0;
            double phase = u * flops;
            double within = phase - std::floor(phase);
            return std::pow(std::sin(Math::Pi * within), 2.0) * std::exp(-decay * 0.35 * phase);
        };

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            double t = static_cast<double>(frame) / static_cast<double>(frameCount - 1);
            double limp = smoothstep((t - 0.55) / 0.4);
            double blow = t < 0.1 ? std::pow(std::sin(Math::Pi * t / 0.1), 2.0) : 0.0;

            // The bend at each joint (about the fish's own dorsal axis) and the body's roll.
            double rolled, lift = 0.0, rise = 0.0;
            std::vector<double> bend(restPoints.size(), 0.0);
            if (onGround) {
                // Drops onto its side under gravity, then the flops arch it (head and tail up).
                rolled = rollMax * fallEnvelope(t * rollSpeed - 0.02, 0.2, 0.15, 0.1);
                // Rolled onto its right side its left faces up: a negative bend (about its
                // dorsal axis) curls head and tail to the left, up off the ground.
                double arch = -0.3 * intensity * flopPulse(t);
                for (size_t i = 1; i + 1 < restPoints.size(); ++i)
                    bend[i] = arch * (1.0 - 0.5 * std::abs(along[i] - along[pivot]));
                lift = 0.05 * bodyLength * intensity * flopPulse(t);
            } else {
                // A thrashing wave from head to tail that dies away; a slow roll belly-up.
                double rollT = std::min(1.0, t * rollSpeed);
                rolled = rollMax * (1.0 - std::pow(1.0 - rollT, 3.0));
                double amplitude = 0.35 * intensity * std::exp(-decay * t) * smoothstep(t / 0.05);
                for (size_t i = 1; i + 1 < restPoints.size(); ++i)
                    bend[i] = amplitude * (0.3 + 0.7 * along[i]) * std::sin(2.0 * Math::Pi * (0.5 * frequency * t - along[i]));
                rise = 0.05 * bodyLength * t;
            }
            // The blow kicks the body into a quick bend first.
            for (size_t i = 1; i + 1 < restPoints.size(); ++i)
                bend[i] += (onGround ? -0.2 : 0.2) * intensity * blow;

            // Bend the spine from its middle outwards: towards the tail each joint turns the rest
            // of the body one way, towards the head the other, so both ends curl to one side.
            std::vector<Vector3> points = restPoints;
            {
                double angle = 0.0;
                for (size_t i = pivot; i + 1 < restPoints.size(); ++i) {
                    angle += bend[i];
                    Matrix4x4 turn = rotationAbout(Vector3(), up, angle);
                    points[i + 1] = points[i] + turn.transformVector(restPoints[i + 1] - restPoints[i]);
                }
                angle = 0.0;
                for (size_t i = pivot; i > 0; --i) {
                    angle -= bend[i];
                    Matrix4x4 turn = rotationAbout(Vector3(), up, angle);
                    points[i - 1] = points[i] + turn.transformVector(restPoints[i - 1] - restPoints[i]);
                }
            }

            // The whole body: rolls over about its length (towards its right side; arching
            // then lifts head and tail off the ground), tilts, and on the ground lies on its
            // flank; in water it drifts up.
            double support = std::abs(std::cos(rolled)) * bodyRadius + std::abs(std::sin(rolled)) * flankHalfWidth;
            Matrix4x4 body;
            body.translate(centre + up * ((onGround ? support - bodyRadius : 0.0) + lift + rise));
            body.rotate(forward, rolled);
            body.rotate(right, -tilt * 0.25 * Math::Pi * limp);
            body.translate(Vector3() - centre);

            std::map<std::string, Matrix4x4> world = rest;
            std::map<std::string, Matrix4x4> delta;
            for (size_t i = 0; i < spine.size(); ++i) {
                Matrix4x4 m = body;
                m *= boneFromRest(rest[spine[i]], restPoints[i], restPoints[i + 1], points[i], points[i + 1]);
                world[spine[i]] = m;
                Matrix4x4 d = m;
                d *= rest[spine[i]].inverted();
                delta[spine[i]] = d;
            }
            world["Root"] = body;
            world["Root"] *= rest["Root"];

            // Fins ride on their body bone; they flap with the thrashing, then fold back.
            double flap = finFlop * (onGround ? 0.5 * flopPulse(t) : 0.4 * std::exp(-decay * 0.6 * t) * std::sin(2.0 * Math::Pi * 0.5 * frequency * t));
            for (const auto& bone : rigStructure.bones) {
                if (bone.name.find("Fin") == std::string::npos)
                    continue;
                auto parent = delta.find(bone.parent);
                if (parent == delta.end())
                    continue;
                Vector3 finRoot = bonePos(bone.name);
                Vector3 finDir = boneEnd(bone.name) - finRoot;
                // Fold back toward the tail (about the axis across fin and body).
                Vector3 foldAxis = Vector3::crossProduct(finDir, forward);
                Matrix4x4 local;
                if (foldAxis.lengthSquared() > 1e-12)
                    local = rotationAbout(finRoot, foldAxis.normalized(), 0.6 * limp + flap);
                Matrix4x4 m = parent->second;
                m *= local;
                m *= rest[bone.name];
                world[bone.name] = m;
            }

            auto& animFrame = animationClip.frames[frame];
            animFrame.time = static_cast<float>(frame) / static_cast<float>(frameCount) * durationSeconds;
            animFrame.boneWorldTransforms = world;
            finishFrame(animFrame, inverseBindMatrices);
            if (onGround)
                keepBonesAboveGround(rigStructure, boneIdx, inverseBindMatrices, animFrame, groundY, [](const std::string& name) { return name.find("Fin") != std::string::npos || name == "TailEnd" || name == "Head"; }, 0.3, false);
        }
        return true;
    }

} // namespace fish

} // namespace dust3d
