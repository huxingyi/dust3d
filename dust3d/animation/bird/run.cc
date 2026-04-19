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

// Procedural run animation for bird rig (chicken, ground-bird, etc.).
//
// Key biomechanical differences from walk:
//
//   Gait timing:
//     - Running gait has an aerial phase where both feet are off the ground.
//     - Duty factor ~40% stance, ~60% swing (inverted from walk).
//     - Left/right legs offset by half a stride cycle.
//
//   Body dynamics:
//     - Greater forward lean (pitched forward for speed).
//     - Larger vertical oscillation (body launches during aerial phase).
//     - More aggressive lateral sway for balance at speed.
//     - Spring-damper integration for organic motion.
//
//   Head bobbing:
//     - Faster thrust-hold cycle matching increased stride frequency.
//     - Larger amplitude to compensate for faster body movement.
//
//   Legs:
//     - Longer stride length, higher foot lift.
//     - More aggressive knee flexion during swing for ground clearance.
//     - Stronger toe-off push.
//
//   Tail:
//     - Larger counterbalance motion to compensate for increased body dynamics.
//
//   Wings:
//     - More pronounced sway/flapping for balance at speed.
//
// Adjustable parameters:
//   - stepLengthFactor:   stride length multiplier
//   - stepHeightFactor:   foot lift height multiplier
//   - bodyBobFactor:      vertical body oscillation intensity
//   - gaitSpeedFactor:    gait cycle speed (number of full strides per clip)
//   - headBobFactor:      head thrust amplitude
//   - tailSwayFactor:     tail lateral sway amplitude
//   - wingFlapFactor:     wing flap intensity during run

