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

// Procedural glide animation for bird rig.
//
// The bird soars with wings held outstretched in a mostly static pose,
// with subtle body banking, gentle wing-tip flex, and slow altitude
// oscillation to convey lift and air currents. Legs remain tucked.
//
// Adjustable animation parameters:
//   - bankAmplitudeFactor:   lateral banking/rolling intensity
//   - altitudeOscFactor:     vertical altitude oscillation
//   - wingTipFlexFactor:     passive wing-tip flexing from air pressure
//   - wingSpreadAngle:       how far wings are held out (radians)
//   - headStabilizeFactor:   head counter-rotation for gaze stability
//   - tailSteerFactor:       tail rudder-like yaw during banking

#include <cmath>
#include <dust3d/animation/bird/glide.h>
#include <dust3d/animation/common.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace bird {

    bool glide(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& /* inverseBindMatrices */,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = static_cast<int>(parameters.getValue("frameCount", 60));
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
            "LeftWingShoulder", "LeftWingElbow", "LeftWingHand",
            "RightWingShoulder", "RightWingElbow", "RightWingHand",
            "LeftUpperLeg", "LeftLowerLeg", "LeftFoot",
            "RightUpperLeg", "RightLowerLeg", "RightFoot"
        };
        if (!validateRequiredBones(boneIdx, requiredBones, sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;

        // Determine body axes
        Vector3 pelvisPos = bonePos("Pelvis");
        Vector3 chestEnd = boneEnd("Chest");
        Vector3 bodyVector = chestEnd - pelvisPos;
        if (bodyVector.isZero())
            return false;

        Vector3 forwardDir = bodyVector.normalized();
        Vector3 worldUp(0.0, 1.0, 0.0);
        Vector3 right = Vector3::crossProduct(forwardDir, worldUp);
        if (right.lengthSquared() < 1e-8)
            right = Vector3::crossProduct(forwardDir, Vector3(0.0, 0.0, 1.0));
        right.normalize();
        Vector3 up = Vector3::crossProduct(right, forwardDir).normalized();

        double bodyLength = bodyVector.length();

        // Animation parameters
        double bankAmplitudeFactor = parameters.getValue("bankAmplitudeFactor", 1.0);
        double altitudeOscFactor = parameters.getValue("altitudeOscFactor", 1.0);
        double wingTipFlexFactor = parameters.getValue("wingTipFlexFactor", 1.0);
        double wingSpreadAngle = parameters.getValue("wingSpreadAngle", 0.15); // radians upward from horizontal
        double headStabilizeFactor = parameters.getValue("headStabilizeFactor", 1.0);
        double tailSteerFactor = parameters.getValue("tailSteerFactor", 1.0);

        // Derived amplitudes
        double bankAmp = 0.12 * bankAmplitudeFactor; // radians, roll around forward axis
        double altitudeAmp = bodyLength * 0.03 * altitudeOscFactor;
        double pitchAmp = 0.04 * altitudeOscFactor; // subtle nose-up/down with altitude
        double wingTipAmp = 0.08 * wingTipFlexFactor; // radians, passive flex at hand
        double tailYawAmp = 0.06 * tailSteerFactor; // radians

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            double tNormalized = static_cast<double>(frame) / static_cast<double>(frameCount);
            double phase = tNormalized * 2.0 * Math::Pi;

            // Slow sinusoidal banking and altitude oscillation
            double bankAngle = bankAmp * std::sin(phase);
            double altitudeOffset = altitudeAmp * std::sin(phase); // same frequency as bank for seamless loop
            double pitchAngle = pitchAmp * std::sin(phase + Math::Pi * 0.25); // slightly leading altitude

            // Body transform: translate vertically + roll (bank) + pitch
            Matrix4x4 bodyTransform;
            bodyTransform.translate(up * altitudeOffset);
            bodyTransform.rotate(forwardDir, bankAngle);
            bodyTransform.rotate(right, pitchAngle);

            std::map<std::string, Matrix4x4> boneWorldTransforms;
            std::map<std::string, Matrix4x4> boneSkinMatrices;

            // Helper: compute world transform from skin matrix and bind-pose endpoints.
            // Using skinMatrix * bindPoseWorldTransform preserves bone roll, which
            // buildBoneWorldTransform(pos, end) cannot recover from endpoints alone.
            // This is critical for correct GLB/FBX export.
            auto worldFromSkin = [&](const std::string& name, const Matrix4x4& skin) -> Matrix4x4 {
                Matrix4x4 bindWorld = buildBoneWorldTransform(bonePos(name), boneEnd(name));
                Matrix4x4 result = skin;
                result *= bindWorld;
                return result;
            };

            // Rigid body bone helper
            auto computeBodyBone = [&](const std::string& name) {
                boneSkinMatrices[name] = bodyTransform;
                boneWorldTransforms[name] = worldFromSkin(name, bodyTransform);
            };

            // Body chain
            computeBodyBone("Root");
            computeBodyBone("Pelvis");
            computeBodyBone("Spine");
            computeBodyBone("Chest");

            // Neck and head: counter-rotate bank for gaze stability
            {
                double neckCounterBank = -bankAngle * 0.3 * headStabilizeFactor;
                double neckCounterPitch = -pitchAngle * 0.4 * headStabilizeFactor;

                Vector3 neckPos = bodyTransform.transformPoint(bonePos("Neck"));
                Vector3 neckEnd = bodyTransform.transformPoint(boneEnd("Neck"));

                Matrix4x4 neckRot;
                neckRot.rotate(forwardDir, neckCounterBank);
                neckRot.rotate(right, neckCounterPitch);
                Vector3 neckDir = neckEnd - neckPos;
                Vector3 rotatedNeckDir = neckRot.transformVector(neckDir);
                Vector3 neckBindPos = bonePos("Neck");
                Matrix4x4 neckSkin = bodyTransform;
                neckSkin.translate(neckBindPos);
                neckSkin.rotate(forwardDir, neckCounterBank);
                neckSkin.rotate(right, neckCounterPitch);
                neckSkin.translate(-neckBindPos);
                boneSkinMatrices["Neck"] = neckSkin;
                boneWorldTransforms["Neck"] = worldFromSkin("Neck", neckSkin);

                // Head
                Vector3 headPos = neckPos + rotatedNeckDir;
                Vector3 headDir = boneEnd("Head") - bonePos("Head");
                Matrix4x4 headRot;
                headRot.rotate(forwardDir, neckCounterBank * 0.5);
                headRot.rotate(right, neckCounterPitch * 0.5);
                Vector3 rotatedHeadDir = headRot.transformVector(neckRot.transformVector(headDir));
                if (rotatedHeadDir.isZero())
                    rotatedHeadDir = forwardDir;
                boneSkinMatrices["Head"] = neckSkin;
                boneWorldTransforms["Head"] = worldFromSkin("Head", neckSkin);

                // Beak
                if (boneIdx.count("Beak")) {
                    Vector3 beakPos = headPos + rotatedHeadDir;
                    Vector3 beakDir = boneEnd("Beak") - bonePos("Beak");
                    Vector3 rotatedBeakDir = headRot.transformVector(neckRot.transformVector(beakDir));
                    if (rotatedBeakDir.isZero())
                        rotatedBeakDir = rotatedHeadDir.normalized();
                    boneSkinMatrices["Beak"] = neckSkin;
                    boneWorldTransforms["Beak"] = worldFromSkin("Beak", neckSkin);
                }
            }

            // Tail: yaw in opposition to bank for steering effect
            {
                double tailYaw = tailYawAmp * std::sin(phase) * -1.0; // oppose bank
                bool hasTailBase = boneIdx.count("TailBase") > 0;
                bool hasTailFeathers = boneIdx.count("TailFeathers") > 0;

                if (hasTailBase) {
                    Vector3 tailBasePos = bodyTransform.transformPoint(bonePos("TailBase"));
                    Vector3 tailBaseEndOrig = bodyTransform.transformPoint(boneEnd("TailBase"));
                    Matrix4x4 tailRot;
                    tailRot.rotate(up, tailYaw);
                    Vector3 tailDir = tailBaseEndOrig - tailBasePos;
                    Vector3 rotatedTailDir = tailRot.transformVector(tailDir);
                    Vector3 tailBaseEnd = tailBasePos + rotatedTailDir;
                    Vector3 tailBindPos = bonePos("TailBase");
                    Matrix4x4 tailSkin = bodyTransform;
                    tailSkin.translate(tailBindPos);
                    tailSkin.rotate(up, tailYaw);
                    tailSkin.translate(-tailBindPos);
                    boneSkinMatrices["TailBase"] = tailSkin;
                    boneWorldTransforms["TailBase"] = worldFromSkin("TailBase", tailSkin);

                    if (hasTailFeathers) {
                        Vector3 featherDir = bodyTransform.transformPoint(boneEnd("TailFeathers")) - bodyTransform.transformPoint(bonePos("TailFeathers"));
                        Vector3 rotatedFeatherDir = tailRot.transformVector(featherDir);
                        boneSkinMatrices["TailFeathers"] = tailSkin;
                        boneWorldTransforms["TailFeathers"] = worldFromSkin("TailFeathers", tailSkin);
                    }
                }
            }

            // Wings: held outstretched with subtle tip flex from air pressure
            for (int side = 0; side < 2; ++side) {
                const char* shoulderName = (side == 0) ? "LeftWingShoulder" : "RightWingShoulder";
                const char* elbowName = (side == 0) ? "LeftWingElbow" : "RightWingElbow";
                const char* handName = (side == 0) ? "LeftWingHand" : "RightWingHand";

                double sideSign = (side == 0) ? 1.0 : -1.0;

                Vector3 shoulderPos = bodyTransform.transformPoint(bonePos(shoulderName));
                Vector3 shoulderEnd = bodyTransform.transformPoint(boneEnd(shoulderName));
                Vector3 elbowEnd = bodyTransform.transformPoint(boneEnd(elbowName));
                Vector3 handEnd = bodyTransform.transformPoint(boneEnd(handName));

                // Wings held slightly upward (dihedral) — static spread
                double spreadAngle = wingSpreadAngle * sideSign;
                Quaternion spreadRot = Quaternion::fromAxisAndAngle(forwardDir, spreadAngle);
                Matrix4x4 spreadMat;
                spreadMat.rotate(spreadRot);

                // Shoulder
                Vector3 shoulderDir = shoulderEnd - shoulderPos;
                Vector3 rotatedShoulderDir = spreadMat.transformVector(shoulderDir);
                Vector3 newShoulderEnd = shoulderPos + rotatedShoulderDir;
                Vector3 shoulderBindPos = bonePos(shoulderName);
                Matrix4x4 wingSkin = bodyTransform;
                wingSkin.translate(shoulderBindPos);
                wingSkin.rotate(spreadRot);
                wingSkin.translate(-shoulderBindPos);
                boneSkinMatrices[shoulderName] = wingSkin;
                boneWorldTransforms[shoulderName] = worldFromSkin(shoulderName, wingSkin);

                // Elbow: held straight, follow shoulder rotation
                Vector3 elbowDir = elbowEnd - shoulderEnd;
                Vector3 rotatedElbowDir = spreadMat.transformVector(elbowDir);
                Vector3 newElbowEnd = newShoulderEnd + rotatedElbowDir;
                boneSkinMatrices[elbowName] = wingSkin;
                boneWorldTransforms[elbowName] = worldFromSkin(elbowName, wingSkin);

                // Hand: passive flex from air — gentle oscillation
                double tipFlex = wingTipAmp * std::sin(phase + sideSign * 0.5) * sideSign;
                Quaternion tipRot = Quaternion::fromAxisAndAngle(forwardDir, tipFlex);
                Matrix4x4 tipMat;
                tipMat.rotate(tipRot);

                Vector3 handDir = handEnd - elbowEnd;
                Vector3 rotatedHandDir = tipMat.transformVector(spreadMat.transformVector(handDir));
                Vector3 newHandEnd = newElbowEnd + rotatedHandDir;
                Vector3 elbowBindPos = bonePos(elbowName);
                Matrix4x4 handSkin = wingSkin;
                handSkin.translate(elbowBindPos);
                handSkin.rotate(tipRot);
                handSkin.translate(-elbowBindPos);
                boneSkinMatrices[handName] = handSkin;
                boneWorldTransforms[handName] = worldFromSkin(handName, handSkin);
            }

            // Legs: tucked with minimal movement
            for (int side = 0; side < 2; ++side) {
                const char* upperName = (side == 0) ? "LeftUpperLeg" : "RightUpperLeg";
                const char* lowerName = (side == 0) ? "LeftLowerLeg" : "RightLowerLeg";
                const char* footName = (side == 0) ? "LeftFoot" : "RightFoot";

                Vector3 upperPos = bodyTransform.transformPoint(bonePos(upperName));
                Vector3 upperEnd = bodyTransform.transformPoint(boneEnd(upperName));
                Vector3 lowerEnd = bodyTransform.transformPoint(boneEnd(lowerName));
                Vector3 footEnd = bodyTransform.transformPoint(boneEnd(footName));

                // Tuck legs backward
                double tuckAngle = -0.4;
                Matrix4x4 legTuckMat;
                legTuckMat.rotate(right, tuckAngle);

                Vector3 upperDir = upperEnd - upperPos;
                Vector3 rotatedUpperDir = legTuckMat.transformVector(upperDir);
                Vector3 newUpperEnd = upperPos + rotatedUpperDir;
                Vector3 upperBindPos = bonePos(upperName);
                Matrix4x4 legSkin = bodyTransform;
                legSkin.translate(upperBindPos);
                legSkin.rotate(right, tuckAngle);
                legSkin.translate(-upperBindPos);
                boneSkinMatrices[upperName] = legSkin;
                boneWorldTransforms[upperName] = worldFromSkin(upperName, legSkin);

                double lowerTuckAngle = -0.6;
                Matrix4x4 lowerTuckMat;
                lowerTuckMat.rotate(right, lowerTuckAngle);
                Vector3 lowerDir = lowerEnd - upperEnd;
                Vector3 rotatedLowerDir = lowerTuckMat.transformVector(legTuckMat.transformVector(lowerDir));
                Vector3 newLowerEnd = newUpperEnd + rotatedLowerDir;
                boneSkinMatrices[lowerName] = legSkin;
                boneWorldTransforms[lowerName] = worldFromSkin(lowerName, legSkin);

                Vector3 footDir = footEnd - lowerEnd;
                Vector3 rotatedFootDir = lowerTuckMat.transformVector(legTuckMat.transformVector(footDir));
                Vector3 newFootEnd = newLowerEnd + rotatedFootDir;
                boneSkinMatrices[footName] = legSkin;
                boneWorldTransforms[footName] = worldFromSkin(footName, legSkin);
            }

            // Write frame
            auto& animFrame = animationClip.frames[frame];
            animFrame.time = static_cast<float>(tNormalized) * durationSeconds;
            animFrame.boneWorldTransforms = boneWorldTransforms;
            animFrame.boneSkinMatrices = boneSkinMatrices;
        }

        return true;
    }

} // namespace bird

} // namespace dust3d
