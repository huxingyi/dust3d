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

#include <dust3d/animation/biped/combat_motion.h>
#include <dust3d/animation/biped/pose_sequence.h>
#include <dust3d/base/math.h>
namespace dust3d {
namespace biped {
    bool combatMotion(const RigStructure& rig, const std::map<std::string, Matrix4x4>& inverse,
        RigAnimationClip& clip, const AnimationParams& params, CombatKind kind)
    {
        double intensity = std::clamp(params.getValue("actionIntensityFactor", 1), 0.0, 1.5);
        double reach = std::clamp(params.getValue("weaponReachFactor", 1), 0.3, 1.4);
        double spacing = 0.12 * std::clamp(params.getValue("gripSpacingFactor", 1), 0.3, 1.8);
        bool result = poseSequence(rig, inverse, clip, params, [&](double t) {
            PoseSample p;
            bool bow = kind == CombatKind::BowDraw || kind == CombatKind::BowAim || kind == CombatKind::BowShot;
            if (bow) {
                double draw = kind == CombatKind::BowDraw ? easePose(t) : kind == CombatKind::BowShot ? 1 - easePose((t - 0.55) / 0.45)
                                                                                                      : 1;
                double release = kind == CombatKind::BowShot ? easePose(t / 0.18) * (1 - easePose((t - 0.45) / 0.3)) : 0;
                p.chestYaw = -0.22 * draw;
                p.headYaw = 0.22 * draw;
                p.hips = Vector3(0, -0.02 * draw, 0);
                p.arms[0].targetHand = p.arms[1].targetHand = true;
                p.arms[0].handTarget = Vector3(spacing * 0.4, 0.08, 0.5 * reach);
                p.arms[1].handTarget = Vector3(-0.15 - spacing * 0.25 - 0.12 * release, 0.12, 0.02 - 0.08 * release);
                p.arms[0].handDirection = p.arms[1].handDirection = Vector3(0, 0, 1);
                for (auto& arm : p.arms)
                    arm.weight = draw;
                if (kind == CombatKind::BowAim)
                    p.headPitch = 0.006 * std::sin(2 * Math::Pi * t) * intensity;
            } else {
                double weight = actionEnvelope(t, 0.18, 0.62);
                double windup = easePose(t / 0.25) * (1 - easePose((t - 0.25) / 0.2));
                double strike = easePose((t - 0.25) / 0.2);
                p.hips = Vector3(0, -0.04 * weight * intensity, 0.03 * weight * intensity);
                p.chestYaw = (-0.35 * windup + 0.45 * strike * weight) * intensity;
                if (kind == CombatKind::Parry) {
                    p.chestYaw = -0.2 * weight * intensity;
                    p.arms[0].upper = Vector3(0.1, -0.6, 0.6);
                    p.arms[0].lower = Vector3(0, 0.6, 0.7);
                    p.arms[1].upper = Vector3(-0.3, 0.25, 0.8);
                    p.arms[1].lower = Vector3(0.7, 0.5, 0.6);
                    for (auto& arm : p.arms)
                        arm.weight = weight;
                } else if (kind == CombatKind::TwoHandSwing) {
                    p.chestPitch = (-0.1 * windup + 0.25 * strike * weight) * intensity;
                    Vector3 grip(0, 0.35 * (1 - strike) - 0.35 * strike, (0.28 + 0.22 * strike) * reach);
                    Vector3 shaft(0, std::cos(strike * 1.8), std::sin(strike * 1.8));
                    for (int i = 0; i < 2; ++i) {
                        p.arms[i].targetHand = true;
                        p.arms[i].handTarget = grip + shaft * (i == 0 ? spacing * 0.5 : -spacing * 0.5);
                        p.arms[i].handDirection = shaft;
                        p.arms[i].weight = weight;
                    }
                } else {
                    p.arms[0].upper = Vector3(0.15, -0.75, 0.5);
                    p.arms[0].lower = Vector3(0, 0.65, 0.65);
                    p.arms[0].weight = weight;
                    p.arms[1].targetHand = true;
                    p.arms[1].handTarget = Vector3(-0.4 + 0.75 * strike, 0.15, (0.2 + 0.35 * std::sin(Math::Pi * strike)) * reach);
                    p.arms[1].handDirection = Vector3(0.8, 0, 0.6);
                    p.arms[1].weight = weight;
                }
            }
            return p;
        });
        if (result) {
            if (kind == CombatKind::OneHandSlash || kind == CombatKind::TwoHandSwing)
                addClipEvent(clip, "hit", 0.43, "RightHand");
            if (kind == CombatKind::BowShot)
                addClipEvent(clip, "release", 0.18, "RightHand");
            if (kind == CombatKind::Parry) {
                addClipEvent(clip, "parryBegin", 0.18, "RightHand");
                addClipEvent(clip, "parryEnd", 0.5, "RightHand");
            }
            if (!clip.loop) {
                addClipEvent(clip, "cancel", 0.65);
                addClipEvent(clip, "recovery", 0.85);
            }
        }
        return result;
    }
} // namespace biped
} // namespace dust3d
