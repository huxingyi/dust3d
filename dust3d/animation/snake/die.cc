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
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/common.h>
#include <dust3d/animation/snake/die.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>
#include <functional>

namespace dust3d {

namespace snake {

    namespace {

        struct BoneRagdollInfo {
            std::string name;
            std::string parent;
            Vector3 headPos;
            Vector3 tailPos;
            Vector3 headVel;
            Vector3 tailVel;
            Vector3 restDir;
            Vector3 parentOffset;
            float restLength;
            float radius;
        };

    } // anonymous namespace

    bool die(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& /* inverseBindMatrices */,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        int frameCount = static_cast<int>(parameters.getValue("frameCount", 30.0));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 1.0));

        auto boneIdx = animation::buildBoneIndexMap(rigStructure);
        static const char* requiredBones[] = {
            "Root", "Head", "Jaw",
            "Spine1", "Spine2", "Spine3", "Spine4", "Spine5", "Spine6",
            "Tail1", "Tail2", "Tail3", "Tail4", "TailTip"
        };
        if (!animation::validateRequiredBones(boneIdx, requiredBones, sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;

        std::vector<BoneRagdollInfo> bones;
        bones.reserve(rigStructure.bones.size());
        for (const auto& b : rigStructure.bones) {
            BoneRagdollInfo info;
            info.name = b.name;
            info.parent = b.parent;
            info.headPos = Vector3(b.posX, b.posY, b.posZ);
            info.tailPos = Vector3(b.endX, b.endY, b.endZ);
            info.headVel = Vector3(0.0, 0.0, 0.0);
            info.tailVel = Vector3(0.0, 0.0, 0.0);
            info.restLength = std::max(static_cast<float>((info.tailPos - info.headPos).length()), 1e-6f);
            info.restDir = (info.tailPos - info.headPos).normalized();
            info.parentOffset = Vector3(0.0, 0.0, 0.0);
            info.radius = b.capsuleRadius;
            bones.push_back(info);
        }

        std::map<std::string, size_t> ragdollBoneIdx;
        for (size_t i = 0; i < bones.size(); ++i)
            ragdollBoneIdx[bones[i].name] = i;

        std::vector<BoneRagdollInfo> sorted;
        sorted.reserve(bones.size());
        std::vector<bool> visited(bones.size(), false);
        std::function<void(size_t)> visit = [&](size_t i) {
            if (visited[i])
                return;
            if (!bones[i].parent.empty()) {
                auto pit = ragdollBoneIdx.find(bones[i].parent);
                if (pit != ragdollBoneIdx.end())
                    visit(pit->second);
            }
            visited[i] = true;
            sorted.push_back(bones[i]);
        };
        for (size_t i = 0; i < bones.size(); ++i)
            visit(i);
        bones = std::move(sorted);
        ragdollBoneIdx.clear();
        for (size_t i = 0; i < bones.size(); ++i)
            ragdollBoneIdx[bones[i].name] = i;

        for (auto& bone : bones) {
            if (!bone.parent.empty()) {
                auto parentIt = ragdollBoneIdx.find(bone.parent);
                if (parentIt != ragdollBoneIdx.end()) {
                    bone.parentOffset = bone.headPos - bones[parentIt->second].tailPos;
                }
            }
        }

        Vector3 gravityDir(0.0, -1.0, 0.0);
        Vector3 forwardDir(0.0, 0.0, 1.0);
        {
            auto headIt = ragdollBoneIdx.find("Head");
            auto rootIt = ragdollBoneIdx.find("Root");
            if (headIt != ragdollBoneIdx.end() && rootIt != ragdollBoneIdx.end()) {
                Vector3 delta = bones[headIt->second].headPos - bones[rootIt->second].headPos;
                if (delta.length() > 1e-6)
                    forwardDir = delta.normalized();
            }
        }

        Vector3 sideDir = Vector3::crossProduct(forwardDir, gravityDir);
        if (sideDir.length() < 1e-6) {
            Vector3 arbitrary = (std::abs(gravityDir.x()) < 0.9) ? Vector3(1.0, 0.0, 0.0) : Vector3(0.0, 0.0, 1.0);
            sideDir = Vector3::crossProduct(arbitrary, gravityDir);
        }
        sideDir.normalize();

        Vector3 rootPos = bones[ragdollBoneIdx["Root"]].headPos;
        std::vector<Vector3> restHead(bones.size()), restTail(bones.size());
        for (size_t i = 0; i < bones.size(); ++i) {
            restHead[i] = bones[i].headPos;
            restTail[i] = bones[i].tailPos;
        }

        auto rotateAroundAxis = [&](const Vector3& vec, const Vector3& axis, double angle) {
            Vector3 axisNormalized = axis.normalized();
            double c = std::cos(angle);
            double s = std::sin(angle);
            return vec * c
                + Vector3::crossProduct(axisNormalized, vec) * s
                + axisNormalized * (Vector3::dotProduct(axisNormalized, vec) * (1.0 - c));
        };

        double flipSpeed = parameters.getValue("flipSpeed", 1.0);
        double flipAngleDeg = parameters.getValue("flipAngle", 180.0);
        double flipAngleMax = flipAngleDeg * (Math::Pi / 180.0);
        double jawOpenDeg = parameters.getValue("jawOpen", 63.0);
        double jawOpenMax = jawOpenDeg * (Math::Pi / 180.0);
        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.clear();
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            double tNormalized = static_cast<double>(frame) / static_cast<double>(std::max(1, frameCount - 1));
            double ease = 1.0 - std::cos(tNormalized * Math::Pi * 0.5);
            double rollT = std::min(tNormalized * flipSpeed, 1.0);
            double smoothRollT = 1.0 - (1.0 - rollT) * (1.0 - rollT) * (1.0 - rollT);
            double bodyRotation = flipAngleMax * smoothRollT;
            double jawOpenAngle = jawOpenMax * ease;

            std::map<std::string, Matrix4x4> boneWorldTransforms;
            for (size_t i = 0; i < bones.size(); ++i) {
                bones[i].headPos = rootPos + rotateAroundAxis(restHead[i] - rootPos, forwardDir, bodyRotation);
                bones[i].tailPos = rootPos + rotateAroundAxis(restTail[i] - rootPos, forwardDir, bodyRotation);
            }

            auto jawIt = ragdollBoneIdx.find("Jaw");
            if (jawIt != ragdollBoneIdx.end()) {
                size_t jawIndex = jawIt->second;
                Vector3 jawRestDir = restTail[jawIndex] - restHead[jawIndex];
                if (!jawRestDir.isZero()) {
                    bones[jawIndex].tailPos = bones[jawIndex].headPos + rotateAroundAxis(jawRestDir, sideDir, jawOpenAngle);
                }
            }

            Matrix4x4 bodyRollMat;
            bodyRollMat.translate(rootPos);
            bodyRollMat.rotate(forwardDir, bodyRotation);
            bodyRollMat.translate(rootPos * -1.0);

            for (const auto& bone : bones) {
                boneWorldTransforms[bone.name] = animation::buildBoneWorldTransform(bone.headPos, bone.tailPos);
            }

            auto& frameData = animationClip.frames[frame];
            frameData.time = static_cast<float>(tNormalized) * durationSeconds;
            frameData.boneWorldTransforms = boneWorldTransforms;
            for (const auto& bone : bones) {
                if (bone.name == "Jaw") {
                    Matrix4x4 jawSkinMat = bodyRollMat;
                    jawSkinMat.translate(bone.headPos);
                    jawSkinMat.rotate(sideDir, jawOpenAngle);
                    jawSkinMat.translate(bone.headPos * -1.0);
                    frameData.boneSkinMatrices[bone.name] = jawSkinMat;
                } else {
                    frameData.boneSkinMatrices[bone.name] = bodyRollMat;
                }
            }
        }

        return true;
    }

} // namespace snake

} // namespace dust3d
