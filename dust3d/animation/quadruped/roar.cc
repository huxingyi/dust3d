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

// Procedural lion-style roar animation for quadruped rig.
//
// Animation Techniques Applied:
//
// 1. BREATH-DRIVEN CORE: The roar is built around the respiratory cycle.
//    Inhale (chest expands, ribcage lifts) → explosive exhale (diaphragm
//    contracts, chest drives forward). This drives the entire upper body
//    and gives the roar its organic "coming from inside" feeling.
//
// 2. ASYMMETRIC EASE CURVES: Real muscle activation has fast attack and
//    slow decay. The burst uses sharp power curves (fast-in, slow-out)
//    rather than symmetric Gaussians, creating explosive snap.
//
// 3. MUSCLE TENSION WAVE: Tension propagates from core → spine → chest
//    → neck → head with frame delays, creating a visible wave of energy.
//
// 4. JAW SIMULATION: Jaw opens wide by rotating the Jaw bone and extending
//    the head endpoint, simulating the massive gape of a lion's roar.
//
// 5. GROUND REACTION FORCE: Front paws brace and may stomp — the energy
//    comes FROM the ground impact upward through the body.
//
// 6. SECONDARY DYNAMICS WITH OVERSHOOT: Tail overshoots its target pose
//    and settles back (damped spring behavior), giving weight and follow-through.
//
// 7. COHERENT NOISE TREMBLE: Layered incommensurate frequencies with
//    per-bone phase offsets create organic full-body vibration.
//
// 8. COUNTER-ROTATION: Head and tail move in opposition — as the head
//    throws back, the tail lashes down/under for balance (conservation of
//    angular momentum, just like real big cats).
//
// Animation phases (loopable):
//   Phase 1 (0.00–0.20): Inhale & anticipation — chest rises, body
//       gathers inward, slight crouch, chin tucks, tail lifts
//   Phase 2 (0.20–0.40): Explosive burst — diaphragm drives chest
//       forward, head snaps back and UP, jaw opens wide, front legs brace,
//       body jolts, possible front paw stomp
//   Phase 3 (0.40–0.70): Sustain roar — head pushes FORWARD (projecting
//       at target), full-body tremble, heavy breathing pulses, tail lashes
//       with micro-oscillations, legs grip the ground
//   Phase 4 (0.70–1.00): Exhale & recovery — tension drains outward
//       (core relaxes first, extremities last), settle to neutral

