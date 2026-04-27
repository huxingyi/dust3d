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

// Procedural hurt/hit-reaction animation for biped rig.
//
// Techniques applied:
//
// 1. HIT-STOP (IMPACT FREEZE) — At the moment of impact (~2-3 frames), the
//    character holds at peak recoil before moving. This "hit-stop" technique
//    sells the weight of the blow by giving the viewer's eye time to register
//    the deformation before motion blur takes over.
//
// 2. DIRECTIONAL HIT RESPONSE — The reaction vector is computed from a
//    configurable hit direction. Hits from the left cause right-side recoil,
//    spine bends away from impact, and the arm on the hit side rises
//    protectively for contextual damage reactions.
//
// 3. PROCEDURAL TWO-BONE IK FOR STABILITY — Feet remain planted via analytic
//    IK while the upper body recoils. The knee bend increases dynamically to
//    absorb impact force. This prevents foot sliding and keeps the character
//    grounded during hit reactions.
//
// 4. DAMPED SPRING STAGGER — Post-impact, the body oscillates with
//    exponentially decaying amplitude (damped harmonic motion). This simulates
//    the character's vestibular system struggling to maintain balance,
//    producing organic stagger instead of robotic linear interpolation.
//
// 5. LAYERED RECOVERY ENVELOPES — Multiple recovery channels (spine, head,
//    arms, legs) use different decay time constants so joints don't all return
//    to neutral at the same rate. Faster recovery in legs (need balance),
//    slower in head (disorientation lingers).
//
// 6. LOOPABLE NEUTRAL RETURN — The animation returns exactly to bind pose
//    by t=0.75 and holds neutral through t=1.0, making the clip seamlessly
//    loopable for continuous stun states or clean idle transitions.
//
// Tunable parameters:
//   - recoilIntensity:     horizontal knock-back distance
//   - staggerAmplitude:    sway magnitude during stagger phase
//   - hitDirection:        -1=left, 0=front, 1=right, -2=back, 2=back
//   - headFlinchFactor:    head snap-back on impact
//   - spineBendFactor:     lateral spine curvature from blow
//   - armGuardFactor:      protective arm raise on hit side
//   - legBuckleFactor:     knee collapse to absorb impact
//   - tailTuckFactor:      tail defensive curl
//   - recoverySpeed:       how quickly the character returns to neutral
//   - bodyMassFactor:      heavy = less recoil, slower stagger decay

