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

// Procedural death for the biped rig: the authored fall a game plays.
//
// One-shot, ends lying still. The knees buckle and the upper body slumps, then
// the body topples over onto its back (or face down) and hits the ground with a
// small bounce: the legs slide out, the arms fall open onto the ground, the head
// rolls to one side and a tail lies along the ground. Every bone is kept on or
// above the ground (using the rig's capsule radii); hair and capes follow the
// head and chest.
//
// Adjustable animation parameters:
//   - collapseSpeedFactor:  how fast the body goes down (> 1 = sooner)
//   - fallDirection:        -1 = onto its back, 1 = face down, 0 = onto its side
//                           (for big-tailed bipeds: kangaroos, dinosaurs)
//   - fallSide:             for a fall onto the side: 1 = its right, -1 = its left
//   - armFlailFactor:       how far the arms fall open
//   - headDropFactor:       how far the head rolls and drops
//   - legBuckleFactor:      how far the knees buckle before the fall
//   - groundBounce:         how much the body rebounds when it hits the ground

#include <algorithm>
#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/biped/die.h>
#include <dust3d/animation/common.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace biped {

    bool die(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = std::max(2, static_cast<int>(parameters.getValue("frameCount", 40)));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 1.3));

        auto boneIdx = buildBoneIndexMap(rigStructure);
        auto bonePos = [&](const std::string& name) -> Vector3 {
            return getBonePos(rigStructure, boneIdx, name);
        };
        auto boneEnd = [&](const std::string& name) -> Vector3 {
            return getBoneEnd(rigStructure, boneIdx, name);
        };

        static const char* requiredBones[] = {
            "Root", "Hips", "Spine", "Chest", "Neck", "Head",
            "LeftUpperLeg", "LeftLowerLeg", "LeftFoot",
            "RightUpperLeg", "RightLowerLeg", "RightFoot"
        };
        if (!validateRequiredBones(boneIdx, requiredBones, sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;

        double speed = std::max(0.3, parameters.getValue("collapseSpeedFactor", 1.0));
        double fallDirectionValue = parameters.getValue("fallDirection", -1.0);
        bool sideways = std::abs(fallDirectionValue) < 0.5;
        double fallDirection = sideways ? 0.0 : (fallDirectionValue > 0.0 ? 1.0 : -1.0);
        double fallSide = parameters.getValue("fallSide", 1.0) >= 0.0 ? 1.0 : -1.0;
        double armFlail = parameters.getValue("armFlailFactor", 1.0);
        double headDrop = parameters.getValue("headDropFactor", 1.0);
        double legBuckle = parameters.getValue("legBuckleFactor", 1.0);
        double bounce = 0.35 * std::clamp(parameters.getValue("groundBounce", 0.22), 0.0, 1.0);

        Vector3 up(0.0, 1.0, 0.0);
        Vector3 toes = (boneEnd("LeftFoot") + boneEnd("RightFoot")) * 0.5 - (boneEnd("LeftLowerLeg") + boneEnd("RightLowerLeg")) * 0.5;
        Vector3 forward(toes.x(), 0.0, toes.z());
        if (forward.lengthSquared() < 1e-10)
            forward = Vector3(0.0, 0.0, 1.0);
        forward.normalize();
        // The creature's right: a positive rotation about it tips the body backwards.
        Vector3 right = Vector3::crossProduct(forward, up).normalized();

        double groundY = restGroundHeight(rigStructure);
        Vector3 hips = bonePos("Hips");
        double legLength = (boneEnd("RightUpperLeg") - bonePos("RightUpperLeg")).length()
            + (boneEnd("RightLowerLeg") - bonePos("RightLowerLeg")).length();
        double torsoRadius = 0.0;
        for (const char* name : { "Hips", "Spine", "Chest" })
            torsoRadius = std::max(torsoRadius, static_cast<double>(rigStructure.bones[boneIdx[name]].capsuleRadius));
        if (torsoRadius < 1e-4)
            torsoRadius = 0.25 * legLength;
        double hipsHeight = hips.y() - groundY;
        // On its side it rests on the wider of the torso radius and the hip half-width.
        double hipHalfWidth = std::abs(Vector3::dotProduct(bonePos("LeftUpperLeg") - hips, right))
            + rigStructure.bones[boneIdx["LeftUpperLeg"]].capsuleRadius;
        double lyingHeight = std::min(hipsHeight, 1.1 * (sideways ? std::max(torsoRadius, hipHalfWidth) : torsoRadius));

        std::map<std::string, Matrix4x4> rest = restBoneWorldTransforms(rigStructure);
        double thighLength = (boneEnd("RightUpperLeg") - bonePos("RightUpperLeg")).length();
        bool faceDown = !sideways && fallDirection > 0.0;
        // Where the hips come to rest when the legs give way: sitting on the ground (falling
        // back or to the side) or kneeling (falling face down).
        double hipsLandHeight = faceDown ? std::max(lyingHeight, 0.8 * thighLength) : lyingHeight;
        Vector3 fallAxis = sideways ? forward : right;
        double fallSign = sideways ? fallSide : -fallDirection;
        Vector3 fallHorizontal = sideways ? right * fallSide : forward * fallDirection;

        // Timeline (normalised; collapseSpeedFactor scales it): the blow at 0, the legs give
        // way and the hips land at hipLand, the torso topples and hits the ground at bodyLand
        // (gravity: slow to start, fastest at the impact), then a small rebound and the
        // limbs settle.
        const double hipLand = faceDown ? 0.34 : 0.4;
        const double toppleStart = faceDown ? 0.26 : 0.18;
        const double bodyLand = 0.62;
        auto legsGive = [&](double x) { return fallEnvelope(x - 0.04, hipLand - 0.04, 0.4 * bounce, 0.1); };
        auto topple = [&](double x) { return fallEnvelope(x - toppleStart, bodyLand - toppleStart, bounce, 0.14); };
        const double toppleRatePeak = 2.0 / (bodyLand - toppleStart);

        double forearmFacing[2] = { 0.0, 0.0 };

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);
        std::map<std::string, Vector3> groundDirections;

        for (int frame = 0; frame < frameCount; ++frame) {
            double t = static_cast<double>(frame) / static_cast<double>(frameCount - 1);
            double x = t * speed;
            double blow = x < 0.16 ? std::pow(std::sin(Math::Pi * x / 0.16), 2.0) : 0.0;
            double give = legsGive(x);
            double fall = topple(x);
            double fallRate = x < bodyLand ? normalisedRate(topple, x, toppleRatePeak) : 1.0 - smoothstep((x - bodyLand) / 0.12);
            double impact = impactWobble(x - bodyLand, 0.16, 0.07);
            double limp = smoothstep((x - bodyLand + 0.06) / 0.3);
            double slideOut = smoothstep((x - hipLand + 0.05) / (bodyLand - hipLand + 0.15));

            // Body: the hips drop as the legs give way (and back, behind the feet, or forward
            // onto the knees), then the torso topples about the hips onto the ground.
            double drop = (hipsHeight - hipsLandHeight) * legBuckle * give + (hipsLandHeight - lyingHeight) * fall;
            drop = std::min(drop, hipsHeight - lyingHeight);
            Vector3 slide = fallHorizontal * (legLength * ((faceDown ? 0.08 : 0.22) * give + (faceDown ? 0.4 : 0.12) * fall));
            double recoil = 0.16 * blow; // the blow jolts the chest toward the fall
            double hunch = (faceDown ? 0.25 : -0.12) * give * (1.0 - fall); // counterbalance while dropping
            Matrix4x4 body;
            body.translate(hips - up * drop + slide);
            body.rotate(fallAxis, fallSign * (0.5 * Math::Pi * fall + recoil));
            if (!sideways)
                body.rotate(right, -fallDirection * hunch);
            body.translate(Vector3() - hips);

            std::map<std::string, Matrix4x4> world = rest;
            std::map<std::string, Matrix4x4> layers;
            auto apply = [&](const std::string& name, const Matrix4x4& layer) {
                if (!boneIdx.count(name))
                    return;
                layers[name] = layer;
                Matrix4x4 m = layer;
                m *= rest[name];
                world[name] = m;
            };
            for (const char* name : { "Hips", "Spine", "Chest" })
                apply(name, body);

            // Head: trails the falling torso (chin toward the chest), whips on the impact,
            // then rolls to one side and rests.
            Vector3 neckJoint = body.transformPoint(bonePos("Neck"));
            Vector3 spineAxis = body.transformVector(bonePos("Neck") - hips).normalized();
            Vector3 bodyFallAxis = body.transformVector(fallAxis);
            double headLag = -0.45 * fallRate + 0.35 * impact - 0.25 * blow;
            Matrix4x4 neck = rotationAbout(neckJoint, bodyFallAxis, fallSign * headLag * headDrop);
            neck *= rotationAbout(neckJoint, spineAxis, 0.75 * headDrop * limp);
            neck *= body;
            apply("Neck", neck);
            apply("Head", neck);

            // Arms: flung up by the fall (they trail the body), then flop open onto the ground,
            // elbows bent, not symmetric.
            for (const char* side : { "Left", "Right" }) {
                std::string shoulder = std::string(side) + "Shoulder";
                std::string upperArm = std::string(side) + "UpperArm";
                std::string lowerArm = std::string(side) + "LowerArm";
                std::string hand = std::string(side) + "Hand";
                if (!boneIdx.count(upperArm))
                    continue;
                apply(shoulder, body);
                Vector3 joint = body.transformPoint(bonePos(upperArm));
                Vector3 armDir = body.transformVector(boneEnd(upperArm) - bonePos(upperArm));
                double sideSign = Vector3::dotProduct(boneEnd(upperArm) - hips, right) >= 0.0 ? 1.0 : -1.0;
                bool lead = sideSign > 0.0; // the two arms land differently
                Vector3 outward = right * sideSign;
                Vector3 headward = Vector3(spineAxis.x(), 0.0, spineAxis.z());
                headward = headward.lengthSquared() > 1e-9 ? headward.normalized() : Vector3();
                Vector3 openDir;
                if (sideways)
                    openDir = forward * 0.85 + headward * (lead ? 0.35 : -0.1) - up * 0.25;
                else if (faceDown)
                    openDir = outward * (lead ? 0.55 : 0.3) - headward * 0.8 - up * 0.1;
                else
                    openDir = outward * 0.85 + headward * (lead ? 0.5 : 0.05) - up * 0.1;
                // While falling the arm swings toward the side the body falls away from.
                Vector3 flungDir = body.transformVector(up * 0.5 - fallHorizontal * 0.8 + outward * 0.4);
                Matrix4x4 fling = turnAbout(joint, armDir, flungDir, armFlail * 0.45 * fallRate);
                Matrix4x4 arm = turnAbout(joint, fling.transformVector(armDir), openDir, armFlail * limp);
                arm *= fling;
                arm *= body;
                apply(upperArm, arm);
                Vector3 elbow = arm.transformPoint(bonePos(lowerArm));
                double elbowBend = (lead ? 0.6 : 0.3) * limp + 0.3 * fallRate;
                Matrix4x4 forearm = rotationAbout(elbow, up, sideSign * (sideways || faceDown ? -1.0 : 1.0) * elbowBend);
                forearm *= arm;
                // The forearm rolls so anything held (pointing forward or back at rest) lies flat.
                Vector3 restForearm = boneEnd(lowerArm) - bonePos(lowerArm);
                Vector3 weaponPlaneNormal = Vector3::crossProduct(restForearm, forward);
                Matrix4x4 roll = rollToFaceUp(forearm, elbow, restForearm, weaponPlaneNormal, limp, &forearmFacing[lead ? 1 : 0]);
                roll *= forearm;
                apply(lowerArm, roll);
                apply(hand, roll);
            }

            // Tail goes limp with the body (clamped onto the ground below).
            for (const char* name : { "TailBase", "TailMid", "TailTip" })
                apply(name, body);

            // Legs: planted while they give way, then slide out as the body goes down; lying,
            // one leg is nearly straight and the other bent with the knee fallen outward, and
            // both feet turn out.
            for (const char* side : { "Left", "Right" }) {
                std::string upper = std::string(side) + "UpperLeg";
                std::string lower = std::string(side) + "LowerLeg";
                std::string foot = std::string(side) + "Foot";
                double sideSign = Vector3::dotProduct(bonePos(upper) - hips, right) >= 0.0 ? 1.0 : -1.0;
                bool lead = sideSign > 0.0;
                Vector3 ankleRest = boneEnd(lower);
                Vector3 hipJoint = body.transformPoint(bonePos(upper));
                // Lying, the legs point away from where the head went down.
                Vector3 alongLegs = fallHorizontal * -1.0;
                Vector3 outward = sideways ? forward : right * sideSign;
                double reach = (lead ? 0.92 : 0.74) * legLength;
                Vector3 lying = hipJoint + alongLegs * reach + outward * (0.12 * legLength);
                lying = Vector3(lying.x(), ankleRest.y(), lying.z());
                Vector3 target = ankleRest + (lying - ankleRest) * slideOut;
                // Going down onto the knees and lying face down, the toes stay tucked under: the
                // heels rise by the foot's length, so the foot rests on its toes.
                if (faceDown && boneIdx.count(foot))
                    target.setY(target.y() + ((boneEnd(foot) - bonePos(foot)).length() + rigStructure.bones[boneIdx[foot]].capsuleRadius) * std::min(1.0, std::max(2.0 * give, slideOut)));
                // Knees point the way the body faces (they barely bend at rest, so the rest bend
                // gives no clear direction); lying on its back they fall open, outward and up.
                Vector3 kneeForward = body.transformVector(forward);
                Vector3 bend = (sideways || faceDown) ? kneeForward
                                                      : kneeForward * (1.0 - slideOut) + (outward * (lead ? 0.6 : 1.0) + up * 0.6) * slideOut;
                poseTwoBoneLeg(rigStructure, boneIdx, upper, lower, foot, body, target, bend, true, world);
                // Dead legs roll outward: the toes turn out.
                if (!sideways && boneIdx.count(lower)) {
                    Vector3 knee = world[lower].transformPoint(Vector3());
                    Vector3 shin = world[lower].transformPoint(Vector3(0.0, 0.0, 1.0)) - knee;
                    Matrix4x4 turnOut = rotationAbout(knee, shin.normalized(), -sideSign * fallDirection * (faceDown ? 0.15 : 0.6) * limp);
                    for (const std::string& name : { lower, foot }) {
                        if (!world.count(name))
                            continue;
                        Matrix4x4 m = turnOut;
                        m *= world[name];
                        world[name] = m;
                    }
                }
            }

            // Anything else (hair, capes, eyelids, accessories) follows its nearest moved ancestor.
            for (const auto& bone : rigStructure.bones) {
                if (layers.count(bone.name) || bone.name == "Root" || bone.name.find("Leg") != std::string::npos
                    || bone.name.find("Foot") != std::string::npos)
                    continue;
                std::string parent = bone.parent;
                while (!parent.empty() && !layers.count(parent)) {
                    auto it = boneIdx.find(parent);
                    parent = it == boneIdx.end() ? std::string() : rigStructure.bones[it->second].parent;
                }
                if (!parent.empty())
                    apply(bone.name, layers[parent]);
            }

            auto& animFrame = animationClip.frames[frame];
            animFrame.time = static_cast<float>(frame) / static_cast<float>(frameCount - 1) * durationSeconds;
            animFrame.boneWorldTransforms = world;
            finishFrame(animFrame, inverseBindMatrices);
            keepBonesAboveGround(
                rigStructure, boneIdx, inverseBindMatrices, animFrame, groundY, [](const std::string& name) { return name != "Root" && name != "Hips" && name != "Spine" && name != "Chest"; }, 0.8, true, &groundDirections);
        }
        return true;
    }

} // namespace biped

} // namespace dust3d
