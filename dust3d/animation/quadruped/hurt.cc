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

// Procedural hurt/hit-reaction animation for quadruped rig.
//
// Plays when the creature receives a head-butt attack from another quadruped.
// The animation shows the impact reaction and recovery:
//
//   Phase 1 — Impact Recoil   (0.00 – 0.15):  Sudden backward jolt from the
//       hit.  Head snaps back, spine compresses on the receiving side, front
//       legs buckle slightly from the force.  This is the "hit-stop" frame
//       that sells the weight of the blow.
//
//   Phase 2 — Stagger         (0.15 – 0.45):  The creature staggers backward,
//       weight shifting unevenly.  Legs scramble to maintain balance with
//       asymmetric timing (the hit side reacts first).  Head drops, spine
//       curves laterally away from the impact.
//
//   Phase 3 — Stabilize       (0.45 – 0.70):  Legs plant wide for stability,
//       spine straightens.  Head shakes briefly (pain reaction / clearing
//       daze).  Tail tucks defensively.
//
//   Phase 4 — Recovery        (0.70 – 1.00):  Smooth return to rest pose.
//       Head lifts back to neutral, legs return to normal stance width.
//       Ease-out so the clip transitions cleanly to idle.
//
// Tunable parameters:
//   - recoilIntensity:     how far the creature is knocked back
//   - staggerAmplitude:    lateral sway during stagger phase
//   - hitDirection:        which side the hit comes from (-1=left, 0=front, 1=right)
//   - headFlinchFactor:    how much the head snaps back on impact
//   - spineBendFactor:     lateral spine deformation from the blow
//   - legBuckleFactor:     how much front legs collapse on impact
//   - tailTuckFactor:      tail defensive curl intensity
//   - headShakeFactor:     pain head-shake amplitude during stabilize
//   - recoverySpeed:       how quickly the creature returns to rest
//   - bodyMassFactor:      heavier = less recoil, slower stagger

