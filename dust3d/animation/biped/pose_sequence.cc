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
#include <dust3d/animation/biped/secondary_motion.h>
#include <dust3d/base/math.h>
namespace dust3d {
namespace biped {
    double actionEnvelope(double t, double prepare, double recover)
    {
        return easePose(t / prepare) * (1.0 - easePose((t - recover) / (1.0 - recover)));
    }
    void addClipEvent(RigAnimationClip& clip, const char* name, double phase, const char* bone)
    {
        clip.events.push_back({ name, static_cast<float>(phase * clip.durationSeconds), bone });
    }
    PoseSample crouchedPose(double weight)
    {
        PoseSample p;
        p.hips = Vector3(0, -0.28 * weight, 0);
        p.chestPitch = 0.2 * weight;
        for (int i = 0; i < 2; ++i) {
            double side = i == 0 ? 1 : -1;
            p.arms[i].upper = Vector3(side * 0.15, -0.8, 0.4);
            p.arms[i].lower = Vector3(side * 0.1, 0.2, 0.9);
            p.arms[i].weight = weight;
        }
        return p;
    }
    PoseSample seatedPose(double weight, double seatHeight)
    {
        PoseSample p;
        p.hips = Vector3(0, -(1.0 - seatHeight) * weight, -0.1 * weight);
        p.chestPitch = 0.08 * weight;
        for (int i = 0; i < 2; ++i) {
            double side = i == 0 ? 1 : -1;
            p.ankle[i] = Vector3(side * 0.04, 0, 0.5 * weight);
            p.arms[i].upper = Vector3(side * 0.1, -1, 0.15);
            p.arms[i].lower = Vector3(0, -0.35, 0.85);
            p.arms[i].weight = weight;
        }
        return p;
    }
    bool poseSequence(const RigStructure& rig, const std::map<std::string, Matrix4x4>& inverse,
        RigAnimationClip& clip, const AnimationParams& params, const PoseSampler& sample)
    {
        using namespace animation;
        auto idx = buildBoneIndexMap(rig);
        auto rest = restBoneWorldTransforms(rig);
        const char* required[] = { "Root", "Hips", "Spine", "Chest", "Neck", "Head", "LeftUpperLeg", "LeftLowerLeg", "LeftFoot", "RightUpperLeg", "RightLowerLeg", "RightFoot", "LeftShoulder", "LeftUpperArm", "LeftLowerArm", "LeftHand", "RightShoulder", "RightUpperArm", "RightLowerArm", "RightHand" };
        if (!validateRequiredBones(idx, required, sizeof(required) / sizeof(required[0])))
            return false;
        Vector3 up(0, 1, 0), forward = facing(rig, idx), left = Vector3::crossProduct(up, forward);
        auto pos = [&](const std::string& n) { return getBonePos(rig, idx, n); };
        auto end = [&](const std::string& n) { return getBoneEnd(rig, idx, n); };
        double leg = 0.5 * ((pos("LeftUpperLeg") - end("LeftLowerLeg")).length() + (pos("RightUpperLeg") - end("RightLowerLeg")).length());
        if (leg < 1e-8)
            return false;
        auto vector = [&](const Vector3& v) { return left * v.x() + up * v.y() + forward * v.z(); };
        clip.durationSeconds = params.getValue("durationSeconds", 1);
        int count = std::clamp(static_cast<int>(params.getValue("frameCount", 40)), 2, 4999);
        clip.frames.resize(count);
        for (int i = 0; i < count; ++i) {
            double t = double(i) / (clip.loop ? count : count - 1);
            PoseSample p = sample(t);
            auto& frame = clip.frames[i];
            auto& world = frame.boneWorldTransforms;
            frame.time = t * clip.durationSeconds;
            auto liftBody = [&]() {
                double ground = restGroundHeight(rig), minimum = ground;
                for (const auto& bone : world) {
                    if (bone.first == "Root")
                        continue;
                    Matrix4x4 delta = composePose(bone.second, rest.at(bone.first).inverted());
                    minimum = std::min({ minimum, delta.transformPoint(pos(bone.first)).y(), delta.transformPoint(end(bone.first)).y() });
                }
                Matrix4x4 lift;
                lift.translate(up * (ground - minimum));
                for (auto& bone : world)
                    bone.second = composePose(lift, bone.second);
                return lift;
            };
            Matrix4x4 pelvis;
            pelvis.translate(vector(p.hips) * leg);
            pelvis *= rotationAbout(pos("Hips"), up, p.yaw);
            pelvis *= rotationAbout(pos("Hips"), forward, p.roll);
            pelvis *= rotationAbout(pos("Hips"), left, p.pitch);
            world["Root"] = composePose(pelvis, rest.at("Root"));
            world["Hips"] = composePose(pelvis, rest.at("Hips"));
            Matrix4x4 chest = rotationAbout(pelvis.transformPoint(pos("Spine")), pelvis.transformVector(left), p.chestPitch);
            chest *= rotationAbout(pelvis.transformPoint(pos("Spine")), pelvis.transformVector(up), p.chestYaw);
            chest *= pelvis;
            for (const std::string name : { "Spine", "Chest" })
                world[name] = composePose(chest, rest.at(name));
            Matrix4x4 head = rotationAbout(chest.transformPoint(pos("Neck")), chest.transformVector(left), p.headPitch);
            head *= rotationAbout(chest.transformPoint(pos("Neck")), chest.transformVector(up), p.headYaw);
            head *= chest;
            for (const std::string name : { "Neck", "Head" })
                world[name] = composePose(head, rest.at(name));
            for (int side = 0; side < 2; ++side) {
                std::string prefix = side == 0 ? "Left" : "Right";
                Vector3 ankle = end(prefix + "LowerLeg") + vector(p.ankle[side]) * leg;
                if (p.freeFeet)
                    ankle = pelvis.transformPoint(ankle);
                Vector3 knee = vector(p.kneeDirection[side]);
                Vector3 restBend = pos(prefix + "LowerLeg") - (pos(prefix + "UpperLeg") + end(prefix + "LowerLeg")) * 0.5;
                Vector3 restAxis = (end(prefix + "LowerLeg") - pos(prefix + "UpperLeg")).normalized();
                restBend -= restAxis * Vector3::dotProduct(restBend, restAxis);
                if (restBend.lengthSquared() < 1e-12)
                    restBend = forward;
                restBend = pelvis.transformVector(restBend.normalized());
                // A strong perpendicular anatomical pole remains stable as hips
                // lower. Rotate toward explicit floor poses instead of cancelling
                // opposing vectors or normalizing a vanishing authored pole.
                Vector3 targetBend = p.worldKnees ? knee : pelvis.transformVector(knee);
                Vector3 bend = knee.lengthSquared() < 1e-12 ? restBend
                                                            : turnAbout(Vector3(), restBend, targetBend, std::min(1.0, knee.length())).transformVector(restBend);
                poseTwoBoneLeg(rig, idx, prefix + "UpperLeg", prefix + "LowerLeg", prefix + "Foot", pelvis, ankle, bend, p.footFollowWeight < 0 && p.freeFeet, world);
                if (p.footFollowWeight > 0) {
                    auto foot = prefix + "Foot", lower = prefix + "LowerLeg";
                    Matrix4x4 delta = composePose(world.at(lower), rest.at(lower).inverted());
                    Vector3 direction = end(foot) - pos(foot);
                    Vector3 pivot = world.at(foot).transformPoint(Vector3());
                    world[foot] = composePose(turnAbout(pivot, direction, delta.transformVector(direction), p.footFollowWeight), world.at(foot));
                }
            }
            if (p.floorContact && p.worldKnees) {
                // Resolve body/leg penetration before solving support hands. A
                // subsequent whole-body lift would pull planted palms off the floor.
                Matrix4x4 lift = liftBody();
                pelvis = composePose(lift, pelvis);
                chest = composePose(lift, chest);
                head = composePose(lift, head);
            }
            for (int side = 0; side < 2; ++side) {
                std::string prefix = side == 0 ? "Left" : "Right";
                auto a = p.arms[side];
                aimArm(rig, idx, rest, world, prefix, chest, chest.transformVector(vector(a.upper)), chest.transformVector(vector(a.lower)), a.weight, params);
                if (a.targetHand && a.weight * a.targetWeight > 0) {
                    // Solve actual shoulder/elbow/wrist joints so paired hands can
                    // share a weapon shaft or meet at a clap without stretching.
                    std::string upper = prefix + "UpperArm", lower = prefix + "LowerArm", hand = prefix + "Hand";
                    Vector3 shoulder = chest.transformPoint(pos(upper));
                    Vector3 elbow = chest.transformPoint(pos(lower));
                    Vector3 wrist = chest.transformPoint(pos(hand));
                    Vector3 current = world.at(hand).transformPoint(Vector3());
                    Vector3 target = a.targetHead ? head.transformPoint(pos("Head") + vector(a.handTarget) * leg)
                                                  : chest.transformPoint(pos("Chest") + vector(a.handTarget) * leg);
                    if (a.targetGround) {
                        target = pos("Hips") + vector(a.handTarget) * leg;
                        target.setY(restGroundHeight(rig) + a.handTarget.y() * leg);
                    }
                    target = current + (target - current) * (a.weight * a.targetWeight);
                    std::vector<Vector3> chain = { shoulder, elbow, wrist };
                    Vector3 hint = shoulder + chest.transformVector(vector(Vector3(side == 0 ? 1 : -1, -0.3, 0.1))) * leg;
                    if (a.targetGround) {
                        // Enter support IK from the existing elbow plane. Changing
                        // the pole immediately can pop the elbow even when the
                        // wrist target is blended by a very small contact weight.
                        Vector3 currentElbow = world.at(lower).transformPoint(Vector3());
                        hint = currentElbow + (hint - currentElbow) * (a.weight * a.targetWeight);
                    }
                    solveTwoBoneIk(chain, target, hint, 0);
                    Matrix4x4 u = turnAbout(shoulder, elbow - shoulder, chain[1] - shoulder, 1);
                    u *= chest;
                    Vector3 carriedElbow = u.transformPoint(pos(lower));
                    Vector3 carriedWrist = u.transformPoint(pos(hand));
                    Matrix4x4 l = turnAbout(carriedElbow, carriedWrist - carriedElbow, chain[2] - chain[1], 1);
                    l *= u;
                    world[upper] = composePose(u, rest.at(upper));
                    world[lower] = composePose(l, rest.at(lower));
                    world[hand] = composePose(l, rest.at(hand));
                    Vector3 wristPos = world[hand].transformPoint(Vector3());
                    Vector3 direction = a.targetGround ? vector(a.handDirection) : chest.transformVector(vector(a.handDirection));
                    world[hand] = composePose(turnAbout(wristPos, world[hand].transformVector(Vector3(0, 0, 1)), direction, a.weight * a.targetWeight), world[hand]);
                }
            }
            if (p.floorContact && !p.worldKnees) {
                liftBody();
            }
        }
        simulateSecondaryMotion(rig, clip);
        for (auto& frame : clip.frames)
            finishFrame(frame, inverse);
        return true;
    }
} // namespace biped
} // namespace dust3d
