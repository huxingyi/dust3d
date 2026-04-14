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

// Procedural idle animation for insect rig.
//
// Designed for game development idle/rest state. The insect stands in
// place with subtle antennae movement, abdomen pulsing (breathing),
// and minor leg adjustments. Works for flies, beetles, ants, mantises,
// and any six-legged creature using the insect bone structure.
//
// Adjustable animation parameters:
//   - breathingAmplitudeFactor:   abdomen pulse intensity
//   - breathingSpeedFactor:       breathing cycle speed multiplier
//   - antennaeSwayFactor:   antennae/head bobbing amplitude
//   - legTwitchFactor:      subtle leg micro-movement
//   - wingFoldFactor:       wing fold/unfold micro-movement (if wing bones exist)
//   - abdomenSwayFactor:    lateral abdomen sway

#include <array>
#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/common.h>
#include <dust3d/animation/insect/idle.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace insect {

    bool idle(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = static_cast<int>(parameters.getValue("frameCount", 90));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 4.0));

        auto boneIdx = buildBoneIndexMap(rigStructure);

        auto bonePos = [&](const std::string& name) -> Vector3 {
            return getBonePos(rigStructure, boneIdx, name);
        };
        auto boneEnd = [&](const std::string& name) -> Vector3 {
            return getBoneEnd(rigStructure, boneIdx, name);
        };

        static const char* requiredBones[] = {
            "Root", "Head", "Thorax", "Abdomen",
            "FrontLeftCoxa", "FrontLeftFemur", "FrontLeftTibia",
            "FrontRightCoxa", "FrontRightFemur", "FrontRightTibia",
            "MiddleLeftCoxa", "MiddleLeftFemur", "MiddleLeftTibia",
            "MiddleRightCoxa", "MiddleRightFemur", "MiddleRightTibia",
            "BackLeftCoxa", "BackLeftFemur", "BackLeftTibia",
            "BackRightCoxa", "BackRightFemur", "BackRightTibia"
        };
        if (!validateRequiredBones(boneIdx, requiredBones, sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;

        double breathingAmplitudeFactor = parameters.getValue("breathingAmplitudeFactor", 1.0);
        double breathingSpeedFactor = parameters.getValue("breathingSpeedFactor", 1.0);
        double antennaeSwayFactor = parameters.getValue("antennaeSwayFactor", 1.0);
        double legTwitchFactor = parameters.getValue("legTwitchFactor", 1.0);
        double wingFoldFactor = parameters.getValue("wingFoldFactor", 1.0);
        double abdomenSwayFactor = parameters.getValue("abdomenSwayFactor", 1.0);

        Vector3 upDir(0.0, 1.0, 0.0);
        Vector3 headPos = bonePos("Head");
        Vector3 abdomenEnd = boneEnd("Abdomen");
        Vector3 bodyDir = headPos - abdomenEnd;
        Vector3 forward(bodyDir.x(), 0.0, bodyDir.z());
        if (forward.lengthSquared() < 1e-8)
            forward = Vector3(0.0, 0.0, 1.0);
        forward.normalize();
        Vector3 right = Vector3::crossProduct(upDir, forward);
        if (right.lengthSquared() < 1e-8)
            right = Vector3(1.0, 0.0, 0.0);
        right.normalize();

        double bodyLength = (headPos - abdomenEnd).length();
        if (bodyLength < 1e-6)
            bodyLength = 0.5;
        double breathAmp = bodyLength * 0.008 * breathingAmplitudeFactor;

        // Gather rest-pose leg data for IK
        struct LegTriplet {
            const char* coxa;
            const char* femur;
            const char* tibia;
            double phaseOffset;
        };
        static const LegTriplet legs[] = {
            { "FrontLeftCoxa", "FrontLeftFemur", "FrontLeftTibia", 0.0 },
            { "FrontRightCoxa", "FrontRightFemur", "FrontRightTibia", 0.5 },
            { "MiddleLeftCoxa", "MiddleLeftFemur", "MiddleLeftTibia", 0.33 },
            { "MiddleRightCoxa", "MiddleRightFemur", "MiddleRightTibia", 0.83 },
            { "BackLeftCoxa", "BackLeftFemur", "BackLeftTibia", 0.67 },
            { "BackRightCoxa", "BackRightFemur", "BackRightTibia", 1.17 },
        };
        static const size_t legCount = sizeof(legs) / sizeof(legs[0]);

        struct LegRest {
            Vector3 coxaPos, coxaEnd;
            Vector3 femurEnd;
            Vector3 tibiaEnd;
            Vector3 restStickDir;
            Vector3 restCoxaToFemurVec;
        };
        std::array<LegRest, 6> legRest;
        for (size_t i = 0; i < legCount; ++i) {
            legRest[i].coxaPos = bonePos(legs[i].coxa);
            legRest[i].coxaEnd = boneEnd(legs[i].coxa);
            legRest[i].femurEnd = boneEnd(legs[i].femur);
            legRest[i].tibiaEnd = boneEnd(legs[i].tibia);
            Vector3 chordVec = legRest[i].tibiaEnd - legRest[i].coxaEnd;
            legRest[i].restStickDir = chordVec.isZero() ? Vector3(1, 0, 0) : chordVec.normalized();
            legRest[i].restCoxaToFemurVec = legRest[i].femurEnd - legRest[i].coxaEnd;
        }

        // Ground level: lowest tibia end projected onto up axis
        double minUpProj = Vector3::dotProduct(legRest[0].tibiaEnd, upDir);
        for (size_t i = 1; i < legCount; ++i) {
            double proj = Vector3::dotProduct(legRest[i].tibiaEnd, upDir);
            if (proj < minUpProj)
                minUpProj = proj;
        }

        // Foot home positions pinned to ground
        std::array<Vector3, 6> footHome;
        for (size_t i = 0; i < legCount; ++i) {
            double proj = Vector3::dotProduct(legRest[i].tibiaEnd, upDir);
            footHome[i] = legRest[i].tibiaEnd - upDir * (proj - minUpProj);
        }

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            double tNormalized = static_cast<double>(frame) / static_cast<double>(frameCount);

            double breathPhase = tNormalized * 2.0 * Math::Pi * breathingSpeedFactor;
            double breathOffset = breathAmp * std::sin(breathPhase);

            Matrix4x4 bodyTransform;
            bodyTransform.translate(upDir * breathOffset);

            std::map<std::string, Matrix4x4> boneWorldTransforms;

            auto computeBone = [&](const std::string& name,
                                   double extraYaw = 0.0,
                                   double extraPitch = 0.0) {
                Vector3 pos = bonePos(name);
                Vector3 end = boneEnd(name);
                Vector3 newPos = bodyTransform.transformPoint(pos);
                Vector3 newEnd = bodyTransform.transformPoint(end);
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

            // Head: antennae-like bobbing
            double headYaw = 0.04 * antennaeSwayFactor * std::sin(tNormalized * 2.0 * Math::Pi * 1.3);
            double headPitch = 0.02 * antennaeSwayFactor * std::sin(tNormalized * 2.0 * Math::Pi * 0.9 + 0.7);

            computeBone("Root");
            computeBone("Head", headYaw, headPitch);
            computeBone("Thorax");

            // Abdomen: breathing pulse + subtle lateral sway
            double abdomenSway = 0.02 * abdomenSwayFactor * std::sin(breathPhase * 0.5);
            double abdomenPulse = 0.015 * breathingAmplitudeFactor * std::sin(breathPhase);
            computeBone("Abdomen", abdomenSway, abdomenPulse);

            // Wings: subtle fold/unfold
            if (boneIdx.count("LeftWing")) {
                double wingAngle = 0.02 * wingFoldFactor * std::sin(tNormalized * 2.0 * Math::Pi * 0.6);
                computeBone("LeftWing", 0.0, wingAngle);
            }
            if (boneIdx.count("RightWing")) {
                double wingAngle = 0.02 * wingFoldFactor * std::sin(tNormalized * 2.0 * Math::Pi * 0.6);
                computeBone("RightWing", 0.0, -wingAngle);
            }

            // Legs: use IK to keep feet grounded with subtle twitching
            for (size_t i = 0; i < legCount; ++i) {
                double legPhase = tNormalized * 2.0 * Math::Pi * 0.3 + legs[i].phaseOffset * Math::Pi;
                double twitch = 0.006 * legTwitchFactor * std::sin(legPhase);

                // Foot stays on ground; only add tiny lateral twitch
                Vector3 footTarget = footHome[i] + right * (twitch * bodyLength * 0.5);

                Vector3 hipPos = bodyTransform.transformPoint(legRest[i].coxaPos);
                Vector3 coxaEndPos = bodyTransform.transformPoint(legRest[i].coxaEnd);
                Vector3 tibiaEndPos = bodyTransform.transformPoint(legRest[i].tibiaEnd);

                std::vector<Vector3> chain = { hipPos, coxaEndPos, tibiaEndPos };
                Vector3 poleVector = coxaEndPos + upDir * 0.5;
                animation::solveTwoBoneIk(chain, footTarget, poleVector, 0.02);

                // Reconstruct femur-tibia junction
                Vector3 newStickDir = (chain[2] - chain[1]);
                if (newStickDir.isZero())
                    newStickDir = legRest[i].restStickDir;
                else
                    newStickDir.normalize();
                Quaternion stickRot = Quaternion::rotationTo(legRest[i].restStickDir, newStickDir);
                Matrix4x4 stickRotMat;
                stickRotMat.rotate(stickRot);
                Vector3 femurEnd = chain[1] + stickRotMat.transformVector(legRest[i].restCoxaToFemurVec);

                boneWorldTransforms[legs[i].coxa] = animation::buildBoneWorldTransform(chain[0], chain[1]);
                boneWorldTransforms[legs[i].femur] = animation::buildBoneWorldTransform(chain[1], femurEnd);
                boneWorldTransforms[legs[i].tibia] = animation::buildBoneWorldTransform(femurEnd, chain[2]);
            }

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

} // namespace insect

} // namespace dust3d