#include <cmath>
#include <dust3d/animation/bird/run.h>
#include <dust3d/animation/common.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace bird {

    bool run(const RigStructure& rigStructure,
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
        double wingFlapFactor = parameters.getValue("wingFlapFactor", 1.0);

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

        // Run: longer strides, higher steps, more body bob than walk
        double stepLength = bodyHeight * 0.30 * stepLengthFactor;
        double stepHeight = bodyHeight * 0.14 * stepHeightFactor;
        double bodyBobAmp = bodyHeight * 0.025 * bodyBobFactor;

        // Head bob: faster and larger for running
        double headThrustAmp = bodyHeight * 0.10 * headBobFactor;

        // Tail dynamics: stronger counterbalance
        double tailYawAmp = 0.10 * tailSwayFactor;
        double tailPitchAmp = 0.12 * tailSwayFactor;

        // Wing flap amplitude for balance
        double wingFlapAmp = 0.12 * wingFlapFactor;

        // Running duty factor: 40% stance, 60% swing (aerial phase exists)
        const double dutyFactor = 0.40;
        const double swingFraction = 1.0 - dutyFactor;

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        const double cycles = std::max(1.0, std::round(gaitSpeedFactor));
        double dt = durationSeconds / static_cast<double>(frameCount);

        // Spring-damper state for secondary body dynamics
        double springStiffness = 180.0; // stiffer for faster motion
        double springDamping = 18.0;

        double sdBodyBob = 0.0, sdBodyBobVel = 0.0;
        double sdBodyPitch = 0.0, sdBodyPitchVel = 0.0;
        double sdBodyRoll = 0.0, sdBodyRollVel = 0.0;
        double sdBodySway = 0.0, sdBodySwayVel = 0.0;

        // Foot state
        bool leftWasSwinging = false;
        bool rightWasSwinging = false;

        auto springStep = [](double& cur, double& vel, double target, double stiffness, double damping, double stepDt) {
            double accel = -stiffness * (cur - target) - damping * vel;
            vel += accel * stepDt;
            cur += vel * stepDt;
        };

        // Warm-up passes for spring-damper steady state
        const int warmupCycles = 2;
        const int warmupFrames = warmupCycles * frameCount;
        const int totalFrames = warmupFrames + frameCount;

        for (int globalFrame = 0; globalFrame < totalFrames; ++globalFrame) {
            bool isWarmup = globalFrame < warmupFrames;
            int frame = isWarmup ? (globalFrame % frameCount) : (globalFrame - warmupFrames);
            double tNormalized = static_cast<double>(frame) / static_cast<double>(frameCount);
            double t = fmod(tNormalized * cycles, 1.0);

            // Compute per-leg phase with running duty factor
            auto computeLegState = [&](double legT) -> std::pair<bool, double> {
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

            // Aerial phase detection: both legs in swing
            bool isAerial = leftIsSwing && rightIsSwing;

            // Body bob: larger amplitude, with extra lift during aerial phase
            double targetBodyBob = -bodyBobAmp * std::cos(t * 4.0 * Math::Pi);
            if (isAerial) {
                targetBodyBob += bodyBobAmp * 0.8; // extra upward during aerial phase
            }
            // Forward lean: birds lean forward more when running
            double forwardLean = 0.10 * bodyBobFactor;
            double targetBodyPitch = forwardLean + 0.06 * bodyBobFactor * std::sin(t * 4.0 * Math::Pi);
            // Lateral sway: more aggressive for balance at speed
            double targetBodyRoll = 0.08 * bodyBobFactor * std::sin(t * 2.0 * Math::Pi);
            // Forward sway
            double targetBodySway = bodyHeight * 0.025 * std::sin(t * 2.0 * Math::Pi);

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

            // Head bobbing: faster thrust-hold for running
            {
                double headPhase = fmod(t * 2.0, 1.0);

                // Thrust occupies ~25% of each half-cycle (faster snap for running)
                double thrustFraction = 0.25;
                double headRelativeOffset;
                double headPitch = 0.0;

                if (headPhase < thrustFraction) {
                    double thrustProgress = headPhase / thrustFraction;
                    double eased = smootherstep(thrustProgress);
                    headRelativeOffset = -headThrustAmp + eased * headThrustAmp * 2.0;
                    // Stronger downward nod during thrust
                    headPitch = 0.08 * headBobFactor * std::sin(thrustProgress * Math::Pi);
                } else {
                    double holdProgress = (headPhase - thrustFraction) / (1.0 - thrustFraction);
                    headRelativeOffset = headThrustAmp - holdProgress * headThrustAmp * 2.0;
                    headPitch = 0.0;
                }

                // Neck
                double neckOffset = headRelativeOffset * 0.5;
                Matrix4x4 neckSkin = bodyTransform;
                Vector3 neckPivot = bonePos("Neck");
                neckSkin.translate(neckPivot);
                neckSkin.rotate(right, sdBodyPitch * 0.3 + headPitch * 0.5);
                neckSkin.translate(forward * neckOffset);
                neckSkin.translate(-neckPivot);
                boneSkinMatrices["Neck"] = neckSkin;
                boneWorldTransforms["Neck"] = worldFromSkin("Neck", neckSkin);

                // Head
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

            // Tail: stronger counterbalance for running
            {
                bool hasTailBase = boneIdx.count("TailBase") > 0;
                bool hasTailFeathers = boneIdx.count("TailFeathers") > 0;

                double tailPitch = -tailPitchAmp * std::sin(t * 4.0 * Math::Pi);
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

            // Wings: more pronounced flapping for balance while running
            {
                // Wings flap at 2x stride frequency; stronger during aerial phase
                double flapPhase = t * 4.0 * Math::Pi;
                double flapBase = wingFlapAmp * std::sin(flapPhase);
                if (isAerial) {
                    flapBase *= 1.5; // more vigorous flapping during aerial phase
                }
                static const char* wingChains[2][3] = {
                    { "LeftWingShoulder", "LeftWingElbow", "LeftWingHand" },
                    { "RightWingShoulder", "RightWingElbow", "RightWingHand" }
                };
                for (int side = 0; side < 2; ++side) {
                    const char* shoulderName = wingChains[side][0];
                    const char* elbowName = wingChains[side][1];
                    const char* handName = wingChains[side][2];
                    bool isLeft = (side == 0);
                    double flap = isLeft ? flapBase : -flapBase;

                    Vector3 shoulderPos0 = bonePos(shoulderName);
                    Vector3 shoulderEnd0 = boneEnd(shoulderName);
                    Vector3 elbowPos0 = bonePos(elbowName);
                    Vector3 elbowEnd0 = boneEnd(elbowName);
                    Vector3 handPos0 = bonePos(handName);
                    Vector3 handEnd0 = boneEnd(handName);

                    // Shoulder: rotate with base flap amount
                    double shoulderFlap = flap * 1.0;
                    Matrix4x4 shoulderSkin = bodyTransform;
                    shoulderSkin.translate(shoulderPos0);
                    shoulderSkin.rotate(forward, shoulderFlap);
                    shoulderSkin.translate(-shoulderPos0);
                    boneSkinMatrices[shoulderName] = shoulderSkin;
                    boneWorldTransforms[shoulderName] = worldFromSkin(shoulderName, shoulderSkin);

                    // Compute shoulder's transformed end position
                    Vector3 shoulderEndWorld = shoulderSkin * shoulderEnd0;

                    // Elbow: starts at shoulder's transformed end, more flap (secondary overlap)
                    double elbowFlap = flap * 1.3;
                    Vector3 elbowDir = elbowEnd0 - elbowPos0;
                    Matrix4x4 elbowRot;
                    elbowRot.rotate(forward, elbowFlap);
                    elbowDir = elbowRot * elbowDir;
                    Vector3 elbowEndWorld = shoulderEndWorld + elbowDir;
                    Matrix4x4 elbowWorld = buildBoneWorldTransform(shoulderEndWorld, elbowEndWorld);
                    Matrix4x4 elbowBind = buildBoneWorldTransform(elbowPos0, elbowEnd0);
                    Matrix4x4 elbowSkin = elbowWorld;
                    elbowSkin *= elbowBind.inverted();
                    boneSkinMatrices[elbowName] = elbowSkin;
                    boneWorldTransforms[elbowName] = elbowWorld;

                    // Hand: starts at elbow's transformed end, most flap
                    double handFlap = flap * 1.6;
                    Vector3 handDir = handEnd0 - handPos0;
                    Matrix4x4 handRot;
                    handRot.rotate(forward, handFlap);
                    handDir = handRot * handDir;
                    Vector3 handEndWorld = elbowEndWorld + handDir;
                    Matrix4x4 handWorld = buildBoneWorldTransform(elbowEndWorld, handEndWorld);
                    Matrix4x4 handBind = buildBoneWorldTransform(handPos0, handEnd0);
                    Matrix4x4 handSkin = handWorld;
                    handSkin *= handBind.inverted();
                    boneSkinMatrices[handName] = handSkin;
                    boneWorldTransforms[handName] = handWorld;
                }
            }

            // Legs: running gait with IK, aerial phase, and aggressive knee flexion
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

                Vector3 currentHipPos = hipPos + upDir * sdBodyBob;

                Vector3 footFront = restFootPos + forward * stepLength;
                Vector3 footBack = restFootPos - forward * stepLength;
                if (footFront.y() < groundY)
                    footFront = Vector3(footFront.x(), groundY, footFront.z());
                if (footBack.y() < groundY)
                    footBack = Vector3(footBack.x(), groundY, footBack.z());

                if (isSwing) {
                    // Running swing: more aggressive high-stepping with longer reach
                    double smoothSwing = smootherstep(legPhaseInState);
                    Vector3 groundPos = footBack + (footFront - footBack) * smoothSwing;
                    // Higher parabolic lift for running
                    double lift = stepHeight * std::sin(smoothSwing * Math::Pi);
                    // More aggressive knee flexion during mid-swing
                    if (legPhaseInState >= 0.15 && legPhaseInState <= 0.75) {
                        double midSwingIntensity = std::sin((legPhaseInState - 0.15) / 0.6 * Math::Pi);
                        lift += stepHeight * 2.0 * midSwingIntensity; // stronger upward pull
                        Vector3 retract = (currentHipPos - groundPos);
                        retract = Vector3(retract.x(), 0.0, retract.z());
                        groundPos = groundPos + retract * 0.35 * midSwingIntensity;
                    }
                    footTarget = groundPos + upDir * lift;
                    wasSwinging = true;
                } else {
                    // Stance phase: foot on ground, aggressive push-off
                    wasSwinging = false;
                    double stanceProgress = smoothstep(legPhaseInState);
                    footTarget = footFront + (footBack - footFront) * stanceProgress;
                    // Push-off: slight upward at end of stance for toe-off energy
                    if (legPhaseInState > 0.7) {
                        double pushOff = (legPhaseInState - 0.7) / 0.3;
                        footTarget = footTarget + upDir * (stepHeight * 0.2 * smoothstep(pushOff));
                    }
                    if (footTarget.y() < groundY)
                        footTarget = Vector3(footTarget.x(), groundY, footTarget.z());
                }

                // Solve two-bone IK
                std::vector<Vector3> chain = { currentHipPos,
                    currentHipPos + (((side == 0) ? leftLowerPos : rightLowerPos) - hipPos),
                    currentHipPos + (restFootPos - hipPos) };

                double poleVertical = isSwing ? 0.7 : 0.35;
                Vector3 poleVector = chain[1] - forward * 0.5 + upDir * poleVertical;
                solveTwoBoneIk(chain, footTarget, poleVector, 0.05);

                Vector3 kneePos = chain[1];
                Vector3 ikTarget = chain[2];

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

                // Foot orientation
                double footPitch = 0.0;
                double footRoll = 0.0;

                if (isSwing) {
                    if (legPhaseInState < 0.35) {
                        // Toe-off to high flexion: aggressive toe clustering
                        double clusterIntensity = smootherstep(legPhaseInState / 0.35);
                        footRoll = 0.20 * clusterIntensity;
                        footPitch = 0.30 * clusterIntensity;
                    } else if (legPhaseInState < 0.75) {
                        // High flexion to extension
                        double spreadIntensity = (legPhaseInState - 0.35) / 0.4;
                        footRoll = 0.20 * (1.0 - spreadIntensity);
                        footPitch = 0.30;
                    } else {
                        // Final descent: prepare for landing
                        double landingIntensity = (legPhaseInState - 0.75) / 0.25;
                        footRoll = -0.08 * landingIntensity;
                        footPitch = 0.30 * (1.0 - landingIntensity);
                    }
                } else {
                    // Stance: foot flat, toes spread
                    footPitch = 0.0;
                    footRoll = -0.06;
                    // Push-off toe flex at end of stance
                    if (legPhaseInState > 0.7) {
                        double pushOff = (legPhaseInState - 0.7) / 0.3;
                        footPitch = 0.15 * smoothstep(pushOff);
                    }
                }

                // Build foot world transform anchored at ikTarget (lower leg end)
                // to maintain bone chain connectivity
                Vector3 restFootEnd = (side == 0) ? leftFootEnd : rightFootEnd;
                Vector3 footDir = restFootEnd - restFootPos;
                // Apply foot pitch/roll rotations to the foot direction
                if (std::abs(footPitch) > 1e-6) {
                    Matrix4x4 pitchRot;
                    pitchRot.rotate(right, footPitch);
                    footDir = pitchRot * footDir;
                }
                if (std::abs(footRoll) > 1e-6) {
                    Matrix4x4 rollRot;
                    rollRot.rotate(forward, footRoll);
                    footDir = rollRot * footDir;
                }
                Vector3 footEndPos = ikTarget + footDir;
                Matrix4x4 footWorld = buildBoneWorldTransform(ikTarget, footEndPos);
                Matrix4x4 footBind = buildBoneWorldTransform(restFootPos, restFootEnd);
                Matrix4x4 footBindInv = footBind.inverted();
                Matrix4x4 footSkin = footWorld;
                footSkin *= footBindInv;
                boneSkinMatrices[footName] = footSkin;
                boneWorldTransforms[footName] = footWorld;
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
