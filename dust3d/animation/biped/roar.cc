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

// Procedural roar animation for biped rig.
//
// Key techniques applied here:
//
// 1. BREATH-DRIVEN CORE: The roar is built around the respiratory cycle.
//    Inhale (chest expands, ribcage lifts) → explosive exhale (diaphragm
//    contracts, chest drives forward, abdomen tightens). This drives the
//    entire upper body and gives the roar its organic "coming from inside"
//    feeling rather than a mechanical pose transition.
//
// 2. ASYMMETRIC EASE CURVES: Real muscle activation has fast attack and
//    slow decay. The burst uses a sharp power curve (fast-in, slow-out)
//    rather than symmetric Gaussians, creating the explosive snap that
//    separates a convincing roar from a gentle stretch.
//
// 3. MUSCLE TENSION WAVE: Tension propagates from core → spine → shoulders
//    → arms → hands with frame delays, creating a visible wave of energy
//    traveling outward. Each body part activates slightly after its parent.
//
// 4. JAW SIMULATION: Head pitch combines with a simulated jaw-open by
//    splitting the head throw into two components: skull pitches back
//    while the chin drives down-and-forward, stretching the head bone.
//
// 5. GROUND REACTION FORCE: The stomp creates a shockwave — foot slams
//    down, body compresses on impact, then bounces slightly upward. This
//    is the opposite of the naive "lift-and-place" — the energy comes
//    FROM the ground impact.
//
// 6. SECONDARY DYNAMICS WITH OVERSHOOT: Arms, hands, and tail overshoot
//    their target pose and settle back (damped spring behavior), giving
//    that "weight and follow-through" quality.
//
// 7. COHERENT NOISE TREMBLE: Rather than plain sine waves, the roar
//    sustain tremble uses layered incommensurate frequencies with
//    per-bone phase offsets, creating organic full-body vibration that
//    looks like muscular strain rather than mechanical oscillation.
//
// Animation phases (loopable):
//   Phase 1 (0.00–0.20): Inhale & anticipation — chest rises, body
//       gathers inward, slight crouch, chin tucks toward chest
//   Phase 2 (0.20–0.40): Explosive burst — diaphragm drives chest
//       forward, head snaps back, arms fling out, body jolts, stomp
//   Phase 3 (0.40–0.70): Sustain roar — head pushes FORWARD (projecting
//       at target), full-body tremble, heavy breathing pulses, arms
//       held tense with micro-oscillations
//   Phase 4 (0.70–1.00): Exhale & recovery — tension drains outward
//       (core relaxes first, extremities last), settle to neutral

