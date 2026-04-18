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

// Procedural idle animation for quadruped rig.
//
// Designed for game development idle/rest state. The animal stands in
// place with subtle breathing, tail sway, head movement, and minor
// weight shifts. Works for horses, dogs, cats, deer, and any
// four-legged creature using the quadruped bone structure.
//
// Adjustable animation parameters:
//   - breathingAmplitudeFactor:   chest rise-fall intensity
//   - breathingSpeedFactor:       breathing cycle speed multiplier
//   - tailIdleFactor:       tail gentle sway amplitude
//   - headLookFactor:       subtle head turn/nod amplitude
//   - earTwitchFactor:      ear twitch intensity (if ear bones exist)
//   - weightShiftFactor:    subtle lateral body sway
//   - jawFactor:            jaw open/close (chewing, panting)
//   - spineSwayFactor:      subtle spine undulation

#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/common.h>
#include <dust3d/animation/quadruped/idle.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace quadruped {

    bool idle(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = static_cast<int>(parameters.getValue("frameCount", 90));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 4.0));

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

        double breathingAmplitudeFactor = parameters.getValue("breathingAmplitudeFactor", 1.0);
        double breathingSpeedFactor = parameters.getValue("breathingSpeedFactor", 1.0);
        double tailIdleFactor = parameters.getValue("tailIdleFactor", 1.0);
        double headLookFactor = parameters.getValue("headLookFactor", 1.0);
        double weightShiftFactor = parameters.getValue("weightShiftFactor", 1.0);
        double jawFactor = parameters.getValue("jawFactor", 1.0);
        double spineSwayFactor = parameters.getValue("spineSwayFactor", 1.0);
        double frontKneeBendFactor = std::max(0.1, parameters.getValue("frontKneeBendFactor", 1.0));
        double backKneeBendFactor = std::max(0.1, parameters.getValue("backKneeBendFactor", 1.0));

        Vector3 up(0.0, 1.0, 0.0);
        Vector3 pelvisPos = bonePos("Pelvis");
        Vector3 chestPos = bonePos("Chest");
        Vector3 bodyVector = (chestPos - pelvisPos);
        Vector3 forward(bodyVector.x(), 0.0, bodyVector.z());
        if (forward.lengthSquared() < 1e-8)
            forward = Vector3(0.0, 0.0, 1.0);
        forward.normalize();
        Vector3 right = Vector3::crossProduct(up, forward);
        if (right.lengthSquared() < 1e-8)
            right = Vector3(1.0, 0.0, 0.0);
        right.normalize();

        // Body scale from spine length
        double bodyLength = bodyVector.length();
        if (bodyLength < 1e-6)
            bodyLength = 1.0;
        double breathAmp = bodyLength * 0.01 * breathingAmplitudeFactor;
        double shiftAmp = bodyLength * 0.008 * weightShiftFactor;

        // ===================================================================
        // Leg setup: rest-pose data for IK-grounded feet
        // ===================================================================
        struct LegDef {
            std::string upperLegName, lowerLegName, footName;
        };
        static const std::array<LegDef, 4> legs = { {
            { "FrontLeftUpperLeg", "FrontLeftLowerLeg", "FrontLeftFoot" },
            { "FrontRightUpperLeg", "FrontRightLowerLeg", "FrontRightFoot" },
            { "BackLeftUpperLeg", "BackLeftLowerLeg", "BackLeftFoot" },
            { "BackRightUpperLeg", "BackRightLowerLeg", "BackRightFoot" },
        } };

        struct LegRest {
            Vector3 upperLegPos, upperLegEnd, lowerLegEnd, footEnd;
            Vector3 restStickDir, restUpperToLowerVec;
        };
        std::array<LegRest, 4> legRest;
        for (size_t i = 0; i < 4; ++i) {
            legRest[i].upperLegPos = bonePos(legs[i].upperLegName);
            legRest[i].upperLegEnd = boneEnd(legs[i].upperLegName);
            legRest[i].lowerLegEnd = boneEnd(legs[i].lowerLegName);
            legRest[i].footEnd = boneEnd(legs[i].footName);
            Vector3 chordVec = legRest[i].footEnd - legRest[i].upperLegEnd;
            legRest[i].restStickDir = chordVec.isZero() ? Vector3(0, -1, 0) : chordVec.normalized();
            legRest[i].restUpperToLowerVec = legRest[i].lowerLegEnd - legRest[i].upperLegEnd;
        }

        // Ground level: lowest foot tip Y
        double minUpProj = Vector3::dotProduct(legRest[0].footEnd, up);
        for (size_t i = 1; i < 4; ++i) {
            double proj = Vector3::dotProduct(legRest[i].footEnd, up);
            if (proj < minUpProj)
                minUpProj = proj;
        }

        // Pin foot targets at ground level (grounded)
        std::array<Vector3, 4> footHome;
        for (size_t i = 0; i < 4; ++i) {
            double proj = Vector3::dotProduct(legRest[i].footEnd, up);
            footHome[i] = legRest[i].footEnd - up * (proj - minUpProj);
        }

        // Per-bone drop based on position along spine:
        // Front half (Chest end) drops by frontKneeBendFactor
        // Back half (Pelvis end) drops by backKneeBendFactor
        // Middle bones interpolate between the two.
        double frontDrop = bodyLength * 0.10 * (frontKneeBendFactor - 1.0);
        double backDrop = bodyLength * 0.10 * (backKneeBendFactor - 1.0);
        if (frontDrop < 0.0)
            frontDrop = 0.0;
        if (backDrop < 0.0)
            backDrop = 0.0;
        double baseDrop = bodyLength * 0.03; // minimum natural drop

        // Precompute forward-axis projections of Pelvis and Chest for interpolation
        double pelvisForwardProj = Vector3::dotProduct(pelvisPos, forward);
        double chestForwardProj = Vector3::dotProduct(chestPos, forward);
        double spineSpan = chestForwardProj - pelvisForwardProj;
        if (std::abs(spineSpan) < 1e-8)
            spineSpan = 1.0;

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            double tNormalized = static_cast<double>(frame) / static_cast<double>(frameCount);

            // -------------------------------------------------------
            // Layered breathing: primary breath + secondary micro-rhythm
            // Uses two overlapping sine waves for organic feel
            // -------------------------------------------------------
            double breathPhase = tNormalized * 2.0 * Math::Pi * breathingSpeedFactor;
            double primaryBreath = std::sin(breathPhase);
            double secondaryBreath = 0.3 * std::sin(breathPhase * 3.0 + 0.8);
            double breathOffset = breathAmp * (primaryBreath + secondaryBreath);

            // -------------------------------------------------------
            // Weight shift: slow lateral sway with hip counter-roll
            // Mimics real animals shifting weight between leg pairs
            // -------------------------------------------------------
            double shiftPhase = tNormalized * 2.0 * Math::Pi * 1.0 * breathingSpeedFactor;
            double lateralShift = shiftAmp * std::sin(shiftPhase);
            double hipRoll = 0.015 * weightShiftFactor * std::sin(shiftPhase);

            // Base body transform (breathing + sway, no knee-bend drop yet)
            Matrix4x4 baseBodyTransform;
            baseBodyTransform.translate(up * (breathOffset - baseDrop) + right * lateralShift);
            baseBodyTransform.rotate(forward, hipRoll);

            std::map<std::string, Matrix4x4> boneWorldTransforms;

            // Compute per-bone drop based on forward position along spine.
            // t=0 at Pelvis (back) → backDrop, t=1 at Chest (front) → frontDrop
            auto getBoneDrop = [&](const Vector3& pos) -> double {
                double proj = Vector3::dotProduct(pos, forward);
                double t = (proj - pelvisForwardProj) / spineSpan;
                t = std::max(0.0, std::min(1.0, t));
                return backDrop * (1.0 - t) + frontDrop * t;
            };

            auto computeBodyBone = [&](const std::string& name,
                                       double extraYaw = 0.0,
                                       double extraPitch = 0.0,
                                       double extraRoll = 0.0) {
                Vector3 pos = bonePos(name);
                Vector3 end = boneEnd(name);
                Vector3 newPos = baseBodyTransform.transformPoint(pos);
                Vector3 newEnd = baseBodyTransform.transformPoint(end);
                // Apply position-dependent knee-bend drop
                double drop = getBoneDrop(pos);
                newPos = newPos - up * drop;
                newEnd = newEnd - up * drop;
                if (std::abs(extraYaw) > 1e-6 || std::abs(extraPitch) > 1e-6 || std::abs(extraRoll) > 1e-6) {
                    Matrix4x4 extraRot;
                    if (std::abs(extraYaw) > 1e-6)
                        extraRot.rotate(up, extraYaw);
                    if (std::abs(extraPitch) > 1e-6)
                        extraRot.rotate(right, extraPitch);
                    if (std::abs(extraRoll) > 1e-6)
                        extraRot.rotate(forward, extraRoll);
                    Vector3 offset = newEnd - newPos;
                    newEnd = newPos + extraRot.transformVector(offset);
                }
                boneWorldTransforms[name] = buildBoneWorldTransform(newPos, newEnd);
            };

            // -------------------------------------------------------
            // Spine: traveling wave undulation
            // Phase-offset sine along spine chain creates organic S-curve
            // -------------------------------------------------------
            double spineWavePhase = tNormalized * 2.0 * Math::Pi * 1.0;
            double pelvisSway = 0.008 * spineSwayFactor * std::sin(spineWavePhase);
            double spineSway = 0.012 * spineSwayFactor * std::sin(spineWavePhase + 0.4);
            double chestSway = 0.008 * spineSwayFactor * std::sin(spineWavePhase + 0.8);

            // -------------------------------------------------------
            // Head: layered look with multi-frequency noise
            // Two yaw frequencies + pitch create lifelike scanning motion
            // -------------------------------------------------------
            double headYaw = headLookFactor * (0.04 * std::sin(tNormalized * 2.0 * Math::Pi * 1.0) + 0.015 * std::sin(tNormalized * 2.0 * Math::Pi * 3.0 + 1.2));
            double headPitch = 0.02 * headLookFactor * std::sin(tNormalized * 2.0 * Math::Pi * 2.0 + 0.5);

            computeBodyBone("Root");
            computeBodyBone("Pelvis", pelvisSway, 0.0, hipRoll);
            computeBodyBone("Spine", spineSway);
            computeBodyBone("Chest", chestSway, 0.0, -hipRoll * 0.5);
            computeBodyBone("Neck", -chestSway * 0.5, headPitch * 0.5);
            computeBodyBone("Head", headYaw, headPitch);

            // Jaw subtle movement
            if (boneIdx.count("Jaw")) {
                double jawAngle = 0.03 * jawFactor * std::max(0.0, std::sin(tNormalized * 2.0 * Math::Pi * 2.0));
                computeBodyBone("Jaw", 0.0, jawAngle);
            }

            // Tail idle sway with traveling wave
            static const char* tailBones[] = { "TailBase", "TailMid", "TailTip" };
            double tailPhase = tNormalized * 2.0 * Math::Pi * 1.0;
            for (int ti = 0; ti < 3; ++ti) {
                if (boneIdx.count(tailBones[ti]) == 0)
                    continue;
                double attenuation = 1.0 - ti * 0.25;
                double tailAngle = 0.06 * tailIdleFactor * attenuation * std::sin(tailPhase - ti * 0.7);
                computeBodyBone(tailBones[ti], tailAngle);
            }

            // -------------------------------------------------------
            // Legs: IK-grounded with feet pinned to rest positions
            // Body moves; feet stay planted; two-bone IK solves joints.
            // Feet stay planted while the body moves above.
            // -------------------------------------------------------
            for (size_t i = 0; i < 4; ++i) {
                // Leg hips use a separate transform WITHOUT breathing offset,
                // so legs stay still during breathing. Only knee-bend drop
                // and lateral shift affect leg positions.
                Matrix4x4 legTransform;
                legTransform.translate(up * (-baseDrop) + right * lateralShift);
                legTransform.rotate(forward, hipRoll);

                Vector3 hipPos = legTransform.transformPoint(legRest[i].upperLegPos);
                Vector3 upperLegEnd = legTransform.transformPoint(legRest[i].upperLegEnd);
                Vector3 footEnd = legTransform.transformPoint(legRest[i].footEnd);
                double legDrop = getBoneDrop(legRest[i].upperLegPos);
                hipPos = hipPos - up * legDrop;
                upperLegEnd = upperLegEnd - up * legDrop;
                footEnd = footEnd - up * legDrop;

                bool isFrontLeg = (i == 0 || i == 1);
                double legBendFactor = isFrontLeg ? frontKneeBendFactor : backKneeBendFactor;

                // Foot target: pinned at ground-level rest position
                Vector3 footTarget = footHome[i];

                // 3-joint chain for IK
                std::vector<Vector3> chain = { hipPos, upperLegEnd, footEnd };

                double bendFactor = legBendFactor;
                Vector3 poleVector = upperLegEnd + forward * (isFrontLeg ? -1.0 : 1.0);

                // Use minimal softness so the foot always reaches the ground target.
                // Knee bend is controlled by minBendRad below, not by IK softness.
                solveTwoBoneIk(chain, footTarget, poleVector, 0.02);

                // Snap end effector exactly onto foot target to guarantee grounding
                chain[2] = footTarget;

                // Preserve rest-pose lateral knee position
                double restLateral = Vector3::dotProduct(legRest[i].upperLegEnd, right);
                double solvedLateral = Vector3::dotProduct(chain[1], right);
                chain[1] = chain[1] + right * (restLateral - solvedLateral);

                // Re-enforce segment lengths after lateral correction
                double len0 = (legRest[i].upperLegEnd - legRest[i].upperLegPos).length();
                double len1 = (legRest[i].footEnd - legRest[i].upperLegEnd).length();
                Vector3 dir0 = chain[1] - chain[0];
                if (!dir0.isZero())
                    chain[1] = chain[0] + dir0.normalized() * len0;
                Vector3 dir1 = chain[2] - chain[1];
                if (!dir1.isZero())
                    chain[2] = chain[1] + dir1.normalized() * len1;

                // Enforce minimum knee bend angle (controlled by bendFactor)
                double minBendRad = 0.35 * bendFactor;
                Vector3 toMid = (chain[1] - chain[0]).normalized();
                Vector3 toEnd = (chain[2] - chain[1]).normalized();
                double cosKnee = std::max(-1.0, std::min(1.0, Vector3::dotProduct(toMid, toEnd)));
                double kneeAngle = std::acos(cosKnee);
                if (kneeAngle > Math::Pi - minBendRad) {
                    Vector3 toPole = poleVector - chain[1];
                    toPole = toPole - toMid * Vector3::dotProduct(toPole, toMid);
                    if (toPole.lengthSquared() > 1e-12) {
                        toPole.normalize();
                        double pushDist = len0 * std::sin(minBendRad);
                        chain[1] = chain[1] + toPole * pushDist;
                        dir0 = chain[1] - chain[0];
                        if (!dir0.isZero())
                            chain[1] = chain[0] + dir0.normalized() * len0;
                    }
                }

                // Always re-pin foot to ground after all knee adjustments.
                // Adjust knee position to satisfy segment length while keeping foot fixed.
                chain[2] = footTarget;
                dir1 = footTarget - chain[1];
                double dist1 = dir1.length();
                if (dist1 > 1e-12 && std::abs(dist1 - len1) > 1e-8) {
                    // Move knee along hip->knee direction so that knee-to-foot = len1
                    // while keeping knee on the circle of radius len0 from hip.
                    // We slide chain[1] toward/away from foot to fix the length.
                    Vector3 hipToFoot = footTarget - chain[0];
                    double hipFootDist = hipToFoot.length();
                    if (hipFootDist > 1e-12) {
                        // Re-solve IK with foot pinned as the definitive target
                        std::vector<Vector3> rechain = { chain[0], chain[1], chain[2] };
                        solveTwoBoneIk(rechain, footTarget, poleVector, 0.0);
                        chain[1] = rechain[1];
                    }
                }
                chain[2] = footTarget;

                // Reconstruct lower-leg position from IK result
                Vector3 newStickDir = (chain[2] - chain[1]);
                if (newStickDir.isZero())
                    newStickDir = legRest[i].restStickDir;
                else
                    newStickDir.normalize();
                Quaternion stickRot = Quaternion::rotationTo(legRest[i].restStickDir, newStickDir);
                Matrix4x4 stickRotMat;
                stickRotMat.rotate(stickRot);
                Vector3 lowerLegEnd = chain[1] + stickRotMat.transformVector(legRest[i].restUpperToLowerVec);

                boneWorldTransforms[legs[i].upperLegName] = buildBoneWorldTransform(chain[0], chain[1]);
                boneWorldTransforms[legs[i].lowerLegName] = buildBoneWorldTransform(chain[1], lowerLegEnd);
                boneWorldTransforms[legs[i].footName] = buildBoneWorldTransform(lowerLegEnd, chain[2]);
            }

            // Skin matrices
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

} // namespace quadruped

} // namespace dust3d
