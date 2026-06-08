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
// Procedural forward stab/thrust for biped rig.
// Right arm coils back, hip drives forward, arm extends explosively along
// creature's forward axis. explosiveEnvelope, per-bone delays, body
// translation forward, left arm rises to guard.

#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/biped/stab.h>
#include <dust3d/animation/common.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace biped {

    bool stab(const RigStructure& rigStructure,
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
        // Damped oscillation: bone overshoots at impulse then rings back to rest.
        auto dampedRing = [](double t, double onset, double freq, double decay) -> double {
            if (t < onset)
                return 0.0;
            double dt = t - onset;
            return std::exp(-decay * dt) * std::sin(freq * dt);
        };

        int frameCount = static_cast<int>(parameters.getValue("frameCount", 48));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 0.7));
        double thrustReachFactor = parameters.getValue("thrustReachFactor", 1.0);
        double hipDriveFactor = parameters.getValue("hipDriveFactor", 1.0);
        double bodyMassFactor = parameters.getValue("bodyMassFactor", 1.0);
        double crouchDepthFactor = parameters.getValue("crouchDepthFactor", 1.0);
        double retractionSpeedFactor = parameters.getValue("retractionSpeedFactor", 1.0);
        double tailReactFactor = parameters.getValue("tailReactFactor", 1.0);
        double thrustSpeedFactor = parameters.getValue("thrustSpeedFactor", 1.0);
        double spineRotateFactor = parameters.getValue("spineRotateFactor", 1.0);
        double shoulderProtractFactor = parameters.getValue("shoulderProtractFactor", 1.0);
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

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        // Hair chain physics: quick forward thrust causes whip-back lag.
        const std::vector<std::string> hairBoneNames = { "HairBack1", "HairBack2", "HairBack3" };
        animation::HairChainSimulator hairSim;
        if (boneIdx.count("HairBack1"))
            hairSim.initialize(rigStructure, boneIdx, hairBoneNames,
                animation::buildBoneWorldTransform(bonePos("Head"), boneEnd("Head")),
                0.14, 0.85, 1.0);
        double hairDt = durationSeconds / std::max(1, frameCount);

        animation::CapeGridSimulator capeSim;
        if (boneIdx.count("CenterCape1"))
            capeSim.initialize(rigStructure, boneIdx,
                animation::buildBoneWorldTransform(bonePos("Chest"), boneEnd("Chest")),
                0.08, 0.85, 1.2, 0.15);

        for (int pass = 0; pass < 2; ++pass) {
            for (int frame = 0; frame < frameCount; ++frame) {
                double t = static_cast<double>(frame) / static_cast<double>(frameCount);
                std::map<std::string, Matrix4x4> boneWorldTransforms;

                // Coil back: hip rotates away, arm pulls back [0.0 - 0.22]
                double coilBack = asymEnvelope(t, 0.0, 0.10, 0.24);
                // Thrust: explosive extension forward [0.22 - 0.32]
                double thrustBurst = explosiveEnvelope(t, 0.20, 0.30, 6.0 * thrustSpeedFactor / massInertia) * thrustReachFactor;
                // Hit-stop: smooth bell so the extended hold eases in/out without frame jumps
                double hitStopVal = asymEnvelope(t, 0.30, 0.35, 0.45);
                // Follow-through [0.40 - 0.56]
                double followThru = asymEnvelope(t, 0.38, 0.50, 0.60) * retractionSpeedFactor;
                // Recovery [0.60 - 1.0]
                double recover = asymEnvelope(t, 0.60, 0.78, 1.0);

                // Hip: rotates away (right side back) during coil, drives forward on thrust
                double hipYaw = coilBack * (-0.22) * hipDriveFactor // coil: hips pull right back
                    + thrustBurst * 0.18 * thrustReachFactor / massInertia // drive forward
                    + hitStopVal * 0.15 * thrustReachFactor
                    - recover * 0.18;

                // Spine: counter-rotates to coil (left side forward during coil)
                double spineYaw = coilBack * 0.14 * hipDriveFactor * spineRotateFactor
                    - thrustBurst * 0.16 * thrustReachFactor * spineRotateFactor
                    + recover * 0.10;

                // Body: shifts BACK during coil (loading weight), lunges FORWARD on thrust
                double forwardOffset = -coilBack * legLen * 0.07 * hipDriveFactor
                    + thrustBurst * legLen * 0.13 * thrustReachFactor / massInertia
                    + hitStopVal * legLen * 0.11 * thrustReachFactor
                    + followThru * legLen * 0.06 * thrustReachFactor // sustain lunge into follow-through
                    - recover * legLen * 0.11;
                double crouchDip = legLen * 0.03 * crouchDepthFactor
                    * (coilBack * 0.7 + thrustBurst * 0.4) * (1.0 - recover);

                Matrix4x4 bodyTransform;
                bodyTransform.translate(upDir * (-crouchDip) + forward * forwardOffset);
                bodyTransform.rotate(upDir, hipYaw);
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
                // Spine lateral lean (roll axis): striker tilts INTO the striking side on thrust,
                // committing the torso to the strike direction rather than staying perfectly upright.
                double spineLean = thrustBurst * 0.055 * thrustReachFactor + hitStopVal * 0.040;
                computeChainBone("Spine", "Hips", spineYaw, thrustBurst * (-0.04) + coilBack * 0.03, spineLean);
                computeChainBone("Chest", "Spine", spineYaw * 0.65, thrustBurst * (-0.03), spineLean * 0.55);
                computeChainBone("Neck", "Chest", -spineYaw * 0.35);
                // Head: drives chin forward toward the target on thrust,
                // pulls back slightly during coil.
                double headForward = thrustBurst * 0.09 * thrustReachFactor
                    + hitStopVal * 0.07 * thrustReachFactor
                    - coilBack * 0.04;
                computeChainBone("Head", "Neck", -spineYaw * 0.25, headForward);
                static const char* tailBones[] = { "TailBase", "TailMid", "TailTip" };
                {
                    Vector3 prevEnd;
                    bool hasPrev = false;
                    for (int ti = 0; ti < 3; ++ti) {
                        if (boneIdx.count(tailBones[ti]) == 0)
                            continue;
                        // Tail swings back on coil (loading), whips forward on thrust then
                        // settles — the secondary tail motion is essentially free realism.
                        double delay = ti * 0.05;
                        double tailCoil = asymEnvelope(t - delay, 0.0, 0.10, 0.24) * tailReactFactor;
                        double tailThrust = explosiveEnvelope(t - delay, 0.20, 0.30, 4.0) * tailReactFactor;
                        double tailPitch = tailCoil * 0.07 * (1.0 + ti * 0.4)
                            - tailThrust * 0.08 * (1.0 + ti * 0.5);
                        Vector3 pos = bonePos(tailBones[ti]), end = boneEnd(tailBones[ti]);
                        Vector3 newPos = bodyTransform.transformPoint(pos);
                        Vector3 newEnd = bodyTransform.transformPoint(end);
                        if (hasPrev) {
                            newPos = prevEnd;
                            newEnd = newPos + (newEnd - newPos);
                        }
                        if (std::abs(tailPitch) > 1e-6) {
                            Matrix4x4 r;
                            r.rotate(right, tailPitch);
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

                // RIGHT ARM (thrusting): coils back, extends explosively forward
                {
                    Vector3 shPos = boneChainEnd.count("Chest") > 0
                        ? boneChainEnd["Chest"]
                        : bodyTransform.transformPoint(bonePos("RightShoulder"));
                    Vector3 shEnd = shPos + bodyTransform.transformVector(boneEnd("RightShoulder") - bonePos("RightShoulder"));
                    // Shoulder: retracts during coil (pulls shoulder blade back), protracts on thrust
                    {
                        double shRetract = coilBack * (-0.10) * hipDriveFactor * shoulderProtractFactor;
                        double shProtract = thrustBurst * 0.12 * thrustReachFactor;
                        Matrix4x4 r;
                        r.rotate(right, shRetract + shProtract);
                        shEnd = shPos + r.transformVector(shEnd - shPos);
                    }
                    boneWorldTransforms["RightShoulder"] = buildBoneWorldTransform(shPos, shEnd);

                    Vector3 upperStart = shEnd;
                    Vector3 upperDir = bodyTransform.transformVector(boneEnd("RightUpperArm") - bonePos("RightUpperArm"));
                    // Upper arm: pitches BACK during coil (arm behind body), drives FORWARD on thrust
                    double upperBurst = explosiveEnvelope(t - 0.02, 0.20, 0.30, 5.5 / massInertia) * thrustReachFactor;
                    double upperPitch = coilBack * 0.45 * hipDriveFactor // pull back (bend elbow behind)
                        - upperBurst * 0.65 * thrustReachFactor // drive forward
                        - hitStopVal * 0.58 * thrustReachFactor // hold extended
                        + followThru * (-0.12) + recover * 0.60; // return
                    // Slight inward roll to aim the thrust
                    double upperRoll = -coilBack * 0.08 + upperBurst * 0.06;
                    Matrix4x4 r1;
                    r1.rotate(right, upperPitch);
                    r1.rotate(forward, upperRoll);
                    Vector3 upperEnd = upperStart + r1.transformVector(upperDir);
                    boneWorldTransforms["RightUpperArm"] = buildBoneWorldTransform(upperStart, upperEnd);

                    // Forearm: heavily bent during coil (elbow bent 90+), snaps to near-extension at thrust
                    Vector3 lowerDir = bodyTransform.transformVector(boneEnd("RightLowerArm") - bonePos("RightLowerArm"));
                    double forearmBurst = explosiveEnvelope(t - 0.04, 0.20, 0.30, 6.0 / massInertia) * thrustReachFactor;
                    // Elbow bend: high during coil, almost zero at thrust peak (full extension)
                    double elbowBend = coilBack * 0.55 * hipDriveFactor // elbow bent back
                        - forearmBurst * 0.60 * thrustReachFactor // snap to straight
                        - hitStopVal * 0.50 * thrustReachFactor // hold extended
                        + recover * 0.50; // rebend on return
                    Matrix4x4 r2;
                    r2.rotate(right, upperPitch * 0.35 + elbowBend);
                    Vector3 lowerEnd = upperEnd + r2.transformVector(lowerDir);
                    boneWorldTransforms["RightLowerArm"] = buildBoneWorldTransform(upperEnd, lowerEnd);

                    // Hand: wrist snaps forward at impact.
                    // Tremble on hold: the arm is fully extended and muscle strain causes
                    // micro-jitter — most noticeable in game cameras at this angle.
                    // dampedRing adds a brief wrist overshoot that settles in ~3 frames.
                    Vector3 handDir = bodyTransform.transformVector(boneEnd("RightHand") - bonePos("RightHand"));
                    double handBurst = explosiveEnvelope(t - 0.06, 0.20, 0.30, 7.0 / massInertia) * thrustReachFactor;
                    double tRad = t * Math::Pi * 2.0 * 20.0;
                    double armTremble = tremble(tRad, 2.3, hitStopVal * 0.018 * thrustReachFactor);
                    double wristRing = dampedRing(t, 0.30, 13.0, 16.0) * 0.04 * thrustReachFactor;
                    double wristSnap = -handBurst * 0.16 * thrustReachFactor + recover * 0.14 + armTremble + wristRing;
                    Matrix4x4 r3;
                    r3.rotate(right, wristSnap);
                    Vector3 handEnd = lowerEnd + r3.transformVector(handDir);
                    boneWorldTransforms["RightHand"] = buildBoneWorldTransform(lowerEnd, handEnd);
                }

                // LEFT ARM (counter-balance): pulls backward when right arm thrusts forward
                {
                    Vector3 shPos = boneChainEnd.count("Chest") > 0
                        ? boneChainEnd["Chest"]
                        : bodyTransform.transformPoint(bonePos("LeftShoulder"));
                    Vector3 shEnd = shPos + bodyTransform.transformVector(boneEnd("LeftShoulder") - bonePos("LeftShoulder"));
                    boneWorldTransforms["LeftShoulder"] = buildBoneWorldTransform(shPos, shEnd);

                    Vector3 upperStart = shEnd;
                    Vector3 upperDir = bodyTransform.transformVector(boneEnd("LeftUpperArm") - bonePos("LeftUpperArm"));
                    double counterBurst = explosiveEnvelope(t - 0.02, 0.20, 0.30, 4.0 / massInertia);
                    // Left arm: instead of just pulling back, it rises into a guard/brace position
                    // as the right arm thrusts. This reads immediately as combat-aware rather than
                    // mechanical. The arm lifts UP and slightly inward (protecting the jaw).
                    double guardLift = thrustBurst * 0.22 + hitStopVal * 0.16 - recover * 0.20; // arm raises
                    double guardInward = thrustBurst * 0.14 + hitStopVal * 0.10 - recover * 0.12; // inward yaw
                    double counterPitch = -coilBack * 0.20 * hipDriveFactor
                        + counterBurst * 0.28 * thrustReachFactor
                        - guardLift // raise arm (negative pitch = up)
                        + followThru * 0.10 - recover * 0.28;
                    Matrix4x4 r;
                    r.rotate(right, counterPitch);
                    r.rotate(upDir, guardInward);
                    Vector3 upperEnd = upperStart + r.transformVector(upperDir);
                    boneWorldTransforms["LeftUpperArm"] = buildBoneWorldTransform(upperStart, upperEnd);

                    Vector3 lowerDir = bodyTransform.transformVector(boneEnd("LeftLowerArm") - bonePos("LeftLowerArm"));
                    double lBurst = explosiveEnvelope(t - 0.04, 0.20, 0.30, 3.5 / massInertia);
                    // Forearm bends into the guard (elbow flexed to protect)
                    double lElbow = -coilBack * 0.12 + lBurst * 0.15 + guardLift * 0.40 - recover * 0.15;
                    Matrix4x4 r2;
                    r2.rotate(right, counterPitch * 0.45 + lElbow);
                    Vector3 lowerEnd = upperEnd + r2.transformVector(lowerDir);
                    boneWorldTransforms["LeftLowerArm"] = buildBoneWorldTransform(upperEnd, lowerEnd);

                    Vector3 handDir = bodyTransform.transformVector(boneEnd("LeftHand") - bonePos("LeftHand"));
                    // Left hand also slightly curls (guard fist)
                    double lWrist = guardLift * 0.15 - recover * 0.10;
                    Matrix4x4 r3;
                    r3.rotate(right, lWrist);
                    Vector3 handEnd = lowerEnd + r3.transformVector(handDir);
                    boneWorldTransforms["LeftHand"] = buildBoneWorldTransform(lowerEnd, handEnd);
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
