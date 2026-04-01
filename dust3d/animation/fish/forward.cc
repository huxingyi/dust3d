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
        double tailAmplitudeRatio = parameters.getValue("tailAmplitudeRatio", 2.5); // tail amplitude vs body ratio

        // Body motion parameters
        double bodyBob = parameters.getValue("bodyBob", 0.02); // vertical bobbing amplitude
        double bodyRoll = parameters.getValue("bodyRoll", 0.05); // subtle body roll (radians)
        double forwardThrust = parameters.getValue("forwardThrust", 0.08); // forward surge amplitude

        // Fin motion parameters
        double pectoralFlapPower = parameters.getValue("pectoralFlapPower", 0.4); // pectoral fin flap amplitude
        double pelvicFlapPower = parameters.getValue("pelvicFlapPower", 0.25); // pelvic fin flap amplitude
        double dorsalSwayPower = parameters.getValue("dorsalSwayPower", 0.2); // dorsal fin sway amplitude
        double ventralSwayPower = parameters.getValue("ventralSwayPower", 0.2); // ventral fin sway amplitude

        // Phase offsets for different fin types
        double pectoralPhaseOffset = parameters.getValue("pectoralPhaseOffset", 0.0); // pectoral fin phase offset
        double pelvicPhaseOffset = parameters.getValue("pelvicPhaseOffset", 0.5); // pelvic fin phase offset

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

            // Create undulating spine motion using wave displacement at spine junctions
            constexpr size_t numSpineBones = sizeof(spineBones) / sizeof(SpineBone);

            // Compute lateral displacement from the traveling wave at each junction
            // junctions[i] = start of spineBones[i], junctions[numSpineBones] = end of last bone
            double junctionLateral[numSpineBones + 1];
            double junctionSpinePos[numSpineBones + 1];

            for (size_t i = 0; i < numSpineBones; ++i) {
                junctionSpinePos[i] = spineBones[i].spinePosition;
            }
            junctionSpinePos[numSpineBones] = spineBones[numSpineBones - 1].spinePosition
                + (spineBones[numSpineBones - 1].spinePosition - spineBones[numSpineBones - 2].spinePosition);

            for (size_t i = 0; i <= numSpineBones; ++i) {
                double pos = junctionSpinePos[i];
                double wavePhase = phase - (pos * waveLength * 2.0 * Math::Pi);
                double ampScale = (i < numSpineBones) ? spineBones[i].amplitudeScale
                                                      : spineBones[numSpineBones - 1].amplitudeScale;
                junctionLateral[i] = spineAmplitude * ampScale * tailAmplitudeRatio * sin(wavePhase);
            }

            // Start chain from root end, offset by the head's wave displacement
            Vector3 currentPos = newRootEnd + right * junctionLateral[0];

            for (size_t i = 0; i < numSpineBones; ++i) {
                const auto& bone = spineBones[i];

                if (boneIdx.find(bone.name) == boneIdx.end())
                    continue;

                Vector3 restPos = getBonePos(bone.name);
                Vector3 restEnd = getBoneEnd(bone.name);
                Vector3 restBoneDir = restEnd - restPos;
                double boneLength = restBoneDir.length();

                // Forward component: bone's rest direction projected onto spine axis
                double fwdComponent = Vector3::dotProduct(restBoneDir, forwardDir);
                // Lateral component: wave displacement difference between junctions
                double latComponent = junctionLateral[i + 1] - junctionLateral[i];

                // Compose bone direction and preserve bone length
                Vector3 newBoneDir = forwardDir * fwdComponent + right * latComponent;
                newBoneDir = newBoneDir.normalized() * boneLength;

                Vector3 boneStart = currentPos;
                Vector3 boneEnd = boneStart + newBoneDir;

                boneWorldTransforms[bone.name] = animation::buildBoneWorldTransform(boneStart, boneEnd);

                // Update position for next bone in chain
                currentPos = boneEnd;
            }

            // Animate pectoral fins (attached to BodyFront)
            for (int side = 0; side < 2; ++side) {
                const char* finName = (side == 0) ? "LeftPectoralFin" : "RightPectoralFin";
                if (boneIdx.find(finName) == boneIdx.end())
                    continue;

                double sideSign = (side == 0) ? 1.0 : -1.0;

                // Get parent body segment transform
                auto bodyFrontIt = boneWorldTransforms.find("BodyFront");
                if (bodyFrontIt == boneWorldTransforms.end())
                    continue;

                Vector3 finRestPos = getBonePos(finName);
                Vector3 finRestEnd = getBoneEnd(finName);
                Vector3 bodyFrontRestPos = getBonePos("BodyFront");
                Vector3 finOffset = finRestPos - bodyFrontRestPos;
                Vector3 finRestDir = finRestEnd - finRestPos;

                // Calculate fin attachment point on animated body
                Vector3 finPos = bodyFrontIt->second.transformPoint(finOffset);

                // Apply pectoral fin flapping motion
                double flapAngle = pectoralFlapPower * sin(phase + pectoralPhaseOffset + sideSign * 0.2) * sideSign;
                Quaternion flapRotation = Quaternion::fromAxisAndAngle(up, flapAngle);
                Matrix4x4 finTransform;
                finTransform.rotate(flapRotation);

                Vector3 finDir = bodyFrontIt->second.transformVector(finTransform.transformVector(finRestDir));
                Vector3 finEnd = finPos + finDir;

                boneWorldTransforms[finName] = animation::buildBoneWorldTransform(finPos, finEnd);
            }

            // Animate pelvic fins (attached to BodyMid)
            for (int side = 0; side < 2; ++side) {
                const char* finName = (side == 0) ? "LeftPelvicFin" : "RightPelvicFin";
                if (boneIdx.find(finName) == boneIdx.end())
                    continue;

                double sideSign = (side == 0) ? 1.0 : -1.0;

                auto bodyMidIt = boneWorldTransforms.find("BodyMid");
                if (bodyMidIt == boneWorldTransforms.end())
                    continue;

                Vector3 finRestPos = getBonePos(finName);
                Vector3 finRestEnd = getBoneEnd(finName);
                Vector3 bodyMidRestPos = getBonePos("BodyMid");
                Vector3 finOffset = finRestPos - bodyMidRestPos;
                Vector3 finRestDir = finRestEnd - finRestPos;

                Vector3 finPos = bodyMidIt->second.transformPoint(finOffset);

                double flapAngle = pelvicFlapPower * sin(phase + pelvicPhaseOffset + sideSign * 0.15) * sideSign;
                Quaternion flapRotation = Quaternion::fromAxisAndAngle(up, flapAngle);
                Matrix4x4 finTransform;
                finTransform.rotate(flapRotation);

                Vector3 finDir = bodyMidIt->second.transformVector(finTransform.transformVector(finRestDir));
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
                Vector3 finOffset = finRestPos - parentRestPos;
                Vector3 finRestDir = finRestEnd - finRestPos;

                Vector3 finPos = parentIt->second.transformPoint(finOffset);

                double swayAngle = dorsalSwayPower * sin(phase - fin.phaseOffset * waveLength);
                Quaternion swayRotation = Quaternion::fromAxisAndAngle(forwardDir, swayAngle);
                Matrix4x4 finTransform;
                finTransform.rotate(swayRotation);

                Vector3 finDir = parentIt->second.transformVector(finTransform.transformVector(finRestDir));
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
                Vector3 finOffset = finRestPos - parentRestPos;
                Vector3 finRestDir = finRestEnd - finRestPos;

                Vector3 finPos = parentIt->second.transformPoint(finOffset);

                // Opposite phase from dorsal fins
                double swayAngle = -ventralSwayPower * sin(phase - fin.phaseOffset * waveLength);
                Quaternion swayRotation = Quaternion::fromAxisAndAngle(forwardDir, swayAngle);
                Matrix4x4 finTransform;
                finTransform.rotate(swayRotation);

                Vector3 finDir = parentIt->second.transformVector(finTransform.transformVector(finRestDir));
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
