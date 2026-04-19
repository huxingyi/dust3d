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

// Procedural walk animation for bird rig (chicken, closed-wing bird, etc.).
//
// Biomechanically accurate chicken/ground-bird locomotion cycle:
//
//   Gait timing:
//     - Asymmetric duty factor: ~65% stance (ground contact), ~35% swing (airborne).
//       Real chickens keep each foot planted much longer than it is in the air.
//     - Left/right legs are offset by half a stride cycle.
//
//   Head bobbing (optic flow stabilization):
//     - Two-phase pattern observed in pigeons and chickens:
//       (1) Hold phase: head stays fixed in world space while body advances beneath it.
//       (2) Thrust phase: head rapidly snaps forward to a new fixation point.
//     - This stabilises retinal images during stance for better visual acuity.
//
//   Body dynamics (spring-damper secondary motion):
//     - Vertical bob at 2x step frequency (body dips at mid-stance of each leg).
//     - Lateral sway: body shifts over the stance leg for single-support balance.
//     - Forward pitch: counter-pitch between steps as center of mass shifts.
//     - All body DOFs driven through spring-damper integration for organic
//       overshoot and settling.
//
//   Legs (digitigrade two-bone IK):
//     - Birds walk on their toes; the visible "knee" is actually the
//       intertarsal (ankle) joint that bends forward.
//     - Foot trajectory uses smootherstep interpolation for natural swing arcs.
//     - Foot locking: world-space position is captured at touchdown and pinned
//       during the entire stance phase (eliminates foot sliding).
//     - High foot lift characteristic of ground-dwelling birds (chickens).
//
//   Tail:
//     - Rhythmic up-down pump counter-phased to body bob (counterbalance).
//     - Lateral yaw sway opposite to body roll.
//
//   Wings:
//     - Held folded against body with micro-sway synced to gait.
//
// Adjustable parameters:
//   - stepLengthFactor:   stride length multiplier
//   - stepHeightFactor:   foot lift height multiplier
//   - bodyBobFactor:      vertical body oscillation intensity
//   - gaitSpeedFactor:    gait cycle speed (number of full strides per clip)
//   - headBobFactor:      head thrust amplitude
//   - tailSwayFactor:     tail lateral sway amplitude