#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/common.h>
#include <dust3d/animation/quadruped/hurt.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace quadruped {

    bool hurt(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = static_cast<int>(parameters.getValue("frameCount", 36));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 1.0));

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

        // User-tunable parameters
        double recoilIntensity = parameters.getValue("recoilIntensity", 1.0);
        double staggerAmplitude = parameters.getValue("staggerAmplitude", 1.0);
        double hitDirection = parameters.getValue("hitDirection", 0.0); // -1=left, 0=front, 1=right
        double headFlinchFactor = parameters.getValue("headFlinchFactor", 1.0);
        double spineBendFactor = parameters.getValue("spineBendFactor", 1.0);
        double legBuckleFactor = parameters.getValue("legBuckleFactor", 1.0);
        double tailTuckFactor = parameters.getValue("tailTuckFactor", 1.0);
        double headShakeFactor = parameters.getValue("headShakeFactor", 1.0);
        double recoverySpeed = parameters.getValue("recoverySpeed", 1.0);
        double bodyMassFactor = parameters.getValue("bodyMassFactor", 1.0);

        // Derive coordinate frame
        Vector3 pelvisPos = bonePos("Pelvis");
        Vector3 chestPos = bonePos("Chest");
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

        // Inertia: heavier creatures recoil less but stagger longer
        double massInv = 1.0 / (0.5 + 0.5 * bodyMassFactor);

        // Scale-relative amplitudes
        double recoilDist = spineLength * 0.4 * recoilIntensity * massInv;
        double staggerSway = spineLength * 0.12 * staggerAmplitude;
        double headFlinch = 0.5 * headFlinchFactor;
        double spineBend = 0.15 * spineBendFactor;
        double legBuckle = spineLength * 0.08 * legBuckleFactor;
        double tailTuck = 0.4 * tailTuckFactor;
        double headShake = 0.12 * headShakeFactor;

        // Hit direction vector (blend of forward and lateral)
        // hitDirection: -1 = hit from left, 0 = hit from front, 1 = hit from right
        double hitLateral = std::max(-1.0, std::min(1.0, hitDirection));
        Vector3 hitVec = (-forward * (1.0 - std::abs(hitLateral)) + right * hitLateral).normalized();
        // Recoil is opposite to hit direction
        Vector3 recoilDir = -hitVec;

        // Phase boundaries
        double hitFreezeEnd = 0.08; // hit-freeze: hold peak recoil for ~2-3 frames
        double impactEnd = 0.15;
        double staggerEnd = 0.45;
        double stabilizeEnd = 0.70;

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            double tNormalized = static_cast<double>(frame) / static_cast<double>(frameCount);

            // === Phase envelopes ===
            double impact = 0.0;
            double stagger = 0.0;
            double stabilize = 0.0;
            double recovery = 0.0;

            if (tNormalized < hitFreezeEnd) {
                // Rapid rise to peak — exponential attack
                double t = tNormalized / hitFreezeEnd;
                impact = 1.0 - std::exp(-t * 10.0);
            } else if (tNormalized < impactEnd) {
                // Hit-freeze plateau: hold at peak recoil (fighting game hit-stop)
                // This 2-3 frame hold sells the weight of the blow
                impact = 1.0;
            } else if (tNormalized < staggerEnd) {
                double t = (tNormalized - impactEnd) / (staggerEnd - impactEnd);
                // Impact decays through stagger
                impact = (1.0 - smoothstep(t)) * 0.6;
                // Stagger builds then peaks at ~60%
                stagger = std::sin(t * Math::Pi);
            } else if (tNormalized < stabilizeEnd) {
                double t = (tNormalized - staggerEnd) / (stabilizeEnd - staggerEnd);
                stabilize = std::sin(t * Math::Pi * 0.5); // rises to 1
                stagger = (1.0 - smoothstep(t)) * 0.3; // stagger fading
            } else {
                double t = (tNormalized - stabilizeEnd) / (1.0 - stabilizeEnd);
                recovery = smootherstep(t * recoverySpeed);
                if (recovery > 1.0)
                    recovery = 1.0;
                stabilize = 1.0 - recovery;
            }

            // === Body translation ===
            // Recoil: knocked backward on impact, drifts during stagger
            double recoilAmount = recoilDist * impact + recoilDist * 0.3 * stagger;
            // Return to origin during recovery
            recoilAmount *= (1.0 - recovery);

            // Lateral stagger sway (damped oscillation)
            double staggerPhase = 0.0;
            if (tNormalized > impactEnd && tNormalized < stabilizeEnd) {
                double staggerT = (tNormalized - impactEnd) / (stabilizeEnd - impactEnd);
                staggerPhase = staggerSway * std::sin(staggerT * Math::Pi * 2.5)
                    * std::exp(-staggerT * 2.0) * (1.0 - recovery);
            }

            // Vertical: slight drop on impact (legs buckle), then recover
            double verticalDrop = -legBuckle * impact * (1.0 - recovery);

            Vector3 bodyOffset = recoilDir * recoilAmount
                + right * staggerPhase
                + upDir * verticalDrop;

            // Spine pitch: tilts backward on impact from the blow
            double bodyPitch = -0.12 * impact * recoilIntensity * (1.0 - recovery);
            // Lateral bend away from hit
            double bodyRoll = spineBend * hitLateral * impact * (1.0 - recovery);

            Matrix4x4 bodyTransform;
            bodyTransform.translate(bodyOffset);

            Matrix4x4 pelvisTransform = bodyTransform;
            pelvisTransform.rotate(right, bodyPitch * 0.3);
            if (std::abs(bodyRoll) > 1e-6)
                pelvisTransform.rotate(forward, bodyRoll * 0.3);

            Matrix4x4 spineTransform = bodyTransform;
            spineTransform.rotate(right, bodyPitch);
            if (std::abs(bodyRoll) > 1e-6)
                spineTransform.rotate(forward, bodyRoll);

            std::map<std::string, Matrix4x4> boneWorldTransforms;

            auto computeBone = [&](const std::string& name,
                                   const Matrix4x4& transform,
                                   double extraYaw = 0.0,
                                   double extraPitch = 0.0,
                                   Vector3* outPos = nullptr,
                                   Vector3* outEnd = nullptr) {
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
                if (outPos)
                    *outPos = newPos;
                if (outEnd)
                    *outEnd = newEnd;
            };

            // === Spine chain ===
            computeBone("Root", bodyTransform);
            computeBone("Pelvis", pelvisTransform);
            computeBone("Spine", spineTransform, 0.0, bodyPitch * 0.6);
            computeBone("Chest", spineTransform, 0.0, bodyPitch * 0.8);

            // === Neck: snaps back on impact ===
            double neckPitch = -headFlinch * 0.5 * impact * (1.0 - recovery);
            Vector3 neckWorldPos;
            Vector3 neckWorldEnd;
            computeBone("Neck", spineTransform, 0.0, neckPitch, &neckWorldPos, &neckWorldEnd);

            // === Head: flinches back hard, then shakes during stabilize ===
            double headPitchAngle = -headFlinch * impact * (1.0 - recovery);
            // Pain head-shake: rapid lateral oscillation during stabilize phase
            double headShakeYaw = 0.0;
            if (tNormalized > staggerEnd && tNormalized < stabilizeEnd + 0.1) {
                double shakeT = (tNormalized - staggerEnd) / (stabilizeEnd - staggerEnd + 1e-8);
                headShakeYaw = headShake * std::sin(shakeT * Math::Pi * 6.0)
                    * std::exp(-shakeT * 3.0) * (1.0 - recovery);
            }
            Vector3 headWorldStart;
            Vector3 headWorldDir;
            {
                Vector3 headPosRest = bonePos("Head");
                Vector3 headEndRest = boneEnd("Head");
                Vector3 headWorldPos = spineTransform.transformPoint(headPosRest);
                Vector3 headWorldEnd = spineTransform.transformPoint(headEndRest);
                Vector3 headDir = headWorldEnd - headWorldPos;
                Vector3 headStart = neckWorldEnd;
                Vector3 headEnd = headStart + headDir;
                if (std::abs(headShakeYaw) > 1e-6 || std::abs(headPitchAngle) > 1e-6) {
                    Matrix4x4 extraRot;
                    if (std::abs(headShakeYaw) > 1e-6)
                        extraRot.rotate(upDir, headShakeYaw);
                    if (std::abs(headPitchAngle) > 1e-6)
                        extraRot.rotate(right, headPitchAngle);
                    headEnd = headStart + extraRot.transformVector(headDir);
                }
                boneWorldTransforms["Head"] = buildBoneWorldTransform(headStart, headEnd);
                headWorldStart = headStart;
                headWorldDir = headEnd - headStart;
            }

            // === Jaw: opens on impact (pain gasp) ===
            if (boneIdx.count("Jaw")) {
                double jawAngle = 0.3 * impact * (1.0 - recovery);
                Vector3 jawPosRest = bonePos("Jaw");
                Vector3 jawEndRest = boneEnd("Jaw");
                Vector3 jawWorldPos = spineTransform.transformPoint(jawPosRest);
                Vector3 jawWorldEnd = spineTransform.transformPoint(jawEndRest);
                Vector3 jawDir = jawWorldEnd - jawWorldPos;
                Vector3 jawStart = headWorldStart;
                Vector3 jawEnd = jawStart + jawDir;
                if (std::abs(jawAngle) > 1e-6) {
                    Matrix4x4 extraRot;
                    extraRot.rotate(right, jawAngle);
                    jawEnd = jawStart + extraRot.transformVector(jawDir);
                }
                boneWorldTransforms["Jaw"] = buildBoneWorldTransform(jawStart, jawEnd);
            }

            // === Tail: tucks defensively, then uncurls ===
            static const char* tailBones[] = { "TailBase", "TailMid", "TailTip" };
            Vector3 prevTailEnd;
            bool hasPrevTail = false;
            for (int ti = 0; ti < 3; ++ti) {
                if (boneIdx.count(tailBones[ti]) == 0)
                    continue;
                double delay = ti * 0.05;
                double effectiveT = std::max(0.0, tNormalized - delay);
                // Tail curls inward (defensive) during impact/stagger, then releases
                double tuckPhase = 0.0;
                if (effectiveT < staggerEnd) {
                    double tt = effectiveT / staggerEnd;
                    tuckPhase = std::sin(tt * Math::Pi * 0.5); // rise to peak
                } else {
                    double tt = (effectiveT - staggerEnd) / (1.0 - staggerEnd);
                    tuckPhase = 1.0 - smootherstep(tt); // release
                }
                double tuckAngle = -tailTuck * (1.0 + ti * 0.3) * tuckPhase;
                // Small lateral flick on impact
                double tailYaw = 0.05 * hitLateral * impact * (1.0 + ti * 0.2) * (1.0 - recovery);

                Vector3 pos = bonePos(tailBones[ti]);
                Vector3 end = boneEnd(tailBones[ti]);
                Vector3 newPos = pelvisTransform.transformPoint(pos);
                Vector3 newEnd = pelvisTransform.transformPoint(end);
                if (hasPrevTail) {
                    Vector3 offset = newEnd - newPos;
                    newPos = prevTailEnd;
                    newEnd = newPos + offset;
                }
                if (std::abs(tailYaw) > 1e-6 || std::abs(tuckAngle) > 1e-6) {
                    Matrix4x4 extraRot;
                    if (std::abs(tailYaw) > 1e-6)
                        extraRot.rotate(upDir, tailYaw);
                    if (std::abs(tuckAngle) > 1e-6)
                        extraRot.rotate(right, tuckAngle);
                    Vector3 offset = newEnd - newPos;
                    newEnd = newPos + extraRot.transformVector(offset);
                }
                boneWorldTransforms[tailBones[ti]] = buildBoneWorldTransform(newPos, newEnd);
                prevTailEnd = newEnd;
                hasPrevTail = true;
            }

            // === Front legs: buckle on impact, widen during stabilize ===
            {
                static const char* frontLegs[][3] = {
                    { "FrontLeftUpperLeg", "FrontLeftLowerLeg", "FrontLeftFoot" },
                    { "FrontRightUpperLeg", "FrontRightLowerLeg", "FrontRightFoot" }
                };

                for (int li = 0; li < 2; ++li) {
                    Vector3 hipPos = spineTransform.transformPoint(bonePos(frontLegs[li][0]));
                    Vector3 footRestPos = boneEnd(frontLegs[li][2]);

                    // Buckle: foot slides back and down on impact
                    double side = (li == 0) ? -1.0 : 1.0;
                    Vector3 footTarget = footRestPos
                        + recoilDir * (legBuckle * 0.5 * impact * (1.0 - recovery))
                        + upDir * (-legBuckle * 0.4 * impact * (1.0 - recovery))
                        // Widen stance during stabilize for balance
                        + right * (side * spineLength * 0.06 * stabilize * (1.0 - recovery));
                    footTarget.setX(footRestPos.x());

                    Vector3 upperEnd = boneEnd(frontLegs[li][0]);
                    Vector3 lowerEnd = boneEnd(frontLegs[li][1]);

                    std::vector<Vector3> chain = { hipPos,
                        spineTransform.transformPoint(upperEnd),
                        spineTransform.transformPoint(lowerEnd) };

                    Vector3 poleVector = chain[1] - forward * spineLength * 0.5;
                    solveTwoBoneIk(chain, footTarget, poleVector);

                    boneWorldTransforms[frontLegs[li][0]] = buildBoneWorldTransform(chain[0], chain[1]);
                    boneWorldTransforms[frontLegs[li][1]] = buildBoneWorldTransform(chain[1], chain[2]);

                    Vector3 footRestDir = boneEnd(frontLegs[li][2]) - bonePos(frontLegs[li][2]);
                    double footLen = footRestDir.length();
                    if (footLen < 1e-6)
                        footLen = 0.01;
                    footRestDir = footRestDir * (1.0 / footLen);
                    Vector3 footEnd = chain[2] + footRestDir * footLen;
                    footEnd.setY(footTarget.y());
                    boneWorldTransforms[frontLegs[li][2]] = buildBoneWorldTransform(chain[2], footEnd);
                }
            }

            // === Back legs: brace against recoil, push to stabilize ===
            {
                static const char* backLegs[][3] = {
                    { "BackLeftUpperLeg", "BackLeftLowerLeg", "BackLeftFoot" },
                    { "BackRightUpperLeg", "BackRightLowerLeg", "BackRightFoot" }
                };

                for (int li = 0; li < 2; ++li) {
                    Vector3 hipPos = pelvisTransform.transformPoint(bonePos(backLegs[li][0]));
                    Vector3 footRestPos = boneEnd(backLegs[li][2]);

                    double side = (li == 0) ? -1.0 : 1.0;
                    // Back legs slide back on impact (bracing), widen during stabilize
                    Vector3 footTarget = footRestPos
                        + recoilDir * (legBuckle * 0.3 * impact * (1.0 - recovery))
                        + right * (side * spineLength * 0.04 * stabilize * (1.0 - recovery));
                    footTarget.setX(footRestPos.x());

                    Vector3 upperEnd = boneEnd(backLegs[li][0]);
                    Vector3 lowerEnd = boneEnd(backLegs[li][1]);

                    std::vector<Vector3> chain = { hipPos,
                        pelvisTransform.transformPoint(upperEnd),
                        pelvisTransform.transformPoint(lowerEnd) };

                    Vector3 poleVector = chain[1] + forward * spineLength * 0.5;
                    solveTwoBoneIk(chain, footTarget, poleVector);

                    boneWorldTransforms[backLegs[li][0]] = buildBoneWorldTransform(chain[0], chain[1]);
                    boneWorldTransforms[backLegs[li][1]] = buildBoneWorldTransform(chain[1], chain[2]);

                    Vector3 footRestDir = boneEnd(backLegs[li][2]) - bonePos(backLegs[li][2]);
                    double footLen = footRestDir.length();
                    if (footLen < 1e-6)
                        footLen = 0.01;
                    footRestDir = footRestDir * (1.0 / footLen);
                    Vector3 footEnd = chain[2] + footRestDir * footLen;
                    footEnd.setY(footTarget.y());
                    boneWorldTransforms[backLegs[li][2]] = buildBoneWorldTransform(chain[2], footEnd);
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