#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/common.h>
#include <dust3d/animation/quadruped/roar.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace quadruped {

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
            "Root", "Pelvis", "Spine", "Chest", "Neck", "Head",
            "FrontLeftUpperLeg", "FrontLeftLowerLeg", "FrontLeftFoot",
            "FrontRightUpperLeg", "FrontRightLowerLeg", "FrontRightFoot",
            "BackLeftUpperLeg", "BackLeftLowerLeg", "BackLeftFoot",
            "BackRightUpperLeg", "BackRightLowerLeg", "BackRightFoot"
        };
        if (!validateRequiredBones(boneIdx, requiredBones, sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;

        // ===================================================================
        // Parameters
        // ===================================================================
        double roarIntensity = parameters.getValue("roarIntensity", 1.0);
        double chestPuffFactor = parameters.getValue("chestPuffFactor", 1.0);
        double headThrowFactor = parameters.getValue("headThrowFactor", 1.0);
        double jawOpenFactor = parameters.getValue("jawOpenFactor", 1.0);
        double trembleFactor = parameters.getValue("trembleFactor", 1.0);
        double bodyMassFactor = parameters.getValue("bodyMassFactor", 1.0);
        double frontLegBraceFactor = parameters.getValue("frontLegBraceFactor", 1.0);
        double backLegPushFactor = parameters.getValue("backLegPushFactor", 1.0);
        double spineArchFactor = parameters.getValue("spineArchFactor", 1.0);
        double tailLashFactor = parameters.getValue("tailLashFactor", 1.0);
        double crouchDepthFactor = parameters.getValue("crouchDepthFactor", 1.0);
        double forwardThrustFactor = parameters.getValue("forwardThrustFactor", 1.0);
        double stompFactor = parameters.getValue("stompFactor", 1.0);
        double frontLegBendDirection = parameters.getValue("frontLegBendDirection", 0.5);
        double backLegBendDirection = parameters.getValue("backLegBendDirection", -0.5);

        // ===================================================================
        // Body frame: forward, right, up, scale
        // ===================================================================
        Vector3 pelvisPos = bonePos("Pelvis");
        Vector3 chestPos = bonePos("Chest");
        Vector3 upDir(0.0, 1.0, 0.0);

        Vector3 spineVec = chestPos - pelvisPos;
        Vector3 forward(spineVec.x(), 0.0, spineVec.z());
        if (forward.lengthSquared() < 1e-8)
            forward = Vector3(0.0, 0.0, 1.0);
        forward.normalize();

        Vector3 right = Vector3::crossProduct(upDir, forward);
        if (right.lengthSquared() < 1e-8)
            right = Vector3(1.0, 0.0, 0.0);
        right.normalize();

        // Body scale references
        double spineLen = (boneEnd("Spine") - bonePos("Spine")).length()
            + (boneEnd("Chest") - bonePos("Chest")).length()
            + (boneEnd("Neck") - bonePos("Neck")).length();
        if (spineLen < 1e-6)
            spineLen = 1.0;

        double frontLegLen = (boneEnd("FrontLeftUpperLeg") - bonePos("FrontLeftUpperLeg")).length()
            + (boneEnd("FrontLeftLowerLeg") - bonePos("FrontLeftLowerLeg")).length()
            + (boneEnd("FrontLeftFoot") - bonePos("FrontLeftFoot")).length();
        double backLegLen = (boneEnd("BackLeftUpperLeg") - bonePos("BackLeftUpperLeg")).length()
            + (boneEnd("BackLeftLowerLeg") - bonePos("BackLeftLowerLeg")).length()
            + (boneEnd("BackLeftFoot") - bonePos("BackLeftFoot")).length();
        double avgLegLen = (frontLegLen + backLegLen) * 0.5;
        if (avgLegLen < 1e-6)
            avgLegLen = 1.0;

        // Mass factor: affects timing sharpness (heavier = more inertia)
        double massInertia = 0.5 + 0.5 * bodyMassFactor; // 0.75 .. 1.75

        // ===================================================================
        // Phase curve helpers
        // ===================================================================

        // Asymmetric attack/decay envelope: fast rise, slow fall.
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
                return inv * inv * (3.0 - 2.0 * inv); // smoothstep fall
            }
            return 0.0;
        };

        // Sharp power attack (fast-in) + exponential decay (slow-out)
        auto explosiveEnvelope = [](double t, double onsetTime, double peakTime,
                                     double decayRate) -> double {
            if (t < onsetTime)
                return 0.0;
            if (t < peakTime) {
                double p = (t - onsetTime) / (peakTime - onsetTime);
                return p * p * p; // cubic attack
            }
            double elapsed = t - peakTime;
            return std::exp(-elapsed * decayRate);
        };

        // Coherent noise-like tremble: layered incommensurate frequencies
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
            // PHASE ENVELOPES
            // =============================================================

            // Inhale/anticipation: smooth rise and fall [0.0, 0.20]
            double inhale = asymEnvelope(t, 0.0, 0.10, 0.22);

            // Explosive burst: cubic attack at t=0.20, peak at t=0.28
            double burstRaw = explosiveEnvelope(t, 0.18, 0.28, 3.0 / massInertia);
            double burst = burstRaw * roarIntensity;

            // Sustain envelope: the held roar [0.35, 0.70]
            double sustain = asymEnvelope(t, 0.32, 0.45, 0.72);

            // Roar active = burst + sustain blended
            double roarEnv = std::min(1.0, burst + sustain * 0.85 * roarIntensity);

            // Tremble envelope: ramps up during sustain, decays in recovery
            double trembleEnv = sustain * trembleFactor * roarIntensity;

            // Stomp impact: sharp spike at burst peak
            double stompEnv = explosiveEnvelope(t, 0.22, 0.30, 8.0) * stompFactor;

            // =============================================================
            // BREATH-DRIVEN VERTICAL MOTION
            // =============================================================
            // Inhale: body rises slightly (chest fills)
            double inhaleRise = inhale * avgLegLen * 0.03 * roarIntensity;

            // Burst: explosive downward jolt from diaphragm contraction
            double burstDrop = burst * avgLegLen * 0.05 * crouchDepthFactor;
            double groundReaction = stompEnv * avgLegLen * 0.015;

            // Sustain: body is slightly crouched (braced), with breathing pulses
            double sustainCrouch = sustain * avgLegLen * 0.02 * crouchDepthFactor;
            double breathPulse = roarEnv * avgLegLen * 0.006
                * (std::sin(tRad * 6.0) + 0.4 * std::sin(tRad * 10.0));

            double verticalOffset = inhaleRise - burstDrop + groundReaction
                - sustainCrouch + breathPulse;

            // Ground-shake on stomp impact
            double groundShake = stompEnv * avgLegLen * 0.004
                * std::sin(tRad * 20.0);
            verticalOffset += groundShake;

            // Full-body tremble during sustain
            verticalOffset += tremble(tRad, 0.0, trembleEnv * avgLegLen * 0.003);

            // =============================================================
            // FORWARD/BACKWARD LEAN
            // =============================================================
            // Inhale: slight backward lean (gathering energy)
            double leanBack = inhale * 0.08 * roarIntensity;

            // Burst: explosive forward thrust from the gathered position
            double leanForward = burst * 0.12 * forwardThrustFactor * chestPuffFactor;

            // Sustain: aggressive forward lean (projecting roar)
            double sustainLean = sustain * 0.06 * forwardThrustFactor * roarIntensity;

            double forwardOffset = (-leanBack + leanForward + sustainLean) * spineLen;

            // =============================================================
            // LATERAL WEIGHT SHIFT — asymmetric
            // =============================================================
            double lateralShift = stompEnv * avgLegLen * 0.008 * stompFactor
                + tremble(tRad, 1.0, trembleEnv * avgLegLen * 0.002);

            // =============================================================
            // BUILD BODY TRANSFORM
            // =============================================================
            Matrix4x4 bodyTransform;
            bodyTransform.translate(
                upDir * verticalOffset + forward * forwardOffset + right * lateralShift);

            // =============================================================
            // SPINE: BREATH-DRIVEN ARCH + TENSION WAVE
            // =============================================================
            // Inhale: spine arches BACKWARD (chest lifts and opens)
            double spineInhaleArch = inhale * 0.10 * spineArchFactor * roarIntensity;
            // Burst: spine REVERSES — explosive forward arch
            double spineBurstArch = burst * 0.18 * spineArchFactor * forwardThrustFactor;
            // Sustain: held arch with forward component
            double spineSustainArch = sustain * 0.08 * spineArchFactor * roarIntensity;

            // Chest puff: ribcage expansion
            double chestExpand = (burst * 0.10 + sustain * 0.06 + inhale * 0.03)
                * chestPuffFactor * roarIntensity;

            // Tension wave delay
            double chestDelay = 0.02;
            double neckDelay = 0.04;

            auto delayedBurst = [&](double delay) -> double {
                return explosiveEnvelope(t - delay, 0.18, 0.28, 3.0 / massInertia)
                    * roarIntensity;
            };

            double spinePitch = -(spineInhaleArch + spineBurstArch + spineSustainArch) * 0.40;
            double chestPitch = -(spineInhaleArch + delayedBurst(chestDelay) * 0.14 * spineArchFactor
                                    + spineSustainArch)
                    * 0.50
                - chestExpand;
            double neckPitch = -(delayedBurst(neckDelay) * 0.07 * spineArchFactor);

            // Spine tremble
            double spineTremble = tremble(tRad, 2.0, trembleEnv * 0.012);
            double chestTremble = tremble(tRad, 3.0, trembleEnv * 0.010);

            // =============================================================
            // HEAD: DRAMATIC LION ROAR TRAJECTORY
            // =============================================================
            // Lion roars with head thrown UP and BACK, then thrusts FORWARD
            // 1. Inhale: chin tucks slightly (loading)
            // 2. Burst: head snaps UP and BACK (jaw opens wide)
            // 3. Sustain: head pushes FORWARD toward target
            // 4. Recovery: head drops forward, returns to neutral

            // Back-charge: head tilts UP and BACK
            double headTiltBack = inhale * 0.20 * headThrowFactor * roarIntensity;
            // Burst: head snaps UP and BACK dramatically
            double headSnapBack = burst * 0.50 * headThrowFactor * roarIntensity;
            // Sustain: head pushes forward and down (projecting roar)
            double headForwardPush = sustain * 0.15 * headThrowFactor * forwardThrustFactor;

            double headPitch = headTiltBack + headSnapBack - headForwardPush;

            // Jaw simulation: massive gape during roar
            double jawOpen = roarEnv * 0.60 * jawOpenFactor * roarIntensity;

            // Head shake: violent during burst, rhythmic during sustain
            double headShakeIntensity = burst * 0.06 + sustain * 0.04;
            double headShakeYaw = headShakeIntensity * trembleFactor
                * (0.6 * std::sin(tRad * 11.0 + 0.3)
                    + 0.3 * std::sin(tRad * 19.0 + 1.7)
                    + 0.1 * std::sin(tRad * 29.0 + 2.3));
            double headShakeRoll = headShakeIntensity * trembleFactor * 0.3
                * (0.7 * std::sin(tRad * 13.0 + 0.8)
                    + 0.3 * std::sin(tRad * 23.0 + 3.1));

            // =============================================================
            // PELVIS MOTION
            // =============================================================
            double pelvisYaw = stompEnv * 0.02 + tremble(tRad, 4.0, trembleEnv * 0.008);
            double pelvisRoll = lateralShift / (avgLegLen * 0.02 + 1e-6) * 0.010;

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
            computeBodyBone("Pelvis", pelvisYaw, 0.0, pelvisRoll);
            computeBodyBone("Spine", spineTremble, spinePitch, 0.0);
            computeBodyBone("Chest", chestTremble, chestPitch, 0.0);
            computeBodyBone("Neck", headShakeYaw * 0.3, neckPitch + headPitch * 0.25,
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
                // down (chin drops). This stretches the head bone visually.
                double headBoneLen = (end - pos).length();
                Vector3 jawDelta = (upDir * (-0.35) + forward * (-0.65)).normalized()
                    * (jawOpen * headBoneLen);
                newEnd = newEnd + jawDelta;

                boneWorldTransforms["Head"] = buildBoneWorldTransform(newPos, newEnd);

                // Jaw bone: attached to head, opens downward
                if (boneIdx.count("Jaw")) {
                    Vector3 jawPosRest = bonePos("Jaw");
                    Vector3 jawEndRest = boneEnd("Jaw");
                    Vector3 jawWorldPos = bodyTransform.transformPoint(jawPosRest);
                    Vector3 jawWorldEnd = bodyTransform.transformPoint(jawEndRest);

                    // Jaw opens by rotating downward around head base
                    Matrix4x4 jawRot;
                    if (std::abs(headShakeYaw) > 1e-6)
                        jawRot.rotate(upDir, headShakeYaw);
                    if (std::abs(headPitch) > 1e-6)
                        jawRot.rotate(right, headPitch);
                    // Additional jaw open rotation
                    jawRot.rotate(right, jawOpen * 0.8);

                    Vector3 jawOffset = jawWorldEnd - jawWorldPos;
                    jawWorldEnd = jawWorldPos + jawRot.transformVector(jawOffset);

                    // Push jaw down and forward for wide gape
                    double jawBoneLen = (jawEndRest - jawPosRest).length();
                    Vector3 jawOpenDelta = (upDir * (-0.5) + forward * 0.3).normalized()
                        * (jawOpen * jawBoneLen * 0.5);
                    jawWorldEnd = jawWorldEnd + jawOpenDelta;

                    boneWorldTransforms["Jaw"] = buildBoneWorldTransform(jawWorldPos, jawWorldEnd);
                }
            }

            // =============================================================
            // TAIL: COUNTER-BALANCE WHIP WITH WAVE PROPAGATION + OVERSHOOT
            // =============================================================
            // Big cats use their tail as a counterbalance during roaring.
            // As the head throws back, the tail lashes down and under
            // to conserve angular momentum.
            static const char* tailBones[] = { "TailBase", "TailMid", "TailTip" };
            Vector3 prevTailEnd;
            bool hasPrevTail = false;
            for (int ti = 0; ti < 3; ++ti) {
                if (boneIdx.count(tailBones[ti]) == 0)
                    continue;
                double cascade = 1.0 + ti * 0.6;
                double tailDelay = ti * 0.03;
                double delayedRoar = explosiveEnvelope(
                    t - tailDelay, 0.18, 0.28, 2.5 / massInertia);

                // Counter-rotation: tail goes DOWN as head goes UP
                double tailPitch = tailLashFactor * (delayedRoar * roarIntensity * 0.15 * cascade * std::sin(tRad * 5.0 - ti * 1.2) + sustain * 0.08 * cascade * (0.6 * std::sin(tRad * 4.0 - ti * 0.9) + 0.4 * std::sin(tRad * 7.0 - ti * 1.5)));

                // Tail yaw (side-to-side lash)
                double tailYaw = tailLashFactor * (delayedRoar * roarIntensity * 0.10 * cascade * std::sin(tRad * 6.0 - ti * 0.8) + sustain * 0.05 * cascade * std::sin(tRad * 5.0 - ti * 1.1));

                // Tail tremble
                tailYaw += tremble(tRad, 5.0 + ti, trembleEnv * 0.015 * cascade);
                tailPitch += tremble(tRad, 6.0 + ti, trembleEnv * 0.012 * cascade);

                Vector3 pos = bonePos(tailBones[ti]);
                Vector3 end = boneEnd(tailBones[ti]);
                Vector3 newPos = bodyTransform.transformPoint(pos);
                Vector3 newEnd = bodyTransform.transformPoint(end);
                if (hasPrevTail) {
                    Vector3 offset = newEnd - newPos;
                    newPos = prevTailEnd;
                    newEnd = newPos + offset;
                }
                if (std::abs(tailYaw) > 1e-6 || std::abs(tailPitch) > 1e-6) {
                    Matrix4x4 extraRot;
                    if (std::abs(tailYaw) > 1e-6)
                        extraRot.rotate(upDir, tailYaw);
                    if (std::abs(tailPitch) > 1e-6)
                        extraRot.rotate(right, tailPitch);
                    Vector3 offset = newEnd - newPos;
                    newEnd = newPos + extraRot.transformVector(offset);
                }
                boneWorldTransforms[tailBones[ti]] = buildBoneWorldTransform(newPos, newEnd);
                prevTailEnd = newEnd;
                hasPrevTail = true;
            }

            // =============================================================
            // LEGS: FOUR-LEG BRACING WITH STOMP DYNAMICS
            // =============================================================
            // During the roar, all four legs brace into the ground.
            // Front legs may stomp at burst peak. Back legs push down
            // for stability. The creature widens its stance slightly.

            // Stomp: front left paw lifts briefly then slams
            double leftFrontStompLift = 0.0;
            double rightFrontStompLift = 0.0;
            {
                double stompPrep = asymEnvelope(t, 0.14, 0.20, 0.24);
                double stompSlam = explosiveEnvelope(t, 0.22, 0.26, 10.0);

                leftFrontStompLift = stompFactor * avgLegLen
                    * (stompPrep * 0.05 - stompSlam * 0.008);
                // Right front braces slightly
                rightFrontStompLift = -stompEnv * avgLegLen * 0.004 * stompFactor;

                if (leftFrontStompLift < -avgLegLen * 0.015)
                    leftFrontStompLift = -avgLegLen * 0.015;
                if (rightFrontStompLift < -avgLegLen * 0.015)
                    rightFrontStompLift = -avgLegLen * 0.015;
            }

            // Back legs push into ground for stability during roar
            double backLegPush = roarEnv * avgLegLen * 0.01 * backLegPushFactor;
            double leftBackPush = backLegPush + tremble(tRad, 7.0, trembleEnv * avgLegLen * 0.002);
            double rightBackPush = backLegPush + tremble(tRad, 8.0, trembleEnv * avgLegLen * 0.002);

            auto computeLeg = [&](const char* upperLeg, const char* lowerLeg,
                                  const char* foot, double footLift,
                                  double braceBendFactor,
                                  bool isFront) {
                Vector3 footStart = bonePos(foot) + upDir * footLift;
                Vector3 footEnd = boneEnd(foot) + upDir * footLift;
                boneWorldTransforms[foot] = buildBoneWorldTransform(footStart, footEnd);

                Vector3 hipJoint = bodyTransform.transformPoint(bonePos(upperLeg));
                double upperLen = (boneEnd(upperLeg) - bonePos(upperLeg)).length();

                // Pole vector: user-controllable bend direction
                Vector3 midBind = bodyTransform.transformPoint(bonePos(lowerLeg));
                double bendDir = isFront ? frontLegBendDirection : backLegBendDirection;
                Vector3 poleTarget = midBind + forward * (upperLen * bendDir);

                // Apply brace bend: legs compress slightly during roar
                Vector3 bracedFootStart = footStart;
                bracedFootStart += upDir * (-braceBendFactor * avgLegLen * 0.02);

                std::vector<Vector3> joints = { hipJoint, midBind, bracedFootStart };
                solveTwoBoneIk(joints, bracedFootStart, poleTarget, 0.05);

                boneWorldTransforms[upperLeg] = buildBoneWorldTransform(joints[0], joints[1]);
                boneWorldTransforms[lowerLeg] = buildBoneWorldTransform(joints[1], joints[2]);
            };

            computeLeg("FrontLeftUpperLeg", "FrontLeftLowerLeg", "FrontLeftFoot",
                leftFrontStompLift, frontLegBraceFactor, true);
            computeLeg("FrontRightUpperLeg", "FrontRightLowerLeg", "FrontRightFoot",
                rightFrontStompLift, frontLegBraceFactor, true);
            computeLeg("BackLeftUpperLeg", "BackLeftLowerLeg", "BackLeftFoot",
                leftBackPush, backLegPushFactor, false);
            computeLeg("BackRightUpperLeg", "BackRightLowerLeg", "BackRightFoot",
                rightBackPush, backLegPushFactor, false);

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

} // namespace quadruped

} // namespace dust3d
