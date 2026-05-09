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
// Procedural spellcast for biped rig.
// Both arms draw IN to gather energy, then snap OUT explosively.
// Body recoils backward on release. Micro-tremor sustain. Recovery.

#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/biped/cast.h>
#include <dust3d/animation/common.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace biped {

    bool cast(const RigStructure& rigStructure,
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

        int frameCount = static_cast<int>(parameters.getValue("frameCount", 48));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 1.0));
        double castForceFactor = parameters.getValue("castForceFactor", 1.0);
        double gatherDepthFactor = parameters.getValue("gatherDepthFactor", 1.0);
        double bodyMassFactor = parameters.getValue("bodyMassFactor", 1.0);
        double recoverySpeedFactor = parameters.getValue("recoverySpeedFactor", 1.0);
        double tailReactFactor = parameters.getValue("tailReactFactor", 1.0);
        double spineRecoilFactor = parameters.getValue("spineRecoilFactor", 1.0);
        double chestExpandFactor = parameters.getValue("chestExpandFactor", 1.0);
        double wristSnapFactor = parameters.getValue("wristSnapFactor", 1.0);
        double trembleIntensityFactor = parameters.getValue("trembleIntensityFactor", 1.0);
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

        for (int frame = 0; frame < frameCount; ++frame) {
            double t = static_cast<double>(frame) / static_cast<double>(frameCount);
            double tRad = t * 2.0 * Math::Pi;
            std::map<std::string, Matrix4x4> boneWorldTransforms;

            // Gather: arms pull inward, spine bends back [0.0 - 0.22]
            double gatherPhase = asymEnvelope(t, 0.0, 0.10, 0.24) * gatherDepthFactor;
            // Release: explosive push-out [0.22 - 0.32]
            double releaseBurst = explosiveEnvelope(t, 0.20, 0.30, 5.0 / massInertia) * castForceFactor;
            // Sustain: arms extended with tremor [0.32 - 0.58]
            double sustainPhase = asymEnvelope(t, 0.30, 0.38, 0.60);
            // Recovery [0.60 - 1.0]
            double recover = asymEnvelope(t, 0.60, 0.78, 1.0);

            // Spine: arches BACK during gather (loading posture), recoils BACKWARD on release
            double spinePitch = gatherPhase * (-0.20) * gatherDepthFactor
                - releaseBurst * 0.15 * castForceFactor * spineRecoilFactor // recoil on release
                + sustainPhase * (-0.06) // slight forward lean during sustain
                + recover * 0.22; // return

            // Body: leans slightly forward during gather (toward target), snaps BACK on release
            double forwardOffset = gatherPhase * legLen * 0.04 * gatherDepthFactor
                - releaseBurst * legLen * 0.08 * castForceFactor / massInertia;

            Matrix4x4 bodyTransform;
            bodyTransform.translate(forward * forwardOffset);
            // Small head-tilt during release (recoil jerk)
            double bodyPitch = -releaseBurst * 0.05 * castForceFactor;
            bodyTransform.rotate(right, bodyPitch);
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
            computeChainBone("Chest", "Spine", 0.0, spinePitch * 0.35 * chestExpandFactor);
            computeChainBone("Neck", "Chest", 0.0, -spinePitch * 0.20 - releaseBurst * 0.10 * castForceFactor);
            computeChainBone("Head", "Neck", 0.0, -spinePitch * 0.25 - releaseBurst * 0.12 * castForceFactor);
            static const char* tailBones[] = { "TailBase", "TailMid", "TailTip" };
            {
                Vector3 prevEnd;
                bool hasPrev = false;
                for (int ti = 0; ti < 3; ++ti) {
                    if (boneIdx.count(tailBones[ti]) == 0)
                        continue;
                    Vector3 pos = bonePos(tailBones[ti]), end = boneEnd(tailBones[ti]);
                    Vector3 newPos = bodyTransform.transformPoint(pos);
                    Vector3 newEnd = bodyTransform.transformPoint(end);
                    if (hasPrev) {
                        newPos = prevEnd;
                        newEnd = newPos + (newEnd - newPos);
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

            // BILATERAL ARMS: gather toward chest, then explode outward/forward
            for (int side = 0; side < 2; ++side) {
                bool isLeft = (side == 0);
                double delay = isLeft ? 0.015 : 0.0; // very slight asymmetry
                double sideSign = isLeft ? -1.0 : 1.0;
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
                // Shoulders: retract during gather, protract on release
                {
                    double shGather = gatherPhase * 0.06 * gatherDepthFactor; // retract
                    double shRelease = -releaseBurst * 0.10 * castForceFactor; // protract
                    Matrix4x4 r;
                    r.rotate(right, shGather + shRelease);
                    shEnd = shPos + r.transformVector(shEnd - shPos);
                }
                boneWorldTransforms[shoulder] = buildBoneWorldTransform(shPos, shEnd);

                Vector3 upperStart = shEnd;
                Vector3 upperDir = bodyTransform.transformVector(boneEnd(upper) - bonePos(upper));

                // Upper arms: draw inward/forward during gather, push outward on release
                // Gather: arms fold IN toward chest (pitch forward, yaw toward body)
                double gatherDelayed = asymEnvelope(t - delay, 0.0, 0.10, 0.24) * gatherDepthFactor;
                double releaseDelayed = explosiveEnvelope(t - delay, 0.20, 0.30, 5.0 / massInertia) * castForceFactor;
                double sustainDelayed = asymEnvelope(t - delay, 0.30, 0.38, 0.60);

                double upperPitch = -gatherDelayed * 0.45 * gatherDepthFactor // arms fold inward (forward)
                    - releaseDelayed * 0.20 * castForceFactor // push forward on release
                    - sustainDelayed * 0.22 * castForceFactor // hold extended
                    + recover * 0.60; // return to sides
                double upperYaw = gatherDelayed * sideSign * 0.40 * gatherDepthFactor // draw toward center
                    - releaseDelayed * sideSign * 0.30 * castForceFactor // spread on release
                    - sustainDelayed * sideSign * 0.28 * castForceFactor // hold spread
                    + recover * sideSign * 0.28; // return
                Matrix4x4 r1;
                r1.rotate(right, upperPitch);
                r1.rotate(upDir, upperYaw);
                Vector3 upperEnd = upperStart + r1.transformVector(upperDir);
                boneWorldTransforms[upper] = buildBoneWorldTransform(upperStart, upperEnd);

                // Forearms: elbow bends deeply during gather (arms close to chest),
                // snaps to near-extension on release
                Vector3 lowerDir = bodyTransform.transformVector(boneEnd(lower) - bonePos(lower));
                double forearmGather = asymEnvelope(t - delay - 0.02, 0.0, 0.10, 0.24) * gatherDepthFactor;
                double forearmRelease = explosiveEnvelope(t - delay - 0.02, 0.20, 0.30, 5.0 / massInertia) * castForceFactor;
                double forearmSustain = asymEnvelope(t - delay - 0.02, 0.30, 0.38, 0.60);
                double elbowBend = forearmGather * 0.65 * gatherDepthFactor // deep bend during gather
                    - forearmRelease * 0.60 * castForceFactor // snap to extension
                    - forearmSustain * 0.55 * castForceFactor // hold extended
                    + recover * 0.58; // return
                // Micro-tremor during sustain — strain of holding the spell
                double sustainTremor = forearmSustain * tremble(tRad, (double)side * 1.3, 0.04);
                Matrix4x4 r2;
                r2.rotate(right, elbowBend + sustainTremor);
                Vector3 lowerEnd = upperEnd + r2.transformVector(lowerDir);
                boneWorldTransforms[lower] = buildBoneWorldTransform(upperEnd, lowerEnd);

                // Hands: wrist flick on release, tremor during sustain
                Vector3 handDir = bodyTransform.transformVector(boneEnd(hand) - bonePos(hand));
                double handRelease = explosiveEnvelope(t - delay - 0.04, 0.20, 0.30, 6.0 / massInertia) * castForceFactor;
                double wristFlick = -handRelease * 0.20 * castForceFactor;
                double handTremor = forearmSustain * tremble(tRad, (double)side * 2.7 + 1.1, 0.05);
                Matrix4x4 r3;
                r3.rotate(right, wristFlick + handTremor);
                Vector3 handEnd = lowerEnd + r3.transformVector(handDir);
                boneWorldTransforms[hand] = buildBoneWorldTransform(lowerEnd, handEnd);
            }
            auto& animFrame = animationClip.frames[frame];
            animFrame.time = static_cast<float>(t) * durationSeconds;
            animFrame.boneWorldTransforms = boneWorldTransforms;
            for (const auto& pair : boneWorldTransforms) {
                auto invIt = inverseBindMatrices.find(pair.first);
                if (invIt != inverseBindMatrices.end()) {
                    Matrix4x4 skinMat = pair.second;
                    skinMat *= invIt->second;
                    animFrame.boneSkinMatrices[pair.first] = skinMat;
                }
            }
        }
        return true;
    }

} // namespace biped

} // namespace dust3d
