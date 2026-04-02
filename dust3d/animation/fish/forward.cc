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

#include <algorithm>
#include <cmath>
#include <dust3d/animation/common.h>
#include <dust3d/animation/fish/forward.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace fish {

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

        static const char* requiredBones[] = {
            "Root", "Head", "BodyFront", "BodyMid", "BodyRear", "TailStart", "TailEnd"
        };

        for (const char* name : requiredBones) {
            if (boneIdx.find(name) == boneIdx.end())
                return false;
        }

        Vector3 bodyFront = getBonePos("Head");
        Vector3 bodyBack = getBoneEnd("TailEnd");
        Vector3 bodyVector = bodyBack - bodyFront;
        if (bodyVector.isZero())
            return false;

        Vector3 forwardDir = bodyVector.normalized();
        Vector3 worldUp(0.0, 1.0, 0.0);
        Vector3 right = Vector3::crossProduct(forwardDir, worldUp);
        if (right.lengthSquared() < 1e-8)
            right = Vector3::crossProduct(forwardDir, Vector3(0.0, 0.0, 1.0));
        right.normalize();
        Vector3 up = Vector3::crossProduct(right, forwardDir).normalized();

        double swimSpeed = parameters.getValue("swimSpeed", 1.0);
        double swimFrequency = parameters.getValue("swimFrequency", 2.0);
        double spineAmplitude = parameters.getValue("spineAmplitude", 0.15);
        double waveLength = std::max(0.001, parameters.getValue("waveLength", 1.0));
        double tailAmplitudeRatio = parameters.getValue("tailAmplitudeRatio", 1.5);
        double bodyBobFactor = parameters.getValue("bodyBob", 0.02);
        double bodyRollFactor = parameters.getValue("bodyRoll", 0.05);
        double forwardThrust = parameters.getValue("forwardThrust", 0.08);
        double pectoralFlapPower = parameters.getValue("pectoralFlapPower", 0.4);
        double pelvicFlapPower = parameters.getValue("pelvicFlapPower", 0.25);
        double dorsalSwayPower = parameters.getValue("dorsalSwayPower", 0.2);
        double ventralSwayPower = parameters.getValue("ventralSwayPower", 0.2);
        double pectoralPhaseOffset = parameters.getValue("pectoralPhaseOffset", 0.0);
        double pelvicPhaseOffset = parameters.getValue("pelvicPhaseOffset", 0.5);

        double bodyLength = bodyVector.length();
        double bodyBobAmp = bodyLength * bodyBobFactor;
        double forwardAmp = bodyLength * forwardThrust;

        animationClip.name = "fishForward";
        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        const std::vector<std::string> spineBones = {
            "Root", "Head", "BodyFront", "BodyMid", "BodyRear", "TailStart", "TailEnd"
        };

        for (int frame = 0; frame < frameCount; ++frame) {
            double tNormalized = static_cast<double>(frame) / static_cast<double>(frameCount);
            // Ensure animation loops seamlessly by completing integer cycles
            double totalCycles = swimSpeed * swimFrequency;
            double completeCycles = std::round(totalCycles);
            double phase = tNormalized * completeCycles * 2.0 * Math::Pi;

            double bodyBob = bodyBobAmp * std::sin(phase);
            double bodyRoll = bodyRollFactor * std::sin(phase * 2.0);
            double bodyForward = forwardAmp * std::sin(phase);

            Matrix4x4 bodyTransform;
            bodyTransform.translate(forwardDir * bodyForward + up * bodyBob);
            bodyTransform.rotate(forwardDir, bodyRoll);

            std::map<std::string, Matrix4x4> boneWorldTransforms;

            std::map<std::string, Vector3> spineLateralOffset;

            for (size_t i = 0; i < spineBones.size(); ++i) {
                const std::string& boneName = spineBones[i];
                if (boneIdx.find(boneName) == boneIdx.end())
                    continue;

                Vector3 pos = getBonePos(boneName);
                Vector3 end = getBoneEnd(boneName);
                Vector3 worldPos = bodyTransform.transformPoint(pos);
                Vector3 worldEnd = bodyTransform.transformPoint(end);

                double segmentFactor = static_cast<double>(i) / static_cast<double>(spineBones.size() - 1);
                double segmentAmp = spineAmplitude * (1.0 + (tailAmplitudeRatio - 1.0) * segmentFactor);
                double phaseOffset = (segmentFactor / waveLength) * 2.0 * Math::Pi;
                double lateral = segmentAmp * std::sin(phase - phaseOffset);

                Vector3 lateralOffset = right * lateral;
                worldPos += lateralOffset;
                worldEnd += lateralOffset;

                boneWorldTransforms[boneName] = animation::buildBoneWorldTransform(worldPos, worldEnd);
                spineLateralOffset[boneName] = lateralOffset;
            }

            auto getParentLateralOffset = [&](const std::string& boneName) {
                if (boneIdx.find(boneName) == boneIdx.end())
                    return Vector3();
                const std::string& parentName = rigStructure.bones[boneIdx[boneName]].parent;
                auto it = spineLateralOffset.find(parentName);
                if (it == spineLateralOffset.end())
                    return Vector3();
                return it->second;
            };

            auto applyFinSway = [&](const char* boneName, double basePower, double offsetFactor) {
                if (boneIdx.count(boneName) == 0)
                    return;
                Vector3 pos = getBonePos(boneName);
                Vector3 end = getBoneEnd(boneName);
                Vector3 worldPos = bodyTransform.transformPoint(pos);
                Vector3 worldEnd = bodyTransform.transformPoint(end);
                Vector3 parentOffset = getParentLateralOffset(boneName);
                worldPos += parentOffset;
                worldEnd += parentOffset;
                Vector3 dir = worldEnd - worldPos;
                double sway = basePower * std::sin(phase + offsetFactor);
                Quaternion rot = Quaternion::fromAxisAndAngle(forwardDir, sway);
                Matrix4x4 rotMat;
                rotMat.rotate(rot);
                Vector3 rotatedEnd = worldPos + rotMat.transformVector(dir);
                boneWorldTransforms[boneName] = animation::buildBoneWorldTransform(worldPos, rotatedEnd);
            };

            applyFinSway("DorsalFinFront", dorsalSwayPower * bodyLength, 0.1);
            applyFinSway("DorsalFinMid", dorsalSwayPower * bodyLength, 0.3);
            applyFinSway("DorsalFinRear", dorsalSwayPower * bodyLength, 0.6);
            applyFinSway("VentralFinFront", ventralSwayPower * bodyLength, 0.1);
            applyFinSway("VentralFinMid", ventralSwayPower * bodyLength, 0.3);
            applyFinSway("VentralFinRear", ventralSwayPower * bodyLength, 0.6);

            auto applyLateralFin = [&](const char* boneName, double power, double phaseOffset, double sideSign) {
                if (boneIdx.count(boneName) == 0)
                    return;
                Vector3 pos = getBonePos(boneName);
                Vector3 end = getBoneEnd(boneName);
                Vector3 worldPos = bodyTransform.transformPoint(pos);
                Vector3 worldEnd = bodyTransform.transformPoint(end);
                Vector3 parentOffset = getParentLateralOffset(boneName);
                worldPos += parentOffset;
                worldEnd += parentOffset;
                Vector3 dir = worldEnd - worldPos;

                double swaying = power * bodyLength * std::sin(phase + phaseOffset) * sideSign;
                Quaternion rot = Quaternion::fromAxisAndAngle(forwardDir, swaying);
                Matrix4x4 rotMat;
                rotMat.rotate(rot);
                Vector3 rotatedEnd = worldPos + rotMat.transformVector(dir);
                boneWorldTransforms[boneName] = animation::buildBoneWorldTransform(worldPos, rotatedEnd);
            };

            applyLateralFin("LeftPectoralFin", pectoralFlapPower, pectoralPhaseOffset, 1.0);
            applyLateralFin("RightPectoralFin", pectoralFlapPower, pectoralPhaseOffset, -1.0);
            applyLateralFin("LeftPelvicFin", pelvicFlapPower, pelvicPhaseOffset, 1.0);
            applyLateralFin("RightPelvicFin", pelvicFlapPower, pelvicPhaseOffset, -1.0);

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

} // namespace fish

} // namespace dust3d
