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
#include <dust3d/animation/snake/slither.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace snake {

    bool slither(const RigStructure& rigStructure,
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

        static const std::vector<std::string> requiredBones = {
            "Root", "Head", "Spine1", "Spine2", "Spine3", "Spine4", "Spine5", "Spine6",
            "Tail1", "Tail2", "Tail3", "Tail4", "TailTip"
        };

        for (const auto& name : requiredBones) {
            if (boneIdx.find(name) == boneIdx.end())
                return false;
        }

        const std::vector<std::string> backboneBones = {
            "TailTip", "Tail4", "Tail3", "Tail2", "Tail1",
            "Spine1", "Spine2", "Spine3", "Spine4", "Spine5", "Spine6"
        };

        Vector3 headPos = getBonePos("Head");
        Vector3 tailPos = getBonePos("TailTip");
        Vector3 bodyVector = headPos - tailPos;
        if (bodyVector.isZero())
            return false;

        Vector3 forwardDir = bodyVector.normalized();
        Vector3 worldUp(0.0, 1.0, 0.0);
        Vector3 right = Vector3::crossProduct(forwardDir, worldUp);
        if (right.lengthSquared() < 1e-8)
            right = Vector3::crossProduct(forwardDir, Vector3(0.0, 0.0, 1.0));
        right.normalize();
        Vector3 up = Vector3::crossProduct(right, forwardDir).normalized();

        double waveSpeedFactor = parameters.getValue("waveSpeedFactor", 1.0);
        double waveFrequency = parameters.getValue("waveFrequency", 2.0);
        double waveAmplitude = parameters.getValue("waveAmplitude", 0.15);
        double waveLength = std::max(0.001, parameters.getValue("waveLength", 1.0));
        double tailAmplitudeRatio = parameters.getValue("tailAmplitudeRatio", 2.5);
        double headYawFactor = parameters.getValue("headYawFactor", 0.05);
        double headPullFactor = parameters.getValue("headPullFactor", 0.3);
        double jawAmplitude = parameters.getValue("jawAmplitude", 0.25);
        double jawFrequency = std::max(0.001, parameters.getValue("jawFrequency", 1.0));

        double bodyLength = bodyVector.length();
        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        double totalCycles = waveSpeedFactor * waveFrequency;
        double completeCycles = std::round(totalCycles);

        for (int frame = 0; frame < frameCount; ++frame) {
            double tNormalized = static_cast<double>(frame) / static_cast<double>(frameCount);
            double phase = tNormalized * completeCycles * 2.0 * Math::Pi;

            Vector3 baseHeadEnd = getBoneEnd("Head");
            Vector3 headLocalDir = baseHeadEnd - headPos;
            double headWaveAmplitudeFactor = 0.25;
            double headWavePhase = -phase;
            double headLateral = waveAmplitude * headWaveAmplitudeFactor * std::sin(headWavePhase);
            Vector3 headWaveOffset = right * headLateral;
            double headForward = headPullFactor * bodyLength * 0.60 * std::sin(phase);
            Vector3 headPullOffset = headWaveOffset + forwardDir * headForward;
            Vector3 pulledHeadPos = headPos + headPullOffset;

            std::map<std::string, Matrix4x4> boneWorldTransforms;

            for (size_t i = 0; i < backboneBones.size(); ++i) {
                const std::string& boneName = backboneBones[i];
                Vector3 basePos = getBonePos(boneName);
                Vector3 baseEnd = getBoneEnd(boneName);
                if (basePos.isZero() && baseEnd.isZero())
                    continue;

                double tailFactor = static_cast<double>(i) / static_cast<double>(backboneBones.size() - 1);
                double x = (1.0 - tailFactor) * bodyLength;
                double curveFactor = 1.0 - tailFactor; // 0 at head side, 1 at tail side
                double headSideCurve = std::max(curveFactor, 0.05);
                double amplitudeFactor = (0.25 + 0.75 * curveFactor) * (1.0 + (tailAmplitudeRatio - 1.0) * curveFactor);
                double taperPower = 1.0 + headPullFactor * 8.0;
                amplitudeFactor *= std::pow(headSideCurve, taperPower);
                double wavePhase = (x / waveLength) * 2.0 * Math::Pi - phase;
                double lateral = waveAmplitude * amplitudeFactor * std::sin(wavePhase);
                double forwardOffset = headPullFactor * bodyLength * headSideCurve * 1.00 * std::sin(wavePhase);

                Vector3 lateralOffset = right * lateral;
                Vector3 forwardOffsetVec = forwardDir * forwardOffset;
                Vector3 worldPos = basePos + lateralOffset + forwardOffsetVec;
                Vector3 worldEnd = baseEnd + lateralOffset + forwardOffsetVec;

                double spinePullFactor = 0.0;
                if (boneName == "Spine6")
                    spinePullFactor = 0.8;
                else if (boneName == "Spine5")
                    spinePullFactor = 0.6;
                else if (boneName == "Spine4")
                    spinePullFactor = 0.4;
                else if (boneName == "Spine3")
                    spinePullFactor = 0.2;
                else if (boneName == "Spine1")
                    spinePullFactor = 0.1;

                if (spinePullFactor > 0.0) {
                    Vector3 spinePullOffset = headPullOffset * spinePullFactor;
                    worldPos += spinePullOffset;
                    worldEnd += spinePullOffset;
                }

                boneWorldTransforms[boneName] = animation::buildBoneWorldTransform(worldPos, worldEnd);
            }

            Quaternion headRotation = Quaternion::fromAxisAndAngle(up, headYawFactor * std::sin(phase));
            Matrix4x4 headTransform;
            headTransform.translate(pulledHeadPos);
            headTransform.rotate(headRotation);
            Matrix4x4 headRotationMatrix;
            headRotationMatrix.rotate(headRotation);
            Vector3 worldHeadEnd = pulledHeadPos + headRotationMatrix.transformVector(headLocalDir);
            boneWorldTransforms["Head"] = animation::buildBoneWorldTransform(pulledHeadPos, worldHeadEnd);

            if (boneIdx.find("Jaw") != boneIdx.end()) {
                Vector3 jawPos = getBonePos("Jaw");
                Vector3 jawEnd = getBoneEnd("Jaw");
                Vector3 localJawPos = jawPos - headPos;
                Vector3 localJawDir = jawEnd - jawPos;
                Vector3 worldJawPos = headTransform.transformPoint(localJawPos);
                Vector3 worldJawDir = headRotationMatrix.transformVector(localJawDir);
                Vector3 jawHingeWorldRight = headRotationMatrix.transformVector(right);
                double jawOpen = jawAmplitude * std::sin(phase * jawFrequency + Math::Pi / 2.0);
                Quaternion jawRotation = Quaternion::fromAxisAndAngle(jawHingeWorldRight, jawOpen);
                Matrix4x4 jawRotationMatrix;
                jawRotationMatrix.rotate(jawRotation);
                Vector3 worldJawEnd = worldJawPos + jawRotationMatrix.transformVector(worldJawDir);
                boneWorldTransforms["Jaw"] = animation::buildBoneWorldTransform(worldJawPos, worldJawEnd);
            }

            // Keep all other bones in their bind pose if they aren't part of the main serpentine chain.
            for (const auto& bone : rigStructure.bones) {
                if (boneWorldTransforms.find(bone.name) != boneWorldTransforms.end())
                    continue;
                Vector3 worldPos(bone.posX, bone.posY, bone.posZ);
                Vector3 worldEnd(bone.endX, bone.endY, bone.endZ);
                boneWorldTransforms[bone.name] = animation::buildBoneWorldTransform(worldPos, worldEnd);
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

} // namespace snake

} // namespace dust3d
