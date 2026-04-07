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

// Procedural walk animation for biped rig.
//
// Designed to work for all bipedal creatures: humans, biped robots,
// anthropomorphic animals, dinosaurs, penguins, and any game character
// that walks on two legs with a biped bone structure.
//
// Adjustable animation parameters:
//   - stepLengthFactor:    stride extent relative to leg length
//   - stepHeightFactor:    foot lift height during swing phase
//   - bodyBobFactor:       vertical oscillation amplitude of hips
//   - gaitSpeedFactor:     number of gait cycles per clip
//   - armSwingFactor:      arm counter-swing amplitude (0 = arms still)
//   - hipSwayFactor:       lateral hip shift toward stance leg
//   - hipRotateFactor:     hip rotation around vertical axis per step
//   - spineFlexFactor:     counter-rotation of spine against hips
//   - headBobFactor:       head stabilization / counter-bob
//   - kneeBendFactor:      additional knee flex at mid-stance (weight)
//   - footRollFactor:      heel-strike to toe-off roll intensity
//   - leanForwardFactor:   forward torso lean (heavy/fast creatures)
//   - bouncinessFactory:   extra vertical bounce (cartoon style)
//   - forearmPhaseOffset:  phase offset of lower arm swing relative to upper arm (0-1)
//   - tailSwayFactor:      tail side-to-side sway amplitude
//   - useFabrikIk:         use FABRIK IK solver (vs CCD)
//   - planeStabilization:  constrain legs to their rest-pose sagittal plane

