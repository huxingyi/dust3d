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
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        int frameCount,
        float durationSeconds,
        const AnimationParams& parameters)
    {
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

        animationClip.name = "birdForward";
        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        const double cycles = std::max(1.0, std::round(gaitSpeedFactor));

        for (int frame = 0; frame < frameCount; ++frame) {
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

            // Helper to transform a body bone rigidly with the body
            auto computeBodyBone = [&](const std::string& name) {
                Vector3 pos = getBonePos(name);
                Vector3 end = getBoneEnd(name);
                Vector3 newPos = bodyTransform.transformPoint(pos);
                Vector3 newEnd = bodyTransform.transformPoint(end);
                boneWorldTransforms[name] = animation::buildBoneWorldTransform(newPos, newEnd);
            };

            // Body chain
            computeBodyBone("Root");
            computeBodyBone("Pelvis");
            computeBodyBone("Spine");
            computeBodyBone("Chest");

            // Neck: slight counter-pitch for head stabilization
            {
                Vector3 neckPos = bodyTransform.transformPoint(getBonePos("Neck"));
                Vector3 neckEnd = bodyTransform.transformPoint(getBoneEnd("Neck"));
                double neckCounterPitch = -bodyPitch * 0.4;
                Matrix4x4 neckRot;
                neckRot.rotate(right, neckCounterPitch);
                Vector3 neckDir = neckEnd - neckPos;
                Vector3 rotatedNeckDir = neckRot.transformVector(neckDir);
                boneWorldTransforms["Neck"] = animation::buildBoneWorldTransform(neckPos, neckPos + rotatedNeckDir);

                // Head follows neck end, using combined body+neck rotation
                Vector3 headPos = neckPos + rotatedNeckDir;
                Matrix4x4 headTransform;
                headTransform.translate(headPos - getBonePos("Head"));
                headTransform.rotate(right, neckCounterPitch);
                headTransform.rotate(right, bodyPitch);
                Vector3 headDir = getBoneEnd("Head") - getBonePos("Head");
                Vector3 rotatedHeadDir = headTransform.transformVector(headDir);
                if (rotatedHeadDir.isZero())
                    rotatedHeadDir = forwardDir;
                boneWorldTransforms["Head"] = animation::buildBoneWorldTransform(headPos, headPos + rotatedHeadDir);

                // Beak rigidly follows Head with the same transform
                if (boneIdx.count("Beak")) {
                    Vector3 beakPos = headPos + rotatedHeadDir;
                    Vector3 beakDir = getBoneEnd("Beak") - getBonePos("Beak");
                    Vector3 rotatedBeakDir = headTransform.transformVector(beakDir);
                    if (rotatedBeakDir.isZero())
                        rotatedBeakDir = rotatedHeadDir.normalized();
                    boneWorldTransforms["Beak"] = animation::buildBoneWorldTransform(beakPos, beakPos + rotatedBeakDir);
                }
            }

            // Tail: gentle up/down fan motion opposite to body bob
            {
                bool hasTailBase = boneIdx.count("TailBase") > 0;
                bool hasTailFeathers = boneIdx.count("TailFeathers") > 0;

                if (hasTailBase) {
                    Vector3 tailBasePos = bodyTransform.transformPoint(getBonePos("TailBase"));
                    Vector3 tailBaseEnd = bodyTransform.transformPoint(getBoneEnd("TailBase"));
                    double tailAngle = tailFlapAmp * std::sin(phase + Math::Pi); // opposite phase to body
                    Matrix4x4 tailRot;
                    tailRot.rotate(right, tailAngle);
                    Vector3 tailDir = tailBaseEnd - tailBasePos;
                    Vector3 rotatedTailDir = tailRot.transformVector(tailDir);
                    boneWorldTransforms["TailBase"] = animation::buildBoneWorldTransform(tailBasePos, tailBasePos + rotatedTailDir);

                    if (hasTailFeathers) {
                        Vector3 featherStart = tailBasePos + rotatedTailDir;
                        Vector3 featherDir = getBoneEnd("TailFeathers") - getBonePos("TailFeathers");
                        double featherLen = featherDir.length();
                        Vector3 rotatedFeatherDir = tailRot.transformVector(bodyTransform.transformVector(featherDir.normalized())) * featherLen;
                        boneWorldTransforms["TailFeathers"] = animation::buildBoneWorldTransform(featherStart, featherStart + rotatedFeatherDir);
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

                Vector3 shoulderPos = bodyTransform.transformPoint(getBonePos(shoulderName));
                Vector3 shoulderEnd = bodyTransform.transformPoint(getBoneEnd(shoulderName));
                Vector3 elbowEnd = bodyTransform.transformPoint(getBoneEnd(elbowName));
                Vector3 handEnd = bodyTransform.transformPoint(getBoneEnd(handName));

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
                Vector3 shoulderDir = shoulderEnd - shoulderPos;
                Vector3 rotatedShoulderDir = shoulderRotMat.transformVector(shoulderDir);
                Vector3 newShoulderEnd = shoulderPos + rotatedShoulderDir;
                boneWorldTransforms[shoulderName] = animation::buildBoneWorldTransform(shoulderPos, newShoulderEnd);

                // Elbow: secondary fold during upstroke (wings fold in on upstroke)
                double elbowFold = elbowFoldAmp * std::max(0.0, -rawWing) * sideSign;
                Quaternion elbowRot = Quaternion::fromAxisAndAngle(flapAxis, elbowFold);
                Matrix4x4 elbowRotMat;
                elbowRotMat.rotate(elbowRot);

                Vector3 elbowDir = elbowEnd - shoulderEnd;
                Vector3 rotatedElbowDir = elbowRotMat.transformVector(shoulderRotMat.transformVector(elbowDir));
                Vector3 newElbowEnd = newShoulderEnd + rotatedElbowDir;
                boneWorldTransforms[elbowName] = animation::buildBoneWorldTransform(newShoulderEnd, newElbowEnd);

                // Hand: tertiary fold, slightly delayed phase
                double handFold = handFoldAmp * std::max(0.0, -rawWing) * sideSign;
                Quaternion handRot = Quaternion::fromAxisAndAngle(flapAxis, handFold);
                Matrix4x4 handRotMat;
                handRotMat.rotate(handRot);

                Vector3 handDir = handEnd - elbowEnd;
                Vector3 rotatedHandDir = handRotMat.transformVector(elbowRotMat.transformVector(shoulderRotMat.transformVector(handDir)));
                Vector3 newHandEnd = newElbowEnd + rotatedHandDir;
                boneWorldTransforms[handName] = animation::buildBoneWorldTransform(newElbowEnd, newHandEnd);
            }

            // Legs: tucked during flight with gentle sway
            for (int side = 0; side < 2; ++side) {
                const char* upperName = (side == 0) ? "LeftUpperLeg" : "RightUpperLeg";
                const char* lowerName = (side == 0) ? "LeftLowerLeg" : "RightLowerLeg";
                const char* footName = (side == 0) ? "LeftFoot" : "RightFoot";

                Vector3 upperPos = bodyTransform.transformPoint(getBonePos(upperName));
                Vector3 upperEnd = bodyTransform.transformPoint(getBoneEnd(upperName));
                Vector3 lowerEnd = bodyTransform.transformPoint(getBoneEnd(lowerName));
                Vector3 footEnd = bodyTransform.transformPoint(getBoneEnd(footName));

                // Tuck legs backward and upward during flight
                double tuckAngle = -0.4; // radians, pull legs back
                double sway = 0.05 * std::sin(phase);
                Matrix4x4 legTuckMat;
                legTuckMat.rotate(right, tuckAngle + sway);

                Vector3 upperDir = upperEnd - upperPos;
                Vector3 rotatedUpperDir = legTuckMat.transformVector(upperDir);
                Vector3 newUpperEnd = upperPos + rotatedUpperDir;
                boneWorldTransforms[upperName] = animation::buildBoneWorldTransform(upperPos, newUpperEnd);

                // Lower leg folds more to tuck
                double lowerTuckAngle = -0.6;
                Matrix4x4 lowerTuckMat;
                lowerTuckMat.rotate(right, lowerTuckAngle);
                Vector3 lowerDir = lowerEnd - upperEnd;
                Vector3 rotatedLowerDir = lowerTuckMat.transformVector(legTuckMat.transformVector(lowerDir));
                Vector3 newLowerEnd = newUpperEnd + rotatedLowerDir;
                boneWorldTransforms[lowerName] = animation::buildBoneWorldTransform(newUpperEnd, newLowerEnd);

                // Foot follows
                Vector3 footDir = footEnd - lowerEnd;
                Vector3 rotatedFootDir = lowerTuckMat.transformVector(legTuckMat.transformVector(footDir));
                Vector3 newFootEnd = newLowerEnd + rotatedFootDir;
                boneWorldTransforms[footName] = animation::buildBoneWorldTransform(newLowerEnd, newFootEnd);
            }

            // Write frame
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

} // namespace bird

} // namespace dust3d
