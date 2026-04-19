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

#include <cmath>
#include <cstring>
#include <dust3d/animation/bird/forward.h>
#include <dust3d/animation/common.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace bird {

    bool forward(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& /* inverseBindMatrices */,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        int frameCount = static_cast<int>(parameters.getValue("frameCount", 30.0));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 1.0));
        std::map<std::string, size_t> boneIdx;
        for (size_t i = 0; i < rigStructure.bones.size(); ++i)
            boneIdx[rigStructure.bones[i].name] = i;

        auto getBonePos = [&](const std::string& name) -> Vector3 {
            auto it = boneIdx.find(name);
            if (it == boneIdx.end())
                return Vector3();
            const auto& b = rigStructure.bones[it->second];
            return Vector3(b.posX, b.posY, b.posZ);
        };

        auto getBoneEnd = [&](const std::string& name) -> Vector3 {
            auto it = boneIdx.find(name);
            if (it == boneIdx.end())
                return Vector3();
            const auto& b = rigStructure.bones[it->second];
            return Vector3(b.endX, b.endY, b.endZ);
        };

        // Required bird bones
        static const char* requiredBones[] = {
            "Root", "Pelvis", "Spine", "Chest", "Neck", "Head",
            "LeftWingShoulder", "LeftWingElbow", "LeftWingHand",
            "RightWingShoulder", "RightWingElbow", "RightWingHand",
            "LeftUpperLeg", "LeftLowerLeg", "LeftFoot",
            "RightUpperLeg", "RightLowerLeg", "RightFoot"
        };

        for (const char* name : requiredBones) {
            if (boneIdx.find(name) == boneIdx.end())
                return false;
        }

        // Determine forward direction from body
        Vector3 bodyVector = getBoneEnd("Chest") - getBonePos("Pelvis");
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
        double bodyBobFactor = parameters.getValue("bodyBobFactor", 1.0);
        double gaitSpeedFactor = parameters.getValue("gaitSpeedFactor", 1.0);
        double wingFlapFactor = parameters.getValue("stepHeightFactor", 1.0);
        double stepLengthFactor = parameters.getValue("stepLengthFactor", 1.0);

        double bodyBobAmp = bodyLength * 0.06 * bodyBobFactor;
        double bodyPitchAmp = 0.12 * bodyBobFactor; // radians
        double bodyForwardAmp = bodyLength * 0.15 * stepLengthFactor;

        // Wing flap parameters - bird wings have larger range than insect
        double shoulderFlapAmp = 0.9 * wingFlapFactor; // radians (~50 deg)
        double elbowFoldAmp = 0.35 * wingFlapFactor; // radians (~20 deg)
        double handFoldAmp = 0.2 * wingFlapFactor; // radians (~11 deg)

        // Tail parameters
        double tailFlapAmp = 0.15; // radians

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        const double cycles = std::max(1.0, std::round(gaitSpeedFactor));

        for (int frame = 0; frame < frameCount; ++frame) {
            // Forward is a loopable clip: tNormalized spans [0, 1) so frame 0 and a
            // hypothetical extra frame are identical, enabling seamless looping.
            // One-shot clips (e.g. die) use frameCount - 1 to reach exactly 1.0.
            double tNormalized = static_cast<double>(frame) / static_cast<double>(frameCount);
            double t = fmod(tNormalized * cycles, 1.0);
            double phase = t * 2.0 * Math::Pi;

            // Body motion: bob up/down, pitch, and forward sway
            double bodyBob = bodyBobAmp * std::sin(phase);
            double bodyPitch = bodyPitchAmp * 0.3 * std::sin(phase);
            double bodyForward = bodyForwardAmp * std::sin(phase);

            Matrix4x4 bodyTransform;
            bodyTransform.translate(forwardDir * bodyForward + up * bodyBob);
            bodyTransform.rotate(right, bodyPitch);

            std::map<std::string, Matrix4x4> boneWorldTransforms;
            std::map<std::string, Matrix4x4> boneSkinMatrices;

            // Compute world transform from skin matrix preserving bone roll
            auto worldFromSkin = [&](const std::string& name, const Matrix4x4& skin) -> Matrix4x4 {
                Matrix4x4 bindWorld = animation::buildBoneWorldTransform(getBonePos(name), getBoneEnd(name));
                Matrix4x4 result = skin;
                result *= bindWorld;
                return result;
            };

            // Helper to transform a body bone rigidly with the body
            auto computeBodyBone = [&](const std::string& name) {
                boneSkinMatrices[name] = bodyTransform;
                boneWorldTransforms[name] = worldFromSkin(name, bodyTransform);
            };

            // Body chain
            computeBodyBone("Root");
            computeBodyBone("Pelvis");
            computeBodyBone("Spine");
            computeBodyBone("Chest");

            // Neck: slight counter-pitch for head stabilization
            {
                Vector3 neckBindPos = getBonePos("Neck");
                double neckCounterPitch = -bodyPitch * 0.4;
                Matrix4x4 neckSkin = bodyTransform;
                neckSkin.translate(neckBindPos);
                neckSkin.rotate(right, neckCounterPitch);
                neckSkin.translate(-neckBindPos);
                boneSkinMatrices["Neck"] = neckSkin;
                boneWorldTransforms["Neck"] = worldFromSkin("Neck", neckSkin);

                // Head follows neck with combined rotation
                Matrix4x4 headSkin = neckSkin;
                boneSkinMatrices["Head"] = headSkin;
                boneWorldTransforms["Head"] = worldFromSkin("Head", headSkin);

                // Beak rigidly follows Head with the same transform
                if (boneIdx.count("Beak")) {
                    boneSkinMatrices["Beak"] = headSkin;
                    boneWorldTransforms["Beak"] = worldFromSkin("Beak", headSkin);
                }
            }

            // Tail: remain fixed to body, no independent tail movement during flight
            {
                bool hasTailBase = boneIdx.count("TailBase") > 0;
                bool hasTailFeathers = boneIdx.count("TailFeathers") > 0;

                if (hasTailBase) {
                    boneSkinMatrices["TailBase"] = bodyTransform;
                    boneWorldTransforms["TailBase"] = worldFromSkin("TailBase", bodyTransform);

                    if (hasTailFeathers) {
                        boneSkinMatrices["TailFeathers"] = bodyTransform;
                        boneWorldTransforms["TailFeathers"] = worldFromSkin("TailFeathers", bodyTransform);
                    }
                }
            }

            // Wings: articulated 3-segment flapping
            // Downstroke is faster than upstroke for realistic bird flight
            for (int side = 0; side < 2; ++side) {
                const char* shoulderName = (side == 0) ? "LeftWingShoulder" : "RightWingShoulder";
                const char* elbowName = (side == 0) ? "LeftWingElbow" : "RightWingElbow";
                const char* handName = (side == 0) ? "LeftWingHand" : "RightWingHand";

                double sideSign = (side == 0) ? 1.0 : -1.0;

                // Shoulder flap: main up/down rotation around forward axis
                // Use asymmetric waveform: faster downstroke
                double rawWing = std::sin(phase);
                double wingAngle = shoulderFlapAmp * rawWing * sideSign;

                // Flap axis is roughly the forward direction of the body
                Vector3 flapAxis = forwardDir;

                Quaternion shoulderRot = Quaternion::fromAxisAndAngle(flapAxis, wingAngle);
                Matrix4x4 shoulderRotMat;
                shoulderRotMat.rotate(shoulderRot);

                // Rotate shoulder segment around shoulder joint
                Vector3 shoulderBindPos = getBonePos(shoulderName);
                Matrix4x4 shoulderSkin = bodyTransform;
                shoulderSkin.translate(shoulderBindPos);
                shoulderSkin.rotate(shoulderRot);
                shoulderSkin.translate(-shoulderBindPos);
                boneSkinMatrices[shoulderName] = shoulderSkin;
                boneWorldTransforms[shoulderName] = worldFromSkin(shoulderName, shoulderSkin);

                // Elbow: secondary fold during upstroke (wings fold in on upstroke)
                double elbowFold = elbowFoldAmp * std::max(0.0, -rawWing) * sideSign;
                Quaternion elbowRot = Quaternion::fromAxisAndAngle(flapAxis, elbowFold);

                Vector3 elbowBindPos = getBonePos(elbowName);
                Matrix4x4 elbowSkin = shoulderSkin;
                elbowSkin.translate(elbowBindPos);
                elbowSkin.rotate(elbowRot);
                elbowSkin.translate(-elbowBindPos);
                boneSkinMatrices[elbowName] = elbowSkin;
                boneWorldTransforms[elbowName] = worldFromSkin(elbowName, elbowSkin);

                // Hand: tertiary fold, slightly delayed phase
                double handFold = handFoldAmp * std::max(0.0, -rawWing) * sideSign;
                Quaternion handRot = Quaternion::fromAxisAndAngle(flapAxis, handFold);

                Vector3 handBindPos = getBonePos(handName);
                Matrix4x4 handSkin = elbowSkin;
                handSkin.translate(handBindPos);
                handSkin.rotate(handRot);
                handSkin.translate(-handBindPos);
                boneSkinMatrices[handName] = handSkin;
                boneWorldTransforms[handName] = worldFromSkin(handName, handSkin);
            }

            // Legs: tucked during flight with gentle sway
            for (int side = 0; side < 2; ++side) {
                const char* upperName = (side == 0) ? "LeftUpperLeg" : "RightUpperLeg";
                const char* lowerName = (side == 0) ? "LeftLowerLeg" : "RightLowerLeg";
                const char* footName = (side == 0) ? "LeftFoot" : "RightFoot";

                // Tuck legs backward and upward during flight
                double tuckAngle = -0.4; // radians, pull legs back
                double sway = 0.05 * std::sin(phase);

                Vector3 upperBindPos = getBonePos(upperName);
                Matrix4x4 legSkin = bodyTransform;
                legSkin.translate(upperBindPos);
                legSkin.rotate(right, tuckAngle + sway);
                legSkin.translate(-upperBindPos);
                boneSkinMatrices[upperName] = legSkin;
                boneWorldTransforms[upperName] = worldFromSkin(upperName, legSkin);

                // Lower leg folds more to tuck
                boneSkinMatrices[lowerName] = legSkin;
                boneWorldTransforms[lowerName] = worldFromSkin(lowerName, legSkin);

                // Foot follows
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
