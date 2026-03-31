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
#include <dust3d/animation/fish/forward.h>
#include <dust3d/animation/insect/common.h>
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

        // Required fish bones
        static const char* requiredBones[] = {
            "Root", "Head", "BodyFront", "BodyMid", "BodyRear",
            "TailStart", "TailEnd",
            "LeftPectoralFin", "RightPectoralFin"
        };

        for (const char* name : requiredBones) {
            if (boneIdx.find(name) == boneIdx.end())
                return false;
        }

        // Determine forward direction from body (Head toward TailEnd along Z)
        Vector3 headPos = getBonePos("Head");
        Vector3 tailEndPos = getBoneEnd("TailEnd");
        Vector3 bodyVector = headPos - tailEndPos;
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
        double stepLengthFactor = parameters.getValue("stepLengthFactor", 1.0);
        double stepHeightFactor = parameters.getValue("stepHeightFactor", 1.0);

        // Swimming motion amplitudes
        double bodyYawAmp = 0.08 * stepHeightFactor; // subtle side-to-side body yaw (radians)
        double bodyForwardAmp = bodyLength * 0.1 * stepLengthFactor; // forward surge
        double bodyBobAmp = bodyLength * 0.015 * bodyBobFactor; // very subtle vertical bob

        // Spine undulation: each body segment lags behind the previous one
        // producing a sinusoidal wave traveling from head to tail
        double spineYawAmp = 0.12 * stepHeightFactor; // radians per segment

        // Tail amplitudes (tail swings more than body)
        double tailStartYawAmp = 0.25 * stepHeightFactor; // radians
        double tailEndYawAmp = 0.40 * stepHeightFactor; // radians

        // Fin parameters
        double pectoralFlapAmp = 0.3 * stepHeightFactor; // radians
        double dorsalSwayAmp = 0.15 * stepHeightFactor; // radians
        double ventralSwayAmp = 0.15 * stepHeightFactor; // radians
        double pelvicFlapAmp = 0.2 * stepHeightFactor; // radians

        animationClip.name = "fishForward";
        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        const double cycles = std::max(1.0, std::round(gaitSpeedFactor));

        for (int frame = 0; frame < frameCount; ++frame) {
            double tNormalized = static_cast<double>(frame) / static_cast<double>(frameCount);
            double t = fmod(tNormalized * cycles, 1.0);
            double phase = t * 2.0 * Math::Pi;

            // Body motion: subtle yaw oscillation and forward surge
            double bodyYaw = bodyYawAmp * std::sin(phase);
            double bodyBob = bodyBobAmp * std::sin(phase * 2.0);
            double bodyForward = bodyForwardAmp * std::sin(phase);

            Matrix4x4 bodyTransform;
            bodyTransform.translate(forwardDir * bodyForward + up * bodyBob);
            bodyTransform.rotate(up, bodyYaw);

            std::map<std::string, Matrix4x4> boneWorldTransforms;

            // Root and Head: move rigidly with body
            auto computeRigidBone = [&](const std::string& name) {
                Vector3 pos = getBonePos(name);
                Vector3 end = getBoneEnd(name);
                Vector3 newPos = bodyTransform.transformPoint(pos);
                Vector3 newEnd = bodyTransform.transformPoint(end);
                boneWorldTransforms[name] = insect::buildBoneWorldTransform(newPos, newEnd);
            };

            computeRigidBone("Root");
            computeRigidBone("Head");

            // Spine undulation: each segment gets an increasing phase lag
            // Phase propagates from head to tail, creating a traveling wave
            struct SpineSegment {
                const char* name;
                double phaseLag; // radians of delay behind head
                double yawScale; // amplitude multiplier (increases toward tail)
            };

            static const SpineSegment spineSegments[] = {
                { "BodyFront", 0.3, 0.4 },
                { "BodyMid", 0.6, 0.7 },
                { "BodyRear", 0.9, 1.0 },
            };

            // Accumulate yaw transforms along the spine
            // Each segment's start is the end of the previous segment
            Matrix4x4 cumulativeTransform = bodyTransform;
            Vector3 prevEnd = bodyTransform.transformPoint(getBoneEnd("Head"));

            for (const auto& seg : spineSegments) {
                double segYaw = spineYawAmp * seg.yawScale * std::sin(phase - seg.phaseLag);

                Vector3 restPos = getBonePos(seg.name);
                Vector3 restEnd = getBoneEnd(seg.name);
                Vector3 restDir = restEnd - restPos;

                // Apply cumulative body transform to get baseline position
                Vector3 segStart = prevEnd;

                // Apply local yaw rotation for this segment
                Matrix4x4 localYaw;
                localYaw.rotate(up, segYaw);
                Vector3 rotatedDir = localYaw.transformVector(cumulativeTransform.transformVector(restDir.normalized())) * restDir.length();

                Vector3 segEnd = segStart + rotatedDir;
                boneWorldTransforms[seg.name] = insect::buildBoneWorldTransform(segStart, segEnd);

                // Update cumulative transform for child segments
                Matrix4x4 segTransform;
                segTransform.translate(segStart - restPos);
                segTransform.rotate(up, segYaw);
                cumulativeTransform = segTransform;

                prevEnd = segEnd;
            }

            // Tail: continues the undulation wave with larger amplitude
            {
                double tailStartYaw = tailStartYawAmp * std::sin(phase - 1.2);
                Vector3 tailStartRestDir = getBoneEnd("TailStart") - getBonePos("TailStart");

                Matrix4x4 tailStartLocalYaw;
                tailStartLocalYaw.rotate(up, tailStartYaw);
                Vector3 tailStartDir = tailStartLocalYaw.transformVector(
                                           cumulativeTransform.transformVector(tailStartRestDir.normalized()))
                    * tailStartRestDir.length();

                Vector3 tailStartPos = prevEnd;
                Vector3 tailStartEnd = tailStartPos + tailStartDir;
                boneWorldTransforms["TailStart"] = insect::buildBoneWorldTransform(tailStartPos, tailStartEnd);

                // TailEnd: even more amplitude
                double tailEndYaw = tailEndYawAmp * std::sin(phase - 1.5);
                Vector3 tailEndRestDir = getBoneEnd("TailEnd") - getBonePos("TailEnd");

                Matrix4x4 tailEndLocalYaw;
                tailEndLocalYaw.rotate(up, tailEndYaw);

                Matrix4x4 tailCumTransform;
                tailCumTransform.translate(tailStartPos - getBonePos("TailStart"));
                tailCumTransform.rotate(up, tailStartYaw);

                Vector3 tailEndDir = tailEndLocalYaw.transformVector(
                                         tailCumTransform.transformVector(tailEndRestDir.normalized()))
                    * tailEndRestDir.length();

                Vector3 tailEndPos = tailStartEnd;
                Vector3 tailEndEnd = tailEndPos + tailEndDir;
                boneWorldTransforms["TailEnd"] = insect::buildBoneWorldTransform(tailEndPos, tailEndEnd);
            }

            // Pectoral fins: rhythmic flapping opposite to body yaw
            for (int side = 0; side < 2; ++side) {
                const char* finName = (side == 0) ? "LeftPectoralFin" : "RightPectoralFin";
                double sideSign = (side == 0) ? 1.0 : -1.0;

                Vector3 finPos = bodyTransform.transformPoint(getBonePos(finName));
                Vector3 finEnd = bodyTransform.transformPoint(getBoneEnd(finName));
                Vector3 finDir = finEnd - finPos;
                double finLen = finDir.length();
                if (finLen < 1e-8) {
                    boneWorldTransforms[finName] = insect::buildBoneWorldTransform(finPos, finPos);
                    continue;
                }

                // Pectoral fins sweep forward/backward with a slight phase offset
                double flapAngle = pectoralFlapAmp * std::sin(phase + sideSign * 0.3) * sideSign;
                Quaternion flapRot = Quaternion::fromAxisAndAngle(up, flapAngle);
                Matrix4x4 flapMat;
                flapMat.rotate(flapRot);

                Vector3 rotatedDir = flapMat.transformVector(finDir.normalized()) * finLen;
                boneWorldTransforms[finName] = insect::buildBoneWorldTransform(finPos, finPos + rotatedDir);
            }

            // Pelvic fins: gentle flapping, slightly delayed from pectorals
            for (int side = 0; side < 2; ++side) {
                const char* finName = (side == 0) ? "LeftPelvicFin" : "RightPelvicFin";
                if (boneIdx.count(finName) == 0)
                    continue;

                double sideSign = (side == 0) ? 1.0 : -1.0;

                // Pelvic fins are attached to BodyMid, use that bone's world transform
                Vector3 bodyMidStart, bodyMidEnd;
                auto bmIt = boneWorldTransforms.find("BodyMid");
                if (bmIt != boneWorldTransforms.end()) {
                    // Approximate: use the BodyMid segment's transformed positions
                    bodyMidStart = bmIt->second.transformPoint(Vector3());
                }

                Vector3 finRestPos = getBonePos(finName);
                Vector3 finRestEnd = getBoneEnd(finName);
                Vector3 finRestDir = finRestEnd - finRestPos;
                double finLen = finRestDir.length();

                // Transform fin base with the BodyMid bone's transform
                Vector3 finPos = bodyTransform.transformPoint(finRestPos);
                Vector3 finEnd = bodyTransform.transformPoint(finRestEnd);
                Vector3 finDir = finEnd - finPos;

                double flapAngle = pelvicFlapAmp * std::sin(phase - 0.5 + sideSign * 0.3) * sideSign;
                Quaternion flapRot = Quaternion::fromAxisAndAngle(up, flapAngle);
                Matrix4x4 flapMat;
                flapMat.rotate(flapRot);

                Vector3 rotatedDir = flapMat.transformVector(finDir.normalized()) * finLen;
                boneWorldTransforms[finName] = insect::buildBoneWorldTransform(finPos, finPos + rotatedDir);
            }

            // Dorsal fins: sway with the body undulation
            static const struct {
                const char* finName;
                const char* parentSegment;
                double phaseLag;
            } dorsalFins[] = {
                { "DorsalFinFront", "BodyFront", 0.3 },
                { "DorsalFinMid", "BodyMid", 0.6 },
                { "DorsalFinRear", "BodyRear", 0.9 },
            };

            for (const auto& df : dorsalFins) {
                if (boneIdx.count(df.finName) == 0)
                    continue;

                Vector3 finRestPos = getBonePos(df.finName);
                Vector3 finRestEnd = getBoneEnd(df.finName);
                Vector3 finRestDir = finRestEnd - finRestPos;
                double finLen = finRestDir.length();

                // Use parent segment's transform
                auto parentIt = boneWorldTransforms.find(df.parentSegment);
                Vector3 finPos, finDir;
                if (parentIt != boneWorldTransforms.end()) {
                    // Reconstruct parent segment positions
                    finPos = parentIt->second.transformPoint(Vector3());
                    // Offset fin base relative to parent start
                    Vector3 parentRestPos = getBonePos(df.parentSegment);
                    Vector3 offsetInParent = finRestPos - parentRestPos;
                    finPos = parentIt->second.transformPoint(offsetInParent);
                    finDir = parentIt->second.transformVector(finRestDir.normalized()) * finLen;
                } else {
                    finPos = bodyTransform.transformPoint(finRestPos);
                    finDir = bodyTransform.transformVector(finRestDir.normalized()) * finLen;
                }

                double swayAngle = dorsalSwayAmp * std::sin(phase - df.phaseLag);
                Quaternion swayRot = Quaternion::fromAxisAndAngle(forwardDir, swayAngle);
                Matrix4x4 swayMat;
                swayMat.rotate(swayRot);

                Vector3 rotatedDir = swayMat.transformVector(finDir.normalized()) * finLen;
                boneWorldTransforms[df.finName] = insect::buildBoneWorldTransform(finPos, finPos + rotatedDir);
            }

            // Ventral fins: sway opposite to dorsals
            static const struct {
                const char* finName;
                const char* parentSegment;
                double phaseLag;
            } ventralFins[] = {
                { "VentralFinFront", "BodyFront", 0.3 },
                { "VentralFinMid", "BodyMid", 0.6 },
                { "VentralFinRear", "BodyRear", 0.9 },
            };

            for (const auto& vf : ventralFins) {
                if (boneIdx.count(vf.finName) == 0)
                    continue;

                Vector3 finRestPos = getBonePos(vf.finName);
                Vector3 finRestEnd = getBoneEnd(vf.finName);
                Vector3 finRestDir = finRestEnd - finRestPos;
                double finLen = finRestDir.length();

                auto parentIt = boneWorldTransforms.find(vf.parentSegment);
                Vector3 finPos, finDir;
                if (parentIt != boneWorldTransforms.end()) {
                    Vector3 parentRestPos = getBonePos(vf.parentSegment);
                    Vector3 offsetInParent = finRestPos - parentRestPos;
                    finPos = parentIt->second.transformPoint(offsetInParent);
                    finDir = parentIt->second.transformVector(finRestDir.normalized()) * finLen;
                } else {
                    finPos = bodyTransform.transformPoint(finRestPos);
                    finDir = bodyTransform.transformVector(finRestDir.normalized()) * finLen;
                }

                double swayAngle = -ventralSwayAmp * std::sin(phase - vf.phaseLag);
                Quaternion swayRot = Quaternion::fromAxisAndAngle(forwardDir, swayAngle);
                Matrix4x4 swayMat;
                swayMat.rotate(swayRot);

                Vector3 rotatedDir = swayMat.transformVector(finDir.normalized()) * finLen;
                boneWorldTransforms[vf.finName] = insect::buildBoneWorldTransform(finPos, finPos + rotatedDir);
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

} // namespace fish

} // namespace dust3d
