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
#include <dust3d/animation/biped/interaction_motion.h>
#include <dust3d/animation/biped/pose_sequence.h>
#include <dust3d/base/math.h>
namespace dust3d {
namespace biped {
    bool interactionMotion(const RigStructure& rig, const std::map<std::string, Matrix4x4>& inverse,
        RigAnimationClip& clip, const AnimationParams& params, InteractionKind kind)
    {
        double intensity = std::clamp(params.getValue("actionIntensityFactor", 1), 0.0, 1.5);
        double seat = std::clamp(params.getValue("seatHeightFactor", 0.5), 0.25, 0.8);
        double reach = std::clamp(params.getValue("weaponReachFactor", 1), 0.3, 1.4);
        double spacing = 0.12 * std::clamp(params.getValue("gripSpacingFactor", 1), 0.3, 1.8);
        bool result = poseSequence(rig, inverse, clip, params, [&](double t) {
            PoseSample p;
            double phase = 2 * Math::Pi * t, weight = actionEnvelope(t);
            switch (kind) {
            case InteractionKind::SitDown:
                p = seatedPose(easePose(t), seat);
                break;
            case InteractionKind::StandUp:
                p = seatedPose(1 - easePose(t), seat);
                p.chestPitch += 0.3 * actionEnvelope(t, 0.25, 0.55);
                break;
            case InteractionKind::SitIdle:
                p = seatedPose(1, seat);
                p.chestPitch += 0.008 * std::sin(phase) * intensity;
                break;
            case InteractionKind::SleepLieDown:
                p = floorTransitionPose(t, FloorTransition::LieDown, intensity);
                break;
            case InteractionKind::WakeUp:
                p = floorTransitionPose(t, FloorTransition::WakeUp, intensity);
                break;
            case InteractionKind::SleepIdle:
                p = floorRestPose(true);
                p.chestPitch += 0.008 * std::sin(phase) * intensity;
                break;
            case InteractionKind::Gather:
            case InteractionKind::PickUp: {
                bool gather = kind == InteractionKind::Gather;
                p.hips = Vector3(0, -0.25 * weight * intensity, 0);
                p.chestPitch = 0.5 * weight * intensity;
                p.headPitch = 0.12 * weight;
                double lift = gather ? 0.025 * std::sin(phase * 3) : 0.25 * easePose((t - 0.45) / 0.3);
                for (int i = 0; i < 2; ++i) {
                    p.arms[i].targetHand = true;
                    p.arms[i].handTarget = Vector3(i == 0 ? 0.13 : -0.13, -0.5 + lift, 0.35);
                    p.arms[i].handDirection = Vector3(0, -0.5, 0.5);
                    p.arms[i].weight = weight;
                }
                break;
            }
            case InteractionKind::Interact:
                p.arms[1].targetHand = true;
                p.arms[1].handTarget = Vector3(-0.1, 0.05, 0.6);
                p.arms[1].weight = weight;
                p.chestYaw = 0.05 * weight;
                break;
            case InteractionKind::Mine:
            case InteractionKind::Chop: {
                bool mine = kind == InteractionKind::Mine;
                double strike = easePose((t - 0.25) / 0.2) * (1 - easePose((t - 0.65) / 0.35));
                p.chestPitch = mine ? 0.25 * strike * intensity : 0.05;
                p.chestYaw = mine ? 0 : (-0.2 + 0.55 * strike) * intensity;
                p.hips = Vector3(0, -0.04 * strike, 0);
                Vector3 grip = mine ? Vector3(0, 0.35 - 0.65 * strike, (0.25 + 0.15 * strike) * reach)
                                    : Vector3(-0.35 + 0.6 * strike, 0, 0.35 * reach);
                Vector3 shaft = mine ? Vector3(0, 1 - strike, strike) : Vector3(0.8, 0.2, 0.5);
                for (int i = 0; i < 2; ++i) {
                    p.arms[i].targetHand = true;
                    p.arms[i].handTarget = grip + shaft * (i == 0 ? spacing * 0.5 : -spacing * 0.5);
                    p.arms[i].handDirection = shaft;
                    p.arms[i].weight = 1;
                }
                break;
            }
            case InteractionKind::Drink:
            case InteractionKind::Eat: {
                bool drink = kind == InteractionKind::Drink;
                double sip = drink ? weight : weight * (0.88 + 0.12 * std::sin(phase * 2));
                p.headPitch = (drink ? -0.12 : 0.04) * sip * intensity;
                p.arms[1].targetHand = true;
                p.arms[1].targetHead = true;
                p.arms[1].handTarget = Vector3(-0.06, std::clamp(params.getValue("mouthHeightFactor", -0.12), -0.5, 0.5), 0.14);
                p.arms[1].handDirection = drink ? Vector3(0, 0.8, -0.3) : Vector3(0, 0.2, 0.8);
                p.arms[1].weight = sip;
                if (!drink) {
                    p.arms[0].upper = Vector3(0.15, -0.8, 0.4);
                    p.arms[0].lower = Vector3(0, 0.1, 1);
                    p.arms[0].weight = weight;
                }
                break;
            }
            }
            return p;
        });
        if (result) {
            if (kind == InteractionKind::Mine || kind == InteractionKind::Chop)
                addClipEvent(clip, "hit", 0.45, "RightHand");
            if (kind == InteractionKind::Gather || kind == InteractionKind::PickUp || kind == InteractionKind::Interact)
                addClipEvent(clip, "interact", 0.5, "RightHand");
            if (kind == InteractionKind::Drink || kind == InteractionKind::Eat)
                addClipEvent(clip, "consume", 0.5, "RightHand");
        }
        return result;
    }
} // namespace biped
} // namespace dust3d
