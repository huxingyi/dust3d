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

    namespace {

        struct SpineBone {
            const char* name;
            double position;
            double amplitudeScale;
        };

        const SpineBone SpineBones[] = {
            { "Head", 0.0, 0.1 },
            { "BodyFront", 0.2, 0.3 },
            { "BodyMid", 0.5, 0.6 },
            { "BodyRear", 0.75, 0.9 },
            { "TailStart", 0.9, 1.2 },
            { "TailEnd", 1.0, 1.5 }
        };

        Vector3 normalizeOrFallback(const Vector3& value, const Vector3& fallback)
        {
            if (value.isZero())
                return fallback;
            return value.normalized();
        }

    } // namespace

    bool forward(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        int frameCount,
        float durationSeconds,
        const AnimationParams& parameters)
    {
        std::map<std::string, size_t> boneIdx = animation::buildBoneIndexMap(rigStructure);

        auto getBonePos = [&](const std::string& name) -> Vector3 {
            return animation::getBonePos(rigStructure, boneIdx, name);
        };

        auto getBoneEnd = [&](const std::string& name) -> Vector3 {
            return animation::getBoneEnd(rigStructure, boneIdx, name);
        };

        static const char* requiredBones[] = {
            "Root", "Head", "BodyFront", "BodyMid", "BodyRear",
            "TailStart", "TailEnd"
        };
        constexpr size_t numRequiredBones = sizeof(requiredBones) / sizeof(requiredBones[0]);

        if (!animation::validateRequiredBones(boneIdx, requiredBones, numRequiredBones))
            return false;

        Vector3 headPos = getBonePos("Head");
        Vector3 tailEndPos = getBoneEnd("TailEnd");
        Vector3 bodyVector = headPos - tailEndPos;
        if (bodyVector.isZero())
            return false;

        double bodyLength = bodyVector.length();
        Vector3 forwardDir = bodyVector.normalized();
        Vector3 worldUp(0.0, 1.0, 0.0);
        Vector3 right = Vector3::crossProduct(forwardDir, worldUp);
        if (right.lengthSquared() < 1e-8)
            right = Vector3::crossProduct(forwardDir, Vector3(0.0, 0.0, 1.0));
        right = normalizeOrFallback(right, Vector3(1.0, 0.0, 0.0));
        Vector3 up = Vector3::crossProduct(right, forwardDir).normalized();

        double swimSpeed = parameters.getValue("swimSpeed", 1.0);
        double swimFrequency = parameters.getValue("swimFrequency", 2.0);

        double spineAmplitude = parameters.getValue("spineAmplitude", 0.15);
        double waveLength = parameters.getValue("waveLength", 1.0);
        double tailAmplitudeRatio = parameters.getValue("tailAmplitudeRatio", 1.5);

        double bodyBob = parameters.getValue("bodyBob", 0.02);
        double bodyRoll = parameters.getValue("bodyRoll", 0.05);
        double forwardThrust = parameters.getValue("forwardThrust", 0.08);

        double pectoralFlapPower = parameters.getValue("pectoralFlapPower", 0.4);
        double pelvicFlapPower = parameters.getValue("pelvicFlapPower", 0.25);
        double dorsalSwayPower = parameters.getValue("dorsalSwayPower", 0.2);
        double ventralSwayPower = parameters.getValue("ventralSwayPower", 0.2);

        double pectoralPhaseOffset = parameters.getValue("pectoralPhaseOffset", 0.0);
        double pelvicPhaseOffset = parameters.getValue("pelvicPhaseOffset", 0.5);
        double dorsalPhaseOffset = parameters.getValue("dorsalPhaseOffset", 0.0);
        double ventralPhaseOffset = parameters.getValue("ventralPhaseOffset", 0.25);

        animationClip.name = "fishForward";
        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        constexpr size_t numSpineBones = sizeof(SpineBones) / sizeof(SpineBones[0]);
        const double twoPi = 2.0 * Math::Pi;
        const double inverseFrameCount = 1.0 / std::max(1, frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            double t = static_cast<double>(frame) * inverseFrameCount;
            double animTime = t * durationSeconds;
            double phase = animTime * swimSpeed * swimFrequency * twoPi;

            std::map<std::string, Matrix4x4> boneWorldTransforms;

            double bobOffset = std::sin(phase * 2.0) * bodyBob * bodyLength;
            double rollOffset = std::sin(phase + Math::Pi * 0.3) * bodyRoll;
            double thrustOffset = std::sin(phase) * forwardThrust * bodyLength;

            Matrix4x4 rootTransform;
            rootTransform.translate(forwardDir * thrustOffset + up * bobOffset);
            rootTransform.rotate(forwardDir, rollOffset);

            Vector3 rootPos = getBonePos("Root");
            Vector3 rootEnd = getBoneEnd("Root");
            Vector3 newRootPos = rootTransform.transformPoint(rootPos);
            Vector3 newRootEnd = rootTransform.transformPoint(rootEnd);
            boneWorldTransforms["Root"] = animation::buildBoneWorldTransform(newRootPos, newRootEnd);

            for (size_t i = 0; i < numSpineBones; ++i) {
                const auto& bone = SpineBones[i];

                if (boneIdx.find(bone.name) == boneIdx.end())
                    continue;

                Vector3 restPos = getBonePos(bone.name);
                Vector3 restEnd = getBoneEnd(bone.name);
                Vector3 restBoneDir = restEnd - restPos;
                if (i == 0)
                    restBoneDir = -restBoneDir;
                double boneLength = restBoneDir.length();
                if (boneLength < 1e-8)
                    continue;

                double pos = bone.position;
                double wavePhase = phase - (pos * waveLength * twoPi);
                double tailGain = 1.0 + (tailAmplitudeRatio - 1.0) * pos;
                double rotAngle = spineAmplitude * bone.amplitudeScale * tailGain * std::sin(wavePhase);
                double rollAngle = bodyRoll * bone.amplitudeScale * 0.45 * std::cos(wavePhase);

                const char* parentName = (i == 0) ? "Root" : SpineBones[i - 1].name;
                auto parentIt = boneWorldTransforms.find(parentName);
                if (parentIt == boneWorldTransforms.end()) {
                    parentIt = boneWorldTransforms.find("Root");
                }
                if (parentIt == boneWorldTransforms.end())
                    continue;

                Vector3 parentRestPos = getBonePos(parentName);
                Vector3 parentRestEnd = getBoneEnd(parentName);
                Matrix4x4 parentRestTransform = animation::buildBoneWorldTransform(parentRestPos, parentRestEnd);
                Matrix4x4 parentRestInv = parentRestTransform.inverted();

                Vector3 localBonePos = parentRestInv.transformPoint(restPos);
                Vector3 localBoneDir = parentRestInv.transformVector(restBoneDir);
                Vector3 localUpAxis = parentRestInv.transformVector(up);
                if (localUpAxis.lengthSquared() < 1e-8)
                    localUpAxis = Vector3(1.0, 0.0, 0.0);
                else
                    localUpAxis.normalize();

                Vector3 localForwardAxis = parentRestInv.transformVector(restBoneDir);
                if (localForwardAxis.lengthSquared() < 1e-8)
                    localForwardAxis = Vector3(0.0, 0.0, 1.0);
                else
                    localForwardAxis.normalize();

                Quaternion swingRot = Quaternion::fromAxisAndAngle(localUpAxis, rotAngle);
                Matrix4x4 swingMat;
                swingMat.rotate(swingRot);

                Quaternion twistRot = Quaternion::fromAxisAndAngle(localForwardAxis, rollAngle);
                Matrix4x4 twistMat;
                twistMat.rotate(twistRot);

                Vector3 rotatedLocalDir = twistMat.transformVector(swingMat.transformVector(localBoneDir.normalized())) * boneLength;

                Matrix4x4 parentWorldTransform = parentIt->second;
                Vector3 boneStart = parentWorldTransform.transformPoint(localBonePos);
                Vector3 boneEnd = boneStart + parentWorldTransform.transformVector(rotatedLocalDir);

                boneWorldTransforms[bone.name] = animation::buildBoneWorldTransform(boneStart, boneEnd);
            }

            auto animateSideFin = [&](const char* finName, const char* parentBone, double power, double phaseOffset, double sidePhase) {
                if (boneIdx.find(finName) == boneIdx.end())
                    return;

                auto parentIt = boneWorldTransforms.find(parentBone);
                if (parentIt == boneWorldTransforms.end())
                    return;

                Vector3 finRestPos = getBonePos(finName);
                Vector3 finRestEnd = getBoneEnd(finName);
                Vector3 parentRestPos = getBonePos(parentBone);
                Vector3 parentRestEnd = getBoneEnd(parentBone);

                Matrix4x4 parentRestTransform = animation::buildBoneWorldTransform(parentRestPos, parentRestEnd);
                Matrix4x4 parentRestInv = parentRestTransform.inverted();
                Vector3 localFinPos = parentRestInv.transformPoint(finRestPos);
                Vector3 localFinDir = parentRestInv.transformVector(finRestEnd - finRestPos);
                if (localFinDir.lengthSquared() < 1e-8)
                    return;

                Vector3 localFlapAxis = parentRestInv.transformVector(right);
                if (localFlapAxis.lengthSquared() < 1e-8)
                    localFlapAxis = Vector3(1.0, 0.0, 0.0);
                else
                    localFlapAxis.normalize();

                double finPhase = phase + phaseOffset * twoPi + sidePhase;
                double flapAngle = std::sin(finPhase) * power;
                Quaternion finRot = Quaternion::fromAxisAndAngle(localFlapAxis, flapAngle);
                Matrix4x4 finRotMat;
                finRotMat.rotate(finRot);

                Vector3 rotatedLocalFinDir = finRotMat.transformVector(localFinDir);
                Vector3 finPos = parentIt->second.transformPoint(localFinPos);
                Vector3 finEnd = finPos + parentIt->second.transformVector(rotatedLocalFinDir);
                boneWorldTransforms[finName] = animation::buildBoneWorldTransform(finPos, finEnd);
            };

            animateSideFin("LeftPectoralFin", "BodyFront", pectoralFlapPower, pectoralPhaseOffset, 0.0);
            animateSideFin("RightPectoralFin", "BodyFront", pectoralFlapPower, pectoralPhaseOffset, Math::Pi);
            animateSideFin("LeftPelvicFin", "BodyMid", pelvicFlapPower, pelvicPhaseOffset, 0.0);
            animateSideFin("RightPelvicFin", "BodyMid", pelvicFlapPower, pelvicPhaseOffset, Math::Pi);

            auto animateMidlineFin = [&](const char* finName, const char* parentBone, double power, double phaseOffset,
                                         double localPhaseOffset, double directionSign) {
                if (boneIdx.find(finName) == boneIdx.end())
                    return;

                auto parentIt = boneWorldTransforms.find(parentBone);
                if (parentIt == boneWorldTransforms.end())
                    return;

                Vector3 finRestPos = getBonePos(finName);
                Vector3 finRestEnd = getBoneEnd(finName);
                Vector3 parentRestPos = getBonePos(parentBone);
                Vector3 parentRestEnd = getBoneEnd(parentBone);

                Matrix4x4 parentRestTransform = animation::buildBoneWorldTransform(parentRestPos, parentRestEnd);
                Matrix4x4 parentRestInv = parentRestTransform.inverted();
                Vector3 localFinPos = parentRestInv.transformPoint(finRestPos);
                Vector3 localFinDir = parentRestInv.transformVector(finRestEnd - finRestPos);
                if (localFinDir.lengthSquared() < 1e-8)
                    return;

                Vector3 localSwayAxis = parentRestInv.transformVector(forwardDir);
                if (localSwayAxis.lengthSquared() < 1e-8)
                    localSwayAxis = Vector3(0.0, 0.0, 1.0);
                else
                    localSwayAxis.normalize();

                double finPhase = phase + (phaseOffset + localPhaseOffset) * twoPi;
                double swayAngle = std::sin(finPhase + (directionSign < 0.0 ? Math::Pi : 0.0)) * power * directionSign;
                Quaternion finRot = Quaternion::fromAxisAndAngle(localSwayAxis, swayAngle);
                Matrix4x4 finRotMat;
                finRotMat.rotate(finRot);

                Vector3 rotatedLocalFinDir = finRotMat.transformVector(localFinDir);
                Vector3 finPos = parentIt->second.transformPoint(localFinPos);
                Vector3 finEnd = finPos + parentIt->second.transformVector(rotatedLocalFinDir);
                boneWorldTransforms[finName] = animation::buildBoneWorldTransform(finPos, finEnd);
            };

            animateMidlineFin("DorsalFinFront", "BodyFront", dorsalSwayPower, dorsalPhaseOffset, 0.2, 1.0);
            animateMidlineFin("DorsalFinMid", "BodyMid", dorsalSwayPower, dorsalPhaseOffset, 0.5, 1.0);
            animateMidlineFin("DorsalFinRear", "BodyRear", dorsalSwayPower, dorsalPhaseOffset, 0.75, 1.0);
            animateMidlineFin("VentralFinFront", "BodyFront", ventralSwayPower, ventralPhaseOffset, 0.2, -1.0);
            animateMidlineFin("VentralFinMid", "BodyMid", ventralSwayPower, ventralPhaseOffset, 0.5, -1.0);
            animateMidlineFin("VentralFinRear", "BodyRear", ventralSwayPower, ventralPhaseOffset, 0.75, -1.0);

            // After full animation for this frame, vertically flip the Root bone
            // and all child bones by rotating 180 degrees around the Root right axis.
            auto rootIt = boneWorldTransforms.find("Root");
            if (rootIt != boneWorldTransforms.end()) {
                Vector3 rootWorldPos = rootIt->second.transformPoint(Vector3(0.0, 0.0, 0.0));
                Vector3 rootRight = rootIt->second.transformVector(Vector3(1.0, 0.0, 0.0));
                if (rootRight.isZero())
                    rootRight = Vector3(1.0, 0.0, 0.0);
                else
                    rootRight.normalize();

                Matrix4x4 rootFlip;
                rootFlip.translate(rootWorldPos);
                rootFlip.rotate(rootRight, Math::Pi);
                rootFlip.translate(rootWorldPos * -1.0);

                for (auto& pair : boneWorldTransforms) {
                    Matrix4x4 transformed = rootFlip;
                    transformed *= pair.second;
                    pair.second = transformed;
                }
            }

            auto& animFrame = animationClip.frames[frame];
            animFrame.time = static_cast<float>(animTime);
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