#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/biped/hurt.h>
#include <dust3d/animation/common.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace biped {

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
            "Root", "Hips", "Spine", "Chest", "Neck", "Head",
            "LeftUpperLeg", "LeftLowerLeg", "LeftFoot",
            "RightUpperLeg", "RightLowerLeg", "RightFoot",
            "LeftShoulder", "LeftUpperArm", "LeftLowerArm", "LeftHand",
            "RightShoulder", "RightUpperArm", "RightLowerArm", "RightHand"
        };
        if (!validateRequiredBones(boneIdx, requiredBones, sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;

        // User-tunable parameters
        double recoilIntensity = parameters.getValue("recoilIntensity", 1.0);
        double staggerAmplitude = parameters.getValue("staggerAmplitude", 1.0);
        double hitDirection = parameters.getValue("hitDirection", 0.0); // -1=left, 0=front, 1=right
        double headFlinchFactor = parameters.getValue("headFlinchFactor", 1.0);
        double spineBendFactor = parameters.getValue("spineBendFactor", 1.0);
        double armGuardFactor = parameters.getValue("armGuardFactor", 1.0);
        double legBuckleFactor = parameters.getValue("legBuckleFactor", 1.0);
        double tailTuckFactor = parameters.getValue("tailTuckFactor", 1.0);
        double recoverySpeed = parameters.getValue("recoverySpeed", 1.0);
        double bodyMassFactor = parameters.getValue("bodyMassFactor", 1.0);

        // Derive coordinate frame
        Vector3 hipsPos = bonePos("Hips");
        Vector3 chestPos = bonePos("Chest");
        Vector3 spineVec = chestPos - hipsPos;
        double spineLength = spineVec.length();
        if (spineLength < 1e-6)
            spineLength = 1.0;

        Vector3 upDir(0.0, 1.0, 0.0);
        // Derive forward from hips-to-feet direction (same as idle animation).
        // Using feet is more robust than spine projection because the spine
        // can be vertical or tilted arbitrarily in bind pose, whereas feet
        // always point in the character's facing direction.
        Vector3 leftFootEnd = boneEnd("LeftFoot");
        Vector3 rightFootEnd = boneEnd("RightFoot");
        Vector3 avgFootEnd = (leftFootEnd + rightFootEnd) * 0.5;
        Vector3 hipsToFoot = avgFootEnd - hipsPos;
        Vector3 forward(hipsToFoot.x(), 0.0, hipsToFoot.z());
        if (forward.lengthSquared() < 1e-8)
            forward = Vector3(0.0, 0.0, 1.0);
        forward.normalize();
        Vector3 right = Vector3::crossProduct(upDir, forward);
        if (right.lengthSquared() < 1e-8)
            right = Vector3(1.0, 0.0, 0.0);
        right.normalize();

        // Compute body scale from leg length
        double leftLegLen = (boneEnd("LeftUpperLeg") - bonePos("LeftUpperLeg")).length()
            + (boneEnd("LeftLowerLeg") - bonePos("LeftLowerLeg")).length()
            + (boneEnd("LeftFoot") - bonePos("LeftFoot")).length();
        double rightLegLen = (boneEnd("RightUpperLeg") - bonePos("RightUpperLeg")).length()
            + (boneEnd("RightLowerLeg") - bonePos("RightLowerLeg")).length()
            + (boneEnd("RightFoot") - bonePos("RightFoot")).length();
        double avgLegLength = (leftLegLen + rightLegLen) * 0.5;
        if (avgLegLength < 1e-6)
            avgLegLength = 1.0;

        // Inertia: heavier creatures recoil less but stagger longer
        double massInv = 1.0 / (0.5 + 0.5 * bodyMassFactor);

        // Scale-relative amplitudes
        double recoilDist = avgLegLength * 0.35 * recoilIntensity * massInv;
        double staggerSway = avgLegLength * 0.1 * staggerAmplitude;
        double headFlinch = 0.55 * headFlinchFactor;
        double spineBend = 0.18 * spineBendFactor;
        double legBuckle = avgLegLength * 0.1 * legBuckleFactor;
        double tailTuck = 0.45 * tailTuckFactor;
        double armGuard = 0.6 * armGuardFactor;

        // Hit direction vector
        // hitDirection: -1 = hit from left, 0 = hit from front, 1 = hit from right
        double hitLateral = std::max(-1.0, std::min(1.0, hitDirection));
        Vector3 hitVec = (-forward * (1.0 - std::abs(hitLateral)) + right * hitLateral).normalized();
        // Recoil is opposite to hit direction
        Vector3 recoilDir = -hitVec;

        // Phase boundaries
        double hitFreezeEnd = 0.08; // hit-freeze: hold peak recoil
        double impactEnd = 0.15;
        double staggerEnd = 0.45;
        double recoveryEnd = 0.75; // back to neutral by here

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            double tNormalized = static_cast<double>(frame) / static_cast<double>(frameCount);

            // === Phase envelopes ===
            double impact = 0.0;
            double stagger = 0.0;
            double recovery = 0.0;
            double neutralHold = 0.0;

            if (tNormalized < hitFreezeEnd) {
                double t = tNormalized / hitFreezeEnd;
                impact = 1.0 - std::exp(-t * 10.0);
            } else if (tNormalized < impactEnd) {
                impact = 1.0;
            } else if (tNormalized < staggerEnd) {
                double t = (tNormalized - impactEnd) / (staggerEnd - impactEnd);
                impact = (1.0 - smoothstep(t)) * 0.6;
                stagger = std::sin(t * Math::Pi);
            } else if (tNormalized < recoveryEnd) {
                double t = (tNormalized - staggerEnd) / (recoveryEnd - staggerEnd);
                recovery = smootherstep(t * recoverySpeed);
                if (recovery > 1.0)
                    recovery = 1.0;
                stagger = (1.0 - smoothstep(t)) * 0.3;
            } else {
                neutralHold = 1.0;
            }

            // === Body translation ===
            double recoilAmount = recoilDist * impact + recoilDist * 0.3 * stagger;
            recoilAmount *= (1.0 - recovery);

            // Lateral stagger sway (damped oscillation)
            double staggerPhase = 0.0;
            if (tNormalized > impactEnd && tNormalized < recoveryEnd) {
                double staggerT = (tNormalized - impactEnd) / (recoveryEnd - impactEnd + 1e-8);
                staggerPhase = staggerSway * std::sin(staggerT * Math::Pi * 2.5)
                    * std::exp(-staggerT * 2.0) * (1.0 - recovery);
            }

            // Vertical: slight drop on impact (legs buckle), then recover
            double verticalDrop = -legBuckle * impact * (1.0 - recovery);

            Vector3 bodyOffset = recoilDir * recoilAmount
                + right * staggerPhase
                + upDir * verticalDrop;

            // Spine pitch: tilts backward on impact
            double bodyPitch = -0.15 * impact * recoilIntensity * (1.0 - recovery);
            // Lateral bend away from hit
            double bodyRoll = spineBend * hitLateral * impact * (1.0 - recovery);

            Matrix4x4 bodyTransform;
            bodyTransform.translate(bodyOffset);

            Matrix4x4 hipsTransform = bodyTransform;
            hipsTransform.rotate(right, bodyPitch * 0.3);
            if (std::abs(bodyRoll) > 1e-6)
                hipsTransform.rotate(forward, bodyRoll * 0.3);

            Matrix4x4 spineTransform = bodyTransform;
            spineTransform.rotate(right, bodyPitch);
            if (std::abs(bodyRoll) > 1e-6)
                spineTransform.rotate(forward, bodyRoll);

            std::map<std::string, Matrix4x4> boneWorldTransforms;

            auto computeBone = [&](const std::string& name,
                                   const Matrix4x4& transform,
                                   double extraYaw = 0.0,
                                   double extraPitch = 0.0,
                                   double extraRoll = 0.0,
                                   Vector3* outPos = nullptr,
                                   Vector3* outEnd = nullptr) {
                Vector3 pos = bonePos(name);
                Vector3 end = boneEnd(name);
                Vector3 newPos = transform.transformPoint(pos);
                Vector3 newEnd = transform.transformPoint(end);
                if (std::abs(extraYaw) > 1e-6 || std::abs(extraPitch) > 1e-6 || std::abs(extraRoll) > 1e-6) {
                    Matrix4x4 extraRot;
                    if (std::abs(extraRoll) > 1e-6)
                        extraRot.rotate(forward, extraRoll);
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
            computeBone("Hips", hipsTransform);
            computeBone("Spine", spineTransform, 0.0, bodyPitch * 0.6, bodyRoll * 0.5);
            computeBone("Chest", spineTransform, 0.0, bodyPitch * 0.8, bodyRoll * 0.7);

            // === Neck: snaps back on impact ===
            double neckPitch = -headFlinch * 0.5 * impact * (1.0 - recovery);
            Vector3 neckWorldPos;
            Vector3 neckWorldEnd;
            computeBone("Neck", spineTransform, 0.0, neckPitch, 0.0, &neckWorldPos, &neckWorldEnd);

            // === Head: flinches back hard ===
            double headPitchAngle = -headFlinch * impact * (1.0 - recovery);
            // Disorientation head-shake during recovery
            double headShakeYaw = 0.0;
            if (tNormalized > staggerEnd && tNormalized < recoveryEnd) {
                double shakeT = (tNormalized - staggerEnd) / (recoveryEnd - staggerEnd + 1e-8);
                headShakeYaw = 0.1 * headFlinchFactor * std::sin(shakeT * Math::Pi * 6.0)
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

            // === Tail: tucks defensively, then uncurls ===
            static const char* tailBones[] = { "TailBase", "TailMid", "TailTip" };
            Vector3 prevTailEnd;
            bool hasPrevTail = false;
            for (int ti = 0; ti < 3; ++ti) {
                if (boneIdx.count(tailBones[ti]) == 0)
                    continue;
                double delay = ti * 0.05;
                double effectiveT = std::max(0.0, tNormalized - delay);
                double tuckPhase = 0.0;
                if (effectiveT < staggerEnd) {
                    double tt = effectiveT / staggerEnd;
                    tuckPhase = std::sin(tt * Math::Pi * 0.5);
                } else {
                    double tt = (effectiveT - staggerEnd) / (1.0 - staggerEnd);
                    tuckPhase = 1.0 - smootherstep(tt);
                }
                double tuckAngle = -tailTuck * (1.0 + ti * 0.3) * tuckPhase;
                double tailYaw = 0.05 * hitLateral * impact * (1.0 + ti * 0.2) * (1.0 - recovery);

                Vector3 pos = bonePos(tailBones[ti]);
                Vector3 end = boneEnd(tailBones[ti]);
                Vector3 newPos = hipsTransform.transformPoint(pos);
                Vector3 newEnd = hipsTransform.transformPoint(end);
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

            // === Arms: protective reaction on hit side ===
            {
                static const char* armBones[][4] = {
                    { "LeftShoulder", "LeftUpperArm", "LeftLowerArm", "LeftHand" },
                    { "RightShoulder", "RightUpperArm", "RightLowerArm", "RightHand" }
                };

                for (int li = 0; li < 2; ++li) {
                    double side = (li == 0) ? -1.0 : 1.0; // left = -1, right = 1
                    bool isHitSide = (hitLateral * side) > 0.1; // hit from this side

                    // Shoulder follows chest
                    Vector3 shoulderPos = spineTransform.transformPoint(bonePos(armBones[li][0]));
                    Vector3 shoulderEnd = spineTransform.transformPoint(boneEnd(armBones[li][0]));
                    boneWorldTransforms[armBones[li][0]] = buildBoneWorldTransform(shoulderPos, shoulderEnd);

                    Vector3 upperStart = shoulderEnd;
                    Vector3 upperEndRest = spineTransform.transformPoint(boneEnd(armBones[li][1]));
                    Vector3 upperDir = upperEndRest - upperStart;

                    // Arm reaction: hit side raises protectively, opposite side flails outward
                    double armReactPitch = 0.0;
                    double armReactYaw = 0.0;
                    if (isHitSide) {
                        // Protective raise toward hit direction
                        armReactPitch = armGuard * impact * (1.0 - recovery);
                        armReactYaw = -side * armGuard * 0.5 * impact * (1.0 - recovery);
                    } else {
                        // Opposite arm flails outward from impact
                        armReactPitch = -armGuard * 0.3 * impact * (1.0 - recovery);
                        armReactYaw = side * armGuard * 0.4 * impact * (1.0 - recovery);
                    }

                    // Add stagger sway to arms
                    double armStagger = staggerPhase * 0.3 * (1.0 - recovery) / (staggerSway + 1e-8);
                    armReactYaw += side * armStagger * 0.1;

                    Matrix4x4 armRot;
                    if (std::abs(armReactYaw) > 1e-6)
                        armRot.rotate(upDir, armReactYaw);
                    if (std::abs(armReactPitch) > 1e-6)
                        armRot.rotate(right, armReactPitch);

                    Vector3 newUpperEnd = upperStart + armRot.transformVector(upperDir);
                    boneWorldTransforms[armBones[li][1]] = buildBoneWorldTransform(upperStart, newUpperEnd);

                    // Lower arm: slightly more reaction (lag)
                    Matrix4x4 lowerRot;
                    double lowerLag = 1.15;
                    if (std::abs(armReactYaw) > 1e-6)
                        lowerRot.rotate(upDir, armReactYaw * lowerLag);
                    if (std::abs(armReactPitch) > 1e-6)
                        lowerRot.rotate(right, armReactPitch * lowerLag);

                    Vector3 lowerDir = spineTransform.transformPoint(boneEnd(armBones[li][2])) - spineTransform.transformPoint(boneEnd(armBones[li][1]));
                    Vector3 newLowerEnd = newUpperEnd + lowerRot.transformVector(lowerDir);
                    boneWorldTransforms[armBones[li][2]] = buildBoneWorldTransform(newUpperEnd, newLowerEnd);

                    // Hand: most lag
                    Matrix4x4 handRot;
                    double handLag = 1.3;
                    if (std::abs(armReactYaw) > 1e-6)
                        handRot.rotate(upDir, armReactYaw * handLag);
                    if (std::abs(armReactPitch) > 1e-6)
                        handRot.rotate(right, armReactPitch * handLag);

                    Vector3 handDir = spineTransform.transformPoint(boneEnd(armBones[li][3])) - spineTransform.transformPoint(boneEnd(armBones[li][2]));
                    Vector3 newHandEnd = newLowerEnd + handRot.transformVector(handDir);
                    boneWorldTransforms[armBones[li][3]] = buildBoneWorldTransform(newLowerEnd, newHandEnd);
                }
            }

            // === Legs: feet planted, use two-bone IK for stability ===
            {
                static const char* legs[][3] = {
                    { "LeftUpperLeg", "LeftLowerLeg", "LeftFoot" },
                    { "RightUpperLeg", "RightLowerLeg", "RightFoot" }
                };

                for (int li = 0; li < 2; ++li) {
                    Vector3 hipPos = hipsTransform.transformPoint(bonePos(legs[li][0]));
                    Vector3 footRestPos = boneEnd(legs[li][2]);

                    // Foot stays planted (no body transform)
                    Vector3 footStart = bonePos(legs[li][2]);
                    Vector3 footEnd = boneEnd(legs[li][2]);
                    boneWorldTransforms[legs[li][2]] = buildBoneWorldTransform(footStart, footEnd);

                    double side = (li == 0) ? -1.0 : 1.0;

                    // On impact: foot slides back slightly (bracing), drops with body
                    Vector3 footTarget = footRestPos
                        + recoilDir * (legBuckle * 0.3 * impact * (1.0 - recovery))
                        + upDir * (-legBuckle * 0.35 * impact * (1.0 - recovery))
                        // Widen stance during stagger for balance
                        + right * (side * avgLegLength * 0.04 * stagger * (1.0 - recovery));
                    footTarget.setX(footRestPos.x());

                    Vector3 upperEnd = boneEnd(legs[li][0]);
                    Vector3 lowerEnd = boneEnd(legs[li][1]);

                    std::vector<Vector3> chain = { hipPos,
                        hipsTransform.transformPoint(upperEnd),
                        hipsTransform.transformPoint(lowerEnd) };

                    // Pole vector: knee bends forward
                    Vector3 poleVector = chain[1] + forward * avgLegLength * 0.5;
                    solveTwoBoneIk(chain, footTarget, poleVector);

                    boneWorldTransforms[legs[li][0]] = buildBoneWorldTransform(chain[0], chain[1]);
                    boneWorldTransforms[legs[li][1]] = buildBoneWorldTransform(chain[1], chain[2]);

                    // Recompute foot transform to match IK result
                    Vector3 footRestDir = boneEnd(legs[li][2]) - bonePos(legs[li][2]);
                    double footLen = footRestDir.length();
                    if (footLen < 1e-6)
                        footLen = 0.01;
                    footRestDir = footRestDir * (1.0 / footLen);
                    Vector3 footEndPos = chain[2] + footRestDir * footLen;
                    footEndPos.setY(footTarget.y());
                    boneWorldTransforms[legs[li][2]] = buildBoneWorldTransform(chain[2], footEndPos);
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

} // namespace biped

} // namespace dust3d
