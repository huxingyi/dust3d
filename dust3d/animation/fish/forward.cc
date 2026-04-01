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

#include <QDebug>
#include <cmath>
#include <cstring>
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
        // Build bone index map using the common utility
        std::map<std::string, size_t> boneIdx = animation::buildBoneIndexMap(rigStructure);

        // Helper lambdas for bone position lookups
        auto getBonePos = [&](const std::string& name) -> Vector3 {
            return animation::getBonePos(rigStructure, boneIdx, name);
        };

        auto getBoneEnd = [&](const std::string& name) -> Vector3 {
            return animation::getBoneEnd(rigStructure, boneIdx, name);
        };

        // Required bones for fish animation
        static const char* requiredBones[] = {
            "Root", "Head", "BodyFront", "BodyMid", "BodyRear",
            "TailStart", "TailEnd"
        };
        constexpr size_t numRequiredBones = sizeof(requiredBones) / sizeof(requiredBones[0]);

        if (!animation::validateRequiredBones(boneIdx, requiredBones, numRequiredBones))
            return false;

        // Calculate body length and coordinate frame
        Vector3 headPos = getBonePos("Head");
        Vector3 tailEndPos = getBoneEnd("TailEnd");
        Vector3 bodyVector = headPos - tailEndPos;
        if (bodyVector.isZero())
            return false;

        double bodyLength = bodyVector.length();
        Vector3 forwardDir = bodyVector.normalized();
        Vector3 worldUp(0.0, 1.0, 0.0);
        Vector3 right = Vector3::crossProduct(forwardDir, worldUp).normalized();
        if (right.isZero()) {
            right = Vector3::crossProduct(forwardDir, Vector3(0.0, 0.0, 1.0)).normalized();
        }
        Vector3 up = Vector3::crossProduct(right, forwardDir).normalized();

        // ANIMATION PARAMETERS - Exposed for user customization
        // Swimming speed and timing
        double swimSpeed = parameters.getValue("swimSpeed", 1.0); // cycles per second (0.5 = slow, 2.0 = fast)
        double swimFrequency = parameters.getValue("swimFrequency", 2.0); // tail beat frequency multiplier

        // Body undulation parameters
        double spineAmplitude = parameters.getValue("spineAmplitude", 0.15); // spine side-to-side amplitude (radians)
        double waveLength = parameters.getValue("waveLength", 1.0); // undulation wavelength factor
        double tailAmplitudeRatio = parameters.getValue("tailAmplitudeRatio", 1.5); // tail amplitude vs body ratio

        // Body motion parameters
        double bodyBob = parameters.getValue("bodyBob", 0.02); // vertical bobbing amplitude
        double bodyRoll = parameters.getValue("bodyRoll", 0.05); // subtle body roll (radians)
        double forwardThrust = parameters.getValue("forwardThrust", 0.08); // forward surge amplitude

        // Fin motion parameters (unused for static fins but kept for future compatibility)
        (void)parameters.getValue("pectoralFlapPower", 0.4);
        (void)parameters.getValue("pelvicFlapPower", 0.25);
        (void)parameters.getValue("dorsalSwayPower", 0.2);
        (void)parameters.getValue("ventralSwayPower", 0.2);

        // Phase offsets for different fin types (unused for static fins)
        (void)parameters.getValue("pectoralPhaseOffset", 0.0);
        (void)parameters.getValue("pelvicPhaseOffset", 0.5);

        // Setup animation
        animationClip.name = "fishForward";
        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        // Define spine bone chain with positions along body (0 = head, 1 = tail tip)
        struct SpineBone {
            const char* name;
            double spinePosition; // 0.0 at head, 1.0 at tail tip
            double amplitudeScale; // amplitude multiplier for this segment
        };

        static const SpineBone spineBones[] = {
            { "Head", 0.0, 0.1 }, // minimal movement at head
            { "BodyFront", 0.2, 0.3 }, // gentle undulation starts
            { "BodyMid", 0.5, 0.6 }, // progressive increase
            { "BodyRear", 0.75, 0.9 }, // stronger movement near tail
            { "TailStart", 0.9, 1.2 }, // tail begins strong motion
            { "TailEnd", 1.0, 1.5 } // maximum amplitude at tail tip
        };

        // Generate animation frames
        for (int frame = 0; frame < frameCount; ++frame) {
            double t = static_cast<double>(frame) / static_cast<double>(frameCount);
            double animTime = t * durationSeconds;
            double phase = animTime * swimSpeed * swimFrequency * 2.0 * Math::Pi;

            std::map<std::string, Matrix4x4> boneWorldTransforms;

            // Calculate root motion: subtle body movements for realism
            double bobOffset = sin(phase * 2.0) * bodyBob * bodyLength;
            double rollOffset = sin(phase + Math::Pi * 0.3) * bodyRoll;
            double thrustOffset = sin(phase) * forwardThrust * bodyLength;

            // Root transform with overall body motion
            Matrix4x4 rootTransform;
            rootTransform.translate(forwardDir * thrustOffset + up * bobOffset);
            rootTransform.rotate(forwardDir, rollOffset);

            // Root bone follows the overall motion
            Vector3 rootPos = getBonePos("Root");
            Vector3 rootEnd = getBoneEnd("Root");
            Vector3 newRootPos = rootTransform.transformPoint(rootPos);
            Vector3 newRootEnd = rootTransform.transformPoint(rootEnd);
            boneWorldTransforms["Root"] = animation::buildBoneWorldTransform(newRootPos, newRootEnd);

            // Create undulating spine motion using rotations around the body axis
            constexpr size_t numSpineBones = sizeof(spineBones) / sizeof(SpineBone);

            for (size_t i = 0; i < numSpineBones; ++i) {
                const auto& bone = spineBones[i];

                if (boneIdx.find(bone.name) == boneIdx.end())
                    continue;

                Vector3 restPos = getBonePos(bone.name);
                Vector3 restEnd = getBoneEnd(bone.name);
                Vector3 restBoneDir = restEnd - restPos;
                if (std::string(bone.name) == "Head") {
                    // head bone is oriented to the natural forward direction,
                    // reverse it to maintain consistent spine tailward chain direction
                    restBoneDir = -restBoneDir;
                }
                double boneLength = restBoneDir.length();

                // Calculate rotation for undulation
                double pos = bone.spinePosition;
                double wavePhase = phase - (pos * waveLength * 2.0 * Math::Pi);
                double ampScale = bone.amplitudeScale;
                double rotAngle = spineAmplitude * ampScale * tailAmplitudeRatio * sin(wavePhase);

                // Determine parent bone for hierarchical spine motion
                const char* parentName = (i == 0) ? "Root" : spineBones[i - 1].name;
                auto parentIt = boneWorldTransforms.find(parentName);
                if (parentIt == boneWorldTransforms.end()) {
                    // fallback to root transform if unexpectedly missing
                    parentIt = boneWorldTransforms.find("Root");
                }

                // Convert bone rest positions into parent-local space
                Vector3 parentRestPos = getBonePos(parentName);
                Vector3 parentRestEnd = getBoneEnd(parentName);
                Matrix4x4 parentRestTransform = animation::buildBoneWorldTransform(parentRestPos, parentRestEnd);
                Matrix4x4 parentRestInv = parentRestTransform.inverted();

                Vector3 localBonePos = parentRestInv.transformPoint(restPos);
                Vector3 localBoneDir = parentRestInv.transformVector(restBoneDir);

                // Use fixed parent-local up axis for consistent spine bend orientation
                Vector3 localUp(0.0, 1.0, 0.0);
                Quaternion rot = Quaternion::fromAxisAndAngle(localUp, rotAngle);
                Matrix4x4 rotMatrix;
                rotMatrix.rotate(rot);
                Vector3 rotatedLocalDir = rotMatrix.transformVector(localBoneDir.normalized()) * boneLength;

                // Transform into world space via parent world transform
                Matrix4x4 parentWorldTransform = parentIt->second;
                Vector3 boneStart = parentWorldTransform.transformPoint(localBonePos);
                Vector3 boneEnd = boneStart + parentWorldTransform.transformVector(rotatedLocalDir);

                Vector3 axisWorld = parentWorldTransform.transformVector(localUp).normalized();

                qDebug()
                    << "[fish::forward] frame" << frame
                    << "bone" << bone.name
                    << "pos" << pos
                    << "wavePhase" << wavePhase
                    << "rotAngle" << rotAngle
                    << "axisLocal" << localUp.x() << localUp.y() << localUp.z()
                    << "axisWorld" << axisWorld.x() << axisWorld.y() << axisWorld.z()
                    << "restBoneDir" << restBoneDir.x() << restBoneDir.y() << restBoneDir.z()
                    << "rotatedDir" << (boneEnd - boneStart).x() << (boneEnd - boneStart).y() << (boneEnd - boneStart).z();

                boneWorldTransforms[bone.name] = animation::buildBoneWorldTransform(boneStart, boneEnd);
            }

            // Animate pectoral fins (attached to BodyFront)
            for (int side = 0; side < 2; ++side) {
                const char* finName = (side == 0) ? "LeftPectoralFin" : "RightPectoralFin";
                if (boneIdx.find(finName) == boneIdx.end())
                    continue;

                // Get parent body segment transform
                auto bodyFrontIt = boneWorldTransforms.find("BodyFront");
                if (bodyFrontIt == boneWorldTransforms.end())
                    continue;

                Vector3 finRestPos = getBonePos(finName);
                Vector3 finRestEnd = getBoneEnd(finName);
                Vector3 bodyFrontRestPos = getBonePos("BodyFront");
                Vector3 bodyFrontRestEnd = getBoneEnd("BodyFront");

                // Parent-space fin transform to avoid flipping artifacts
                Matrix4x4 parentRestTransform = animation::buildBoneWorldTransform(bodyFrontRestPos, bodyFrontRestEnd);
                Matrix4x4 parentRestInv = parentRestTransform.inverted();
                Vector3 localFinPos = parentRestInv.transformPoint(finRestPos);
                Vector3 localFinEnd = parentRestInv.transformPoint(finRestEnd);
                Vector3 localFinDir = localFinEnd - localFinPos;

                Vector3 finPos = bodyFrontIt->second.transformPoint(localFinPos);
                Vector3 finDir = bodyFrontIt->second.transformVector(localFinDir);
                Vector3 finEnd = finPos + finDir;

                boneWorldTransforms[finName] = animation::buildBoneWorldTransform(finPos, finEnd);
            }

            // Animate pelvic fins (attached to BodyMid)
            for (int side = 0; side < 2; ++side) {
                const char* finName = (side == 0) ? "LeftPelvicFin" : "RightPelvicFin";
                if (boneIdx.find(finName) == boneIdx.end())
                    continue;

                auto bodyMidIt = boneWorldTransforms.find("BodyMid");
                if (bodyMidIt == boneWorldTransforms.end())
                    continue;

                Vector3 finRestPos = getBonePos(finName);
                Vector3 finRestEnd = getBoneEnd(finName);
                Vector3 bodyMidRestPos = getBonePos("BodyMid");
                Vector3 bodyMidRestEnd = getBoneEnd("BodyMid");

                Matrix4x4 parentRestTransform = animation::buildBoneWorldTransform(bodyMidRestPos, bodyMidRestEnd);
                Matrix4x4 parentRestInv = parentRestTransform.inverted();
                Vector3 localFinPos = parentRestInv.transformPoint(finRestPos);
                Vector3 localFinEnd = parentRestInv.transformPoint(finRestEnd);
                Vector3 localFinDir = localFinEnd - localFinPos;

                Vector3 finPos = bodyMidIt->second.transformPoint(localFinPos);
                Vector3 finDir = bodyMidIt->second.transformVector(localFinDir);
                Vector3 finEnd = finPos + finDir;

                boneWorldTransforms[finName] = animation::buildBoneWorldTransform(finPos, finEnd);
            }

            // Animate dorsal fins (sway with body undulation)
            static const struct {
                const char* finName;
                const char* parentBone;
                double phaseOffset;
            } dorsalFins[] = {
                { "DorsalFinFront", "BodyFront", 0.2 },
                { "DorsalFinMid", "BodyMid", 0.5 },
                { "DorsalFinRear", "BodyRear", 0.75 }
            };

            for (const auto& fin : dorsalFins) {
                if (boneIdx.find(fin.finName) == boneIdx.end())
                    continue;

                auto parentIt = boneWorldTransforms.find(fin.parentBone);
                if (parentIt == boneWorldTransforms.end())
                    continue;

                Vector3 finRestPos = getBonePos(fin.finName);
                Vector3 finRestEnd = getBoneEnd(fin.finName);
                Vector3 parentRestPos = getBonePos(fin.parentBone);
                Vector3 parentRestEnd = getBoneEnd(fin.parentBone);

                Matrix4x4 parentRestTransform = animation::buildBoneWorldTransform(parentRestPos, parentRestEnd);
                Matrix4x4 parentRestInv = parentRestTransform.inverted();
                Vector3 localFinPos = parentRestInv.transformPoint(finRestPos);
                Vector3 localFinEnd = parentRestInv.transformPoint(finRestEnd);
                Vector3 localFinDir = localFinEnd - localFinPos;

                Vector3 finPos = parentIt->second.transformPoint(localFinPos);
                Vector3 finDir = parentIt->second.transformVector(localFinDir);
                Vector3 finEnd = finPos + finDir;

                boneWorldTransforms[fin.finName] = animation::buildBoneWorldTransform(finPos, finEnd);
            }

            // Animate ventral fins (sway opposite to dorsals)
            static const struct {
                const char* finName;
                const char* parentBone;
                double phaseOffset;
            } ventralFins[] = {
                { "VentralFinFront", "BodyFront", 0.2 },
                { "VentralFinMid", "BodyMid", 0.5 },
                { "VentralFinRear", "BodyRear", 0.75 }
            };

            for (const auto& fin : ventralFins) {
                if (boneIdx.find(fin.finName) == boneIdx.end())
                    continue;

                auto parentIt = boneWorldTransforms.find(fin.parentBone);
                if (parentIt == boneWorldTransforms.end())
                    continue;

                Vector3 finRestPos = getBonePos(fin.finName);
                Vector3 finRestEnd = getBoneEnd(fin.finName);
                Vector3 parentRestPos = getBonePos(fin.parentBone);
                Vector3 parentRestEnd = getBoneEnd(fin.parentBone);

                Matrix4x4 parentRestTransform = animation::buildBoneWorldTransform(parentRestPos, parentRestEnd);
                Matrix4x4 parentRestInv = parentRestTransform.inverted();
                Vector3 localFinPos = parentRestInv.transformPoint(finRestPos);
                Vector3 localFinEnd = parentRestInv.transformPoint(finRestEnd);
                Vector3 localFinDir = localFinEnd - localFinPos;

                Vector3 finPos = parentIt->second.transformPoint(localFinPos);
                Vector3 finDir = parentIt->second.transformVector(localFinDir);
                Vector3 finEnd = finPos + finDir;

                boneWorldTransforms[fin.finName] = animation::buildBoneWorldTransform(finPos, finEnd);
            }

            // Write frame data
            auto& animFrame = animationClip.frames[frame];
            animFrame.time = static_cast<float>(t) * durationSeconds;
            animFrame.boneWorldTransforms = boneWorldTransforms;

            // Calculate skin matrices
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
