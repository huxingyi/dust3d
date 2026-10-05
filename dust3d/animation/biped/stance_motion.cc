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

#include <dust3d/animation/biped/floor_transition.h>
#include <dust3d/animation/biped/pose_sequence.h>
#include <dust3d/animation/biped/stance_motion.h>
#include <dust3d/base/math.h>
namespace dust3d {
namespace biped {
    bool stanceMotion(const RigStructure& rig, const std::map<std::string, Matrix4x4>& inverse,
        RigAnimationClip& clip, const AnimationParams& params, StanceKind kind)
    {
        double intensity = std::clamp(params.getValue("actionIntensityFactor", 1), 0.0, 1.5);
        bool result = poseSequence(rig, inverse, clip, params, [&](double t) {
            double phase = 2 * Math::Pi * t;
            PoseSample p;
            switch (kind) {
            case StanceKind::CrouchEnter:
                p = crouchedPose(easePose(t));
                break;
            case StanceKind::CrouchExit:
                p = crouchedPose(1 - easePose(t));
                break;
            case StanceKind::CrouchIdle:
                p = crouchedPose(1);
                p.hips.setY(p.hips.y() + 0.004 * std::sin(phase) * intensity);
                break;
            case StanceKind::MountedIdle:
            case StanceKind::MountedRide: {
                bool riding = kind == StanceKind::MountedRide;
                p.hips = Vector3(0, -0.12 + (riding ? 0.025 : 0.004) * std::sin(phase) * intensity, 0);
                p.chestPitch = (riding ? 0.08 : 0.01) * std::sin(phase) * intensity;
                for (int i = 0; i < 2; ++i) {
                    double sign = i == 0 ? 1 : -1;
                    p.ankle[i] = Vector3(sign * 0.17, 0.35, 0.1);
                    p.arms[i].upper = Vector3(sign * 0.1, -0.75, 0.55);
                    p.arms[i].lower = Vector3(0, 0.1, 1);
                    p.arms[i].weight = 1;
                }
                break;
            }
            case StanceKind::Stunned:
                p.hips = Vector3(0.025 * std::sin(phase) * intensity, -0.08, 0);
                p.chestPitch = 0.16 + 0.05 * std::sin(phase * 2) * intensity;
                p.headYaw = 0.12 * std::sin(phase) * intensity;
                p.headPitch = 0.18;
                for (auto& arm : p.arms) {
                    arm.upper = Vector3(0, -0.8, 0.3);
                    arm.lower = Vector3(0, 0.3, 0.7);
                    arm.weight = 0.65;
                }
                break;
            case StanceKind::Knockdown:
                p = floorTransitionPose(t, FloorTransition::Knockdown, intensity);
                break;
            case StanceKind::GetUp:
                p = floorTransitionPose(t, FloorTransition::GetUp, intensity);
                break;
            case StanceKind::DodgeRoll: {
                double fold = actionEnvelope(t, 0.18, 0.72);
                p.hips = Vector3(0, -0.55 * fold, 0);
                p.pitch = 2 * Math::Pi * easePose(std::clamp((t - 0.1) / 0.8, 0.0, 1.0));
                p.chestPitch = 0.45 * fold;
                p.freeFeet = true;
                p.floorContact = true;
                for (int i = 0; i < 2; ++i) {
                    p.ankle[i] = Vector3(0, 0.4 * fold, 0.25 * fold);
                    p.arms[i].upper = Vector3(i == 0 ? 0.12 : -0.12, -0.3, 0.8);
                    p.arms[i].lower = Vector3(0, 0.7, -0.3);
                    p.arms[i].weight = fold;
                }
                break;
            }
            }
            return p;
        });
        if (result && kind == StanceKind::DodgeRoll) {
            addClipEvent(clip, "invulnerableBegin", 0.18);
            addClipEvent(clip, "invulnerableEnd", 0.72);
            addClipEvent(clip, "recovery", 0.8);
        }
        if (result && kind == StanceKind::Knockdown)
            addClipEvent(clip, "impact", 0.7, "Hips");
        return result;
    }
} // namespace biped
} // namespace dust3d
