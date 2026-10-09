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
// A planted support foot and a lifted lead-foot lunge carry the thrust.
#include <dust3d/animation/biped/pose.h>
#include <dust3d/animation/biped/stab.h>
#include <dust3d/base/math.h>

namespace dust3d {
namespace biped {

    bool stab(const RigStructure& rig, const std::map<std::string, Matrix4x4>& inverse,
        RigAnimationClip& clip, const AnimationParams& params)
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
        Vector3 up(0, 1, 0), forward = facing(rig, idx);
        Vector3 left = Vector3::crossProduct(up, forward);
        Vector3 hips = getBonePos(rig, idx, "Hips");
        double leg = (getBonePos(rig, idx, "RightUpperLeg") - getBoneEnd(rig, idx, "RightLowerLeg")).length();
        auto factor = [&](const char* key) { return std::clamp(params.getValue(key, 1.0), 0.0, 2.0); };
        double reach = factor("thrustReachFactor"), drive = factor("hipDriveFactor");
        clip.durationSeconds = params.getValue("durationSeconds", 0.7);
        int count = params.getValue("frameCount", 48);
        clip.frames.resize(count);
        for (int i = 0; i < count; ++i) {
            double t = double(i) / (count - 1);
            double mass = 0.7 + 0.3 * factor("bodyMassFactor");
            double strikeSpan = 0.14 * mass / std::max(0.5, factor("thrustSpeedFactor"));
            double recoverySpan = 0.4 / std::max(0.8, factor("retractionSpeedFactor"));
            double prepare = easePose(t / 0.16) * (1.0 - easePose((t - 0.16) / strikeSpan));
            double thrust = easePose((t - 0.16) / strikeSpan) * (1.0 - easePose((t - 0.48) / recoverySpan));
            // The foot plants before impact and remains fixed through the strike.
            double step = easePose((t - 0.14) / 0.16) * (1.0 - easePose((t - 0.66) / 0.26));
            double lift = t < 0.30 ? std::sin(Math::Pi * std::clamp((t - 0.14) / 0.16, 0.0, 1.0))
                                   : std::sin(Math::Pi * std::clamp((t - 0.66) / 0.26, 0.0, 1.0));
            double yaw = -0.12 * prepare * drive + 0.12 * thrust * drive;
            Matrix4x4 pelvis;
            pelvis.translate(forward * (leg * (-0.035 * prepare + 0.09 * thrust) * drive)
                - up * (leg * (0.05 * prepare + 0.035 * thrust) * factor("crouchDepthFactor")));
            pelvis *= rotationAbout(hips, up, yaw);
            auto& f = clip.frames[i];
            auto& world = f.boneWorldTransforms;
            world["Root"] = composePose(pelvis, rest.at("Root"));
            world["Hips"] = composePose(pelvis, rest.at("Hips"));
            Matrix4x4 parent = pelvis;
            for (const std::string name : { "Spine", "Chest", "Neck", "Head" }) {
                Vector3 pivot = parent.transformPoint(getBonePos(rig, idx, name));
                double amount = name == "Spine" ? 1.0 : name == "Chest" ? 0.5
                                                                        : -0.35;
                Matrix4x4 layer = rotationAbout(pivot, up, amount * (0.12 * prepare - 0.18 * thrust) * factor("spineRotateFactor"));
                layer *= rotationAbout(pivot, left, amount * 0.07 * thrust);
                layer *= parent;
                world[name] = composePose(layer, rest.at(name));
                parent = layer;
            }
            posePlantedLeg(rig, idx, "LeftUpperLeg", "LeftLowerLeg", "LeftFoot", pelvis, Vector3(), world);
            posePlantedLeg(rig, idx, "RightUpperLeg", "RightLowerLeg", "RightFoot", pelvis,
                forward * (leg * 0.20 * step * reach) + up * (leg * 0.075 * lift), world);
            Matrix4x4 chest = world.at("Chest");
            chest *= rest.at("Chest").inverted();
            Vector3 rightUpper = (-up * (0.8 * prepare) - forward * (0.5 * prepare) + forward * thrust).normalized();
            Vector3 rightLower = (up * (0.65 * prepare) + forward * (0.3 * prepare + thrust)).normalized();
            aimArm(rig, idx, rest, world, "Right", chest, chest.transformVector(rightUpper), chest.transformVector(rightLower),
                std::clamp(prepare + thrust * reach * (0.75 + 0.25 * factor("shoulderProtractFactor")), 0.0, 1.0), params);
            aimArm(rig, idx, rest, world, "Left", chest, chest.transformVector(-up * 0.75 + forward * 0.5 + left * 0.16),
                chest.transformVector(up * 0.6 + forward * 0.65), std::clamp(prepare + thrust, 0.0, 1.0), params);
            for (const std::string name : { "TailBase", "TailMid", "TailTip" }) {
                if (!rest.count(name))
                    continue;
                Matrix4x4 tail = rotationAbout(pelvis.transformPoint(getBonePos(rig, idx, name)), left,
                    (0.08 * prepare - 0.1 * thrust) * factor("tailReactFactor"));
                tail *= pelvis;
                world[name] = composePose(tail, rest.at(name));
            }
            f.time = t * clip.durationSeconds;
            finishFrame(f, inverse);
        }
        double strikeSpan = 0.14 * (0.7 + 0.3 * factor("bodyMassFactor")) / std::max(0.5, factor("thrustSpeedFactor"));
        clip.events.push_back({ "hit", static_cast<float>((0.16 + strikeSpan) * clip.durationSeconds), "RightHand" });
        return true;
    }

} // namespace biped
} // namespace dust3d
