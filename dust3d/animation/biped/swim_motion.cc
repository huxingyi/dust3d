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

#include <dust3d/animation/biped/pose_sequence.h>
#include <dust3d/animation/biped/swim_motion.h>
#include <dust3d/base/math.h>
namespace dust3d {
namespace biped {
    bool swimMotion(const RigStructure& rig, const std::map<std::string, Matrix4x4>& inverse,
        RigAnimationClip& clip, const AnimationParams& params, SwimKind kind)
    {
        bool idle = kind == SwimKind::Idle;
        bool sculling = kind != SwimKind::Forward;
        double intensity = std::clamp(params.getValue("actionIntensityFactor", 1), 0.0, 1.5);
        bool result = poseSequence(rig, inverse, clip, params, [&](double t) {
            PoseSample p;
            double phase = 2 * Math::Pi * t * (kind == SwimKind::Backward ? -1 : 1);
            p.freeFeet = true;
            p.pitch = sculling ? 0.12 : Math::Pi * 0.5;
            p.hips = Vector3(0, 0.01 * std::sin(phase) * intensity, 0);
            p.roll = idle ? 0 : 0.06 * std::sin(phase) * intensity;
            p.headPitch = idle ? 0 : -0.18;
            for (int i = 0; i < 2; ++i) {
                double sign = i == 0 ? 1 : -1, stroke = phase + (i == 0 ? 0 : Math::Pi);
                p.ankle[i] = Vector3(0, (idle ? 0.15 : 0.08) * (1 + std::sin(stroke)) * intensity, 0.12 * std::cos(stroke) * intensity);
                p.arms[i].upper = sculling ? Vector3(sign * 0.5, -0.3, 0.5 + 0.15 * std::sin(stroke) * intensity)
                                           : Vector3(sign * 0.25, 0.9 * std::cos(stroke), 0.7 * std::sin(stroke));
                p.arms[i].lower = sculling ? Vector3(sign * 0.7, 0.1, 0.6)
                                           : Vector3(sign * 0.2, std::cos(stroke + 0.35), 0.65 * std::sin(stroke + 0.35));
                p.arms[i].weight = 1;
            }
            return p;
        });
        if (result && !idle) {
            auto idx = animation::buildBoneIndexMap(rig);
            Vector3 forward = facing(rig, idx), left = Vector3::crossProduct(Vector3(0, 1, 0), forward);
            Vector3 direction = kind == SwimKind::Backward ? -forward : kind == SwimKind::Left ? left
                : kind == SwimKind::Right                                                      ? -left
                                                                                               : forward;
            double leg = (animation::getBonePos(rig, idx, "LeftUpperLeg") - animation::getBoneEnd(rig, idx, "LeftLowerLeg")).length();
            clip.movementSpeed = leg * 0.65 * intensity / clip.durationSeconds;
            clip.movementDirectionX = direction.x();
            clip.movementDirectionZ = direction.z();
        }
        return result;
    }
} // namespace biped
} // namespace dust3d
