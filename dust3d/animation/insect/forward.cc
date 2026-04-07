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

#include <array>
#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/insect/common.h>
#include <dust3d/animation/insect/forward.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace insect {

    namespace {

    } // anonymous namespace

    bool forward(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
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

        // Required fly bones: body and wings plus leg chain if present.
        static const char* requiredBones[] = {
            "Head", "Thorax", "Abdomen",
            "LeftWing", "RightWing",
            "FrontLeftCoxa", "FrontLeftFemur", "FrontLeftTibia",
            "FrontRightCoxa", "FrontRightFemur", "FrontRightTibia",
            "MiddleLeftCoxa", "MiddleLeftFemur", "MiddleLeftTibia",
            "MiddleRightCoxa", "MiddleRightFemur", "MiddleRightTibia",
            "BackLeftCoxa", "BackLeftFemur", "BackLeftTibia",
            "BackRightCoxa", "BackRightFemur", "BackRightTibia"
        };

        for (const char* name : requiredBones) {
            if (boneIdx.find(name) == boneIdx.end())
                return false;
        }

        Vector3 bodyVector = getBonePos("Thorax") - getBoneEnd("Abdomen");
        if (bodyVector.isZero())
            return false;

        Vector3 forward = bodyVector.normalized();
        Vector3 worldUp(0.0, 1.0, 0.0);
        Vector3 right = Vector3::crossProduct(forward, worldUp);
        if (right.lengthSquared() < 1e-8)
            right = Vector3::crossProduct(forward, Vector3(0.0, 0.0, 1.0));
        right.normalize();
        Vector3 up = Vector3::crossProduct(right, forward).normalized();

        static const std::array<LegDef, 6> legs = { {
            { "FrontLeftCoxa", "FrontLeftFemur", "FrontLeftTibia" },
            { "FrontRightCoxa", "FrontRightFemur", "FrontRightTibia" },
            { "MiddleLeftCoxa", "MiddleLeftFemur", "MiddleLeftTibia" },
            { "MiddleRightCoxa", "MiddleRightFemur", "MiddleRightTibia" },
            { "BackLeftCoxa", "BackLeftFemur", "BackLeftTibia" },
            { "BackRightCoxa", "BackRightFemur", "BackRightTibia" },
        } };

        struct LegRest {
            Vector3 coxaPos, coxaEnd, femurEnd, tibiaEnd;
            Vector3 restStickDir, restCoxaToFemurVec;
        };

        std::array<LegRest, 6> legRest;
        for (size_t i = 0; i < 6; ++i) {
            legRest[i].coxaPos = getBonePos(legs[i].coxaName);
            legRest[i].coxaEnd = getBoneEnd(legs[i].coxaName);
            legRest[i].femurEnd = getBoneEnd(legs[i].femurName);
            legRest[i].tibiaEnd = getBoneEnd(legs[i].tibiaName);
            Vector3 chordVec = legRest[i].tibiaEnd - legRest[i].coxaEnd;
            legRest[i].restStickDir = chordVec.isZero() ? Vector3(1, 0, 0) : chordVec.normalized();
            legRest[i].restCoxaToFemurVec = legRest[i].femurEnd - legRest[i].coxaEnd;
        }

        double stepLengthFactor = parameters.getValue("stepLengthFactor", 1.0);
        double stepHeightFactor = parameters.getValue("stepHeightFactor", 1.0);
        double bodyBobFactor = parameters.getValue("bodyBobFactor", 1.0);
        double gaitSpeedFactor = parameters.getValue("gaitSpeedFactor", 1.0);

        double bodyLength = bodyVector.length();
        double bodyBobAmp = bodyLength * 0.03 * bodyBobFactor;
        double bodyForwardAmp = bodyLength * 0.2 * stepLengthFactor;
        double bodyLeanAmp = 0.15 * stepHeightFactor; // radians
        double wingFlapAmp = 0.8; // radians (approx 45 degrees)

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        const double cycles = std::max(1.0, std::round(gaitSpeedFactor));

        for (int frame = 0; frame < frameCount; ++frame) {
            // Forward is a loopable clip: tNormalized spans [0, 1) so frame 0 and a
            // hypothetical extra frame are identical, enabling seamless looping.
            // One-shot clips (e.g. die) use frameCount - 1 to reach exactly 1.0.
            double tNormalized = static_cast<double>(frame) / static_cast<double>(frameCount);
            double t = fmod(tNormalized * cycles, 1.0);

            double bodyBob = bodyBobAmp * std::sin(t * 2.0 * Math::Pi);
            double forwardShift = bodyForwardAmp * std::sin(t * 2.0 * Math::Pi);
            double bodyPitch = -bodyLeanAmp + bodyLeanAmp * 0.3 * std::sin(t * 2.0 * Math::Pi);
            double bodyRoll = 0.08 * std::sin(t * 4.0 * Math::Pi);

            Matrix4x4 bodyTransform;
            bodyTransform.translate(forward * forwardShift + up * bodyBob);
            bodyTransform.rotate(right, bodyPitch);
            bodyTransform.rotate(forward, bodyRoll);

            std::array<Vector3, 6> footTarget;

            // In-flight legs are folded and kept high near the body; they should not track a ground path.
            // Add a gentle idle-sway to avoid a rigid, starting-walk feel.
            for (size_t i = 0; i < 6; ++i) {
                Vector3 hipPos = bodyTransform.transformPoint(legRest[i].coxaPos);
                Vector3 tipOffset = (legRest[i].tibiaEnd - legRest[i].coxaPos).normalized();

                double legPhase = static_cast<double>(i) / 6.0 * Math::Pi;
                double liftBias = bodyLength * 0.22; // high flight leg position
                double foldBack = bodyLength * 0.14;
                double sway = bodyLength * 0.02 * std::sin(t * 4.0 * Math::Pi + legPhase);

                Vector3 desired = hipPos
                    + forward * -foldBack
                    + up * (liftBias + sway)
                    + right * (bodyLength * 0.05 * ((i % 2 == 0) ? 1.0 : -1.0));

                // Prevent fully straight legs by slightly pulling toward hip
                Vector3 foldTarget = hipPos + (desired - hipPos) * 0.7;
                footTarget[i] = foldTarget;
            }

            std::map<std::string, Matrix4x4> boneWorldTransforms;

            auto computeBodyBone = [&](const std::string& name) {
                Vector3 pos = getBonePos(name);
                Vector3 end = getBoneEnd(name);
                Vector3 newPos = bodyTransform.transformPoint(pos);
                Vector3 newEnd = bodyTransform.transformPoint(end);
                boneWorldTransforms[name] = insect::buildBoneWorldTransform(newPos, newEnd);
            };

            computeBodyBone("Head");
            computeBodyBone("Thorax");
            computeBodyBone("Abdomen");

            for (const char* wingName : { "LeftWing", "RightWing" }) {
                if (boneIdx.count(wingName) == 0)
                    continue;

                Vector3 wingPos = getBonePos(wingName);
                Vector3 wingEnd = getBoneEnd(wingName);
                Vector3 worldWingStart = bodyTransform.transformPoint(wingPos);
                Vector3 worldWingEnd = bodyTransform.transformPoint(wingEnd);
                Vector3 wingDir = worldWingEnd - worldWingStart;
                double wingLen = wingDir.length();

                if (wingLen < 1e-8) {
                    boneWorldTransforms[wingName] = insect::buildBoneWorldTransform(worldWingStart, worldWingStart);
                    continue;
                }

                Vector3 wingAxis = Vector3::crossProduct(wingDir.normalized(), up);
                if (wingAxis.lengthSquared() < 1e-8)
                    wingAxis = right;
                else
                    wingAxis.normalize();

                double side = (std::strcmp(wingName, "LeftWing") == 0) ? 1.0 : -1.0;
                double wingAngle = wingFlapAmp * std::sin(t * 6.0 * Math::Pi) * side;
                Quaternion wingRot = Quaternion::fromAxisAndAngle(wingAxis, wingAngle);
                Matrix4x4 wingRotMat;
                wingRotMat.rotate(wingRot);

                Vector3 rotatedDir = wingRotMat.transformVector(wingDir.normalized()) * wingLen;
                Vector3 rotatedEnd = worldWingStart + rotatedDir;
                boneWorldTransforms[wingName] = insect::buildBoneWorldTransform(worldWingStart, rotatedEnd);
            }

            for (size_t i = 0; i < 6; ++i) {
                Vector3 hipPos = bodyTransform.transformPoint(legRest[i].coxaPos);
                Vector3 coxaEnd = bodyTransform.transformPoint(legRest[i].coxaEnd);
                Vector3 tibiaEnd = bodyTransform.transformPoint(legRest[i].tibiaEnd);
                std::vector<Vector3> chain = { hipPos, coxaEnd, tibiaEnd };

                // Pole vector: insect legs bend upward
                Vector3 poleVector = coxaEnd + up * 0.5;
                insect::solveTwoBoneIk(chain, footTarget[i], poleVector);

                Vector3 newStickDir = (chain[2] - chain[1]);
                if (newStickDir.isZero())
                    newStickDir = legRest[i].restStickDir;
                else
                    newStickDir.normalize();

                Quaternion stickRot = Quaternion::rotationTo(legRest[i].restStickDir, newStickDir);
                Matrix4x4 stickRotMat;
                stickRotMat.rotate(stickRot);
                Vector3 femurEnd = chain[1] + stickRotMat.transformVector(legRest[i].restCoxaToFemurVec);

                boneWorldTransforms[legs[i].coxaName] = buildBoneWorldTransform(chain[0], chain[1]);
                boneWorldTransforms[legs[i].femurName] = buildBoneWorldTransform(chain[1], femurEnd);
                boneWorldTransforms[legs[i].tibiaName] = buildBoneWorldTransform(femurEnd, chain[2]);
            }

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
