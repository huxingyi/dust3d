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

// Procedural jump animation for biped rig.
//
// Industry techniques used:
//
// 1. BALLISTIC PARABOLA — The airborne phase follows a real gravity-based
//    projectile arc (h(t) = v0*t - 0.5*g*t^2) so the body traces a
//    physically correct trajectory. Launch velocity is derived from desired
//    jump height so the apex is frame-exact.
//
// 2. SPRING-DAMPER SECONDARY DYNAMICS — Tail, head, and arms use a second-
//    order damped harmonic oscillator (critical/under-damped) that reacts
//    to the body's vertical acceleration. This produces realistic follow-
//    through, overshoot, and drag with no hand-tuned per-phase curves.
//    State is integrated per-frame with semi-implicit Euler.
//
// 3. SQUASH & STRETCH — Disney's classic principle applied via non-uniform
//    bone scaling along the spine vertical axis. Compresses during crouch
//    and landing impact, elongates during launch and apex. Controlled by
//    a squashStretchFactor parameter (0 = off, for realistic; high = cartoony).
//
// 4. CRITICALLY DAMPED SPRING RECOVERY — Landing settle uses a critically
//    damped spring (zeta=1) so the body returns to rest with exactly one
//    settle without overshoot.
//
// Phases:
//   1. Anticipation / crouch   (0.00 - 0.20)  — crouch with squash
//   2. Launch / push-off       (0.20 - 0.28)  — explosive extension with stretch
//   3. Ballistic airborne      (0.28 - 0.72)  — parabolic arc, tuck, secondary dynamics
//   4. Landing / impact        (0.72 - 0.82)  — compression with squash
//   5. Recovery / settle       (0.82 - 1.00)  — critically damped spring return