#include <array>
#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/biped/walk.h>
#include <dust3d/animation/common.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace biped {

    struct LegDef {
        const char* upperLegName;
        const char* lowerLegName;
        const char* footName;
    };

    struct ArmDef {
        const char* shoulderName;
        const char* upperArmName;
        const char* lowerArmName;
        const char* handName;
    };

    struct LegRest {
        Vector3 upperLegPos, upperLegEnd;
        Vector3 lowerLegEnd;
        Vector3 footEnd;
        Vector3 restStickDir;
        Vector3 restUpperToLowerVec;
        double legLength; // total chain length
    };

    struct ArmRest {
        Vector3 shoulderPos, shoulderEnd;
        Vector3 upperArmEnd;
        Vector3 lowerArmEnd;
        Vector3 handEnd;
    };

    bool walk(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = static_cast<int>(parameters.getValue("frameCount", 30.0));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 1.0));

        // ----- bone lookup -----
        auto boneIdx = buildBoneIndexMap(rigStructure);

        auto bonePos = [&](const std::string& name) -> Vector3 {
            return getBonePos(rigStructure, boneIdx, name);
        };
        auto boneEnd = [&](const std::string& name) -> Vector3 {
            return getBoneEnd(rigStructure, boneIdx, name);
        };

        // ----- verify required bones -----
        static const char* requiredBones[] = {
            "Root", "Hips", "Spine", "Chest", "Neck", "Head",
            "LeftUpperLeg", "LeftLowerLeg", "LeftFoot",
            "RightUpperLeg", "RightLowerLeg", "RightFoot",
            "LeftShoulder", "LeftUpperArm", "LeftLowerArm", "LeftHand",
            "RightShoulder", "RightUpperArm", "RightLowerArm", "RightHand"
        };
        if (!validateRequiredBones(boneIdx, requiredBones, sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;

        // ===================================================================
        // 1. Body orientation: up is +Y, forward is +Z from Hips→Spine
        // ===================================================================
        Vector3 hipsPos = bonePos("Hips");
        Vector3 spineEnd = boneEnd("Spine");
        Vector3 upDir(0.0, 1.0, 0.0);

        // Forward direction: perpendicular to up in the spine direction
        // For biped rigs, forward is typically +Z. We derive from chest direction projected to ground plane.
        Vector3 chestPos = bonePos("Chest");
        Vector3 spineDir = (chestPos - hipsPos);
        // The spine goes upward; forward is the cross product trick
        // Use the foot positions to determine forward: average foot end Z
        Vector3 leftFootEnd = boneEnd("LeftFoot");
        Vector3 rightFootEnd = boneEnd("RightFoot");
        Vector3 avgFootEnd = (leftFootEnd + rightFootEnd) * 0.5;
        Vector3 hipsToFoot = avgFootEnd - hipsPos;
        // Forward is the horizontal component of the foot direction from hips
        Vector3 forward(hipsToFoot.x(), 0.0, hipsToFoot.z());
        if (forward.lengthSquared() < 1e-8)
            forward = Vector3(0.0, 0.0, 1.0);
        forward.normalize();

        Vector3 right = Vector3::crossProduct(upDir, forward);
        if (right.lengthSquared() < 1e-8)
            right = Vector3(1.0, 0.0, 0.0);
        right.normalize();

        // ===================================================================
        // 2. Define legs and arms
        // ===================================================================
        static const LegDef leftLeg = { "LeftUpperLeg", "LeftLowerLeg", "LeftFoot" };
        static const LegDef rightLeg = { "RightUpperLeg", "RightLowerLeg", "RightFoot" };
        static const ArmDef leftArm = { "LeftShoulder", "LeftUpperArm", "LeftLowerArm", "LeftHand" };
        static const ArmDef rightArm = { "RightShoulder", "RightUpperArm", "RightLowerArm", "RightHand" };

        // Phase offsets: left leg at 0.0, right leg at 0.5
        const double leftLegPhase = 0.0;
        const double rightLegPhase = 0.5;

        // Gather leg rest data
        auto gatherLegRest = [&](const LegDef& leg) -> LegRest {
            LegRest r;
            r.upperLegPos = bonePos(leg.upperLegName);
            r.upperLegEnd = boneEnd(leg.upperLegName);
            r.lowerLegEnd = boneEnd(leg.lowerLegName);
            r.footEnd = boneEnd(leg.footName);
            Vector3 chordVec = r.footEnd - r.upperLegEnd;
            r.restStickDir = chordVec.isZero() ? Vector3(0, -1, 0) : chordVec.normalized();
            r.restUpperToLowerVec = r.lowerLegEnd - r.upperLegEnd;
            r.legLength = (r.upperLegEnd - r.upperLegPos).length()
                + (r.lowerLegEnd - r.upperLegEnd).length()
                + (r.footEnd - r.lowerLegEnd).length();
            return r;
        };

        LegRest leftLegRest = gatherLegRest(leftLeg);
        LegRest rightLegRest = gatherLegRest(rightLeg);

        auto gatherArmRest = [&](const ArmDef& arm) -> ArmRest {
            ArmRest r;
            r.shoulderPos = bonePos(arm.shoulderName);
            r.shoulderEnd = boneEnd(arm.shoulderName);
            r.upperArmEnd = boneEnd(arm.upperArmName);
            r.lowerArmEnd = boneEnd(arm.lowerArmName);
            r.handEnd = boneEnd(arm.handName);
            return r;
        };

        ArmRest leftArmRest = gatherArmRest(leftArm);
        ArmRest rightArmRest = gatherArmRest(rightArm);

        // ===================================================================
        // 3. Walking parameters
        // ===================================================================
        double avgLegLength = (leftLegRest.legLength + rightLegRest.legLength) * 0.5;
        if (avgLegLength < 1e-6)
            avgLegLength = 1.0;

        double stepLengthFactor = parameters.getValue("stepLengthFactor", 1.0);
        double stepHeightFactor = parameters.getValue("stepHeightFactor", 1.0);
        double bodyBobFactor = parameters.getValue("bodyBobFactor", 1.0);
        double gaitSpeedFactor = parameters.getValue("gaitSpeedFactor", 1.0);
        double armSwingFactor = parameters.getValue("armSwingFactor", 1.0);
        double hipSwayFactor = parameters.getValue("hipSwayFactor", 1.0);
        double hipRotateFactor = parameters.getValue("hipRotateFactor", 1.0);
        double spineFlexFactor = parameters.getValue("spineFlexFactor", 1.0);
        double headBobFactor = parameters.getValue("headBobFactor", 1.0);
        double kneeBendFactor = parameters.getValue("kneeBendFactor", 1.0);
        double leanForwardFactor = parameters.getValue("leanForwardFactor", 1.0);
        double bouncinessFactor = parameters.getValue("bouncinessFactor", 1.0);
        double forearmPhaseOffset = parameters.getValue("forearmPhaseOffset", 0.5);

        double stepLength = avgLegLength * 0.35 * stepLengthFactor;
        double stepHeight = avgLegLength * 0.08 * stepHeightFactor;
        double bodyBobAmp = avgLegLength * 0.02 * bodyBobFactor * bouncinessFactor;
        double hipSwayAmp = avgLegLength * 0.03 * hipSwayFactor;
        double hipRotateAmp = 0.08 * hipRotateFactor; // radians
        double spineCounterAmp = 0.05 * spineFlexFactor;
        double armSwingAngle = 0.4 * armSwingFactor; // radians, peak swing
        double headCounterAmp = 0.02 * headBobFactor;
        double kneeBendExtra = avgLegLength * 0.015 * kneeBendFactor;
        double leanAngle = 0.03 * leanForwardFactor; // radians forward lean
        double tailSwayFactor = parameters.getValue("tailSwayFactor", 1.0);
        double tailSwayAmp = 0.08 * tailSwayFactor;

        // Ground level: lowest foot tip
        double leftFootUp = Vector3::dotProduct(leftLegRest.footEnd, upDir);
        double rightFootUp = Vector3::dotProduct(rightLegRest.footEnd, upDir);
        double groundLevel = std::min(leftFootUp, rightFootUp);

        // Foot home positions at ground
        auto footHome = [&](const LegRest& lr) -> Vector3 {
            double proj = Vector3::dotProduct(lr.footEnd, upDir);
            return lr.footEnd - upDir * (proj - groundLevel);
        };

        Vector3 leftFootHome = footHome(leftLegRest);
        Vector3 rightFootHome = footHome(rightLegRest);

        bool useFabrikIk = parameters.getBool("useFabrikIk", true);
        bool planeStabilization = parameters.getBool("planeStabilization", true);

        // ===================================================================
        // 4. Generate frames
        // ===================================================================
        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        const double cycles = std::max(1.0, std::round(gaitSpeedFactor));
        const double swingDuty = 0.30; // biped swing phase is ~30% of cycle

        for (int frame = 0; frame < frameCount; ++frame) {
            double tNormalized = static_cast<double>(frame) / static_cast<double>(frameCount);
            double t = fmod(tNormalized * cycles, 1.0);

            std::map<std::string, Matrix4x4> boneWorldTransforms;

            // -------------------------------------------------------
            // 4a. Hip / body motion
            // -------------------------------------------------------
            // Vertical bob: two bobs per cycle (one per step)
            double bodyBob = bodyBobAmp * std::sin(t * 4.0 * Math::Pi);

            // Lateral sway: shifts toward stance leg, one cycle per step
            double lateralSway = hipSwayAmp * std::sin(t * 2.0 * Math::Pi);

            // Hip rotation around vertical: rotates forward with swing leg
            double hipYaw = hipRotateAmp * std::sin(t * 2.0 * Math::Pi);

            // Forward lean
            double forwardPitch = leanAngle;

            // Extra knee-bend dip at mid-stance
            double stanceDip = kneeBendExtra * std::abs(std::sin(t * 2.0 * Math::Pi));

            Matrix4x4 bodyTransform;
            bodyTransform.translate(upDir * (bodyBob - stanceDip) + right * lateralSway);
            bodyTransform.rotate(upDir, hipYaw);
            bodyTransform.rotate(right, forwardPitch);

            // -------------------------------------------------------
            // 4b. Spine and upper body
            // -------------------------------------------------------
            // Spine counter-rotates against hip yaw
            double spineYaw = -spineCounterAmp * std::sin(t * 2.0 * Math::Pi);

            // Head counter-bob to stabilize gaze
            double headPitch = -headCounterAmp * std::sin(t * 4.0 * Math::Pi);

            auto computeBodyBone = [&](const std::string& name,
                                       double extraYaw = 0.0,
                                       double extraPitch = 0.0) {
                Vector3 pos = bonePos(name);
                Vector3 end = boneEnd(name);
                Vector3 newPos = bodyTransform.transformPoint(pos);
                Vector3 newEnd = bodyTransform.transformPoint(end);
                if (std::abs(extraYaw) > 1e-6 || std::abs(extraPitch) > 1e-6) {
                    Matrix4x4 extraRot;
                    if (std::abs(extraYaw) > 1e-6)
                        extraRot.rotate(upDir, extraYaw);
                    if (std::abs(extraPitch) > 1e-6)
                        extraRot.rotate(right, extraPitch);
                    Vector3 offset = newEnd - newPos;
                    newEnd = newPos + extraRot.transformVector(offset);
                }
                boneWorldTransforms[name] = buildBoneWorldTransform(newPos, newEnd);
            };

            computeBodyBone("Root");
            computeBodyBone("Hips", hipYaw * 0.3);
            computeBodyBone("Spine", spineYaw);
            computeBodyBone("Chest", spineYaw * 0.5);
            computeBodyBone("Neck", -spineYaw * 0.3, headPitch);
            computeBodyBone("Head", 0.0, headPitch);

            // Tail sway / wave
            static const char* tailBones[] = { "TailBase", "TailMid", "TailTip" };
            double tailPhase = t * 2.0 * Math::Pi;
            for (int ti = 0; ti < 3; ++ti) {
                if (boneIdx.count(tailBones[ti]) == 0)
                    continue;
                double attenuation = 1.0 - ti * 0.28;
                double tailAngle = tailSwayAmp * attenuation * std::sin(tailPhase - ti * 0.75 + 0.2);
                computeBodyBone(tailBones[ti], tailAngle);
            }

            // -------------------------------------------------------
            // 4c. Leg IK: compute foot targets then solve
            // -------------------------------------------------------
            auto computeFootTarget = [&](const Vector3& home, double phase) -> Vector3 {
                double legT = fmod(t + phase, 1.0);

                Vector3 footFront = home + forward * stepLength;
                Vector3 footBack = home - forward * stepLength;

                if (legT < swingDuty) {
                    // Swing phase
                    double legPhase = legT / swingDuty;
                    double smoothSwing = smootherstep(legPhase);
                    Vector3 groundPos = footBack + (footFront - footBack) * smoothSwing;
                    double lift = stepHeight * std::sin(smoothSwing * Math::Pi);
                    return groundPos + upDir * lift;
                } else {
                    // Stance phase
                    double legPhase = (legT - swingDuty) / (1.0 - swingDuty);
                    return footFront + (footBack - footFront) * smoothstep(legPhase);
                }
            };

            Vector3 leftFootTarget = computeFootTarget(leftFootHome, leftLegPhase);
            Vector3 rightFootTarget = computeFootTarget(rightFootHome, rightLegPhase);

            auto solveLeg = [&](const LegDef& leg, const LegRest& rest, const Vector3& target) {
                Vector3 hipPos = bodyTransform.transformPoint(rest.upperLegPos);
                Vector3 upperLegEnd = bodyTransform.transformPoint(rest.upperLegEnd);
                Vector3 footEndPt = bodyTransform.transformPoint(rest.footEnd);

                std::vector<Vector3> chain = { hipPos, upperLegEnd, footEndPt };

                Vector3 preferPlane = Vector3::crossProduct(chain[1] - chain[0], chain[2] - chain[1]);
                if (preferPlane.lengthSquared() < 1e-10)
                    preferPlane = Vector3::crossProduct(chain[1] - chain[0], upDir);

                if (useFabrikIk) {
                    Vector3 plane = planeStabilization ? preferPlane : Vector3();
                    solveFabrikIk(chain, target, 15, plane);
                } else {
                    solveCcdIk(chain, target, 15);
                }

                // Stabilize knee: keep it in the sagittal plane
                Vector3 kneeOffset = chain[1] - hipPos;
                double lateralDrift = Vector3::dotProduct(kneeOffset, right)
                    - Vector3::dotProduct(upperLegEnd - hipPos, right);
                chain[1] = chain[1] - right * lateralDrift;

                // Re-enforce segment lengths
                double len0 = (rest.upperLegEnd - rest.upperLegPos).length();
                double len1 = (rest.footEnd - rest.upperLegEnd).length();
                Vector3 dir0 = chain[1] - chain[0];
                if (!dir0.isZero())
                    chain[1] = chain[0] + dir0.normalized() * len0;
                Vector3 dir1 = chain[2] - chain[1];
                if (!dir1.isZero())
                    chain[2] = chain[1] + dir1.normalized() * len1;

                // Reconstruct lower leg position
                Vector3 newStickDir = (chain[2] - chain[1]);
                if (newStickDir.isZero())
                    newStickDir = rest.restStickDir;
                else
                    newStickDir.normalize();
                Quaternion stickRot = Quaternion::rotationTo(rest.restStickDir, newStickDir);
                Matrix4x4 stickRotMat;
                stickRotMat.rotate(stickRot);
                Vector3 lowerLegEnd = chain[1] + stickRotMat.transformVector(rest.restUpperToLowerVec);

                boneWorldTransforms[leg.upperLegName] = buildBoneWorldTransform(chain[0], chain[1]);
                boneWorldTransforms[leg.lowerLegName] = buildBoneWorldTransform(chain[1], lowerLegEnd);
                boneWorldTransforms[leg.footName] = buildBoneWorldTransform(lowerLegEnd, chain[2]);
            };

            solveLeg(leftLeg, leftLegRest, leftFootTarget);
            solveLeg(rightLeg, rightLegRest, rightFootTarget);

            // -------------------------------------------------------
            // 4d. Arm swing: counter to opposite leg
            // -------------------------------------------------------
            auto computeArmSwing = [&](const ArmDef& arm, const ArmRest& rest, double phase) {
                // Arms swing opposite to legs: left arm swings with right leg
                double armT = fmod(t + phase, 1.0);
                double swingAngle = armSwingAngle * std::sin(armT * 2.0 * Math::Pi);

                // Lower arm swings with a phase offset from the upper arm
                double forearmT = fmod(t + phase + forearmPhaseOffset, 1.0);
                double forearmSwingAngle = armSwingAngle * std::sin(forearmT * 2.0 * Math::Pi);

                // Shoulder follows body
                Vector3 shoulderPos = bodyTransform.transformPoint(rest.shoulderPos);
                Vector3 shoulderEnd = bodyTransform.transformPoint(rest.shoulderEnd);
                boneWorldTransforms[arm.shoulderName] = buildBoneWorldTransform(shoulderPos, shoulderEnd);

                // Upper arm: swing forward/backward around the right axis
                Vector3 upperArmStart = shoulderEnd;
                Vector3 upperArmEndRest = bodyTransform.transformPoint(rest.upperArmEnd);
                Vector3 armDir = upperArmEndRest - upperArmStart;

                Matrix4x4 swingMat;
                swingMat.rotate(right, swingAngle);
                Vector3 newUpperArmEnd = upperArmStart + swingMat.transformVector(armDir);
                boneWorldTransforms[arm.upperArmName] = buildBoneWorldTransform(upperArmStart, newUpperArmEnd);

                // Lower arm: swing with phase offset plus additional bend
                Vector3 lowerArmDir = bodyTransform.transformPoint(rest.lowerArmEnd) - bodyTransform.transformPoint(rest.upperArmEnd);
                double elbowBend = -std::abs(forearmSwingAngle) * 0.3; // bend elbow more during swing
                Matrix4x4 elbowMat;
                elbowMat.rotate(right, forearmSwingAngle + elbowBend);
                Vector3 newLowerArmEnd = newUpperArmEnd + elbowMat.transformVector(lowerArmDir);
                boneWorldTransforms[arm.lowerArmName] = buildBoneWorldTransform(newUpperArmEnd, newLowerArmEnd);

                // Hand follows lower arm
                Vector3 handDir = bodyTransform.transformPoint(rest.handEnd) - bodyTransform.transformPoint(rest.lowerArmEnd);
                Vector3 newHandEnd = newLowerArmEnd + elbowMat.transformVector(handDir);
                boneWorldTransforms[arm.handName] = buildBoneWorldTransform(newLowerArmEnd, newHandEnd);
            };

            // Left arm counter-swings with right leg (phase 0.5), right arm with left leg (phase 0.0)
            computeArmSwing(leftArm, leftArmRest, rightLegPhase);
            computeArmSwing(rightArm, rightArmRest, leftLegPhase);

            // -------------------------------------------------------
            // 4e. Skin matrices
            // -------------------------------------------------------
            auto& animFrame = animationClip.frames[frame];
            animFrame.time = static_cast<float>(tNormalized) * durationSeconds;
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