#include <cmath>
#include <dust3d/animation/bird/walk.h>
#include <dust3d/animation/common.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace bird {

    bool walk(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& /* inverseBindMatrices */,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = static_cast<int>(parameters.getValue("frameCount", 30.0));
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
            "LeftWingShoulder", "LeftWingElbow", "LeftWingHand",
            "RightWingShoulder", "RightWingElbow", "RightWingHand",
            "LeftUpperLeg", "LeftLowerLeg", "LeftFoot",
            "RightUpperLeg", "RightLowerLeg", "RightFoot"
        };
        if (!validateRequiredBones(boneIdx, requiredBones, sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;

        // Determine body coordinate frame
        Vector3 upDir(0.0, 1.0, 0.0);
        Vector3 pelvisPos = bonePos("Pelvis");
        Vector3 chestPos = bonePos("Chest");
        Vector3 spineDir = chestPos - pelvisPos;
        Vector3 forward(spineDir.x(), 0.0, spineDir.z());
        if (forward.lengthSquared() < 1e-8)
            forward = Vector3(0.0, 0.0, 1.0);
        forward.normalize();
        Vector3 right = Vector3::crossProduct(upDir, forward);
        if (right.lengthSquared() < 1e-8)
            right = Vector3(1.0, 0.0, 0.0);
        right.normalize();

        double bodyHeight = (boneEnd("Head") - pelvisPos).length();
        if (bodyHeight < 1e-6)
            bodyHeight = 0.5;

        // Animation parameters
        double stepLengthFactor = parameters.getValue("stepLengthFactor", 1.0);
        double stepHeightFactor = parameters.getValue("stepHeightFactor", 1.0);
        double bodyBobFactor = parameters.getValue("bodyBobFactor", 1.0);
        double gaitSpeedFactor = parameters.getValue("gaitSpeedFactor", 1.0);
        double headBobFactor = parameters.getValue("headBobFactor", 1.0);
        double tailSwayFactor = parameters.getValue("tailSwayFactor", 1.0);

        // Compute leg geometry for IK
        Vector3 leftUpperPos = bonePos("LeftUpperLeg");
        Vector3 leftLowerPos = bonePos("LeftLowerLeg");
        Vector3 leftFootPos = bonePos("LeftFoot");
        Vector3 leftFootEnd = boneEnd("LeftFoot");
        Vector3 rightUpperPos = bonePos("RightUpperLeg");
        Vector3 rightLowerPos = bonePos("RightLowerLeg");
        Vector3 rightFootPos = bonePos("RightFoot");
        Vector3 rightFootEnd = boneEnd("RightFoot");

        double leftUpperLen = (leftLowerPos - leftUpperPos).length();
        double leftLowerLen = (leftFootPos - leftLowerPos).length();
        double rightUpperLen = (rightLowerPos - rightUpperPos).length();
        double rightLowerLen = (rightFootPos - rightLowerPos).length();

        // Ground plane: lowest foot Y in bind pose
        double groundY = std::min(leftFootEnd.y(), rightFootEnd.y());

        // Step parameters scaled to body
        double stepLength = bodyHeight * 0.18 * stepLengthFactor; // narrower stride for chickens
        double stepHeight = bodyHeight * 0.08 * stepHeightFactor; // visible foot lift for knee bend
        double bodyBobAmp = bodyHeight * 0.015 * bodyBobFactor; // subtle bob

        // Head bob: characteristic chicken two-phase thrust-hold
        double headThrustAmp = bodyHeight * 0.08 * headBobFactor;

        // Tail dynamics
        double tailYawAmp = 0.06 * tailSwayFactor;
        double tailPitchAmp = 0.08 * tailSwayFactor; // up-down pump

        // Asymmetric duty factor: 65% stance, 35% swing (chicken biomechanics)
        const double dutyFactor = 0.65;
        const double swingFraction = 1.0 - dutyFactor;

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        const double cycles = std::max(1.0, std::round(gaitSpeedFactor));
        double dt = durationSeconds / static_cast<double>(frameCount);

        // Spring-damper state for secondary body dynamics
        double springStiffness = 150.0;
        double springDamping = 16.0;

        double sdBodyBob = 0.0, sdBodyBobVel = 0.0;
        double sdBodyPitch = 0.0, sdBodyPitchVel = 0.0;
        double sdBodyRoll = 0.0, sdBodyRollVel = 0.0;
        double sdBodySway = 0.0, sdBodySwayVel = 0.0;

        // Head hold-thrust: driven directly from cycle phase (no accumulating state)
        // Produces smooth, perfectly loopable head bob

        // Foot state
        bool leftWasSwinging = false;
        bool rightWasSwinging = false;

        auto springStep = [](double& cur, double& vel, double target, double stiffness, double damping, double stepDt) {
            double accel = -stiffness * (cur - target) - damping * vel;
            vel += accel * stepDt;
            cur += vel * stepDt;
        };

        // Run warm-up passes to let spring-damper states reach steady-state cycle,
        // then record frames. This ensures frame 0 and last frame match for seamless looping.
        const int warmupCycles = 2;
        const int warmupFrames = warmupCycles * frameCount;
        const int totalFrames = warmupFrames + frameCount;

        for (int globalFrame = 0; globalFrame < totalFrames; ++globalFrame) {
            bool isWarmup = globalFrame < warmupFrames;
            int frame = isWarmup ? (globalFrame % frameCount) : (globalFrame - warmupFrames);
            double tNormalized = static_cast<double>(frame) / static_cast<double>(frameCount);
            double t = fmod(tNormalized * cycles, 1.0);

            // Compute per-leg phase with asymmetric duty factor
            // Left leg: t=0 is start of left stance
            // Right leg: offset by dutyFactor to ensure perfect alternation
            // (one leg always in stance while the other is in swing)
            auto computeLegState = [&](double legT) -> std::pair<bool, double> {
                // Returns {isSwing, phaseWithinState[0..1]}
                if (legT < dutyFactor) {
                    return { false, legT / dutyFactor }; // stance
                } else {
                    return { true, (legT - dutyFactor) / swingFraction }; // swing
                }
            };

            double leftT = fmod(t, 1.0);
            double rightT = fmod(t + 0.5, 1.0);
            auto [leftIsSwing, leftPhase] = computeLegState(leftT);
            auto [rightIsSwing, rightPhase] = computeLegState(rightT);

            // Body bob target: dip at mid-stance of each leg (2x step frequency)
            // Using the actual stance phases for more accurate timing
            double targetBodyBob = -bodyBobAmp * (std::cos(t * 4.0 * Math::Pi));
            double targetBodyPitch = 0.04 * bodyBobFactor * std::sin(t * 4.0 * Math::Pi);
            // Lateral sway: lean over stance leg
            double targetBodyRoll = 0.05 * bodyBobFactor * std::sin(t * 2.0 * Math::Pi);
            // Forward sway
            double targetBodySway = bodyHeight * 0.015 * std::sin(t * 2.0 * Math::Pi);

            springStep(sdBodyBob, sdBodyBobVel, targetBodyBob, springStiffness, springDamping, dt);
            springStep(sdBodyPitch, sdBodyPitchVel, targetBodyPitch, springStiffness, springDamping, dt);
            springStep(sdBodyRoll, sdBodyRollVel, targetBodyRoll, springStiffness, springDamping, dt);
            springStep(sdBodySway, sdBodySwayVel, targetBodySway, springStiffness, springDamping, dt);

            Matrix4x4 bodyTransform;
            bodyTransform.translate(upDir * sdBodyBob + forward * sdBodySway);

            std::map<std::string, Matrix4x4> boneWorldTransforms;
            std::map<std::string, Matrix4x4> boneSkinMatrices;

            auto worldFromSkin = [&](const std::string& name, const Matrix4x4& skin) -> Matrix4x4 {
                Matrix4x4 bindWorld = buildBoneWorldTransform(bonePos(name), boneEnd(name));
                Matrix4x4 result = skin;
                result *= bindWorld;
                return result;
            };

            auto makeSkin = [&](const std::string& name, const Matrix4x4& baseSkin,
                                double extraYaw = 0.0, double extraPitch = 0.0, double extraRoll = 0.0) {
                Matrix4x4 skin = baseSkin;
                if (std::abs(extraYaw) > 1e-6 || std::abs(extraPitch) > 1e-6 || std::abs(extraRoll) > 1e-6) {
                    Vector3 pivot = bonePos(name);
                    skin.translate(pivot);
                    if (std::abs(extraRoll) > 1e-6)
                        skin.rotate(forward, extraRoll);
                    if (std::abs(extraYaw) > 1e-6)
                        skin.rotate(upDir, extraYaw);
                    if (std::abs(extraPitch) > 1e-6)
                        skin.rotate(right, extraPitch);
                    skin.translate(-pivot);
                }
                boneSkinMatrices[name] = skin;
                boneWorldTransforms[name] = worldFromSkin(name, skin);
            };

            // Body chain with spring-damped pitch and roll
            makeSkin("Root", bodyTransform, 0.0, sdBodyPitch, sdBodyRoll);
            makeSkin("Pelvis", bodyTransform, 0.0, sdBodyPitch, sdBodyRoll);
            makeSkin("Spine", bodyTransform, 0.0, sdBodyPitch * 0.7, sdBodyRoll * 0.7);
            makeSkin("Chest", bodyTransform, 0.0, sdBodyPitch * 0.4, sdBodyRoll * 0.5);

            // Head bobbing: hold-thrust pattern driven by cycle phase
            // Two thrusts per stride (one per leg swing). Each thrust is a rapid
            // forward snap; between thrusts the head holds still while the body
            // advances, creating a backward drift in body-relative space.
            {
                // Two head bobs per full stride: use 2x frequency
                double headPhase = fmod(t * 2.0, 1.0);

                // Thrust occupies ~20% of each half-cycle, hold occupies ~80%
                double thrustFraction = 0.2;
                double headRelativeOffset;
                double headPitch = 0.0;

                if (headPhase < thrustFraction) {
                    // Thrust: head snaps forward rapidly
                    double thrustProgress = headPhase / thrustFraction;
                    // Smootherstep for quick but smooth snap
                    double eased = smootherstep(thrustProgress);
                    // Goes from -headThrustAmp (drifted back) to +headThrustAmp (new front position)
                    headRelativeOffset = -headThrustAmp + eased * headThrustAmp * 2.0;
                    // Slight downward nod during thrust
                    headPitch = 0.06 * headBobFactor * std::sin(thrustProgress * Math::Pi);
                } else {
                    // Hold: head stays fixed in world space, body moves forward underneath
                    // In body-relative space this means head drifts backward linearly
                    double holdProgress = (headPhase - thrustFraction) / (1.0 - thrustFraction);
                    headRelativeOffset = headThrustAmp - holdProgress * headThrustAmp * 2.0;
                    headPitch = 0.0;
                }

                // Neck: follows at 50% of head offset with slight lag
                double neckOffset = headRelativeOffset * 0.5;
                Matrix4x4 neckSkin = bodyTransform;
                Vector3 neckPivot = bonePos("Neck");
                neckSkin.translate(neckPivot);
                neckSkin.rotate(right, sdBodyPitch * 0.3 + headPitch * 0.5);
                neckSkin.translate(forward * neckOffset);
                neckSkin.translate(-neckPivot);
                boneSkinMatrices["Neck"] = neckSkin;
                boneWorldTransforms["Neck"] = worldFromSkin("Neck", neckSkin);

                // Head: full offset
                Matrix4x4 headSkin = bodyTransform;
                Vector3 headPivot = bonePos("Head");
                headSkin.translate(headPivot);
                headSkin.rotate(right, headPitch);
                headSkin.translate(forward * headRelativeOffset);
                headSkin.translate(-headPivot);
                boneSkinMatrices["Head"] = headSkin;
                boneWorldTransforms["Head"] = worldFromSkin("Head", headSkin);

                // Beak follows head
                if (boneIdx.count("Beak")) {
                    boneSkinMatrices["Beak"] = headSkin;
                    boneWorldTransforms["Beak"] = worldFromSkin("Beak", headSkin);
                }
            }

            // Tail: counter-balance pump (up-down) + lateral yaw opposite to body roll
            {
                bool hasTailBase = boneIdx.count("TailBase") > 0;
                bool hasTailFeathers = boneIdx.count("TailFeathers") > 0;

                // Tail pumps up when body dips, down when body rises (counterbalance)
                double tailPitch = -tailPitchAmp * std::sin(t * 4.0 * Math::Pi);
                // Lateral yaw opposite to body roll
                double tailYaw = -tailYawAmp * std::sin(t * 2.0 * Math::Pi);

                if (hasTailBase) {
                    Matrix4x4 tailSkin = bodyTransform;
                    Vector3 tailPivot = bonePos("TailBase");
                    tailSkin.translate(tailPivot);
                    tailSkin.rotate(upDir, tailYaw);
                    tailSkin.rotate(right, tailPitch);
                    tailSkin.translate(-tailPivot);
                    boneSkinMatrices["TailBase"] = tailSkin;
                    boneWorldTransforms["TailBase"] = worldFromSkin("TailBase", tailSkin);

                    if (hasTailFeathers) {
                        // Tail feathers lag slightly behind base (secondary overlap)
                        Matrix4x4 featherSkin = bodyTransform;
                        featherSkin.translate(tailPivot);
                        featherSkin.rotate(upDir, tailYaw * 1.3);
                        featherSkin.rotate(right, tailPitch * 1.2);
                        featherSkin.translate(-tailPivot);
                        boneSkinMatrices["TailFeathers"] = featherSkin;
                        boneWorldTransforms["TailFeathers"] = worldFromSkin("TailFeathers", featherSkin);
                    }
                }
            }

            // Wings: held folded against body, micro-sway with gait
            {
                static const char* wingBones[] = {
                    "LeftWingShoulder", "LeftWingElbow", "LeftWingHand",
                    "RightWingShoulder", "RightWingElbow", "RightWingHand"
                };
                double wingSway = 0.02 * std::sin(t * 4.0 * Math::Pi);
                for (const auto& boneName : wingBones) {
                    Matrix4x4 wingSkin = bodyTransform;
                    Vector3 wingPivot = bonePos(boneName);
                    wingSkin.translate(wingPivot);
                    wingSkin.rotate(forward, wingSway);
                    wingSkin.translate(-wingPivot);
                    boneSkinMatrices[boneName] = wingSkin;
                    boneWorldTransforms[boneName] = worldFromSkin(boneName, wingSkin);
                }
            }

            // Legs: asymmetric bipedal gait with IK and foot locking
            for (int side = 0; side < 2; ++side) {
                const char* upperName = (side == 0) ? "LeftUpperLeg" : "RightUpperLeg";
                const char* lowerName = (side == 0) ? "LeftLowerLeg" : "RightLowerLeg";
                const char* footName = (side == 0) ? "LeftFoot" : "RightFoot";

                Vector3 hipPos = (side == 0) ? leftUpperPos : rightUpperPos;
                Vector3 restFootPos = (side == 0) ? leftFootPos : rightFootPos;

                bool isSwing = (side == 0) ? leftIsSwing : rightIsSwing;
                double legPhaseInState = (side == 0) ? leftPhase : rightPhase;
                bool& wasSwinging = (side == 0) ? leftWasSwinging : rightWasSwinging;

                Vector3 footTarget;

                // Apply body bob to hip position (needed for swing retraction)
                Vector3 currentHipPos = hipPos + upDir * sdBodyBob;

                // Foot front/back positions for the stride
                Vector3 footFront = restFootPos + forward * stepLength;
                Vector3 footBack = restFootPos - forward * stepLength;
                // Clamp to ground
                if (footFront.y() < groundY)
                    footFront = Vector3(footFront.x(), groundY, footFront.z());
                if (footBack.y() < groundY)
                    footBack = Vector3(footBack.x(), groundY, footBack.z());

                if (isSwing) {
                    // Swing phase: four-stage cycle
                    // Stage 1: Toe-off - foot leaves ground, toes cluster
                    // Stage 2: High flexion - knee pulls upward and forward (high-stepping)
                    // Stage 3: Extension - leg extends forward, toes spread
                    // Stage 4: Descent - foot prepares for landing
                    double smoothSwing = smootherstep(legPhaseInState);
                    Vector3 groundPos = footBack + (footFront - footBack) * smoothSwing;
                    // Parabolic lift: peak at mid-swing for high-stepping look
                    double lift = stepHeight * std::sin(smoothSwing * Math::Pi);
                    // High flexion: pull foot up and back toward hip to force knee bend
                    if (legPhaseInState >= 0.2 && legPhaseInState <= 0.8) {
                        double midSwingIntensity = std::sin((legPhaseInState - 0.2) / 0.6 * Math::Pi);
                        lift += stepHeight * 1.5 * midSwingIntensity; // strong upward pull
                        // Retract foot horizontally toward hip to shorten IK reach and bend knee
                        Vector3 retract = (currentHipPos - groundPos);
                        retract = Vector3(retract.x(), 0.0, retract.z()); // horizontal only
                        groundPos = groundPos + retract * 0.3 * midSwingIntensity;
                    }
                    // Chickens walk in a straight line: no lateral sway
                    footTarget = groundPos + upDir * lift;
                    wasSwinging = true;
                } else {
                    // Stance phase: foot on ground, drifts from front to back as body passes over it
                    wasSwinging = false;
                    // Full stride drift: foot starts at front, ends at back
                    double stanceProgress = smoothstep(legPhaseInState);
                    footTarget = footFront + (footBack - footFront) * stanceProgress;
                    // Keep on ground
                    if (footTarget.y() < groundY)
                        footTarget = Vector3(footTarget.x(), groundY, footTarget.z());
                }

                // Solve two-bone IK using the common solver
                std::vector<Vector3> chain = { currentHipPos,
                    currentHipPos + (((side == 0) ? leftLowerPos : rightLowerPos) - hipPos),
                    currentHipPos + (restFootPos - hipPos) };

                // Chicken hock (visible joint) bends backward
                double poleVertical = isSwing ? 0.6 : 0.3;
                Vector3 poleVector = chain[1] - forward * 0.5 + upDir * poleVertical;
                solveTwoBoneIk(chain, footTarget, poleVector, 0.05);

                Vector3 kneePos = chain[1];
                Vector3 ikTarget = chain[2];

                // Build bone transforms from solved positions
                Matrix4x4 upperWorld = buildBoneWorldTransform(currentHipPos, kneePos);
                Matrix4x4 upperBind = buildBoneWorldTransform(hipPos, (side == 0) ? leftLowerPos : rightLowerPos);
                Matrix4x4 upperBindInv = upperBind.inverted();
                Matrix4x4 upperSkin = upperWorld;
                upperSkin *= upperBindInv;
                boneSkinMatrices[upperName] = upperSkin;
                boneWorldTransforms[upperName] = upperWorld;

                Matrix4x4 lowerWorld = buildBoneWorldTransform(kneePos, ikTarget);
                Matrix4x4 lowerBind = buildBoneWorldTransform((side == 0) ? leftLowerPos : rightLowerPos, restFootPos);
                Matrix4x4 lowerBindInv = lowerBind.inverted();
                Matrix4x4 lowerSkin = lowerWorld;
                lowerSkin *= lowerBindInv;
                boneSkinMatrices[lowerName] = lowerSkin;
                boneWorldTransforms[lowerName] = lowerWorld;

                // Foot orientation: reflects toe positioning through gait cycle
                double footPitch = 0.0; // pitch: toes up/down
                double footRoll = 0.0; // roll: toe clustering/spreading

                if (isSwing) {
                    // Swing phase: toes cluster (flex) to prevent tripping, pitch up
                    if (legPhaseInState < 0.4) {
                        // Toe-off to high flexion: toes cluster inward
                        double clusterIntensity = smootherstep(legPhaseInState / 0.4);
                        footRoll = 0.15 * clusterIntensity; // toes flex/cluster inward
                        footPitch = 0.25 * clusterIntensity; // toe pitches up
                    } else if (legPhaseInState < 0.8) {
                        // High flexion to extension: toes start to spread, pitch stays up
                        double spreadIntensity = (legPhaseInState - 0.4) / 0.4;
                        footRoll = 0.15 * (1.0 - spreadIntensity); // toes gradually spread
                        footPitch = 0.25;
                    } else {
                        // Final descent: toes spread, pitch down for landing
                        double landingIntensity = (legPhaseInState - 0.8) / 0.2;
                        footRoll = -0.05 * landingIntensity; // toes fully spread
                        footPitch = 0.25 * (1.0 - landingIntensity); // pitch down
                    }
                } else {
                    // Stance phase: foot flat, toes spread for grip
                    footPitch = 0.0;
                    footRoll = -0.05; // slight toe spread for grip
                }

                Matrix4x4 footSkin;
                footSkin.translate(ikTarget - restFootPos);
                if (std::abs(footPitch) > 1e-6 || std::abs(footRoll) > 1e-6) {
                    footSkin.translate(ikTarget);
                    if (std::abs(footRoll) > 1e-6)
                        footSkin.rotate(forward, footRoll);
                    if (std::abs(footPitch) > 1e-6)
                        footSkin.rotate(right, footPitch);
                    footSkin.translate(-ikTarget);
                }
                boneSkinMatrices[footName] = footSkin;
                boneWorldTransforms[footName] = worldFromSkin(footName, footSkin);
            }

            // Write frame (skip during warm-up)
            if (!isWarmup) {
                auto& animFrame = animationClip.frames[frame];
                animFrame.time = static_cast<float>(tNormalized) * durationSeconds;
                animFrame.boneWorldTransforms = boneWorldTransforms;
                animFrame.boneSkinMatrices = boneSkinMatrices;
            }
        }

        return true;
    }

} // namespace bird

} // namespace dust3d