#include <array>
#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/biped/jump.h>
#include <dust3d/animation/biped/pose.h>
#include <dust3d/animation/common.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace biped {

    // =====================================================================
    // Second-order damped harmonic oscillator for secondary dynamics.
    // Models spring-mass-damper: x'' + 2*zeta*omega*x' + omega^2*x = omega^2*target
    // Semi-implicit Euler integration per frame.
    // =====================================================================
    struct SpringState {
        double pos = 0.0;
        double vel = 0.0;

        void step(double target, double omega, double zeta, double dt)
        {
            // Substeps keep the oscillator stable for long clips and sparse sampling.
            int steps = std::max(1, static_cast<int>(std::ceil(dt * std::abs(omega) * 4.0)));
            double h = dt / steps;
            for (int i = 0; i < steps; ++i) {
                double acceleration = omega * omega * (target - pos) - 2.0 * zeta * omega * vel;
                vel += acceleration * h;
                pos += vel * h;
            }
        }
    };

    struct JumpLegDef {
        const char* upperLegName;
        const char* lowerLegName;
        const char* footName;
    };

    struct JumpLegRest {
        Vector3 upperLegPos, upperLegEnd;
        Vector3 lowerLegEnd;
        Vector3 footEnd;
        Vector3 restStickDir;
        Vector3 restUpperToLowerVec;
        double legLength;
    };

    bool jump(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = static_cast<int>(parameters.getValue("frameCount", 40.0));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 1.2));

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
        // 1. Body orientation
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

        // ===================================================================
        // 2. Define legs and arms
        // ===================================================================
        static const JumpLegDef leftLeg = { "LeftUpperLeg", "LeftLowerLeg", "LeftFoot" };
        static const JumpLegDef rightLeg = { "RightUpperLeg", "RightLowerLeg", "RightFoot" };
        auto gatherLegRest = [&](const JumpLegDef& leg) -> JumpLegRest {
            JumpLegRest r;
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

        JumpLegRest leftLegRest = gatherLegRest(leftLeg);
        JumpLegRest rightLegRest = gatherLegRest(rightLeg);

        // ===================================================================
        // 3. Parameters
        // ===================================================================
        double avgLegLength = (leftLegRest.legLength + rightLegRest.legLength) * 0.5;
        if (avgLegLength < 1e-6)
            avgLegLength = 1.0;

        double jumpHeightFactor = parameters.getValue("jumpHeightFactor", 1.0);
        double crouchDepthFactor = parameters.getValue("crouchDepthFactor", 1.0);
        double armRaiseFactor = parameters.getValue("armRaiseFactor", 1.0);
        double leanForwardFactor = parameters.getValue("leanForwardFactor", 1.0);
        double kneeTuckFactor = parameters.getValue("kneeTuckFactor", 1.0);
        double landingImpactFactor = parameters.getValue("landingImpactFactor", 1.0);
        double bouncinessFactor = parameters.getValue("bouncinessFactor", 1.0);
        double armSpreadFactor = parameters.getValue("armSpreadFactor", 1.0);
        double headLookUpFactor = parameters.getValue("headLookUpFactor", 1.0);
        double spineArchFactor = parameters.getValue("spineArchFactor", 1.0);
        double recoverySpeedFactor = parameters.getValue("recoverySpeedFactor", 1.0);
        double hipSwayFactor = parameters.getValue("hipSwayFactor", 1.0);
        double squashStretchFactor = parameters.getValue("squashStretchFactor", 1.0);
        double secondaryDynamicsFactor = parameters.getValue("secondaryDynamicsFactor", 1.0);
        double tailLiftFactor = parameters.getValue("tailLiftFactor", 1.0);
        double tailStiffnessFactor = parameters.getValue("tailStiffnessFactor", 1.0);
        double tailSwayFactor = parameters.getValue("tailSwayFactor", 1.0);

        // Derived amplitudes
        double jumpHeight = avgLegLength * 0.6 * jumpHeightFactor * bouncinessFactor;
        double crouchDepth = avgLegLength * 0.15 * crouchDepthFactor;
        double landingDepth = avgLegLength * 0.12 * landingImpactFactor;
        double armRaiseAngle = 0.8 * armRaiseFactor;

        double leanAngle = 0.10 * leanForwardFactor;
        double kneeTuckAmount = 0.6 * kneeTuckFactor;
        double headLookAngle = 0.15 * headLookUpFactor;
        double spineArchAngle = 0.08 * spineArchFactor;
        double hipSwayAmp = avgLegLength * 0.01 * hipSwayFactor;

        // Phase boundaries
        const double tCrouchEnd = 0.20; // end of anticipation
        const double tLaunchEnd = 0.28; // feet leave ground
        const double tLandStart = 0.72; // feet touch ground
        const double tLandEnd = 0.82; // end of impact compression
        // 0.82 - 1.00 = critically damped recovery

        // Ballistic parameters: derive launch velocity from desired height.
        // Airborne duration in normalized time
        double airborneSpan = tLandStart - tLaunchEnd; // 0.44
        double airborneHalf = airborneSpan * 0.5; // apex at midpoint
        // Using h = v0*t_half - 0.5*g*t_half^2 and v0 = g*t_half (apex condition):
        //   h = 0.5*g*t_half^2  =>  g = 2*h / t_half^2
        double ballisticG = 2.0 * jumpHeight / (airborneHalf * airborneHalf);
        double ballisticV0 = ballisticG * airborneHalf; // launch velocity

        // ===================================================================
        // 4. Secondary dynamics spring states (persist across frames)
        // ===================================================================
        // Spring natural frequency and damping ratio
        double secondaryOmega = 12.0 * secondaryDynamicsFactor; // responsiveness
        double secondaryZeta = 0.35; // underdamped for visible overshoot

        // Head pitch spring: reacts to vertical acceleration
        SpringState headSpring;
        // Arm pitch springs: left and right react to body vertical
        SpringState leftArmSpring, rightArmSpring;
        // Tail springs: one per segment, react to body vertical + lateral
        SpringState tailPitchSprings[3];
        SpringState tailYawSprings[3];

        double tailOmega = 8.0 / std::max(0.05, 0.5 + tailStiffnessFactor * 0.5);
        double tailZeta = 0.25; // underdamped for floppy/whippy feel

        double recoveryOmega = 8.0 * recoverySpeedFactor;

        double dt = durationSeconds / static_cast<double>(frameCount - 1);
        double prevBodyVertical = 0.0;
        double prevBodyVelocity = 0.0;

        // ===================================================================
        // 5. Generate frames
        // ===================================================================
        const auto armBindWorld = animation::restBoneWorldTransforms(rigStructure);
        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        // Hair chain physics: floaty feel with reduced gravity during airborne phase.
        const std::vector<std::string> hairBoneNames = { "HairBack1", "HairBack2", "HairBack3" };
        animation::HairChainSimulator hairSim;
        if (boneIdx.count("HairBack1"))
            hairSim.initialize(rigStructure, boneIdx, hairBoneNames,
                animation::buildBoneWorldTransform(bonePos("Head"), boneEnd("Head")),
                0.07, 0.92, 0.8);
        // dt is already computed above; reuse it as the hair simulation step size.

        animation::CapeGridSimulator capeSim;
        if (boneIdx.count("CenterCape1"))
            capeSim.initialize(rigStructure, boneIdx,
                animation::buildBoneWorldTransform(bonePos("Chest"), boneEnd("Chest")),
                0.08, 0.85, 1.2, 0.15);

        auto restingSecondary = restBoneWorldTransforms(rigStructure);
        for (int warmup = 0; warmup < 120; ++warmup) {
            if (hairSim.active)
                hairSim.step(restingSecondary["Head"], 1.0 / 60.0, restingSecondary);
            if (capeSim.active)
                capeSim.step(restingSecondary["Chest"], 1.0 / 60.0, restingSecondary);
        }
        const auto secondaryRest = restingSecondary;
        const auto rigRest = restBoneWorldTransforms(rigStructure);

        // A one-shot starts from stationary secondary motion, never a replayed jump.
        for (int frame = 0; frame < frameCount; ++frame) {
            double t = static_cast<double>(frame) / static_cast<double>(frameCount - 1);

            std::map<std::string, Matrix4x4> boneWorldTransforms;
            double secondarySettle = 1.0 - smootherstep((t - tLandEnd) / (1.0 - tLandEnd));

            // -------------------------------------------------------
            // 5a. Body vertical position per phase
            // -------------------------------------------------------
            double bodyVertical = 0.0;
            double bodyLean = 0.0;
            double spineArch = 0.0;
            double driveHeadLook = 0.0; // spring target, not final
            double driveArmSwing = 0.0; // -1..1 drive signal for arm spring
            double legTuckPhase = 0.0;
            double squashStretch = 0.0; // <0 = squash, >0 = stretch

            if (t < tCrouchEnd) {
                // Anticipation: ease into squat
                double p = t / tCrouchEnd;
                double ease = smootherstep(p);
                bodyVertical = -crouchDepth * ease;
                bodyLean = leanAngle * 0.3 * ease;
                driveArmSwing = -0.5 * ease;
                driveHeadLook = -headLookAngle * 0.3 * ease;
                squashStretch = -0.5 * ease * squashStretchFactor; // squash
            } else if (t < tLaunchEnd) {
                // Launch: explosive extension
                double p = (t - tCrouchEnd) / (tLaunchEnd - tCrouchEnd);
                double ease = smootherstep(p);
                // Smoothly blend from crouch to initial ballistic position
                double ballisticStart = 0.0; // at tLaunchEnd, airborne t=0
                bodyVertical = -crouchDepth * (1.0 - ease) + ballisticStart * ease;
                bodyLean = leanAngle * (0.3 * (1.0 - ease) + 1.0 * ease);
                driveArmSwing = -0.5 * (1.0 - ease) + 1.0 * ease;
                driveHeadLook = headLookAngle * ease;
                spineArch = spineArchAngle * ease;
                squashStretch = (-0.5 * (1.0 - ease) + 0.8 * ease) * squashStretchFactor; // stretch
            } else if (t < tLandStart) {
                // BALLISTIC AIRBORNE: real parabolic arc
                double airT = (t - tLaunchEnd) / airborneSpan; // 0..1 normalized
                // h(airT) = v0 * airT*span - 0.5*g*(airT*span)^2
                // but we normalize to use airT directly:
                double tSec = airT * airborneSpan; // "time" in normalized units
                bodyVertical = ballisticV0 * tSec - 0.5 * ballisticG * tSec * tSec;

                // Apex detection for secondary dynamics
                double apexT = 0.5; // apex at midpoint by construction
                double distFromApex = std::abs(airT - apexT) / 0.5; // 0 at apex, 1 at edges

                bodyLean = leanAngle * (0.7 + 0.3 * (1.0 - distFromApex));
                driveArmSwing = 1.0 - 0.2 * distFromApex;
                driveHeadLook = headLookAngle;
                spineArch = spineArchAngle;

                // Knee tuck: peaks at apex, eases in/out
                legTuckPhase = kneeTuckAmount * (1.0 - distFromApex * distFromApex);

                // Stretch at launch/descent boundaries, slight squash at apex (hang time feel)
                // Actually: stretch during ascent/descent, neutral at apex
                double ascentDescent = std::abs(airT - 0.5) * 2.0; // 0 at apex, 1 at edges
                squashStretch = 0.4 * ascentDescent * squashStretchFactor;
            } else if (t < tLandEnd) {
                // Landing impact: compression
                double p = (t - tLandStart) / (tLandEnd - tLandStart);
                // Quick compression then hold
                double compress = 1.0 - (1.0 - smootherstep(p)) * (1.0 - smootherstep(p));
                // Peak compression at ~40% through landing, then ease
                compress = (p < 0.4) ? smootherstep(p / 0.4) : 1.0;
                bodyVertical = -landingDepth * compress;
                bodyLean = leanAngle * 0.5 * compress;
                driveArmSwing = -0.3 * compress;
                driveHeadLook = -headLookAngle * 0.3 * compress;
                squashStretch = -0.6 * compress * squashStretchFactor; // squash on impact
            } else {
                // Analytic critical damping starts at the exact landing compression.
                // The terminal fade settles completely, including at low recovery speeds.
                double elapsed = (t - tLandEnd) * durationSeconds;
                double w = std::max(0.0, recoveryOmega) * elapsed;
                double settle = (1.0 + w) * std::exp(-w)
                    * (1.0 - smootherstep((t - tLandEnd) / (1.0 - tLandEnd)));
                bodyVertical = -landingDepth * settle;
                bodyLean = leanAngle * 0.5 * settle;
                driveArmSwing = -0.3 * settle;
                driveHeadLook = -headLookAngle * 0.3 * settle;
                squashStretch = -0.6 * settle * squashStretchFactor;
            }

            // Lateral hip sway during airborne
            double lateralSway = 0.0;
            if (t > tLaunchEnd && t < tLandStart) {
                double airP = (t - tLaunchEnd) / (tLandStart - tLaunchEnd);
                lateralSway = hipSwayAmp * std::sin(airP * 2.0 * Math::Pi);
            }

            // -------------------------------------------------------
            // 5b. Secondary dynamics: step springs with body acceleration
            // -------------------------------------------------------
            double bodyVelocity = (bodyVertical - prevBodyVertical) / dt;
            double bodyAcceleration = (bodyVelocity - prevBodyVelocity) / dt;
            prevBodyVertical = bodyVertical;
            prevBodyVelocity = bodyVelocity;

            // Head spring reacts to vertical acceleration (looks up when decelerating, down when accelerating)
            double headDriveTarget = -bodyAcceleration * 0.003 * secondaryDynamicsFactor + driveHeadLook;
            headSpring.step(headDriveTarget, secondaryOmega, secondaryZeta, dt);

            // Arm springs: react similarly
            double armDriveTarget = -bodyAcceleration * 0.005 * secondaryDynamicsFactor + driveArmSwing * armRaiseAngle;
            leftArmSpring.step(armDriveTarget, secondaryOmega * 0.8, secondaryZeta, dt);
            rightArmSpring.step(armDriveTarget, secondaryOmega * 0.8, secondaryZeta, dt);

            // Tail springs: react to acceleration with cascade delay
            for (int ti = 0; ti < 3; ++ti) {
                if (boneIdx.count(ti == 0 ? "TailBase" : (ti == 1 ? "TailMid" : "TailTip")) == 0)
                    continue;
                double cascade = (ti + 1.0) / 3.0;
                double tailPitchTarget = -bodyAcceleration * 0.006 * cascade * tailLiftFactor;
                double tailYawTarget = lateralSway * 3.0 * cascade * tailSwayFactor;
                // Lower omega for outer segments = more lag = whip effect
                double segOmega = tailOmega / (1.0 + ti * 0.4);
                tailPitchSprings[ti].step(tailPitchTarget, segOmega, tailZeta, dt);
                tailYawSprings[ti].step(tailYawTarget, segOmega, tailZeta, dt);
            }

            // -------------------------------------------------------
            // 5c. Body transform with squash & stretch
            // -------------------------------------------------------
            Matrix4x4 bodyTransform;
            bodyTransform.translate(upDir * bodyVertical + right * lateralSway);
            bodyTransform.rotate(right, bodyLean);

            // Squash & stretch scale factors (volume-preserving approximation):
            // If stretch along Y by (1+s), compress X,Z by 1/sqrt(1+s)
            double stretchY = 1.0 + squashStretch * 0.15;
            double stretchXZ = 1.0 / std::sqrt(std::max(0.3, stretchY));

            auto computeBodyBone = [&](const std::string& name,
                                       double extraYaw = 0.0,
                                       double extraPitch = 0.0,
                                       bool applySquashStretch = false) {
                Vector3 pos = bonePos(name);
                Vector3 end = boneEnd(name);
                Vector3 newPos = bodyTransform.transformPoint(pos);
                Vector3 newEnd = bodyTransform.transformPoint(end);

                // Apply squash & stretch to bone direction
                if (applySquashStretch && std::abs(squashStretch) > 1e-4) {
                    Vector3 dir = newEnd - newPos;
                    double origLen = dir.length();
                    if (origLen > 1e-8) {
                        // Decompose direction into up-component and lateral
                        double upComp = Vector3::dotProduct(dir, upDir);
                        Vector3 lateralComp = dir - upDir * upComp;
                        // Scale: stretch vertical, compress lateral
                        Vector3 scaledDir = upDir * (upComp * stretchY) + lateralComp * stretchXZ;
                        // Preserve bone length
                        double scaledLen = scaledDir.length();
                        if (scaledLen > 1e-8)
                            newEnd = newPos + scaledDir * (origLen / scaledLen);
                    }
                }

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
            computeBodyBone("Hips", 0.0, 0.0, true);
            computeBodyBone("Spine", 0.0, spineArch * 0.5, true);
            computeBodyBone("Chest", 0.0, spineArch, true);
            // Head uses spring-driven pitch instead of direct drive
            computeBodyBone("Neck", 0.0, headSpring.pos * 0.5 * secondarySettle);
            computeBodyBone("Head", 0.0, headSpring.pos * secondarySettle);

            // -------------------------------------------------------
            // 5d. Tail with spring dynamics (optional bones)
            // -------------------------------------------------------
            static const char* tailBones[] = { "TailBase", "TailMid", "TailTip" };
            Vector3 prevTailEnd;
            Vector3 prevTailRestEnd; // rigs may leave a gap between tail bones: keep it
            bool hasPrevTail = false;
            for (int ti = 0; ti < 3; ++ti) {
                if (boneIdx.count(tailBones[ti]) == 0)
                    continue;
                double tailYaw = tailYawSprings[ti].pos * secondarySettle;
                double tailPitch = tailPitchSprings[ti].pos * secondarySettle;
                Vector3 pos = bonePos(tailBones[ti]);
                Vector3 end = boneEnd(tailBones[ti]);
                Vector3 newPos = bodyTransform.transformPoint(pos);
                Vector3 newEnd = bodyTransform.transformPoint(end);
                if (hasPrevTail) {
                    Vector3 offset = newEnd - newPos;
                    newPos = prevTailEnd + (pos - prevTailRestEnd);
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
                prevTailRestEnd = end;
                hasPrevTail = true;
            }

            // -------------------------------------------------------
            // 5e. Leg IK
            // -------------------------------------------------------
            // Solve to the ankle, with an independently flat foot. Combining the
            // shin and foot into one IK link deforms the sole and leaves bent legs
            // even when the body has returned to rest.
            auto poseLeg = [&](const JumpLegDef& leg, const JumpLegRest& rest) {
                Vector3 ankleTarget = rest.lowerLegEnd;
                if (t >= tLaunchEnd && t < tLandStart) {
                    Vector3 tuckOffset = upDir * (avgLegLength * 0.2 * legTuckPhase)
                        + forward * (avgLegLength * 0.08 * legTuckPhase);
                    ankleTarget = bodyTransform.transformPoint(rest.lowerLegEnd) + tuckOffset;
                }
                poseTwoBoneLeg(rigStructure, boneIdx, leg.upperLegName, leg.lowerLegName,
                    leg.footName, bodyTransform, ankleTarget, Vector3(), false, boneWorldTransforms);
            };
            poseLeg(leftLeg, leftLegRest);
            poseLeg(rightLeg, rightLegRest);

            // -------------------------------------------------------
            // 5f. Arm animation with spring dynamics
            // -------------------------------------------------------
            // Keyed directions make the preparation, launch and balance readable.
            // Springs add a small follow-through; they no longer define the gesture.
            double prep = t < tCrouchEnd ? easePose(t / tCrouchEnd)
                                         : 1.0 - easePose((t - tCrouchEnd) / (tLaunchEnd - tCrouchEnd));
            double raised = easePose((t - tCrouchEnd) / (tLaunchEnd - tCrouchEnd))
                * (1.0 - easePose((t - 0.58) / (tLandStart - 0.58)));
            double balance = easePose((t - 0.58) / (tLandStart - 0.58)) * secondarySettle;
            auto computeArmJump = [&](const std::string& prefix, double sideMirror, double spring) {
                Vector3 outward = right * sideMirror;
                Vector3 upper = -upDir + outward * 0.16 + forward * 0.06;
                upper += -forward * (0.75 * prep) + forward * (1.4 * raised * armRaiseFactor);
                upper += upDir * (1.7 * raised * armRaiseFactor);
                upper += outward * (0.7 * balance * armSpreadFactor);
                upper += forward * (std::clamp(spring, -0.15, 0.15) * secondarySettle);
                Vector3 lower = upper + forward * (0.35 * prep + 0.55 * raised) + upDir * (0.3 * raised);
                Matrix4x4 chestLayer = boneWorldTransforms.at("Chest");
                chestLayer *= armBindWorld.at("Chest").inverted();
                aimArm(rigStructure, boneIdx, armBindWorld, boneWorldTransforms,
                    prefix, chestLayer, bodyTransform.transformVector(upper), bodyTransform.transformVector(lower),
                    std::clamp(prep + raised + balance, 0.0, 1.0), parameters);
            };
            computeArmJump("Left", 1.0, leftArmSpring.pos);
            computeArmJump("Right", -1.0, rightArmSpring.pos);

            // -------------------------------------------------------
            // 5g. Skin matrices
            // -------------------------------------------------------
            {
                if (hairSim.active)
                    hairSim.step(boneWorldTransforms["Head"], dt, boneWorldTransforms);
                if (capeSim.active)
                    capeSim.step(boneWorldTransforms["Chest"], dt, boneWorldTransforms);

                // Blend simulated cloth and hair back to their stationary drape as
                // recovery finishes. Keep the parent motion and each bone's length.
                double settle = smootherstep((t - tLandEnd) / (1.0 - tLandEnd));
                for (const auto& bone : rigStructure.bones) {
                    bool hair = bone.name.find("HairBack") == 0;
                    if (!hair && bone.name.find("Cape") == std::string::npos)
                        continue;
                    auto posed = boneWorldTransforms.find(bone.name);
                    if (posed == boneWorldTransforms.end() || settle <= 0.0)
                        continue;
                    const std::string parent = hair ? "Head" : "Chest";
                    Matrix4x4 target = boneWorldTransforms.at(parent);
                    target *= rigRest.at(parent).inverted();
                    target *= secondaryRest.at(bone.name);
                    Vector3 start = posed->second.transformPoint(Vector3());
                    Vector3 dir = posed->second.transformVector(Vector3(0, 0, 1));
                    Vector3 targetStart = target.transformPoint(Vector3());
                    Vector3 targetDir = target.transformVector(Vector3(0, 0, 1));
                    Quaternion turn = Quaternion::slerp(Quaternion(), Quaternion::rotationTo(dir, targetDir), settle);
                    Matrix4x4 rotation;
                    rotation.rotate(turn);
                    Vector3 newStart = start + (targetStart - start) * settle;
                    posed->second = boneFromRest(posed->second, start, start + dir,
                        newStart, newStart + rotation.transformVector(dir));
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
        }

        return true;
    }

} // namespace biped

} // namespace dust3d
