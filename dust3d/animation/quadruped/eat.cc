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
// Simulates a four-legged animal lowering its head to eat grass from the
// ground.  The animation cycle has four phases modeled after real herbivore
// grazing behavior (horses, cows, deer, goats):
//
//   Phase 1 — Lower Head    (0.00 – 0.25):  The animal lowers its head and
//       neck toward the ground.  Front legs may splay slightly to help reach.
//       Spine pitches forward, weight shifts onto front legs.
//
//   Phase 2 — Graze/Bite    (0.25 – 0.55):  Head is at ground level.  Jaw
//       opens and closes rhythmically to simulate tearing grass.  Small head
//       nods and lateral micro-movements add realism.  Tail swishes idly.
//
//   Phase 3 — Chew / Pause  (0.55 – 0.75):  Head remains low or lifts very
//       slightly.  Jaw performs slower chewing motions.  Ears (head yaw)
//       twitch.  Body is relaxed and still.
//
//   Phase 4 — Raise Head    (0.75 – 1.00):  Head returns smoothly to rest
//       pose.  Spine straightens.  Front legs return to neutral stance.
//       Weight re-centers.
//
// Animation principles applied:
//   - Anticipation: slight weight-shift before head lowers
//   - Overlapping action: tail, jaw, ears move independently
//   - Ease-in / ease-out via smootherstep curves
//   - Secondary motion: tail idle sway, jaw chewing, subtle body shift
//   - Follow-through: head settles with slight overshoot on raise
//
// Tunable parameters:
//   - headLowerDepthFactor:   how far the head drops (tall vs short neck)
//   - headLowerSpeedFactor:   speed of lowering/raising the head
//   - neckCurveFactor:        how much the neck arcs during lowering
//   - jawChewFactor:          jaw opening amplitude during biting/chewing
//   - jawChewFrequency:       number of chew cycles during graze phase
//   - biteTearFactor:         head pull-back intensity when tearing grass
//   - headNodFactor:          subtle vertical head nod while grazing
//   - headSwayFactor:         lateral micro-movement while grazing
//   - frontLegSplayFactor:    front leg spread to help reach ground
//   - spineDropFactor:        how much the spine pitches forward
//   - tailSwayFactor:         idle tail sway amplitude
//   - bodyShiftFactor:        forward weight shift amount
//   - grazeDurationFactor:    proportion of time spent at ground level

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
        double headLowerSpeedFactor = parameters.getValue("headLowerSpeedFactor", 1.0);
        double neckCurveFactor = parameters.getValue("neckCurveFactor", 1.0);
        double jawChewFactor = parameters.getValue("jawChewFactor", 1.0);
        double jawChewFrequency = parameters.getValue("jawChewFrequency", 1.0);
        double biteTearFactor = parameters.getValue("biteTearFactor", 1.0);
        double headNodFactor = parameters.getValue("headNodFactor", 1.0);
        double headSwayFactor = parameters.getValue("headSwayFactor", 1.0);
        double frontLegSplayFactor = parameters.getValue("frontLegSplayFactor", 1.0);
        double spineDropFactor = parameters.getValue("spineDropFactor", 1.0);
        double tailSwayFactor = parameters.getValue("tailSwayFactor", 1.0);
        double bodyShiftFactor = parameters.getValue("bodyShiftFactor", 1.0);
        double grazeDurationFactor = std::max(0.1, std::min(2.0, parameters.getValue("grazeDurationFactor", 1.0)));

        // Phase control: allow user to generate only a specific portion of
        // the full eat cycle.  phaseStart/phaseEnd are 0.0–1.0 where the
        // full cycle phases are:
        //   0.00–0.25  Head lowering to ground
        //   0.25–0.55  Eating grass on the ground
        //   0.55–0.75  Head raising from ground to air
        //   0.75–1.00  Chewing in the air with subtle head movement
        double phaseStart = std::max(0.0, std::min(1.0, parameters.getValue("phaseStart", 0.0)));
        double phaseEnd = std::max(0.0, std::min(1.0, parameters.getValue("phaseEnd", 1.0)));
        if (phaseEnd <= phaseStart)
            phaseEnd = std::min(1.0, phaseStart + 0.01);

        // How much the head lifts when chewing in the air (phase 4)
        // 0 = stays at ground level, 1 = returns fully to rest height
        double chewHeadLift = std::max(0.0, std::min(1.0, parameters.getValue("chewHeadLift", 1.0)));

        // Chew sway/nod intensity while head is in the air
        double airChewSwayFactor = parameters.getValue("airChewSwayFactor", 1.0);

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

        // Front legs splay wider to help lower the body
        double frontSplay = spineLength * 0.15 * frontLegSplayFactor;

        double tailSwayAmp = 0.12 * tailSwayFactor;
        double jawOpenAmp = 0.35 * jawChewFactor;
        double headNodAmp = 0.06 * headNodFactor;
        double headSwayAmp = 0.04 * headSwayFactor;
        double biteTearAmp = spineLength * 0.03 * biteTearFactor;

        // Phase timing (adjusted by grazeDurationFactor)
        // The four phases must always sum to 1.0 for proper looping.
        // Base proportions: lower=20%, graze=35%, chew=15%, raise=30%
        double lowerProp = 0.20 / grazeDurationFactor;
        double grazeProp = 0.35 * grazeDurationFactor;
        double chewProp = 0.15 * grazeDurationFactor;
        double raiseProp = 0.30 / grazeDurationFactor;
        // Normalize so they sum to 1.0
        double totalProp = lowerProp + grazeProp + chewProp + raiseProp;
        double lowerEnd = lowerProp / totalProp;
        double grazeEnd = (lowerProp + grazeProp) / totalProp;
        double chewEnd = (lowerProp + grazeProp + chewProp) / totalProp;
        // raisePhase: chewEnd to 1.0

        // Number of jaw cycles during graze + chew
        int chewCycles = static_cast<int>(std::round(4.0 * jawChewFrequency));
        chewCycles = std::max(1, chewCycles);

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            // Map frame to the user-selected phase sub-range.
            // For loopable clips (phaseStart=0, phaseEnd=1), frame 0 and
            // the implicit frame after the last must produce the same pose.
            // Using frameCount as divisor ensures the last frame is one step
            // before wrapping back to frame 0.
            double frameT = static_cast<double>(frame) / static_cast<double>(frameCount);
            double tNormalized = phaseStart + frameT * (phaseEnd - phaseStart);

            // === Phase envelope computation ===
            double lowerEnvelope = 0.0; // 0→1 head going down
            double grazeEnvelope = 0.0; // 1 while at ground
            double chewEnvelope = 0.0; // 1 while chewing (head slightly up)
            double raiseEnvelope = 0.0; // 0→1 head coming back up

            double headDown = 0.0; // overall 0→1→1→0 envelope for head position
            double airChewPhase = 0.0; // 0→1 during phase 4 air chewing

            if (tNormalized < lowerEnd) {
                // Phase 1: Lower head
                double phase = tNormalized / lowerEnd;
                lowerEnvelope = smootherstep(phase);
                headDown = lowerEnvelope;
            } else if (tNormalized < grazeEnd) {
                // Phase 2: Graze/bite at ground level
                lowerEnvelope = 1.0;
                double phase = (tNormalized - lowerEnd) / (grazeEnd - lowerEnd);
                grazeEnvelope = phase;
                headDown = 1.0;
            } else if (tNormalized < chewEnd) {
                // Phase 3: Chew / pause (head lifts very slightly)
                lowerEnvelope = 1.0;
                grazeEnvelope = 1.0;
                double phase = (tNormalized - grazeEnd) / (chewEnd - grazeEnd);
                chewEnvelope = phase;
                // Head lifts ~10% during chewing
                headDown = 1.0 - 0.1 * smoothstep(phase);
            } else {
                // Phase 4: Head in air, chewing with subtle movement
                double phase = (tNormalized - chewEnd) / (1.0 - chewEnd);
                raiseEnvelope = smootherstep(phase);
                double targetHeadDown = 0.9 * (1.0 - chewHeadLift);
                headDown = 0.9 * (1.0 - raiseEnvelope) + targetHeadDown * raiseEnvelope;
                if (headDown < 0.01)
                    headDown = 0.0;

                // Subtle head movement while chewing in the air,
                // fading out toward the end for a clean loop
                double fadeOut = 1.0 - smoothstep(phase);
                airChewPhase = phase * fadeOut;
            }

            // === Body translation ===
            // Forward shift: body moves slightly forward when head is down
            double bodyFwd = bodyForwardShift * headDown;
            // Body drops as front legs bend (cow lowers its front end)
            double bodyDrop = -bodyDropMax * headDown;

            Matrix4x4 bodyTransform;
            bodyTransform.translate(forward * bodyFwd + upDir * bodyDrop);

            // Spine pitch: forward tilt when head is lowered
            double spinePitch = spinePitchMax * headDown;

            Matrix4x4 spineTransform = bodyTransform;
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

            // === Spine chain ===
            computeBone("Root", bodyTransform);
            computeBone("Pelvis", bodyTransform, 0.0, spinePitch * 0.2);
            computeBone("Spine", spineTransform, 0.0, spinePitch * 0.5);
            computeBone("Chest", spineTransform, 0.0, spinePitch * 0.8);

            // === Neck, Head, Jaw: proper forward-kinematics chain ===
            // Each bone is positioned from its parent's new end position,
            // with cumulative rotation, so the head actually reaches the
            // ground instead of floating at rest height.

            // Chest new end = neck attachment point
            Vector3 chestNewPos = spineTransform.transformPoint(chestPos);
            Vector3 chestNewEnd = spineTransform.transformPoint(boneEnd("Chest"));
            {
                // Extra pitch on chest
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
            double grazeTime = 0.0;
            if (tNormalized >= lowerEnd && tNormalized < chewEnd) {
                grazeTime = (tNormalized - lowerEnd) / (chewEnd - lowerEnd);
            }

            double headPitchAngle = headPitchMax * headDown;

            // Rhythmic head nod during grazing (simulates biting/tearing)
            if (grazeEnvelope > 0.0 && chewEnvelope < 1.0) {
                double bitePhase = grazeTime * chewCycles * 2.0 * Math::Pi;
                headPitchAngle += headNodAmp * std::sin(bitePhase) * (1.0 - chewEnvelope);
            }

            // Slower chewing nod during chew phase (head still at ground)
            if (chewEnvelope > 0.0 && raiseEnvelope < 0.01) {
                double chewPhase = chewEnvelope * chewCycles * 1.5 * Math::Pi;
                headPitchAngle += headNodAmp * 0.5 * std::sin(chewPhase);
            }

            // Phase 4: subtle head movement while chewing in the air
            if (airChewPhase > 0.0) {
                // Gentle nod (chewing rhythm)
                double airNodPhase = airChewPhase * chewCycles * 2.0 * Math::Pi;
                headPitchAngle += headNodAmp * 0.4 * airChewSwayFactor * std::sin(airNodPhase);
                // Occasional larger nod (swallowing)
                headPitchAngle += headNodAmp * 0.2 * airChewSwayFactor
                    * std::sin(airChewPhase * Math::Pi * 1.5);
            }

            // Lateral micro-sway during grazing (searching for grass)
            double headYaw = 0.0;
            if (headDown > 0.5) {
                double swayPhase = grazeTime * 3.0 * Math::Pi;
                headYaw = headSwayAmp * std::sin(swayPhase);
            }
            // Subtle lateral look-around while chewing in the air
            if (airChewPhase > 0.0) {
                double airSwayPhase = airChewPhase * 2.5 * Math::Pi;
                headYaw += headSwayAmp * 1.5 * airChewSwayFactor * std::sin(airSwayPhase);
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
                if (grazeEnvelope > 0.0 && chewEnvelope < 1.0) {
                    double bitePhase = grazeTime * chewCycles * 2.0 * Math::Pi;
                    jawAngle = jawOpenAmp * std::max(0.0, std::sin(bitePhase)) * (1.0 - chewEnvelope);
                }
                if (chewEnvelope > 0.0 && raiseEnvelope < 0.01) {
                    double chewPhase = chewEnvelope * chewCycles * 1.5 * Math::Pi;
                    jawAngle += jawOpenAmp * 0.4 * std::max(0.0, std::sin(chewPhase));
                }
                // Jaw chewing while head is in the air (phase 4)
                if (airChewPhase > 0.0) {
                    double airJawPhase = airChewPhase * chewCycles * 2.0 * Math::Pi;
                    jawAngle += jawOpenAmp * 0.3 * std::max(0.0, std::sin(airJawPhase));
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
                computeBone(tailBones[ti], bodyTransform, tailAngle, 0.0);
            }

            // === Front legs: splay slightly forward to help head reach ground ===
            {
                static const char* frontLegs[][3] = {
                    { "FrontLeftUpperLeg", "FrontLeftLowerLeg", "FrontLeftFoot" },
                    { "FrontRightUpperLeg", "FrontRightLowerLeg", "FrontRightFoot" }
                };

                for (int li = 0; li < 2; ++li) {
                    Vector3 hipPos = bodyTransform.transformPoint(bonePos(frontLegs[li][0]));
                    Vector3 footRestPos = boneEnd(frontLegs[li][2]);

                    // Front legs widen slightly but stay grounded.
                    // Feet remain at ground level; the hip drops and IK
                    // bends the knees to accommodate.
                    double splaySign = (li == 0) ? -1.0 : 1.0;
                    Vector3 footTarget = footRestPos
                        + forward * (frontSplay * headDown * 0.3)
                        + right * (splaySign * frontSplay * headDown * 0.4);

                    Vector3 upperEnd = boneEnd(frontLegs[li][0]);
                    Vector3 lowerEnd = boneEnd(frontLegs[li][1]);

                    std::vector<Vector3> chain = { hipPos,
                        bodyTransform.transformPoint(upperEnd),
                        bodyTransform.transformPoint(lowerEnd) };

                    Vector3 poleVector = chain[1] - forward * spineLength * 0.5;
                    solveTwoBoneIk(chain, footTarget, poleVector);

                    boneWorldTransforms[frontLegs[li][0]] = buildBoneWorldTransform(chain[0], chain[1]);
                    boneWorldTransforms[frontLegs[li][1]] = buildBoneWorldTransform(chain[1], chain[2]);
                    // Foot direction: always use rest-pose forward direction so
                    // the foot never flips between frames.
                    Vector3 footRestDir = (boneEnd(frontLegs[li][2]) - bonePos(frontLegs[li][2]));
                    double footLen = footRestDir.length();
                    if (footLen < 1e-6)
                        footLen = 0.01;
                    footRestDir = footRestDir * (1.0 / footLen);
                    boneWorldTransforms[frontLegs[li][2]] = buildBoneWorldTransform(chain[2], chain[2] + footRestDir * footLen);
                }
            }

            // === Back legs: mostly stationary, slight weight adjustment ===
            {
                static const char* backLegs[][3] = {
                    { "BackLeftUpperLeg", "BackLeftLowerLeg", "BackLeftFoot" },
                    { "BackRightUpperLeg", "BackRightLowerLeg", "BackRightFoot" }
                };

                for (int li = 0; li < 2; ++li) {
                    Vector3 hipPos = bodyTransform.transformPoint(bonePos(backLegs[li][0]));
                    Vector3 footRestPos = boneEnd(backLegs[li][2]);

                    // Back legs stay grounded at rest position
                    Vector3 footTarget = footRestPos;

                    Vector3 upperEnd = boneEnd(backLegs[li][0]);
                    Vector3 lowerEnd = boneEnd(backLegs[li][1]);

                    std::vector<Vector3> chain = { hipPos,
                        bodyTransform.transformPoint(upperEnd),
                        bodyTransform.transformPoint(lowerEnd) };

                    Vector3 poleVector = chain[1] + forward * spineLength * 0.5;
                    solveTwoBoneIk(chain, footTarget, poleVector);

                    boneWorldTransforms[backLegs[li][0]] = buildBoneWorldTransform(chain[0], chain[1]);
                    boneWorldTransforms[backLegs[li][1]] = buildBoneWorldTransform(chain[1], chain[2]);
                    Vector3 footRestDir = (boneEnd(backLegs[li][2]) - bonePos(backLegs[li][2]));
                    double footLen = footRestDir.length();
                    if (footLen < 1e-6)
                        footLen = 0.01;
                    footRestDir = footRestDir * (1.0 / footLen);
                    boneWorldTransforms[backLegs[li][2]] = buildBoneWorldTransform(chain[2], chain[2] + footRestDir * footLen);
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
