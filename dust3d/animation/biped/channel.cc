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
// Procedural loopable channel/beam animation for biped rig.
// Arms extended forward, held with organic strain oscillation.
// ALL oscillation frequencies are integer multiples of 2*pi/durationSeconds
// so the animation loops perfectly clean with no pop at the seam.
//
// Frequency palette (durationSeconds = 2.0 default):
//   f=1 cycle/loop : tRad * 1   (slow breathing sway)
//   f=2 cycles/loop: tRad * 2   (medium strain bob)
//   f=3 cycles/loop: tRad * 3   (fast tremor base)
//   f=5 cycles/loop: tRad * 5   (high-freq tremor)
//   f=7 cycles/loop: tRad * 7   (micro-tremor top)
// tRad = t * 2*pi covers exactly one loop.

#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/biped/channel.h>
#include <dust3d/animation/common.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace biped {

    bool channel(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;
        // Envelope helpers
        auto asymEnvelope = [](double t, double onset, double peak, double end) -> double {
            if (t < onset)
                return 0.0;
            if (t < peak) {
                double p = (t - onset) / (peak - onset);
                return p * p * (3.0 - 2.0 * p);
            }
            if (t < end) {
                double p = (t - peak) / (end - peak);
                double inv = 1.0 - p;
                return inv * inv * (3.0 - 2.0 * inv);
            }
            return 0.0;
        };
        auto explosiveEnvelope = [](double t, double onset, double peak, double decay) -> double {
            if (t < onset)
                return 0.0;
            if (t < peak) {
                double p = (t - onset) / (peak - onset);
                return p * p * p;
            }
            return std::exp(-(t - peak) * decay);
        };
        auto tremble = [](double tRad, double seed, double intensity) -> double {
            return intensity * (0.4 * std::sin(tRad * 11.0 + seed * 3.7) + 0.25 * std::sin(tRad * 17.0 + seed * 5.3) + 0.2 * std::sin(tRad * 23.0 + seed * 7.1) + 0.15 * std::sin(tRad * 31.0 + seed * 11.3));
        };

        int frameCount = static_cast<int>(parameters.getValue("frameCount", 64));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 2.0));
        double channelIntensityFactor = parameters.getValue("channelIntensityFactor", 1.0);
        double bodyMassFactor = parameters.getValue("bodyMassFactor", 1.0);
        double armHoldAngleFactor = parameters.getValue("armHoldAngleFactor", 1.0);
        double tailChannelFactor = parameters.getValue("tailChannelFactor", 1.0);
        double trembleAmplitudeFactor = parameters.getValue("trembleAmplitudeFactor", 1.0);
        double breathingStrainFactor = parameters.getValue("breathingStrainFactor", 1.0);
        double bodyLeanFactor = parameters.getValue("bodyLeanFactor", 1.0);
        double headLockParam = parameters.getValue("headLockFactor", 0.55);
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
            "RightUpperLeg", "RightLowerLeg", "RightFoot",
            "LeftShoulder", "LeftUpperArm", "LeftLowerArm", "LeftHand",
            "RightShoulder", "RightUpperArm", "RightLowerArm", "RightHand"
        };
        if (!validateRequiredBones(boneIdx, requiredBones,
                sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;

        Vector3 upDir(0.0, 1.0, 0.0);
        Vector3 hipsPos = bonePos("Hips");
        Vector3 avgFootEnd = (boneEnd("LeftFoot") + boneEnd("RightFoot")) * 0.5;
        Vector3 hipsToFoot = avgFootEnd - hipsPos;
        Vector3 forward(hipsToFoot.x(), 0.0, hipsToFoot.z());
        if (forward.lengthSquared() < 1e-8)
            forward = Vector3(0.0, 0.0, 1.0);
        forward.normalize();
        Vector3 right = Vector3::crossProduct(upDir, forward);
        if (right.lengthSquared() < 1e-8)
            right = Vector3(1.0, 0.0, 0.0);
        right.normalize();

        double legLen = ((boneEnd("LeftUpperLeg") - bonePos("LeftUpperLeg")).length()
                            + (boneEnd("LeftLowerLeg") - bonePos("LeftLowerLeg")).length()
                            + (boneEnd("LeftFoot") - bonePos("LeftFoot")).length()
                            + (boneEnd("RightUpperLeg") - bonePos("RightUpperLeg")).length()
                            + (boneEnd("RightLowerLeg") - bonePos("RightLowerLeg")).length()
                            + (boneEnd("RightFoot") - bonePos("RightFoot")).length())
            * 0.5;
        if (legLen < 1e-6)
            legLen = 1.0;

        double massInertia = 0.6 + 0.4 * bodyMassFactor;
        (void)massInertia;

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        // Hair chain physics: steady sustained sway from channeling energy.
        const std::vector<std::string> hairBoneNames = { "HairBack1", "HairBack2", "HairBack3" };
        animation::HairChainSimulator hairSim;
        if (boneIdx.count("HairBack1"))
            hairSim.initialize(rigStructure, boneIdx, hairBoneNames,
                animation::buildBoneWorldTransform(bonePos("Head"), boneEnd("Head")),
                0.08, 0.93, 1.0);
        double hairDt = durationSeconds / std::max(1, frameCount);

        animation::CapeGridSimulator capeSim;
        if (boneIdx.count("CenterCape1"))
            capeSim.initialize(rigStructure, boneIdx,
                animation::buildBoneWorldTransform(bonePos("Chest"), boneEnd("Chest")),
                0.08, 0.85, 1.2, 0.15);

        for (int pass = 0; pass < 2; ++pass) {
            for (int frame = 0; frame < frameCount; ++frame) {
                double t = static_cast<double>(frame) / static_cast<double>(frameCount);
                // tRad covers exactly 2*pi per loop — guarantees loop-clean oscillations
                double tRad = t * 2.0 * Math::Pi;
                std::map<std::string, Matrix4x4> boneWorldTransforms;

                // === BODY OSCILLATIONS (all integer harmonics, loop-safe) ===
                // Slow breathing sway (1 cycle/loop): body rises and falls
                double breathSway = std::sin(tRad * 1.0) * 0.006 * legLen * channelIntensityFactor * breathingStrainFactor;
                // Medium strain bob (2 cycles/loop): slight weight shift
                double strainBob = std::sin(tRad * 2.0 + 0.8) * 0.004 * legLen * channelIntensityFactor;
                // Micro lean forward (sustained casting posture, no oscillation needed)
                double forwardLean = legLen * 0.04 * channelIntensityFactor * bodyLeanFactor;

                // Spine oscillation: 1 cycle breathing pitch + 3 cycle tremor
                double spinePitch = std::sin(tRad * 1.0) * 0.025 * channelIntensityFactor
                    + std::sin(tRad * 3.0 + 1.2) * 0.010 * channelIntensityFactor * trembleAmplitudeFactor;
                // Head stabilizes against spine (reduces wobble reaching head)
                double headLockFactor = headLockParam; // proportion of spine pitch cancelled at head

                Matrix4x4 bodyTransform;
                bodyTransform.translate(upDir * (breathSway + strainBob) + forward * forwardLean);
                // FK chain: tracks each bone's animated world-space end position.
                std::map<std::string, Vector3> boneChainEnd;
                auto computeChainBone = [&](const std::string& name, const std::string& parentName,
                                            double extraYaw = 0.0, double extraPitch = 0.0, double extraRoll = 0.0) {
                    Vector3 pos = bonePos(name), end = boneEnd(name);
                    // Bone direction in body space -- preserves rest-pose bone length.
                    Vector3 boneDir = bodyTransform.transformVector(end - pos);
                    // Apply extra rotations to the direction only (not to the start pos).
                    if (std::abs(extraYaw) > 1e-6 || std::abs(extraPitch) > 1e-6 || std::abs(extraRoll) > 1e-6) {
                        Matrix4x4 r;
                        if (std::abs(extraRoll) > 1e-6)
                            r.rotate(forward, extraRoll);
                        if (std::abs(extraYaw) > 1e-6)
                            r.rotate(upDir, extraYaw);
                        if (std::abs(extraPitch) > 1e-6)
                            r.rotate(right, extraPitch);
                        boneDir = r.transformVector(boneDir);
                    }
                    // Start: parent's animated end + the rest-pose gap DISTANCE offset
                    // along boneDir. This keeps |parent-end to child-begin| equal to the
                    // rest-pose distance without locking the gap direction, so the child
                    // always continues naturally from the parent with no mesh shrinkage.
                    Vector3 newPos;
                    if (!parentName.empty() && boneChainEnd.count(parentName) > 0) {
                        double gapDist = (pos - boneEnd(parentName)).length();
                        if (gapDist > 1e-8) {
                            double boneDirLen = boneDir.length();
                            Vector3 gapOffset = (boneDirLen > 1e-8)
                                ? boneDir * (gapDist / boneDirLen)
                                : Vector3();
                            newPos = boneChainEnd[parentName] + gapOffset;
                        } else {
                            newPos = boneChainEnd[parentName];
                        }
                    } else {
                        newPos = bodyTransform.transformPoint(pos);
                    }
                    Vector3 newEnd = newPos + boneDir;
                    boneChainEnd[name] = newEnd;
                    boneWorldTransforms[name] = buildBoneWorldTransform(newPos, newEnd);
                };

                computeChainBone("Root", "");
                computeChainBone("Hips", "");
                computeChainBone("Spine", "Hips", 0.0, spinePitch * 0.55);
                computeChainBone("Chest", "Spine", 0.0, spinePitch * 0.35);
                computeChainBone("Neck", "Chest", 0.0, -spinePitch * headLockFactor * 0.50);
                computeChainBone("Head", "Neck", 0.0, -spinePitch * headLockFactor * 0.50);

                // Tail: slow lateral sway (1 cycle/loop), tip with larger amplitude
                {
                    static const char* tailBones[] = { "TailBase", "TailMid", "TailTip" };
                    Vector3 prevEnd;
                    bool hasPrev = false;
                    for (int ti = 0; ti < 3; ++ti) {
                        if (boneIdx.count(tailBones[ti]) == 0)
                            continue;
                        // Each segment lags by 1 integer harmonic (loop safe)
                        int harmonic = 1 + ti;
                        double phase = ti * 0.7;
                        double tailYaw = tailChannelFactor * std::sin(tRad * harmonic + phase)
                            * 0.12 * channelIntensityFactor * (1.0 + ti * 0.4);
                        Vector3 pos = bonePos(tailBones[ti]), end = boneEnd(tailBones[ti]);
                        Vector3 newPos = bodyTransform.transformPoint(pos);
                        Vector3 newEnd = bodyTransform.transformPoint(end);
                        if (hasPrev) {
                            newPos = prevEnd;
                            newEnd = newPos + (newEnd - newPos);
                        }
                        if (std::abs(tailYaw) > 1e-6) {
                            Matrix4x4 r;
                            r.rotate(upDir, tailYaw);
                            newEnd = newPos + r.transformVector(newEnd - newPos);
                        }
                        boneWorldTransforms[tailBones[ti]] = buildBoneWorldTransform(newPos, newEnd);
                        prevEnd = newEnd;
                        hasPrev = true;
                    }
                }
                auto computeLeg = [&](const char* ul, const char* ll, const char* f, double lift) {
                    Vector3 footStart = bonePos(f) + upDir * lift;
                    Vector3 footVec = boneEnd(f) - bonePos(f); // rest-pose foot direction+length
                    Vector3 hipJoint = bodyTransform.transformPoint(bonePos(ul));
                    double upperLen = (boneEnd(ul) - bonePos(ul)).length();
                    double lowerLen = (boneEnd(ll) - bonePos(ll)).length();
                    Vector3 midBind = bodyTransform.transformPoint(bonePos(ll));
                    Vector3 poleTarget = midBind + forward * (upperLen * 0.5);
                    // Init joints with rest-pose lengths so IK never stretches bones
                    Vector3 hipToKnee = midBind - hipJoint;
                    double hkLen = hipToKnee.length();
                    Vector3 kneeInit = (hkLen > 1e-6) ? hipJoint + hipToKnee * (upperLen / hkLen) : hipJoint + upDir * (-upperLen);
                    Vector3 kneeToFoot = footStart - kneeInit;
                    double kfLen = kneeToFoot.length();
                    Vector3 ankleInit = (kfLen > 1e-6) ? kneeInit + kneeToFoot * (lowerLen / kfLen) : kneeInit + upDir * (-lowerLen);
                    std::vector<Vector3> joints = { hipJoint, kneeInit, ankleInit };
                    solveTwoBoneIk(joints, footStart, poleTarget, 0.05);
                    boneWorldTransforms[ul] = buildBoneWorldTransform(joints[0], joints[1]);
                    boneWorldTransforms[ll] = buildBoneWorldTransform(joints[1], joints[2]);
                    // Foot starts at IK-solved ankle so there is no gap with the lower leg
                    boneWorldTransforms[f] = buildBoneWorldTransform(joints[2], joints[2] + footVec);
                };
                computeLeg("LeftUpperLeg", "LeftLowerLeg", "LeftFoot", 0.0);
                computeLeg("RightUpperLeg", "RightLowerLeg", "RightFoot", 0.0);

                // ARMS: extended forward with loopable micro-fluctuations
                for (int side = 0; side < 2; ++side) {
                    bool isLeft = (side == 0);
                    double sideSign = isLeft ? -1.0 : 1.0;
                    double phaseOff = isLeft ? Math::Pi : 0.0; // arms slightly out of phase
                    const char* shoulder = isLeft ? "LeftShoulder" : "RightShoulder";
                    const char* upper = isLeft ? "LeftUpperArm" : "RightUpperArm";
                    const char* lower = isLeft ? "LeftLowerArm" : "RightLowerArm";
                    const char* hand = isLeft ? "LeftHand" : "RightHand";

                    // Shoulder start: chest's FK-propagated end so the arm chain
                    // is physically connected to the spine (no gap under spine rotation).
                    Vector3 shPos = boneChainEnd.count("Chest") > 0
                        ? boneChainEnd["Chest"]
                        : bodyTransform.transformPoint(bonePos(shoulder));
                    Vector3 shEnd = shPos + bodyTransform.transformVector(boneEnd(shoulder) - bonePos(shoulder));
                    // Shoulder: 2 cycles/loop micro-shift
                    {
                        double shFluct = std::sin(tRad * 2.0 + phaseOff) * 0.015 * channelIntensityFactor;
                        Matrix4x4 r;
                        r.rotate(right, shFluct);
                        shEnd = shPos + r.transformVector(shEnd - shPos);
                    }
                    boneWorldTransforms[shoulder] = buildBoneWorldTransform(shPos, shEnd);

                    Vector3 upperStart = shEnd;
                    Vector3 upperDir = bodyTransform.transformVector(boneEnd(upper) - bonePos(upper));
                    // Upper arm: pitched forward to aim arms forward, with 3 cycle tremor
                    double upperPitch = -0.55 * armHoldAngleFactor * channelIntensityFactor // forward extension
                        + std::sin(tRad * 3.0 + phaseOff + 0.4) * 0.025 * channelIntensityFactor;
                    double upperYaw = sideSign * (-0.12) * armHoldAngleFactor * channelIntensityFactor // slight inward
                        + std::sin(tRad * 2.0 + phaseOff) * 0.015 * channelIntensityFactor;
                    Matrix4x4 r1;
                    r1.rotate(right, upperPitch);
                    r1.rotate(upDir, upperYaw);
                    Vector3 upperEnd = upperStart + r1.transformVector(upperDir);
                    boneWorldTransforms[upper] = buildBoneWorldTransform(upperStart, upperEnd);

                    // Forearm: nearly extended, 5-cycle strain tremor
                    Vector3 lowerDir = bodyTransform.transformVector(boneEnd(lower) - bonePos(lower));
                    double elbowBend = 0.10 * armHoldAngleFactor // slight bend (not fully locked)
                        + std::sin(tRad * 5.0 + phaseOff + 1.0) * 0.020 * channelIntensityFactor;
                    Matrix4x4 r2;
                    r2.rotate(right, upperPitch * 0.50 + elbowBend);
                    Vector3 lowerEnd = upperEnd + r2.transformVector(lowerDir);
                    boneWorldTransforms[lower] = buildBoneWorldTransform(upperEnd, lowerEnd);

                    // Hand: 7-cycle micro-tremor (highest frequency, least amplitude)
                    Vector3 handDir = bodyTransform.transformVector(boneEnd(hand) - bonePos(hand));
                    double wristTremor = std::sin(tRad * 7.0 + phaseOff + 2.3) * 0.018 * channelIntensityFactor
                        + std::sin(tRad * 5.0 + phaseOff + 0.7) * 0.014 * channelIntensityFactor;
                    Matrix4x4 r3;
                    r3.rotate(right, wristTremor);
                    r3.rotate(upDir, wristTremor * 0.5);
                    Vector3 handEnd = lowerEnd + r3.transformVector(handDir);
                    boneWorldTransforms[hand] = buildBoneWorldTransform(lowerEnd, handEnd);
                }
                if (pass == 1) {
                    auto& animFrame = animationClip.frames[frame];
                    animFrame.time = static_cast<float>(t) * durationSeconds;
                    if (hairSim.active)
                        hairSim.step(boneWorldTransforms["Head"], hairDt, boneWorldTransforms);
                    if (capeSim.active)
                        capeSim.step(boneWorldTransforms["Chest"], hairDt, boneWorldTransforms);
                    animFrame.boneWorldTransforms = boneWorldTransforms;
                    for (const auto& pair : boneWorldTransforms) {
                        auto invIt = inverseBindMatrices.find(pair.first);
                        if (invIt != inverseBindMatrices.end()) {
                            Matrix4x4 skinMat = pair.second;
                            skinMat *= invIt->second;
                            animFrame.boneSkinMatrices[pair.first] = skinMat;
                        }
                    }
                } else {
                    if (hairSim.active)
                        hairSim.step(boneWorldTransforms["Head"], hairDt, boneWorldTransforms);
                    if (capeSim.active)
                        capeSim.step(boneWorldTransforms["Chest"], hairDt, boneWorldTransforms);
                }
            }
        } // end pass
        return true;
    }

} // namespace biped

} // namespace dust3d
