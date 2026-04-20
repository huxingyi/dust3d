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

// Procedural grazing / eating animation for quadruped rig.
//
// Simulates a four-legged animal eating grass with its head already lowered
// to the ground.  The base pose (frame 0) is the eating posture: spine
// tilted forward, neck swung down, head aimed at the ground, mouth near
// grass level.  Every frame maintains this lowered position.  The animation
// consists of continuous grazing motions (biting, tearing, chewing) while
// the head stays down.  The clip is seamlessly loopable.
//
// Grazing motions (repeated throughout the clip):
//   - Rhythmic jaw open/close (biting/tearing grass)
//   - Small head nods during bites
//   - Lateral head micro-sway (searching for grass)
//   - Idle tail sway
//   - Front legs splayed slightly to help reach ground
//
// Tunable parameters:
//   - headLowerDepthFactor:   how far the head drops (tall vs short neck)
//   - neckCurveFactor:        how much the neck arcs during lowering
//   - jawChewFactor:          jaw opening amplitude during biting/chewing
//   - jawChewFrequency:       number of chew cycles during graze phase
//   - biteTearFactor:         head pull-back intensity when tearing grass
//   - headNodFactor:          subtle vertical head nod while grazing
//   - headSwayFactor:         lateral micro-movement while grazing
//   - frontLegSpreadFactor:    front leg spread (negative=left in front, positive=right in front)
//   - spineDropFactor:        how much the spine pitches forward
//   - tailSwayFactor:         idle tail sway amplitude
//   - bodyShiftFactor:        forward weight shift amount

