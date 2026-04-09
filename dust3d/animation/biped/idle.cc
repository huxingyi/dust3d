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

// Procedural idle animation for biped rig.
//
// Designed for game development idle/rest state. The character stands in
// place with subtle breathing motion, weight shifting, and minor
// secondary movements. Works for humanoids, robots, dinosaurs, penguins,
// and any creature using the biped bone structure.
//
// Uses layered sinusoidal motion (sum of incommensurate frequencies) to
// break robotic periodicity, contra-posto weight shifting with hip tilt
// and shoulder counter-rotation, and breathing with lateral chest
// expansion for a more lifelike idle.
//
// Adjustable animation parameters:
//   - breathingAmplitudeFactor:   chest/torso rise-fall intensity
//   - breathingSpeedFactor:       breathing cycle speed multiplier
//   - weightShiftFactor:    lateral hip sway (weight shift side to side)
//   - weightShiftSpeedFactor:     speed of weight shifting cycle
//   - headLookFactor:       subtle head turn/nod amplitude
//   - armRestFactor:        slight arm sway while resting
//   - spineSwayFactor:      subtle spine lateral sway
//   - tailIdleFactor:       tail gentle sway amplitude (if tail bones exist)

#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/biped/idle.h>
#include <dust3d/animation/common.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace biped {

    bool idle(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = static_cast<int>(parameters.getValue("frameCount", 90));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 4.0));

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
        // 1. Idle parameters
        // ===================================================================
        double breathingAmplitudeFactor = parameters.getValue("breathingAmplitudeFactor", 1.0);
        double breathingSpeedFactor = parameters.getValue("breathingSpeedFactor", 1.0);
        double weightShiftFactor = parameters.getValue("weightShiftFactor", 1.0);
        double weightShiftSpeedFactor = parameters.getValue("weightShiftSpeedFactor", 1.0);
        double headLookFactor = parameters.getValue("headLookFactor", 1.0);
        double armRestFactor = parameters.getValue("armRestFactor", 1.0);
        double spineSwayFactor = parameters.getValue("spineSwayFactor", 1.0);
        double tailIdleFactor = parameters.getValue("tailIdleFactor", 1.0);

        // Body reference
        Vector3 hipsPos = bonePos("Hips");
        Vector3 chestPos = bonePos("Chest");
        Vector3 upDir(0.0, 1.0, 0.0);

        // Derive forward and right from foot positions
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

        // Breathing amplitudes scaled to body
        double breathAmp = avgLegLength * 0.008 * breathingAmplitudeFactor;
        double weightShiftAmp = avgLegLength * 0.012 * weightShiftFactor;

        // ===================================================================
        // 2. Generate frames — layered sinusoidal motion
        // ===================================================================
        // Use incommensurate frequency ratios (based on golden ratio) so
        // the motion never repeats exactly within a single loop, giving a
        // more organic/living feel than single-frequency sine waves.
        // ===================================================================
        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        const double phi = 1.6180339887; // golden ratio

        for (int frame = 0; frame < frameCount; ++frame) {
            double tNormalized = static_cast<double>(frame) / static_cast<double>(frameCount);
            double tRadians = tNormalized * 2.0 * Math::Pi;

            // -----------------------------------------------------------
            // Layered breathing: primary + secondary harmonic for organic
            // chest rise/fall. Secondary is at an incommensurate frequency.
            // -----------------------------------------------------------
            double breathPhase1 = tRadians * breathingSpeedFactor;
            double breathPhase2 = tRadians * breathingSpeedFactor * phi;
            double breathOffset = breathAmp * (0.7 * std::sin(breathPhase1) + 0.3 * std::sin(breathPhase2));

            // -----------------------------------------------------------
            // Weight shift with contra-posto: layered lateral sway
            // -----------------------------------------------------------
            double shiftPhase1 = tRadians * weightShiftSpeedFactor * 0.5;
            double shiftPhase2 = tRadians * weightShiftSpeedFactor * 0.5 * phi;
            double lateralShift = weightShiftAmp * (0.75 * std::sin(shiftPhase1) + 0.25 * std::sin(shiftPhase2));

            // Hip tilt: when weight shifts left, left hip drops
            double hipTilt = 0.025 * weightShiftFactor * std::sin(shiftPhase1);
            // Shoulder counter-tilt (contra-posto): opposite to hip tilt
            double shoulderTilt = -hipTilt * 0.6;
            // Hip yaw rotation: hips rotate slightly toward weight-bearing side
            double hipYaw = 0.018 * weightShiftFactor * std::sin(shiftPhase1 + 0.2);

            // -----------------------------------------------------------
            // Spine sway: layered for organic feel
            // -----------------------------------------------------------
            double spineSway = spineSwayFactor * (0.012 * std::sin(shiftPhase1 + 0.3) + 0.005 * std::sin(tRadians * 1.7 + 1.0));

            // -----------------------------------------------------------
            // Head look: layered yaw and pitch at different frequencies
            // to simulate subtle looking around
            // -----------------------------------------------------------
            double headYaw = headLookFactor * (0.025 * std::sin(tRadians * 0.7) + 0.012 * std::sin(tRadians * 0.7 * phi + 0.8));
            double headPitch = headLookFactor * (0.008 * std::sin(tRadians * 1.1 + 0.5) + 0.005 * std::sin(tRadians * 1.1 * phi + 1.2));
            // Head stabilization: counter the body sway so gaze stays level
            double headCounterTilt = -(hipTilt + shoulderTilt) * 0.5;

            // -----------------------------------------------------------
            // Body transform: breathing lift + weight shift
            // -----------------------------------------------------------
            Matrix4x4 bodyTransform;
            bodyTransform.translate(upDir * breathOffset + right * lateralShift);

            // Hip transform includes tilt and yaw
            Matrix4x4 hipTransform = bodyTransform;
            Matrix4x4 hipTiltMat;
            hipTiltMat.rotate(forward, hipTilt);
            Matrix4x4 hipYawMat;
            hipYawMat.rotate(upDir, hipYaw);

            std::map<std::string, Matrix4x4> boneWorldTransforms;

            auto computeBodyBone = [&](const std::string& name,
                                       double extraYaw = 0.0,
                                       double extraPitch = 0.0,
                                       double extraRoll = 0.0) {
                Vector3 pos = bonePos(name);
                Vector3 end = boneEnd(name);
                Vector3 newPos = bodyTransform.transformPoint(pos);
                Vector3 newEnd = bodyTransform.transformPoint(end);
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
            };

            computeBodyBone("Root");
            // Hips: weight shift tilt + subtle yaw rotation (contra-posto)
            computeBodyBone("Hips", hipYaw, 0.0, hipTilt);
            // Spine: lateral sway + counter-rotation against hips
            computeBodyBone("Spine", spineSway - hipYaw * 0.3, 0.0, -hipTilt * 0.3);
            // Chest: breathing pitch + shoulder counter-tilt + chest expansion
            double chestPitch = breathOffset * 0.3 / (avgLegLength * 0.01 + 1e-6);
            computeBodyBone("Chest", spineSway * 0.5 - hipYaw * 0.5, chestPitch, shoulderTilt);
            // Neck: counter-sway + partial head pitch
            computeBodyBone("Neck", -spineSway * 0.3 + hipYaw * 0.2, headPitch * 0.3, headCounterTilt * 0.5);
            // Head: look around + stabilization counter-tilt
            computeBodyBone("Head", headYaw, headPitch, headCounterTilt);

            // Tail idle sway: layered
            static const char* tailBones[] = { "TailBase", "TailMid", "TailTip" };
            double tailPhase1 = tRadians * 0.8;
            double tailPhase2 = tRadians * 0.8 * phi;
            for (int ti = 0; ti < 3; ++ti) {
                if (boneIdx.count(tailBones[ti]) == 0)
                    continue;
                double attenuation = 1.0 - ti * 0.28;
                double tailAngle = tailIdleFactor * attenuation
                    * (0.03 * std::sin(tailPhase1 - ti * 0.6)
                        + 0.015 * std::sin(tailPhase2 - ti * 0.9));
                computeBodyBone(tailBones[ti], tailAngle);
            }

            // -----------------------------------------------------------
            // Legs: feet stay planted, use two-bone analytic IK to
            // connect the moved hips to the fixed foot positions.
            // Weight-bearing leg gets subtle extra knee bend.
            // -----------------------------------------------------------
            double weightOnLeft = -lateralShift / (weightShiftAmp + 1e-8); // -1..+1
            double weightOnRight = -weightOnLeft;

            auto computeLegPlanted = [&](const char* upperLegName, const char* lowerLegName,
                                         const char* footName, double weightFraction) {
                // Foot stays at bind pose (no body transform applied)
                Vector3 footStart = bonePos(footName);
                Vector3 footEnd = boneEnd(footName);
                boneWorldTransforms[footName] = buildBoneWorldTransform(footStart, footEnd);

                // Upper leg origin follows the body (hips moved)
                Vector3 hipJoint = bodyTransform.transformPoint(bonePos(upperLegName));
                double upperLen = (boneEnd(upperLegName) - bonePos(upperLegName)).length();
                double lowerLen = (boneEnd(lowerLegName) - bonePos(lowerLegName)).length();

                // Target: knee must reach from hipJoint to footStart
                Vector3 toFoot = footStart - hipJoint;
                double dist = toFoot.length();
                double totalLen = upperLen + lowerLen;
                if (dist < 1e-6)
                    dist = 1e-6;
                if (dist > totalLen * 0.999)
                    dist = totalLen * 0.999;

                // Two-bone IK: law of cosines for knee angle
                double cosKnee = (upperLen * upperLen + lowerLen * lowerLen - dist * dist) / (2.0 * upperLen * lowerLen);
                cosKnee = std::max(-1.0, std::min(1.0, cosKnee));

                // Angle at hip
                double cosHip = (upperLen * upperLen + dist * dist - lowerLen * lowerLen) / (2.0 * upperLen * dist);
                cosHip = std::max(-1.0, std::min(1.0, cosHip));
                double hipAngle = std::acos(cosHip);

                // Weight-bearing knee flex: slightly more bend
                double extraFlex = 0.015 * std::max(0.0, weightFraction) * weightShiftFactor;
                hipAngle += extraFlex;

                // Build upper leg direction: rotate toFoot by hipAngle toward forward (knee hint)
                Vector3 toFootDir = toFoot;
                toFootDir.normalize();
                // Knee hint direction: forward of character
                Vector3 kneeHint = forward;
                Vector3 bendAxis = Vector3::crossProduct(toFootDir, kneeHint);
                if (bendAxis.lengthSquared() < 1e-8)
                    bendAxis = right;
                bendAxis.normalize();

                Matrix4x4 hipRot;
                hipRot.rotate(bendAxis, hipAngle);
                Vector3 upperDir = hipRot.transformVector(toFootDir);
                Vector3 kneePos = hipJoint + upperDir * upperLen;

                boneWorldTransforms[upperLegName] = buildBoneWorldTransform(hipJoint, kneePos);
                boneWorldTransforms[lowerLegName] = buildBoneWorldTransform(kneePos, footStart);
            };

            computeLegPlanted("LeftUpperLeg", "LeftLowerLeg", "LeftFoot", weightOnLeft);
            computeLegPlanted("RightUpperLeg", "RightLowerLeg", "RightFoot", weightOnRight);

            // -----------------------------------------------------------
            // Arms: breathing-coupled sway + gravity drape
            // Each arm sways slightly with breathing and has a secondary
            // frequency for organic feel. Counter-swing with shoulders.
            // -----------------------------------------------------------
            auto computeArmIdle = [&](const char* shoulderName, const char* upperArmName,
                                      const char* lowerArmName, const char* handName,
                                      double phase, double sideShoulderTilt) {
                // Layered arm sway
                double armSway = armRestFactor * (0.018 * std::sin(breathPhase1 + phase) + 0.008 * std::sin(breathPhase1 * phi + phase + 0.5));

                Vector3 shoulderPos = bodyTransform.transformPoint(bonePos(shoulderName));
                Vector3 shoulderEnd = bodyTransform.transformPoint(boneEnd(shoulderName));
                // Apply shoulder tilt from contra-posto
                if (std::abs(sideShoulderTilt) > 1e-6) {
                    Matrix4x4 sTilt;
                    sTilt.rotate(forward, sideShoulderTilt);
                    Vector3 sOff = shoulderEnd - shoulderPos;
                    shoulderEnd = shoulderPos + sTilt.transformVector(sOff);
                }
                boneWorldTransforms[shoulderName] = buildBoneWorldTransform(shoulderPos, shoulderEnd);

                Vector3 upperArmStart = shoulderEnd;
                Vector3 upperArmEndRest = bodyTransform.transformPoint(boneEnd(upperArmName));
                Vector3 armDir = upperArmEndRest - upperArmStart;
                Matrix4x4 swayMat;
                swayMat.rotate(right, armSway);
                Vector3 newUpperArmEnd = upperArmStart + swayMat.transformVector(armDir);
                boneWorldTransforms[upperArmName] = buildBoneWorldTransform(upperArmStart, newUpperArmEnd);

                // Lower arm: slightly more sway (pendulum effect)
                Matrix4x4 lowerSwayMat;
                lowerSwayMat.rotate(right, armSway * 1.3);
                Vector3 lowerArmDir = bodyTransform.transformPoint(boneEnd(lowerArmName)) - bodyTransform.transformPoint(boneEnd(upperArmName));
                Vector3 newLowerArmEnd = newUpperArmEnd + lowerSwayMat.transformVector(lowerArmDir);
                boneWorldTransforms[lowerArmName] = buildBoneWorldTransform(newUpperArmEnd, newLowerArmEnd);

                // Hand: even more pendulum lag
                Matrix4x4 handSwayMat;
                handSwayMat.rotate(right, armSway * 1.5);
                Vector3 handDir = bodyTransform.transformPoint(boneEnd(handName)) - bodyTransform.transformPoint(boneEnd(lowerArmName));
                Vector3 newHandEnd = newLowerArmEnd + handSwayMat.transformVector(handDir);
                boneWorldTransforms[handName] = buildBoneWorldTransform(newLowerArmEnd, newHandEnd);
            };

            computeArmIdle("LeftShoulder", "LeftUpperArm", "LeftLowerArm", "LeftHand", 0.0, shoulderTilt);
            computeArmIdle("RightShoulder", "RightUpperArm", "RightLowerArm", "RightHand", Math::Pi, -shoulderTilt);

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

} // namespace biped

} // namespace dust3d
