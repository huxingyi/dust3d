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

// Shared pose engine for guarded, airborne, strike and turn motions. Each
// animation supplies a typed style through its own entry point, like gait.cc.
// Translations are local weight shifts; the controller owns travel and airtime.
#include <dust3d/animation/biped/action.h>
#include <dust3d/animation/biped/pose.h>
#include <dust3d/animation/biped/secondary_motion.h>
#include <dust3d/base/math.h>

namespace dust3d {
namespace biped {

    bool animateAction(const RigStructure& rig, const std::map<std::string, Matrix4x4>& inverse,
        RigAnimationClip& clip, const AnimationParams& params, const ActionStyle& style)
    {
        using namespace animation;
        auto idx = buildBoneIndexMap(rig);
        auto rest = restBoneWorldTransforms(rig);
        const char* required[] = { "Root", "Hips", "Spine", "Chest", "Neck", "Head",
            "LeftUpperLeg", "LeftLowerLeg", "LeftFoot", "RightUpperLeg", "RightLowerLeg", "RightFoot",
            "LeftShoulder", "LeftUpperArm", "LeftLowerArm", "LeftHand",
            "RightShoulder", "RightUpperArm", "RightLowerArm", "RightHand" };
        if (!validateRequiredBones(idx, required, sizeof(required) / sizeof(required[0])))
            return false;
        bool combat = style.kind == ActionKind::CombatIdle, block = style.kind == ActionKind::Block;
        bool fall = style.kind == ActionKind::Fall, start = style.kind == ActionKind::JumpStart;
        bool land = style.kind == ActionKind::Land, slash = style.kind == ActionKind::Slash;
        bool turn = style.kind == ActionKind::Turn, dodge = style.kind == ActionKind::Dodge;
        clip.durationSeconds = params.getValue("durationSeconds", style.durationSeconds);
        int count = params.getValue("frameCount", style.frameCount);
        clip.frames.resize(count);
        Vector3 up(0, 1, 0), forward = facing(rig, idx), left = Vector3::crossProduct(up, forward);
        Vector3 hips = getBonePos(rig, idx, "Hips");
        double leg = (getBonePos(rig, idx, "LeftUpperLeg") - getBoneEnd(rig, idx, "LeftLowerLeg")).length();
        double intensity = std::clamp(params.getValue("actionIntensityFactor", 1.0), 0.0, 1.5);
        for (int i = 0; i < count; ++i) {
            double t = double(i) / (clip.loop ? count : count - 1);
            double phase = 2 * Math::Pi * t;
            double breath = (combat || block || fall) ? 0.004 * std::sin(phase) : 0.0;
            double air = fall ? 1.0 : start ? easePose((t - 0.55) / 0.45)
                : land                      ? 1.0 - easePose(t / 0.35)
                                            : 0.0;
            double crouch = start ? easePose(t / 0.55) * (1.0 - air)
                : land            ? easePose(t / 0.30) * (1.0 - easePose((t - 0.35) / 0.65))
                                  : 0.0;
            double wound = slash ? easePose(t / 0.22) * (1.0 - easePose((t - 0.22) / 0.16)) : 0.0;
            double strike = slash ? easePose((t - 0.22) / 0.16) * (1.0 - easePose((t - 0.52) / 0.48)) : 0.0;
            double evasion = dodge ? easePose(t / 0.28) * (1.0 - easePose((t - 0.5) / 0.5)) : 0.0;
            double yaw = turn ? style.turnDirection
                    * std::clamp(params.getValue("turnAngleDegrees", 90.0), 15.0, 120.0) * Math::Pi / 180.0 * easePose(t)
                : slash ? (-0.25 * wound + 0.35 * strike) * intensity
                        : 0.0;
            Matrix4x4 pelvis;
            pelvis.translate(up * (leg * (breath - 0.16 * crouch * intensity - 0.02 * air - 0.13 * evasion * intensity - (combat || block ? 0.035 : 0.0) - (turn ? 0.06 * std::sin(Math::Pi * t) : 0.0)))
                - left * (leg * 0.09 * evasion * intensity));
            pelvis *= rotationAbout(hips, up, yaw);
            auto& frame = clip.frames[i];
            auto& world = frame.boneWorldTransforms;
            world["Root"] = composePose(pelvis, rest.at("Root"));
            world["Hips"] = composePose(pelvis, rest.at("Hips"));
            Matrix4x4 chest = composePose(rotationAbout(pelvis.transformPoint(getBonePos(rig, idx, "Spine")), left,
                                              0.12 * crouch + 0.2 * evasion),
                pelvis);
            for (const std::string name : { "Spine", "Chest", "Neck", "Head" })
                world[name] = composePose(chest, rest.at(name));
            for (const std::string prefix : { "Left", "Right" }) {
                double sign = prefix == "Left" ? 1.0 : -1.0;
                Vector3 ankle = getBoneEnd(rig, idx, prefix + "LowerLeg");
                double footYaw = 0.0;
                if (turn) {
                    double footT = prefix == "Left" ? std::clamp((t - 0.05) / 0.4, 0.0, 1.0)
                                                    : std::clamp((t - 0.5) / 0.4, 0.0, 1.0);
                    footYaw = style.turnDirection
                        * std::clamp(params.getValue("turnAngleDegrees", 90.0), 15.0, 120.0) * Math::Pi / 180.0 * easePose(footT);
                    ankle = rotationAbout(hips, up, footYaw).transformPoint(ankle) + up * (leg * 0.09 * std::pow(std::sin(Math::Pi * footT), 2));
                } else if (air > 0.0) {
                    ankle += up * (leg * 0.12 * air) + forward * (leg * 0.04 * air);
                } else if (dodge && prefix == "Right") {
                    double step = easePose(t / 0.28) * (1.0 - easePose((t - 0.62) / 0.35));
                    double liftPhase = t < 0.28 ? t / 0.28 : std::clamp((t - 0.62) / 0.35, 0.0, 1.0);
                    ankle -= left * (leg * 0.2 * step);
                    ankle += up * (leg * 0.065 * std::pow(std::sin(Math::Pi * liftPhase), 2));
                }
                // Preserve the model's anatomical knee plane, including the neutral
                // endpoints used by idle. A universal forward pole reshapes animal legs.
                poseTwoBoneLeg(rig, idx, prefix + "UpperLeg", prefix + "LowerLeg", prefix + "Foot", pelvis, ankle, Vector3(), false, world);
                if (turn) {
                    auto& foot = world[prefix + "Foot"];
                    foot = composePose(rotationAbout(foot.transformPoint(Vector3()), up, footYaw), foot);
                }
                Vector3 u = -up + left * (sign * 0.16) + forward * 0.06;
                Vector3 l = u;
                double weight = air;
                if (air > 0) {
                    u = -up * 0.25 + left * (sign * 0.6) + forward * 0.65;
                    l = up * 0.25 + forward * 0.8 + left * (sign * 0.3);
                }
                if (combat || block) {
                    u = -up * 0.75 + forward * 0.5 + left * (sign * 0.18);
                    l = up * (block ? 0.95 : 0.6) + forward * (block ? 0.35 : 0.75) - left * (sign * (block ? 0.3 : 0.0));
                    weight = 1.0;
                } else if (slash) {
                    if (prefix == "Right") {
                        u = -up * (0.15 * wound + 0.2 * strike) - left * (0.85 * wound) + left * (0.75 * strike) + forward * (0.2 * wound + 0.8 * strike);
                        l = up * (0.4 * wound) + forward * 0.85 + left * (0.3 * strike);
                    } else {
                        u = -up * 0.75 + forward * 0.45 + left * 0.16;
                        l = up * 0.6 + forward * 0.75;
                    }
                    weight = std::clamp((wound + strike) * intensity, 0.0, 1.0);
                } else if (dodge) {
                    u = -up * 0.5 + forward * 0.6 + left * (sign * 0.4);
                    l = up * 0.5 + forward * 0.7;
                    weight = evasion;
                } else if (start) {
                    Vector3 prepared = -up - forward * 0.7 + left * (sign * 0.16);
                    Vector3 airborne = -up * 0.25 + left * (sign * 0.6) + forward * 0.65;
                    Vector3 airLower = up * 0.25 + forward * 0.8 + left * (sign * 0.3);
                    u = prepared * (1.0 - air) + airborne * air;
                    l = (prepared + forward * 0.25) * (1.0 - air) + airLower * air;
                    weight = std::clamp(crouch + air, 0.0, 1.0);
                }
                aimArm(rig, idx, rest, world, prefix, chest, chest.transformVector(u), chest.transformVector(l), weight, params);
            }
            frame.time = t * clip.durationSeconds;
        }
        if (slash)
            clip.events.push_back({ "hit", static_cast<float>(0.38 * clip.durationSeconds), "RightHand" });
        simulateSecondaryMotion(rig, clip);
        for (auto& frame : clip.frames)
            finishFrame(frame, inverse);
        return true;
    }
} // namespace biped
} // namespace dust3d
