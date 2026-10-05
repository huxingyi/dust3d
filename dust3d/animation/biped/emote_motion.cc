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

#include <dust3d/animation/biped/emote_motion.h>
#include <dust3d/animation/biped/pose_sequence.h>
#include <dust3d/base/math.h>
namespace dust3d {
namespace biped {
    bool emoteMotion(const RigStructure& rig, const std::map<std::string, Matrix4x4>& inverse,
        RigAnimationClip& clip, const AnimationParams& params, EmoteKind kind)
    {
        double intensity = std::clamp(params.getValue("actionIntensityFactor", 1), 0.0, 1.5);
        bool result = poseSequence(rig, inverse, clip, params, [&](double t) {
            PoseSample p;
            double phase = 2 * Math::Pi * t;
            double weight = clip.loop ? 1 : actionEnvelope(t);
            switch (kind) {
            case EmoteKind::Wave:
                p.arms[1].upper = Vector3(-0.65, 0.5, 0.1);
                p.arms[1].lower = Vector3(0.35 * std::sin(phase * 3) * intensity, 1, 0.1);
                p.arms[1].weight = weight;
                p.headYaw = -0.05 * weight;
                break;
            case EmoteKind::Cheer:
                p.hips = Vector3(0, -0.02 * (1 - std::cos(phase * 2)) * weight * intensity, 0);
                p.headPitch = -0.12 * weight;
                for (int i = 0; i < 2; ++i) {
                    p.arms[i].upper = Vector3(i == 0 ? 0.4 : -0.4, 0.8, 0.1);
                    p.arms[i].lower = Vector3(0, 1, 0.12 + 0.08 * std::sin(phase * 2) * intensity);
                    p.arms[i].weight = weight;
                }
                break;
            case EmoteKind::Bow:
                p.chestPitch = 0.7 * weight * intensity;
                p.headPitch = 0.16 * weight;
                p.arms[1].targetHand = true;
                p.arms[1].handTarget = Vector3(0.03, -0.15, 0.18);
                p.arms[1].weight = weight;
                break;
            case EmoteKind::Point:
                p.arms[1].targetHand = true;
                p.arms[1].handTarget = Vector3(-0.1, 0.1, 0.65);
                p.arms[1].weight = weight;
                p.headYaw = -0.08 * weight;
                break;
            case EmoteKind::Clap: {
                double distance = 0.015 + 0.15 * std::pow(std::sin(phase * 2), 2) * intensity;
                for (int i = 0; i < 2; ++i) {
                    p.arms[i].targetHand = true;
                    p.arms[i].handTarget = Vector3(i == 0 ? distance : -distance, 0.2, 0.35);
                    p.arms[i].handDirection = Vector3(0, 1, 0);
                    p.arms[i].weight = weight;
                }
                break;
            }
            case EmoteKind::Dance:
                p.hips = Vector3(0.045 * std::sin(phase) * intensity, -0.07 + 0.02 * std::cos(phase * 2) * intensity, 0);
                p.yaw = 0.15 * std::sin(phase) * intensity;
                p.chestYaw = -0.15 * std::sin(phase) * intensity;
                for (int i = 0; i < 2; ++i) {
                    double sign = i == 0 ? 1 : -1;
                    double swing = std::max(0.0, sign * std::sin(phase));
                    p.ankle[i] = Vector3(sign * 0.1 * swing, 0.06 * swing, 0);
                    p.arms[i].upper = Vector3(sign * 0.3, -0.55, 0.5 + sign * 0.2 * std::sin(phase) * intensity);
                    p.arms[i].lower = Vector3(0, 0.5, 0.65);
                    p.arms[i].weight = 1;
                }
                break;
            case EmoteKind::Talk:
                p.headPitch = 0.03 * std::sin(phase * 3) * intensity;
                p.headYaw = 0.04 * std::sin(phase) * intensity;
                p.chestYaw = 0.02 * std::sin(phase) * intensity;
                for (int i = 0; i < 2; ++i) {
                    double sign = i == 0 ? 1 : -1;
                    p.arms[i].upper = Vector3(sign * 0.2, -0.8, 0.4);
                    p.arms[i].lower = Vector3(sign * (0.15 + 0.1 * std::sin(phase + i) * intensity), 0.2, 0.8);
                    p.arms[i].weight = 0.65 + 0.2 * std::sin(phase + i) * intensity;
                }
                break;
            }
            return p;
        });
        if (result && kind == EmoteKind::Clap)
            for (double t : { 0.25, 0.5, 0.75 })
                addClipEvent(clip, "clap", t, "RightHand");
        return result;
    }
} // namespace biped
} // namespace dust3d
