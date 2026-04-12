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

// Procedural head-butt/gore attack animation for quadruped rig.
//
// Full attack cycle with four distinct phases modeled after real animal
// combat behavior (bulls, rams, rhinos, large cats head-strike):
//
//   Phase 1 — Anticipation  (0.00 – 0.25):  Weight shifts back onto hind legs,
//       head lowers, spine compresses like a spring.  Front legs plant and
//       brace.  This "wind-up" is critical for telegraphing the attack in
//       games (12 principles of animation: anticipation).
//
//   Phase 2 — Charge/Lunge  (0.25 – 0.50):  Explosive forward thrust from the
//       hind legs.  The whole body surges forward, spine extends, head stays
//       low and aimed at the target.  Back legs push off the ground with
//       secondary overlap on tail.
//
//   Phase 3 — Strike/Impact (0.50 – 0.65):  Head snaps upward in a gore/headbutt
//       motion.  Jaw opens on impact.  Front legs absorb the shock with a
//       compression bend.  Spine recoils.  This is the damage frame.
//
//   Phase 4 — Recovery      (0.65 – 1.00):  Head returns to neutral, body
//       settles back to rest pose, tail dampens.  Smooth ease-out back to
//       idle stance so the clip can loop or transition cleanly.
//
// Animation principles applied:
//   - Anticipation & follow-through (Disney 12 principles)
//   - Overlapping action on tail, jaw, and spine segments
//   - Ease-in / ease-out via smootherstep curves
//   - Secondary motion on tail whip during strike
//   - Squash & stretch approximated via spine compression/extension
//
// Tunable parameters for different quadruped creatures:
//   - chargeDistanceFactor:   how far the lunge covers (bull=large, cat=short)
//   - chargeSpeedFactor:      acceleration intensity of the lunge
//   - headDropFactor:         how low the head drops during anticipation
//   - headStrikeIntensity:    upward gore/snap force at impact
//   - jawOpenFactor:          jaw gape on impact (0=closed, 1=wide)
//   - spineCompressionFactor: how much the spine coils during wind-up
//   - tailWhipFactor:         tail lash amplitude during strike
//   - frontLegBraceFactor:    front leg stiffness during charge
//   - backLegPushFactor:      hind leg extension force during lunge
//   - anticipationDuration:   fraction of clip spent in wind-up  (0-1)
//   - strikeDuration:         fraction of clip for the impact phase (0-1)
//   - recoverySpeed:          how quickly the animal returns to rest
//   - bodyMassFactor:         overall inertia (heavy=slow settle, light=snappy)