#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/biped/roar.h>
#include <dust3d/animation/common.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace biped {

    bool roar(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = static_cast<int>(parameters.getValue("frameCount", 120));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 3.0));

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

        // ===================================================================
        // Parameters
        // ===================================================================
        double roarIntensity = parameters.getValue("roarIntensity", 1.0);
        double chestPuffFactor = parameters.getValue("chestPuffFactor", 1.0);
        double headThrowFactor = parameters.getValue("headThrowFactor", 1.0);
        double armSpreadFactor = parameters.getValue("armSpreadFactor", 1.0);
        double crouchDepthFactor = parameters.getValue("crouchDepthFactor", 1.0);
        double trembleFactor = parameters.getValue("trembleFactor", 1.0);
        double bodyMassFactor = parameters.getValue("bodyMassFactor", 1.0);
        double stompFactor = parameters.getValue("stompFactor", 1.0);
        double spineArchFactor = parameters.getValue("spineArchFactor", 1.0);
        double tailLashFactor = parameters.getValue("tailLashFactor", 1.0);
        double backChargeFactor = parameters.getValue("backChargeFactor", 1.0);
        double forwardThrustFactor = parameters.getValue("forwardThrustFactor", 1.0);

        // ===================================================================
        // Body frame: forward, right, up, scale
        // ===================================================================
        Vector3 hipsPos = bonePos("Hips");
        Vector3 upDir(0.0, 1.0, 0.0);

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

        double leftLegLen = (boneEnd("LeftUpperLeg") - bonePos("LeftUpperLeg")).length()
            + (boneEnd("LeftLowerLeg") - bonePos("LeftLowerLeg")).length()
            + (boneEnd("LeftFoot") - bonePos("LeftFoot")).length();
        double rightLegLen = (boneEnd("RightUpperLeg") - bonePos("RightUpperLeg")).length()
            + (boneEnd("RightLowerLeg") - bonePos("RightLowerLeg")).length()
            + (boneEnd("RightFoot") - bonePos("RightFoot")).length();
        double legLen = (leftLegLen + rightLegLen) * 0.5;
        if (legLen < 1e-6)
            legLen = 1.0;

        // Spine length for proportional spine/chest motion
        double spineLen = (boneEnd("Spine") - bonePos("Spine")).length()
            + (boneEnd("Chest") - bonePos("Chest")).length()
            + (boneEnd("Neck") - bonePos("Neck")).length();
        if (spineLen < 1e-6)
            spineLen = legLen * 0.6;

        // Mass factor: affects timing sharpness (heavier = more inertia)
        double massInertia = 0.5 + 0.5 * bodyMassFactor; // 0.75 .. 1.75

        // ===================================================================
        // Phase curve helpers
        // ===================================================================

        // Asymmetric attack/decay envelope: fast rise, slow fall.
        // attack: 0→1 speed, decay: 1→0 speed (higher = faster)
        auto asymEnvelope = [](double t, double onsetTime, double peakTime,
                                double decayEnd) -> double {
            if (t < onsetTime)
                return 0.0;
            if (t < peakTime) {
                double p = (t - onsetTime) / (peakTime - onsetTime);
                return p * p * (3.0 - 2.0 * p); // smoothstep rise
            }
            if (t < decayEnd) {
                double p = (t - peakTime) / (decayEnd - peakTime);
                double inv = 1.0 - p;
                return inv * inv * (3.0 - 2.0 * inv); // smoothstep fall (reversed)
            }
            return 0.0;
        };

        // Sharp power attack (fast-in) + exponential decay (slow-out)
        // Mimics explosive muscle contraction: near-instant peak, lingering release
        auto explosiveEnvelope = [](double t, double onsetTime, double peakTime,
                                     double decayRate) -> double {
            if (t < onsetTime)
                return 0.0;
            if (t < peakTime) {
                double p = (t - onsetTime) / (peakTime - onsetTime);
                return p * p * p; // cubic attack
            }
            // Exponential decay after peak
            double elapsed = t - peakTime;
            return std::exp(-elapsed * decayRate);
        };

        // Coherent noise-like tremble: layered incommensurate frequencies
        // with a phase seed so each bone gets unique vibration
        auto tremble = [](double tRad, double seed, double intensity) -> double {
            return intensity * (0.4 * std::sin(tRad * 11.0 + seed * 3.7) + 0.25 * std::sin(tRad * 17.0 + seed * 5.3) + 0.2 * std::sin(tRad * 23.0 + seed * 7.1) + 0.15 * std::sin(tRad * 31.0 + seed * 11.3));
        };

        // ===================================================================
        // Generate frames
        // ===================================================================
        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            double t = static_cast<double>(frame) / static_cast<double>(frameCount);
            double tRad = t * 2.0 * Math::Pi;

            // =============================================================
            // PHASE ENVELOPES — asymmetric, explosive
            // =============================================================

            // Inhale/anticipation: smooth rise and fall [0.0, 0.20]
            double inhale = asymEnvelope(t, 0.0, 0.10, 0.22);

            // Explosive burst: cubic attack at t=0.20, peak at t=0.28
            // Fast exponential decay (but still present through sustain)
            double burstRaw = explosiveEnvelope(t, 0.18, 0.28, 3.0 / massInertia);
            double burst = burstRaw * roarIntensity;

            // Sustain envelope: the held roar [0.35, 0.70]
            double sustain = asymEnvelope(t, 0.32, 0.45, 0.72);

            // Recovery: gradual return [0.68, 1.0]
            double recovery = asymEnvelope(t, 0.68, 0.84, 1.0);

            // Roar active = burst + sustain blended
            double roarEnv = std::min(1.0, burst + sustain * 0.85 * roarIntensity);

            // Tremble envelope: ramps up during sustain, decays in recovery
            double trembleEnv = sustain * trembleFactor * roarIntensity;

            // Stomp impact: sharp spike at burst peak
            double stompEnv = explosiveEnvelope(t, 0.22, 0.30, 8.0) * stompFactor;

            // =============================================================
            // BREATH-DRIVEN VERTICAL MOTION
            // =============================================================
            // Inhale/back-charge: body rises AND shifts weight backward
            // (chest lifts to fill lungs, hips pull back to coil)
            double inhaleRise = inhale * legLen * 0.04 * roarIntensity * backChargeFactor;

            // Burst: explosive downward jolt from the back-charge release
            // (body snaps forward and DOWN — diaphragm contracts, stomp
            // drives body into ground), then ground reaction pushes up
            double burstDrop = burst * legLen * 0.06 * crouchDepthFactor * forwardThrustFactor;
            double groundReaction = stompEnv * legLen * 0.02;

            // Sustain: body is slightly crouched (braced), with breathing pulses
            double sustainCrouch = sustain * legLen * 0.02 * crouchDepthFactor;
            double breathPulse = roarEnv * legLen * 0.008
                * (std::sin(tRad * 6.0) + 0.4 * std::sin(tRad * 10.0));

            // Recovery: return to zero
            double verticalOffset = inhaleRise - burstDrop + groundReaction
                - sustainCrouch + breathPulse;

            // Stomp ground-shake: brief high-freq vibration on impact
            double groundShake = stompEnv * legLen * 0.006
                * std::sin(tRad * 20.0);
            verticalOffset += groundShake;

            // Full-body tremble during sustain
            verticalOffset += tremble(tRad, 0.0, trembleEnv * legLen * 0.004);

            // =============================================================
            // FORWARD/BACKWARD LEAN — DRAMATIC BACK-CHARGE → FORWARD ROAR
            // =============================================================
            // Inhale/Anticipation: body charges BACKWARD dramatically.
            // The creature pulls its entire torso back, coiling like a
            // spring — chest lifts, hips shift back, weight transfers
            // to heels. This is the "gathering" that makes the forward
            // burst feel powerful.
            double leanBack = inhale * 0.14 * backChargeFactor * roarIntensity;

            // Burst: EXPLOSIVE forward thrust — the body snaps forward
            // from the back-charged position. The contrast between the
            // deep backward lean and the violent forward snap creates
            // the dramatic "whiplash" effect of a real roar.
            double leanForward = burst * 0.18 * forwardThrustFactor * chestPuffFactor;

            // Sustain: aggressive forward lean (projecting roar at target),
            // body stays committed forward with slight pulsing
            double sustainLean = sustain * 0.08 * forwardThrustFactor * roarIntensity;

            double forwardOffset = (-leanBack + leanForward + sustainLean) * legLen;

            // =============================================================
            // LATERAL WEIGHT SHIFT — asymmetric (favoring stomp side)
            // =============================================================
            double lateralShift = stompEnv * legLen * 0.015 * stompFactor
                + tremble(tRad, 1.0, trembleEnv * legLen * 0.003);

            // =============================================================
            // BUILD BODY TRANSFORM
            // =============================================================
            Matrix4x4 bodyTransform;
            bodyTransform.translate(
                upDir * verticalOffset + forward * forwardOffset + right * lateralShift);

            // =============================================================
            // SPINE: BREATH-DRIVEN ARCH + TENSION WAVE
            // Each vertebra activates with a slight delay (wave propagation)
            // =============================================================

            // Inhale/back-charge: spine arches BACKWARD dramatically
            // (chest lifts and opens, shoulders pull back, the whole
            // upper body coils away from the target)
            double spineInhaleArch = inhale * 0.14 * spineArchFactor * backChargeFactor;
            // Burst: spine REVERSES — explosive forward arch
            // (chest drives forward and down, belly tightens)
            double spineBurstArch = burst * 0.22 * spineArchFactor * forwardThrustFactor;
            // Sustain: held arch with forward component (projecting)
            double spineSustainArch = sustain * 0.10 * spineArchFactor * roarIntensity;

            // Chest puff: ribcage expansion (lateral + forward)
            double chestExpand = (burst * 0.12 + sustain * 0.08 + inhale * 0.04)
                * chestPuffFactor * roarIntensity;

            // Tension wave delay: spine activates first, chest slightly after
            double spineDelay = 0.0; // spine leads
            double chestDelay = 0.02; // chest follows
            double neckDelay = 0.04; // neck follows chest

            // Apply delay by shifting t for each bone
            auto delayedBurst = [&](double delay) -> double {
                return explosiveEnvelope(t - delay, 0.18, 0.28, 3.0 / massInertia)
                    * roarIntensity;
            };

            double spinePitch = -(spineInhaleArch + spineBurstArch + spineSustainArch) * 0.45;
            double chestPitch = -(spineInhaleArch + delayedBurst(chestDelay) * 0.16 * spineArchFactor
                                    + spineSustainArch)
                    * 0.55
                - chestExpand;
            double neckPitch = -(delayedBurst(neckDelay) * 0.08 * spineArchFactor);

            // Spine tremble (each vertebra with unique phase)
            double spineTremble = tremble(tRad, 2.0, trembleEnv * 0.015);
            double chestTremble = tremble(tRad, 3.0, trembleEnv * 0.012);

            // =============================================================
            // HEAD: DRAMATIC JAW-OPEN TRAJECTORY
            // =============================================================
            // The head follows a complex trajectory:
            // 1. Inhale: chin tucks toward chest (loading energy)
            // 2. Burst: head snaps UP and BACK (jaw opens — skull pitches
            //    backward while chin drives down, stretching the head bone)
            // 3. Sustain: head pushes FORWARD toward target (projecting the
            //    roar) with violent shaking
            // 4. Recovery: head drops forward, chin rises, returns to neutral

            // Back-charge: head tilts UP and BACK (chin lifts, looking
            // skyward as the body charges backward — gathering energy)
            double headTiltBack = inhale * 0.30 * headThrowFactor * backChargeFactor * roarIntensity;
            // Burst: head snaps FORWARD and DOWN toward the target
            // (the opposite of back-charge — projecting the roar)
            double headSnapForward = burst * 0.40 * headThrowFactor * forwardThrustFactor;
            // Sustain: head pushes forward and down (aggressive projection)
            double headForwardPush = sustain * 0.12 * headThrowFactor * roarIntensity;

            double headPitch = headTiltBack - headSnapForward - headForwardPush;

            // Jaw simulation: stretch the head bone endpoint further back
            // during the roar to simulate the jaw opening
            double jawOpen = roarEnv * 0.15 * headThrowFactor * roarIntensity;

            // Head shake: violent during burst, rhythmic during sustain
            // Uses multiple frequencies for incoherent organic shake
            double headShakeIntensity = burst * 0.08 + sustain * 0.05;
            double headShakeYaw = headShakeIntensity * trembleFactor
                * (0.6 * std::sin(tRad * 11.0 + 0.3)
                    + 0.3 * std::sin(tRad * 19.0 + 1.7)
                    + 0.1 * std::sin(tRad * 29.0 + 2.3));
            double headShakeRoll = headShakeIntensity * trembleFactor * 0.4
                * (0.7 * std::sin(tRad * 13.0 + 0.8)
                    + 0.3 * std::sin(tRad * 23.0 + 3.1));

            // =============================================================
            // HIP MOTION
            // =============================================================
            double hipYaw = stompEnv * 0.03 + tremble(tRad, 4.0, trembleEnv * 0.01);
            double hipRoll = lateralShift / (legLen * 0.02 + 1e-6) * 0.012;

            // =============================================================
            // COMPUTE BODY BONES
            // =============================================================
            std::map<std::string, Matrix4x4> boneWorldTransforms;

            auto computeBodyBone = [&](const std::string& name,
                                       double extraYaw = 0.0,
                                       double extraPitch = 0.0,
                                       double extraRoll = 0.0) {
                Vector3 pos = bonePos(name);
                Vector3 end = boneEnd(name);
                Vector3 newPos = bodyTransform.transformPoint(pos);
                Vector3 newEnd = bodyTransform.transformPoint(end);
                if (std::abs(extraYaw) > 1e-6 || std::abs(extraPitch) > 1e-6
                    || std::abs(extraRoll) > 1e-6) {
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
            };

            computeBodyBone("Root");
            computeBodyBone("Hips", hipYaw, 0.0, hipRoll);
            computeBodyBone("Spine", spineTremble, spinePitch, 0.0);
            computeBodyBone("Chest", chestTremble, chestPitch, 0.0);
            computeBodyBone("Neck", headShakeYaw * 0.3, neckPitch + headPitch * 0.3,
                headShakeRoll * 0.3);

            // Head: apply jaw-open by extending the head bone endpoint
            {
                Vector3 pos = bonePos("Head");
                Vector3 end = boneEnd("Head");
                Vector3 newPos = bodyTransform.transformPoint(pos);
                Vector3 newEnd = bodyTransform.transformPoint(end);

                // Head pitch rotation
                Matrix4x4 headRot;
                if (std::abs(headShakeRoll) > 1e-6)
                    headRot.rotate(forward, headShakeRoll);
                if (std::abs(headShakeYaw) > 1e-6)
                    headRot.rotate(upDir, headShakeYaw);
                if (std::abs(headPitch) > 1e-6)
                    headRot.rotate(right, headPitch);
                Vector3 offset = newEnd - newPos;
                newEnd = newPos + headRot.transformVector(offset);

                // Jaw open: push end point backward (skull tilts back) and
                // down (chin drops). This stretches the head bone visually
                // to simulate mouth opening.
                double headBoneLen = (end - pos).length();
                Vector3 jawDelta = (upDir * (-0.3) + forward * (-0.7)).normalized()
                    * (jawOpen * headBoneLen);
                newEnd = newEnd + jawDelta;

                boneWorldTransforms["Head"] = buildBoneWorldTransform(newPos, newEnd);
            }

            // =============================================================
            // TAIL: WHIP WITH WAVE PROPAGATION + OVERSHOOT
            // =============================================================
            static const char* tailBones[] = { "TailBase", "TailMid", "TailTip" };
            for (int ti = 0; ti < 3; ++ti) {
                if (boneIdx.count(tailBones[ti]) == 0)
                    continue;
                double cascade = 1.0 + ti * 0.6;
                double tailDelay = ti * 0.03; // wave propagation delay
                double delayedRoar = explosiveEnvelope(
                    t - tailDelay, 0.18, 0.28, 2.5 / massInertia);

                // Violent lashing yaw — overshoot at burst, sustained oscillation
                double tailYaw = tailLashFactor * (delayedRoar * roarIntensity * 0.12 * cascade * std::sin(tRad * 5.0 - ti * 1.2) + sustain * 0.06 * cascade * (0.6 * std::sin(tRad * 4.0 - ti * 0.9) + 0.4 * std::sin(tRad * 7.0 - ti * 1.5)));
                // Tail lifts during roar (aggressive posture)
                double tailLift = roarEnv * tailLashFactor * 0.06 * (1.0 + ti * 0.3);
                // Tail tremble
                tailYaw += tremble(tRad, 5.0 + ti, trembleEnv * 0.02 * cascade);

                computeBodyBone(tailBones[ti], tailYaw, tailLift, 0.0);
            }

            // =============================================================
            // LEGS: GROUND-BRACED WITH STOMP DYNAMICS
            // =============================================================
            // During the roar, the creature braces into the ground (knees
            // slightly more bent, feet gripping). One foot stomps at burst.
            //
            // Stomp sequence: foot lifts BRIEFLY during anticipation,
            // SLAMS down at burst peak, body compresses on impact.

            // Left foot stomps at burst, right stays planted
            double leftStompLift = 0.0;
            double rightStompLift = 0.0;
            {
                // Stomp preparation: foot lifts during late anticipation
                double stompPrep = asymEnvelope(t, 0.14, 0.20, 0.24);
                // Stomp slam: foot returns to ground with overshoot (pushes into ground)
                double stompSlam = explosiveEnvelope(t, 0.22, 0.26, 10.0);

                leftStompLift = stompFactor * legLen
                    * (stompPrep * 0.06 - stompSlam * 0.01);
                // Right foot slightly braces (pushes down)
                rightStompLift = -stompEnv * legLen * 0.005 * stompFactor;

                // Clamp: feet don't go below ground
                if (leftStompLift < -legLen * 0.02)
                    leftStompLift = -legLen * 0.02;
                if (rightStompLift < -legLen * 0.02)
                    rightStompLift = -legLen * 0.02;
            }

            auto computeLeg = [&](const char* upperLeg, const char* lowerLeg,
                                  const char* foot, double footLift) {
                Vector3 footStart = bonePos(foot) + upDir * footLift;
                Vector3 footEnd = boneEnd(foot) + upDir * footLift;
                boneWorldTransforms[foot] = buildBoneWorldTransform(footStart, footEnd);

                Vector3 hipJoint = bodyTransform.transformPoint(bonePos(upperLeg));
                double upperLen = (boneEnd(upperLeg) - bonePos(upperLeg)).length();
                double lowerLen = (boneEnd(lowerLeg) - bonePos(lowerLeg)).length();

                // Use the two-bone IK solver with forward-biased pole vector
                // The pole vector pushes knees forward (natural bent stance)
                Vector3 midBind = bodyTransform.transformPoint(bonePos(lowerLeg));
                Vector3 poleTarget = midBind + forward * (upperLen * 0.5);

                std::vector<Vector3> joints = { hipJoint, midBind, footStart };
                solveTwoBoneIk(joints, footStart, poleTarget, 0.05);

                boneWorldTransforms[upperLeg] = buildBoneWorldTransform(joints[0], joints[1]);
                boneWorldTransforms[lowerLeg] = buildBoneWorldTransform(joints[1], joints[2]);
            };

            computeLeg("LeftUpperLeg", "LeftLowerLeg", "LeftFoot", leftStompLift);
            computeLeg("RightUpperLeg", "RightLowerLeg", "RightFoot", rightStompLift);

            // =============================================================
            // ARMS: TENSION WAVE + OVERSHOOT + CLENCH
            // =============================================================
            // The arms follow a dramatic arc driven by the muscle tension wave:
            // 1. Inhale: arms pull inward and DOWN (fists near hips, coiling)
            // 2. Burst: arms EXPLODE outward-and-back (flinging open, claws spread)
            //    with overshoot past the target pose
            // 3. Sustain: arms held wide and tense, shaking with effort,
            //    hands clenched into fists or claws extended
            // 4. Recovery: arms drop with gravity (heavy, not snapping back)
            //
            // Each joint activates with delay: shoulder → upper arm → forearm → hand

            auto computeArm = [&](const char* shoulderName, const char* upperArmName,
                                  const char* lowerArmName, const char* handName,
                                  double side, double armSeed) {
                // Shoulder follows body with slight delay
                Vector3 shoulderPos = bodyTransform.transformPoint(bonePos(shoulderName));
                Vector3 shoulderEnd = bodyTransform.transformPoint(boneEnd(shoulderName));

                // Shoulder lifts during inhale, locks during roar
                double shoulderLift = (inhale * 0.04 + roarEnv * 0.06)
                    * armSpreadFactor * roarIntensity;
                {
                    Matrix4x4 sRot;
                    sRot.rotate(right, shoulderLift);
                    sRot.rotate(forward, side * roarEnv * 0.03 * armSpreadFactor);
                    Vector3 off = shoulderEnd - shoulderPos;
                    shoulderEnd = shoulderPos + sRot.transformVector(off);
                }
                boneWorldTransforms[shoulderName] = buildBoneWorldTransform(shoulderPos, shoulderEnd);

                Vector3 upperStart = shoulderEnd;
                Vector3 upperEndBind = bodyTransform.transformPoint(boneEnd(upperArmName));
                Vector3 armDir = upperEndBind - upperStart;

                // Arm tension wave: delayed burst for upper arm
                double armBurst = explosiveEnvelope(t - 0.02, 0.18, 0.28,
                                      2.5 / massInertia)
                    * roarIntensity;
                // Overshoot: arm goes PAST target then settles back
                double overshoot = explosiveEnvelope(t, 0.26, 0.32, 6.0)
                    * 0.15 * armSpreadFactor;

                // Back-charge: arms pull inward AND back (coiling with body)
                double pullIn = inhale * 0.18 * backChargeFactor * roarIntensity;
                // Fling out during burst (with overshoot)
                double spreadAngle = (armBurst * 0.45 + overshoot + sustain * 0.30)
                    * armSpreadFactor;
                // Raise up during roar
                double raiseAngle = (armBurst * 0.35 + overshoot * 0.8 + sustain * 0.22)
                    * armSpreadFactor;
                // Recovery: gravity pulls arms down
                double gravityDrop = recovery * 0.15;

                // Arm tremble (unique per arm)
                double armShake = tremble(tRad, armSeed, trembleEnv * 0.025);

                Matrix4x4 armRot;
                armRot.rotate(upDir, side * (spreadAngle - pullIn));
                armRot.rotate(right, raiseAngle - pullIn * 0.5 - gravityDrop + armShake);
                // Slight backward rotation during burst (opening chest)
                armRot.rotate(forward, -side * armBurst * 0.08 * armSpreadFactor);

                Vector3 newUpperEnd = upperStart + armRot.transformVector(armDir);
                boneWorldTransforms[upperArmName] = buildBoneWorldTransform(upperStart, newUpperEnd);

                // Forearm: delayed further, elbow bends during anticipation,
                // extends during burst, re-bends during sustain (clench)
                Vector3 lowerDirBind = bodyTransform.transformPoint(boneEnd(lowerArmName))
                    - bodyTransform.transformPoint(boneEnd(upperArmName));
                double forearmBurst = explosiveEnvelope(t - 0.04, 0.18, 0.28,
                                          2.0 / massInertia)
                    * roarIntensity;

                // Coil during anticipation, extend during burst, re-bend for clench
                double elbowAngle = inhale * 0.30 * roarIntensity
                    - forearmBurst * 0.15 // extend on burst
                    + sustain * 0.18 * roarIntensity // re-clench
                    + tremble(tRad, armSeed + 1.0, trembleEnv * 0.02);

                Matrix4x4 forearmRot;
                forearmRot.rotate(right, elbowAngle);
                forearmRot.rotate(upDir, side * spreadAngle * 0.25);
                Vector3 newLowerEnd = newUpperEnd + forearmRot.transformVector(lowerDirBind);
                boneWorldTransforms[lowerArmName] = buildBoneWorldTransform(newUpperEnd, newLowerEnd);

                // Hand: delayed even further, clenches during sustained roar
                Vector3 handDirBind = bodyTransform.transformPoint(boneEnd(handName))
                    - bodyTransform.transformPoint(boneEnd(lowerArmName));
                double handBurst = explosiveEnvelope(t - 0.06, 0.18, 0.28,
                                       2.0 / massInertia)
                    * roarIntensity;

                // Hand spread on burst (claws flair), clench during sustain
                double handClench = -sustain * 0.12 * roarIntensity;
                double handFlare = handBurst * 0.10 * armSpreadFactor;

                Matrix4x4 handRot;
                handRot.rotate(upDir, side * (handFlare + handClench));
                handRot.rotate(right, elbowAngle * 0.3 + tremble(tRad, armSeed + 2.0, trembleEnv * 0.015));
                Vector3 newHandEnd = newLowerEnd + handRot.transformVector(handDirBind);
                boneWorldTransforms[handName] = buildBoneWorldTransform(newLowerEnd, newHandEnd);
            };

            computeArm("LeftShoulder", "LeftUpperArm", "LeftLowerArm", "LeftHand",
                1.0, 10.0);
            computeArm("RightShoulder", "RightUpperArm", "RightLowerArm", "RightHand",
                -1.0, 20.0);

            // =============================================================
            // SKIN MATRICES
            // =============================================================
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

}

}