#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/common.h>
#include <dust3d/animation/quadruped/eat.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace quadruped {

    bool eat(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = static_cast<int>(parameters.getValue("frameCount", 40));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 2.0));

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
        double headLowerDepthFactor = parameters.getValue("headLowerDepthFactor", 1.0);
        double neckCurveFactor = parameters.getValue("neckCurveFactor", 1.0);
        double jawChewFactor = parameters.getValue("jawChewFactor", 1.0);
        double jawChewFrequency = parameters.getValue("jawChewFrequency", 1.0);
        double biteTearFactor = parameters.getValue("biteTearFactor", 1.0);
        double headNodFactor = parameters.getValue("headNodFactor", 1.0);
        double headSwayFactor = parameters.getValue("headSwayFactor", 1.0);
        double frontLegSpreadFactor = parameters.getValue("frontLegSpreadFactor", 0.0);
        double backLegSpreadFactor = parameters.getValue("backLegSpreadFactor", 0.0);
        double frontLegBendFactor = parameters.getValue("frontLegBendFactor", 0.0);
        double spineDropFactor = parameters.getValue("spineDropFactor", 1.0);
        double tailSwayFactor = parameters.getValue("tailSwayFactor", 1.0);
        double bodyShiftFactor = parameters.getValue("bodyShiftFactor", 1.0);

        // Derive coordinate frame
        Vector3 pelvisPos = bonePos("Pelvis");
        Vector3 chestPos = bonePos("Chest");
        Vector3 headPos = bonePos("Head");
        Vector3 headEndRest = boneEnd("Head");
        Vector3 neckPos = bonePos("Neck");

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

        // Measure how far the head needs to drop to reach ground level.
        // Use the lowest foot position as ground reference.
        double groundY = boneEnd("FrontLeftFoot").y();
        groundY = std::min(groundY, boneEnd("FrontRightFoot").y());
        groundY = std::min(groundY, boneEnd("BackLeftFoot").y());
        groundY = std::min(groundY, boneEnd("BackRightFoot").y());

        // Measure the bone chain lengths to compute required pitch angles.
        // A cow lowers its head by: (1) body drops at front legs, (2) spine
        // tilts forward, (3) neck swings down like a pendulum, (4) head
        // pitches to face the grass.
        Vector3 neckEnd = boneEnd("Neck");
        double neckLength = (neckEnd - neckPos).length();
        double headLength = (headEndRest - headPos).length();
        double chestToNeckLen = (neckPos - chestPos).length();

        // Target: head end (mouth) should reach near ground level.
        // headEndRest.y() is the rest height of the mouth above ground.
        double headRestHeight = headEndRest.y() - groundY;
        // We want the mouth to reach groundY + small offset (grass height)
        double grassHeight = spineLength * 0.02; // just above ground
        double targetDrop = (headRestHeight - grassHeight) * headLowerDepthFactor;
        targetDrop = std::max(0.0, targetDrop);

        // Distribute the required drop across body drop, spine pitch, neck
        // pitch, and head pitch.  Emphasis on vertical lowering (body drop
        // and steep neck angle) rather than forward spine tilt, to keep the
        // head dropping mostly straight down instead of stretching forward.
        //   - 25% from body dropping (front legs bending)
        //   - 15% from spine pitching forward (modest tilt)
        //   - 45% from neck swinging down (nearly vertical)
        //   - 15% from head pitching down
        double bodyDropContribution = targetDrop * 0.25 * spineDropFactor;
        double spineDropContribution = targetDrop * 0.15 * spineDropFactor;
        double neckDropContribution = targetDrop * 0.45 * neckCurveFactor;
        double headDropContribution = targetDrop * 0.15 * headLowerDepthFactor;

        // Convert vertical drop contributions to pitch angles using arc geometry.
        // For a bone of length L pitched by angle θ from horizontal, the
        // vertical component changes by L·sin(θ).  We solve θ = asin(drop/L),
        // clamping to safe range.
        auto safeAsin = [](double ratio) -> double {
            return std::asin(std::max(-1.0, std::min(1.0, ratio)));
        };

        // Spine pitch: the spine tilts the chest downward
        double spinePitchMax = (spineLength > 1e-6)
            ? safeAsin(spineDropContribution / spineLength)
            : 0.3;
        // Clamp to modest range — spine tilts only slightly to avoid
        // pushing the head too far forward
        spinePitchMax = std::min(spinePitchMax, 0.35);

        // Neck pitch: the neck swings down from chest like a pendulum
        double neckPitchMax = (neckLength > 1e-6)
            ? safeAsin(neckDropContribution / neckLength)
            : 0.5;
        // Cow neck can swing nearly vertical (~80°)
        neckPitchMax = std::min(neckPitchMax, 1.4);

        // Head pitch: tips forward to face the grass
        double headPitchMax = (headLength > 1e-6)
            ? safeAsin(headDropContribution / headLength)
            : 0.3;
        headPitchMax = std::min(headPitchMax, 0.8);

        // Body vertical drop from front leg bending
        double bodyDropMax = bodyDropContribution;

        // Minimal forward shift — the cow lowers its head mostly
        // straight down, not reaching far forward
        double bodyForwardShift = spineLength * 0.05 * bodyShiftFactor;

        // Front legs spread along Z-axis (signed: negative=left forward, positive=right forward)
        double frontSpreadAmount = spineLength * 0.15 * frontLegSpreadFactor;

        double tailSwayAmp = 0.12 * tailSwayFactor;
        double jawOpenAmp = 0.35 * jawChewFactor;
        double headNodAmp = 0.06 * headNodFactor;
        double headSwayAmp = 0.04 * headSwayFactor;
        double biteTearAmp = spineLength * 0.03 * biteTearFactor;

        // Number of jaw/bite cycles per animation loop
        int chewCycles = static_cast<int>(std::round(4.0 * jawChewFrequency));
        chewCycles = std::max(1, chewCycles);

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            double tNormalized = static_cast<double>(frame) / static_cast<double>(frameCount);

            // Head is always fully down — constant eating posture
            double headDown = 1.0;

            // Grazing time drives bite/chew oscillations
            double grazeTime = tNormalized;

            // === Body translation ===
            // Front leg bend only affects the front of the body.
            // Back leg spread only affects the back.
            // The Spine bone bridges them: its start is at the back (pelvis),
            // its end drops with the front (chest).

            // Front leg drop: bending reduces effective vertical reach
            double frontUpperLen = (boneEnd("FrontLeftUpperLeg") - bonePos("FrontLeftUpperLeg")).length();
            double frontLowerLen = (boneEnd("FrontLeftLowerLeg") - bonePos("FrontLeftLowerLeg")).length();
            double frontTotalLen = frontUpperLen + frontLowerLen;
            double frontDropAmount = frontTotalLen * 0.25 * frontLegBendFactor;

            // Back leg drop: spreading reduces vertical reach (Pythagorean)
            double backUpperLen = (boneEnd("BackLeftUpperLeg") - bonePos("BackLeftUpperLeg")).length();
            double backLowerLen = (boneEnd("BackLeftLowerLeg") - bonePos("BackLeftLowerLeg")).length();
            double backTotalLen = backUpperLen + backLowerLen;
            double backSpreadDist = spineLength * 0.15 * std::abs(backLegSpreadFactor);
            double backVerticalReach = std::sqrt(std::max(0.0, backTotalLen * backTotalLen - backSpreadDist * backSpreadDist));
            double backDropAmount = backTotalLen - backVerticalReach;

            double frontDrop = -frontDropAmount * headDown;
            double backDrop = -backDropAmount * headDown;
            double bodyFwd = bodyForwardShift * headDown;

            // Back body transform: only affected by back leg spread
            Matrix4x4 backBodyTransform;
            backBodyTransform.translate(forward * bodyFwd + upDir * backDrop);

            // Front body transform: affected by both back spread and front bend
            Matrix4x4 frontBodyTransform;
            frontBodyTransform.translate(forward * bodyFwd + upDir * (backDrop + frontDrop));

            // Spine pitch: constant forward tilt + extra pitch from front drop
            double spinePitch = spinePitchMax * headDown;
            // Additional pitch from the height difference between back and front
            double frontBackHeightDiff = -frontDrop; // positive = front lower
            double extraSpinePitch = (spineLength > 1e-6)
                ? std::asin(std::max(-1.0, std::min(1.0, frontBackHeightDiff / spineLength)))
                : 0.0;

            Matrix4x4 spineTransform = frontBodyTransform;
            spineTransform.rotate(right, spinePitch);

            std::map<std::string, Matrix4x4> boneWorldTransforms;

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

            // === Spine bones ===
            // Root/Pelvis stay at back height, unaffected by front leg bend.
            // Spine bridges back→front: start at back height, end tilts down toward front.
            // Chest is at front height.
            computeBone("Root", backBodyTransform);
            computeBone("Pelvis", backBodyTransform, 0.0, spinePitch * 0.2);

            // Spine: start at back height (pelvis), end pitches down toward front
            {
                Vector3 spineStart = backBodyTransform.transformPoint(bonePos("Spine"));
                Vector3 spineRestDir = boneEnd("Spine") - bonePos("Spine");
                double spineLen2 = spineRestDir.length();
                if (spineLen2 < 1e-6)
                    spineLen2 = 1.0;
                spineRestDir = spineRestDir * (1.0 / spineLen2);
                Matrix4x4 spineRotMat;
                spineRotMat.rotate(right, spinePitch * 0.5 + extraSpinePitch);
                Vector3 spineNewDir = spineRotMat.transformVector(spineRestDir);
                Vector3 spineEnd2 = spineStart + spineNewDir * spineLen2;
                boneWorldTransforms["Spine"] = buildBoneWorldTransform(spineStart, spineEnd2);
            }

            computeBone("Chest", spineTransform, 0.0, spinePitch * 0.8);

            // === FK chain: Chest end → Neck → Head ===
            // Compute Chest end position for chaining
            Vector3 chestNewPos = spineTransform.transformPoint(chestPos);
            Vector3 chestNewEnd = spineTransform.transformPoint(boneEnd("Chest"));
            {
                Matrix4x4 chestExtraRot;
                chestExtraRot.rotate(right, spinePitch * 0.8);
                Vector3 chestOffset = chestNewEnd - chestNewPos;
                chestNewEnd = chestNewPos + chestExtraRot.transformVector(chestOffset);
            }

            // Neck pitch: cumulative spine rotation + neck's own downward swing
            double neckPitch = neckPitchMax * headDown;
            // The neck bone direction in rest pose
            Vector3 neckRestDir = (boneEnd("Neck") - bonePos("Neck"));
            double neckLen = neckRestDir.length();
            if (neckLen < 1e-6)
                neckLen = 1.0;
            neckRestDir = neckRestDir * (1.0 / neckLen);

            // Build cumulative rotation: spine pitch + neck pitch
            double totalNeckPitch = spinePitch + neckPitch;
            Matrix4x4 neckRot;
            neckRot.rotate(right, totalNeckPitch);
            Vector3 neckNewDir = neckRot.transformVector(neckRestDir);
            Vector3 neckNewStart = chestNewEnd;
            Vector3 neckNewEnd = neckNewStart + neckNewDir * neckLen;

            boneWorldTransforms["Neck"] = buildBoneWorldTransform(neckNewStart, neckNewEnd);

            // === Head: pitches forward to face ground ===
            double headPitchAngle = headPitchMax * headDown;

            // Rhythmic head nod during grazing (simulates biting/tearing)
            {
                double bitePhase = grazeTime * chewCycles * 2.0 * Math::Pi;
                headPitchAngle += headNodAmp * std::sin(bitePhase);
            }

            // Lateral micro-sway during grazing (searching for grass)
            double headYaw = 0.0;
            {
                double swayPhase = grazeTime * 3.0 * Math::Pi;
                headYaw = headSwayAmp * std::sin(swayPhase);
            }

            // Head FK from neck end
            Vector3 headRestDir = (headEndRest - headPos);
            double headLen = headRestDir.length();
            if (headLen < 1e-6)
                headLen = 1.0;
            headRestDir = headRestDir * (1.0 / headLen);

            double totalHeadPitch = totalNeckPitch + headPitchAngle;
            Matrix4x4 headRot;
            headRot.rotate(right, totalHeadPitch);
            if (std::abs(headYaw) > 1e-6)
                headRot.rotate(upDir, headYaw);
            Vector3 headNewDir = headRot.transformVector(headRestDir);
            Vector3 headNewStart = neckNewEnd;
            Vector3 headNewEnd = headNewStart + headNewDir * headLen;

            boneWorldTransforms["Head"] = buildBoneWorldTransform(headNewStart, headNewEnd);

            // === Jaw: opens/closes rhythmically ===
            if (boneIdx.count("Jaw")) {
                double jawAngle = 0.0;
                // Continuous biting/chewing while head is down
                {
                    double bitePhase = grazeTime * chewCycles * 2.0 * Math::Pi;
                    jawAngle = jawOpenAmp * std::max(0.0, std::sin(bitePhase));
                }
                Vector3 jawRestDir = (boneEnd("Jaw") - bonePos("Jaw"));
                double jawLen = jawRestDir.length();
                if (jawLen < 1e-6)
                    jawLen = 0.1;
                jawRestDir = jawRestDir * (1.0 / jawLen);
                Matrix4x4 jawRot;
                jawRot.rotate(right, totalHeadPitch + jawAngle);
                if (std::abs(headYaw) > 1e-6)
                    jawRot.rotate(upDir, headYaw);
                Vector3 jawNewDir = jawRot.transformVector(jawRestDir);
                Vector3 jawNewStart = headNewStart; // jaw attaches at head start
                Vector3 jawNewEnd = jawNewStart + jawNewDir * jawLen;
                boneWorldTransforms["Jaw"] = buildBoneWorldTransform(jawNewStart, jawNewEnd);
            }

            // === Tail: idle sway (relaxed animal) ===
            static const char* tailBones[] = { "TailBase", "TailMid", "TailTip" };
            double tailPhase = tNormalized * 4.0 * Math::Pi; // gentle sway
            for (int ti = 0; ti < 3; ++ti) {
                if (boneIdx.count(tailBones[ti]) == 0)
                    continue;
                double delay = ti * 0.6;
                double tailAngle = tailSwayAmp * (1.0 + ti * 0.3)
                    * std::sin(tailPhase - delay);
                computeBone(tailBones[ti], backBodyTransform, tailAngle, 0.0);
            }

            // === Front legs: stretched with optional bend (">"-shaped backward bend) ===
            // FrontLeft stretches forward, FrontRight stretches backward.
            // frontLegBendFactor: 0 = rest pose (straight), 1 = full ">" bend backward.
            {
                static const char* frontLegs[][3] = {
                    { "FrontLeftUpperLeg", "FrontLeftLowerLeg", "FrontLeftFoot" },
                    { "FrontRightUpperLeg", "FrontRightLowerLeg", "FrontRightFoot" }
                };
                double spreadDirs[] = { -1.0, 1.0 }; // left opposite, right same as factor sign

                for (int li = 0; li < 2; ++li) {
                    Vector3 hipPos = frontBodyTransform.transformPoint(bonePos(frontLegs[li][0]));
                    Vector3 footRestEnd = boneEnd(frontLegs[li][2]);

                    double stretchAmount = frontSpreadAmount * headDown * spreadDirs[li];
                    Vector3 footTarget = footRestEnd + forward * stretchAmount;
                    footTarget.setY(groundY);

                    double upperLen = (boneEnd(frontLegs[li][0]) - bonePos(frontLegs[li][0])).length();
                    double lowerLen = (boneEnd(frontLegs[li][1]) - bonePos(frontLegs[li][1])).length();

                    if (frontLegBendFactor > 1e-4) {
                        // Use IK with pole vector pushed backward to create ">" bend
                        Vector3 kneeRestPos = bonePos(frontLegs[li][1]);
                        Vector3 ankleRestPos = boneEnd(frontLegs[li][1]);
                        std::vector<Vector3> chain = { hipPos,
                            frontBodyTransform.transformPoint(kneeRestPos),
                            frontBodyTransform.transformPoint(ankleRestPos) };
                        // Pole vector behind the knee — the further back, the sharper the bend
                        Vector3 kneeHint = (chain[0] + footTarget) * 0.5;
                        kneeHint = kneeHint - forward * (upperLen * 0.8 * frontLegBendFactor);
                        solveTwoBoneIk(chain, footTarget, kneeHint);
                        boneWorldTransforms[frontLegs[li][0]] = buildBoneWorldTransform(chain[0], chain[1]);
                        boneWorldTransforms[frontLegs[li][1]] = buildBoneWorldTransform(chain[1], chain[2]);

                        Vector3 footRestDir = (boneEnd(frontLegs[li][2]) - bonePos(frontLegs[li][2]));
                        double footLen = footRestDir.length();
                        if (footLen < 1e-6)
                            footLen = 0.01;
                        footRestDir = footRestDir * (1.0 / footLen);
                        Vector3 footEnd = chain[2] + footRestDir * footLen;
                        footEnd.setY(groundY);
                        boneWorldTransforms[frontLegs[li][2]] = buildBoneWorldTransform(chain[2], footEnd);
                    } else {
                        // Straight leg (rest pose shape)
                        double totalLegLen = upperLen + lowerLen;
                        Vector3 legDir = footTarget - hipPos;
                        double legDist = legDir.length();
                        if (legDist < 1e-6)
                            legDist = 1.0;
                        legDir = legDir * (1.0 / legDist);
                        double kneeT = upperLen / totalLegLen;
                        Vector3 kneePos = hipPos + legDir * (legDist * kneeT);

                        boneWorldTransforms[frontLegs[li][0]] = buildBoneWorldTransform(hipPos, kneePos);
                        boneWorldTransforms[frontLegs[li][1]] = buildBoneWorldTransform(kneePos, footTarget);

                        Vector3 footRestDir = (boneEnd(frontLegs[li][2]) - bonePos(frontLegs[li][2]));
                        double footLen = footRestDir.length();
                        if (footLen < 1e-6)
                            footLen = 0.01;
                        footRestDir = footRestDir * (1.0 / footLen);
                        Vector3 footEnd = footTarget + footRestDir * footLen;
                        footEnd.setY(groundY);
                        boneWorldTransforms[frontLegs[li][2]] = buildBoneWorldTransform(footTarget, footEnd);
                    }
                }
            }

            // === Back legs: IK to ground with lateral spread on Z-axis ===
            {
                static const char* backLegs[][3] = {
                    { "BackLeftUpperLeg", "BackLeftLowerLeg", "BackLeftFoot" },
                    { "BackRightUpperLeg", "BackRightLowerLeg", "BackRightFoot" }
                };
                double spreadDirs[] = { -1.0, 1.0 }; // left opposite, right same as factor sign
                double backSpreadAmount = spineLength * 0.15 * backLegSpreadFactor;

                for (int li = 0; li < 2; ++li) {
                    Vector3 hipPos = backBodyTransform.transformPoint(bonePos(backLegs[li][0]));
                    Vector3 footRestEnd = boneEnd(backLegs[li][2]);

                    // Spread foot target laterally on Z-axis, keep grounded
                    Vector3 spreadOffset = Vector3(0.0, 0.0, spreadDirs[li] * backSpreadAmount);
                    Vector3 footTarget = footRestEnd + spreadOffset;
                    footTarget.setY(groundY);

                    // IK solve — use bonePos of lower leg as knee to ensure
                    // upper end == lower start (connected chain)
                    Vector3 kneeRestPos = bonePos(backLegs[li][1]);
                    Vector3 ankleRestPos = boneEnd(backLegs[li][1]);
                    std::vector<Vector3> chain = { hipPos,
                        backBodyTransform.transformPoint(kneeRestPos),
                        backBodyTransform.transformPoint(ankleRestPos) };

                    // Pole vector behind (backward) for natural back-leg bend
                    Vector3 poleVector = chain[1] + forward * spineLength * 0.5;
                    solveTwoBoneIk(chain, footTarget, poleVector);

                    boneWorldTransforms[backLegs[li][0]] = buildBoneWorldTransform(chain[0], chain[1]);
                    boneWorldTransforms[backLegs[li][1]] = buildBoneWorldTransform(chain[1], chain[2]);

                    // Foot: flat on ground
                    Vector3 footRestDir = (boneEnd(backLegs[li][2]) - bonePos(backLegs[li][2]));
                    double footLen = footRestDir.length();
                    if (footLen < 1e-6)
                        footLen = 0.01;
                    footRestDir = footRestDir * (1.0 / footLen);
                    Vector3 footEnd = chain[2] + footRestDir * footLen;
                    footEnd.setY(groundY);
                    boneWorldTransforms[backLegs[li][2]] = buildBoneWorldTransform(chain[2], footEnd);
                }
            }

            // === Build skin matrices ===
            auto& frameData = animationClip.frames[frame];
            frameData.time = static_cast<float>(frame) / static_cast<float>(frameCount) * durationSeconds;
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