#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/common.h>
#include <dust3d/animation/quadruped/attack.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace quadruped {

    bool attack(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = static_cast<int>(parameters.getValue("frameCount", 40));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 1.2));

        auto boneIdx = buildBoneIndexMap(rigStructure);

        auto bonePos = [&](const std::string& name) -> Vector3 {
            return getBonePos(rigStructure, boneIdx, name);
        };
        auto boneEnd = [&](const std::string& name) -> Vector3 {
            return getBoneEnd(rigStructure, boneIdx, name);
        };

        static const char* requiredBones[] = {
            "Root", "Pelvis", "Spine", "Chest", "Neck", "Head",
            "FrontLeftUpperLeg", "FrontLeftLowerLeg", "FrontLeftFoot",
            "FrontRightUpperLeg", "FrontRightLowerLeg", "FrontRightFoot",
            "BackLeftUpperLeg", "BackLeftLowerLeg", "BackLeftFoot",
            "BackRightUpperLeg", "BackRightLowerLeg", "BackRightFoot"
        };
        if (!validateRequiredBones(boneIdx, requiredBones, sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;

        // User-tunable attack parameters
        double chargeDistanceFactor = parameters.getValue("chargeDistanceFactor", 1.0);
        double chargeSpeedFactor = parameters.getValue("chargeSpeedFactor", 1.0);
        double headDropFactor = parameters.getValue("headDropFactor", 1.0);
        double headStrikeIntensity = parameters.getValue("headStrikeIntensity", 1.0);
        double jawOpenFactor = parameters.getValue("jawOpenFactor", 1.0);
        double spineCompressionFactor = parameters.getValue("spineCompressionFactor", 1.0);
        double tailWhipFactor = parameters.getValue("tailWhipFactor", 1.0);
        double frontLegBraceFactor = parameters.getValue("frontLegBraceFactor", 1.0);
        double backLegPushFactor = parameters.getValue("backLegPushFactor", 1.0);
        double anticipationDuration = parameters.getValue("anticipationDuration", 0.25);
        double strikeMoment = parameters.getValue("strikeMoment", 0.50);
        double strikeEnd = parameters.getValue("strikeEnd", 0.65);
        double recoverySpeed = parameters.getValue("recoverySpeed", 1.0);
        double bodyMassFactor = parameters.getValue("bodyMassFactor", 1.0);

        // Derive coordinate frame from rest pose
        Vector3 pelvisPos = bonePos("Pelvis");
        Vector3 chestPos = bonePos("Chest");
        Vector3 headPos = bonePos("Head");
        Vector3 headEnd = boneEnd("Head");

        Vector3 spineVec = chestPos - pelvisPos;
        double spineLength = spineVec.length();
        if (spineLength < 1e-6)
            spineLength = 1.0;

        Vector3 upDir(0.0, 1.0, 0.0);
        Vector3 forward(spineVec.x(), 0.0, spineVec.z());
        if (forward.lengthSquared() < 1e-8)
            forward = Vector3(0.0, 0.0, 1.0);
        forward.normalize();
        Vector3 right = Vector3::crossProduct(upDir, forward);
        if (right.lengthSquared() < 1e-8)
            right = Vector3(1.0, 0.0, 0.0);
        right.normalize();

        // Scale-relative amplitudes
        double chargeDistance = spineLength * 0.8 * chargeDistanceFactor;
        double headDrop = spineLength * 0.35 * headDropFactor;
        double headStrikeUp = spineLength * 0.5 * headStrikeIntensity;
        double spineCompressAmt = spineLength * 0.08 * spineCompressionFactor;
        double tailWhipAmp = 0.5 * tailWhipFactor;
        double legBrace = spineLength * 0.06 * frontLegBraceFactor;
        double legPush = spineLength * 0.12 * backLegPushFactor;

        // Inertia damping: heavier creatures recover slower
        double recoverDamping = 1.0 / (0.5 + 0.5 * bodyMassFactor);

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            double tNormalized = static_cast<double>(frame) / static_cast<double>(frameCount);

            // === Phase envelope computation ===
            // anticipation:  ramps up from 0 to anticipationDuration
            // charge:        anticipationDuration to strikeMoment
            // strike:        strikeMoment to strikeEnd
            // recovery:      strikeEnd to 1.0

            double anticipation = 0.0; // 0→1 during wind-up
            double charge = 0.0; // 0→1 during lunge
            double strike = 0.0; // 0→1→0 bell curve at impact
            double recovery = 0.0; // 0→1 during settle

            if (tNormalized < anticipationDuration) {
                anticipation = smootherstep(tNormalized / anticipationDuration);
            } else if (tNormalized < strikeMoment) {
                anticipation = 1.0; // hold the coiled pose briefly
                double chargeT = (tNormalized - anticipationDuration) / (strikeMoment - anticipationDuration);
                // Ease-in for explosive start (fast out of anticipation)
                charge = smootherstep(chargeT) * chargeSpeedFactor;
                if (charge > 1.0)
                    charge = 1.0;
            } else if (tNormalized < strikeEnd) {
                double strikeT = (tNormalized - strikeMoment) / (strikeEnd - strikeMoment);
                // Bell curve: sin(pi*t) peaks at 0.5
                strike = std::sin(strikeT * Math::Pi);
                // Anticipation releases during strike
                anticipation = 1.0 - smoothstep(strikeT);
                charge = 1.0;
            } else {
                double recoveryT = (tNormalized - strikeEnd) / (1.0 - strikeEnd);
                recovery = smootherstep(recoveryT * recoverDamping * recoverySpeed);
                if (recovery > 1.0)
                    recovery = 1.0;
                charge = 1.0 - recovery;
                anticipation = 0.0;
            }

            // === Body translation ===
            // Anticipation: pull back slightly
            // Charge: surge forward
            // Recovery: return to origin
            double bodyForward = -spineCompressAmt * anticipation
                + chargeDistance * charge * (1.0 - recovery);
            double bodyVertical = -spineCompressAmt * 0.5 * anticipation
                + spineCompressAmt * 0.3 * strike;

            // Spine pitch: nose down during anticipation, level during charge,
            // snap up on strike
            double spinePitch = 0.15 * anticipation * spineCompressionFactor
                - 0.25 * strike * headStrikeIntensity;

            Matrix4x4 bodyTransform;
            bodyTransform.translate(forward * bodyForward + upDir * bodyVertical);

            // Spine compression during anticipation (squash)
            Matrix4x4 spineTransform = bodyTransform;
            spineTransform.rotate(right, spinePitch);

            std::map<std::string, Matrix4x4> boneWorldTransforms;

            // Helper: transform a bone with an optional additional rotation
            auto computeBone = [&](const std::string& name,
                                   const Matrix4x4& transform,
                                   double extraYaw = 0.0,
                                   double extraPitch = 0.0) {
                Vector3 pos = bonePos(name);
                Vector3 end = boneEnd(name);
                Vector3 newPos = transform.transformPoint(pos);
                Vector3 newEnd = transform.transformPoint(end);
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

            // === Spine chain ===
            computeBone("Root", bodyTransform);
            computeBone("Pelvis", bodyTransform, 0.0, spinePitch * 0.3);
            computeBone("Spine", spineTransform, 0.0, spinePitch * 0.5);
            computeBone("Chest", spineTransform, 0.0, spinePitch * 0.8);

            // === Neck: drops during anticipation, extends during charge ===
            double neckPitch = 0.3 * anticipation * headDropFactor // nose down
                - 0.15 * charge * (1.0 - strike) // level out during charge
                - 0.4 * strike * headStrikeIntensity; // snap up on impact
            computeBone("Neck", spineTransform, 0.0, neckPitch);

            // === Head: low during anticipation/charge, snaps up on strike ===
            double headPitchAngle = 0.35 * anticipation * headDropFactor
                + 0.2 * charge * headDropFactor * (1.0 - strike)
                - 0.6 * strike * headStrikeIntensity;
            // Small lateral shake on impact (secondary motion)
            double headYaw = 0.08 * strike * std::sin(strike * Math::Pi * 3.0);
            computeBone("Head", spineTransform, headYaw, headPitchAngle);

            // === Jaw: opens on impact ===
            if (boneIdx.count("Jaw")) {
                double jawAngle = 0.4 * strike * jawOpenFactor;
                computeBone("Jaw", spineTransform, 0.0, jawAngle);
            }

            // === Tail: whips as counter-balance ===
            static const char* tailBones[] = { "TailBase", "TailMid", "TailTip" };
            for (int ti = 0; ti < 3; ++ti) {
                if (boneIdx.count(tailBones[ti]) == 0)
                    continue;
                double delay = ti * 0.08; // overlapping action delay per segment
                double effectiveT = tNormalized - delay;
                double tailSwing = 0.0;
                if (effectiveT > strikeMoment && effectiveT < 1.0) {
                    double tailT = (effectiveT - strikeMoment) / (1.0 - strikeMoment);
                    // Damped oscillation: whip then settle
                    tailSwing = tailWhipAmp * (1.0 + ti * 0.3)
                        * std::sin(tailT * Math::Pi * 3.0)
                        * std::exp(-tailT * 3.0);
                }
                // Tail rises slightly during anticipation
                double tailLift = -0.1 * anticipation * (1.0 + ti * 0.2);
                computeBone(tailBones[ti], bodyTransform, tailSwing, tailLift);
            }

            // === Front legs: brace during anticipation, absorb impact ===
            {
                // During anticipation: front legs widen stance slightly and bend
                // During charge: legs reach forward
                // During strike: legs compress (shock absorption)
                double frontLegPitch = 0.12 * anticipation * frontLegBraceFactor
                    - 0.08 * charge * (1.0 - strike)
                    + 0.15 * strike; // compression on impact

                static const char* frontLegs[][3] = {
                    { "FrontLeftUpperLeg", "FrontLeftLowerLeg", "FrontLeftFoot" },
                    { "FrontRightUpperLeg", "FrontRightLowerLeg", "FrontRightFoot" }
                };

                for (int li = 0; li < 2; ++li) {
                    Vector3 hipPos = bodyTransform.transformPoint(bonePos(frontLegs[li][0]));
                    Vector3 footRestPos = boneEnd(frontLegs[li][2]);

                    // Foot target: plants forward during charge, compresses on strike
                    Vector3 footTarget = footRestPos
                        + forward * (legBrace * charge * 0.5)
                        + upDir * (-legBrace * strike * 0.3);

                    // Use IK for the 3-joint leg chain
                    Vector3 upperEnd = boneEnd(frontLegs[li][0]);
                    Vector3 lowerEnd = boneEnd(frontLegs[li][1]);

                    std::vector<Vector3> chain = { hipPos,
                        bodyTransform.transformPoint(upperEnd),
                        bodyTransform.transformPoint(lowerEnd) };

                    // Pole vector: front legs bend backward (knees back)
                    Vector3 poleVector = chain[1] - forward * spineLength * 0.5;
                    solveTwoBoneIk(chain, footTarget, poleVector);

                    boneWorldTransforms[frontLegs[li][0]] = buildBoneWorldTransform(chain[0], chain[1]);
                    boneWorldTransforms[frontLegs[li][1]] = buildBoneWorldTransform(chain[1], chain[2]);
                    boneWorldTransforms[frontLegs[li][2]] = buildBoneWorldTransform(chain[2], footTarget);
                }
            }

            // === Back legs: push off during charge ===
            {
                double backLegExtension = 0.0;
                if (tNormalized >= anticipationDuration && tNormalized < strikeEnd) {
                    double pushT = (tNormalized - anticipationDuration) / (strikeEnd - anticipationDuration);
                    // Explosive extension then settle
                    backLegExtension = legPush * std::sin(pushT * Math::Pi) * backLegPushFactor;
                }

                static const char* backLegs[][3] = {
                    { "BackLeftUpperLeg", "BackLeftLowerLeg", "BackLeftFoot" },
                    { "BackRightUpperLeg", "BackRightLowerLeg", "BackRightFoot" }
                };

                for (int li = 0; li < 2; ++li) {
                    Vector3 hipPos = bodyTransform.transformPoint(bonePos(backLegs[li][0]));
                    Vector3 footRestPos = boneEnd(backLegs[li][2]);

                    // Foot pushes backward during charge
                    Vector3 footTarget = footRestPos
                        - forward * backLegExtension
                        + upDir * (-backLegExtension * 0.15);

                    Vector3 upperEnd = boneEnd(backLegs[li][0]);
                    Vector3 lowerEnd = boneEnd(backLegs[li][1]);

                    std::vector<Vector3> chain = { hipPos,
                        bodyTransform.transformPoint(upperEnd),
                        bodyTransform.transformPoint(lowerEnd) };

                    // Pole vector: back legs bend forward (hocks forward)
                    Vector3 poleVector = chain[1] + forward * spineLength * 0.5;
                    solveTwoBoneIk(chain, footTarget, poleVector);

                    boneWorldTransforms[backLegs[li][0]] = buildBoneWorldTransform(chain[0], chain[1]);
                    boneWorldTransforms[backLegs[li][1]] = buildBoneWorldTransform(chain[1], chain[2]);
                    boneWorldTransforms[backLegs[li][2]] = buildBoneWorldTransform(chain[2], footTarget);
                }
            }

            // === Build skin matrices ===
            auto& frameData = animationClip.frames[frame];
            frameData.time = static_cast<float>(tNormalized) * durationSeconds;
            frameData.boneWorldTransforms = boneWorldTransforms;

            for (const auto& pair : boneWorldTransforms) {
                auto invIt = inverseBindMatrices.find(pair.first);
                if (invIt != inverseBindMatrices.end()) {
                    Matrix4x4 skinMat = pair.second;
                    skinMat *= invIt->second;
                    frameData.boneSkinMatrices[pair.first] = skinMat;
                }
            }
        }

        return true;
    }

} // namespace quadruped

} // namespace dust3d
