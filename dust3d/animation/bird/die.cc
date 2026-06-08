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
#include <dust3d/animation/bird/die.h>
#include <dust3d/animation/common.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>
#include <functional>
#include <set>

namespace dust3d {

namespace bird {

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
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        int frameCount = static_cast<int>(parameters.getValue("frameCount", 30.0));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 1.2));

        auto boneIdx = animation::buildBoneIndexMap(rigStructure);
        static const char* requiredBones[] = {
            "Root", "Pelvis", "Spine", "Chest", "Neck", "Head", "Beak",
            "LeftWingShoulder", "LeftWingElbow", "LeftWingHand",
            "RightWingShoulder", "RightWingElbow", "RightWingHand",
            "LeftUpperLeg", "LeftLowerLeg", "LeftFoot",
            "RightUpperLeg", "RightLowerLeg", "RightFoot"
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
        {
            Vector3 spineCenter;
            auto sit = ragdollBoneIdx.find("Spine");
            if (sit != ragdollBoneIdx.end())
                spineCenter = (bones[sit->second].headPos + bones[sit->second].tailPos) * 0.5;

            Vector3 legDirSum(0.0, 0.0, 0.0);
            int legCount = 0;
            for (const auto& b : bones) {
                if (b.name.find("Foot") == std::string::npos)
                    continue;
                Vector3 dir = b.tailPos - spineCenter;
                double len = dir.length();
                if (len > 1e-6) {
                    legDirSum += dir / len;
                    ++legCount;
                }
            }
            if (legCount > 0) {
                Vector3 candidate = (legDirSum / legCount).normalized();
                if (candidate.length() > 1e-6)
                    gravityDir = candidate;
            }
        }

        Vector3 forwardDir(0.0, 0.0, 1.0);
        {
            auto headIt = ragdollBoneIdx.find("Head");
            auto pelvisIt = ragdollBoneIdx.find("Pelvis");
            if (headIt != ragdollBoneIdx.end() && pelvisIt != ragdollBoneIdx.end()) {
                Vector3 delta = bones[headIt->second].headPos - bones[pelvisIt->second].headPos;
                if (delta.length() > 1e-6)
                    forwardDir = delta.normalized();
            }
        }

        Vector3 sideDir = Vector3::crossProduct(forwardDir, gravityDir);
        if (sideDir.length() < 1e-6) {
            Vector3 arbitrary = (std::abs(gravityDir.x()) < 0.9) ? Vector3(1.0, 0.0, 0.0) : Vector3(0.0, 1.0, 0.0);
            sideDir = Vector3::crossProduct(arbitrary, gravityDir);
        }
        sideDir = sideDir.normalized();

        double groundLevel = 0.0;
        {
            double maxDot = -1e18;
            for (const auto& b : bones) {
                if (b.name.find("Foot") == std::string::npos)
                    continue;
                double d = Vector3::dotProduct(b.tailPos, gravityDir);
                if (d > maxDot)
                    maxDot = d;
            }
            if (maxDot > -1e17)
                groundLevel = maxDot;
        }

        float collapseSpeedFactor = static_cast<float>(parameters.getValue("collapseSpeedFactor", 1.0));
        float wingFlapFactor = static_cast<float>(parameters.getValue("wingFlapFactor", 1.0));
        float rollIntensityFactor = static_cast<float>(parameters.getValue("rollIntensityFactor", 1.0));
        float lengthStiffness = static_cast<float>(parameters.getValue("lengthStiffness", 0.92));
        float parentJointStiffness = static_cast<float>(parameters.getValue("parentStiffness", 0.78));
        float maxJointAngleDeg = static_cast<float>(parameters.getValue("maxJointAngleDeg", 120.0));
        double maxJointAngleRad = maxJointAngleDeg * (Math::Pi / 180.0);
        float damping = static_cast<float>(parameters.getValue("damping", 0.94));
        float groundBounce = static_cast<float>(parameters.getValue("groundBounce", 0.20));

        const double bodyDropVel = 0.5 * collapseSpeedFactor;
        const double bodyForwardVel = 1.5 * rollIntensityFactor;

        for (const char* bodyBone : { "Pelvis", "Spine", "Chest", "Neck", "Head" }) {
            auto it = ragdollBoneIdx.find(bodyBone);
            if (it != ragdollBoneIdx.end()) {
                auto& b = bones[it->second];
                b.headVel = forwardDir * bodyForwardVel + gravityDir * bodyDropVel;
                b.tailVel = forwardDir * bodyForwardVel + gravityDir * bodyDropVel;
            }
        }

        {
            auto it = ragdollBoneIdx.find("Head");
            if (it != ragdollBoneIdx.end()) {
                auto& head = bones[it->second];
                head.tailVel += gravityDir * (1.5 * collapseSpeedFactor) + forwardDir * (1.0 * rollIntensityFactor);
            }
        }

        {
            auto it = ragdollBoneIdx.find("Beak");
            if (it != ragdollBoneIdx.end()) {
                auto& b = bones[it->second];
                b.tailVel += gravityDir * (2.0 * collapseSpeedFactor) + forwardDir * (0.8 * rollIntensityFactor);
            }
        }

        for (auto& b : bones) {
            bool isWing = (b.name.find("Wing") != std::string::npos);
            if (!isWing)
                continue;
            double outerSide = (b.name.find("Left") != std::string::npos) ? 1.0 : -1.0;
            double wingOutVel = 0.8 * wingFlapFactor;
            double wingLiftVel = 0.3;

            if (b.name.find("Elbow") != std::string::npos || b.name.find("Hand") != std::string::npos) {
                wingOutVel *= 1.5;
                wingLiftVel *= 1.4;
            }

            b.headVel += sideDir * (outerSide * wingOutVel * 0.5) + gravityDir * (-wingLiftVel);
            b.tailVel += sideDir * (outerSide * wingOutVel) + gravityDir * (-wingLiftVel * 1.3);
        }

        for (auto& b : bones) {
            bool isLeg = (b.name.find("UpperLeg") != std::string::npos
                || b.name.find("LowerLeg") != std::string::npos
                || b.name.find("Foot") != std::string::npos);
            if (!isLeg)
                continue;
            double outerSide = (b.name.find("Left") != std::string::npos) ? 1.0 : -1.0;
            double legOutwardVel = 0.8 * collapseSpeedFactor;
            double legDropVel = 0.3 * collapseSpeedFactor;

            if (b.name.find("LowerLeg") != std::string::npos || b.name.find("Foot") != std::string::npos) {
                legOutwardVel *= 1.5;
                legDropVel *= 1.2;
            }

            b.headVel += sideDir * (outerSide * legOutwardVel * 0.5) + gravityDir * legDropVel;
            b.tailVel += sideDir * (outerSide * legOutwardVel) + gravityDir * (legDropVel * 1.3);
        }

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.clear();
        animationClip.frames.resize(frameCount);

        std::set<std::string> spineChainBones = { "Spine", "Chest", "Neck", "Head", "Beak" };
        float spineJointStiffness = static_cast<float>(parameters.getValue("spineStiffness", 0.98));

        double dt = durationSeconds / std::max(1, frameCount);
        Vector3 gravity = gravityDir * 9.8;
        const size_t constraintIterations = 6;

        animation::CapeGridSimulator capeSim;
        if (boneIdx.count("CenterCape1"))
            capeSim.initialize(rigStructure, boneIdx,
                animation::buildBoneWorldTransform(
                    animation::getBonePos(rigStructure, boneIdx, "Chest"),
                    animation::getBoneEnd(rigStructure, boneIdx, "Chest")),
                0.08, 0.85, 1.2, 0.15);

        for (int frame = 0; frame < frameCount; ++frame) {
            double tNormalized = static_cast<double>(frame) / static_cast<double>(std::max(1, frameCount - 1));
            std::vector<Vector3> savedHead(bones.size()), savedTail(bones.size());
            for (size_t i = 0; i < bones.size(); ++i) {
                savedHead[i] = bones[i].headPos;
                savedTail[i] = bones[i].tailPos;
            }

            for (auto& bone : bones) {
                bone.headVel += gravity * dt;
                bone.tailVel += gravity * dt;
                bone.headPos += bone.headVel * dt;
                bone.tailPos += bone.tailVel * dt;
            }

            for (size_t iter = 0; iter < constraintIterations; ++iter) {
                for (auto& bone : bones) {
                    Vector3 delta = bone.tailPos - bone.headPos;
                    double len = delta.length();
                    if (len > 1e-6) {
                        double correction = (len - bone.restLength) / len * 0.5 * lengthStiffness;
                        Vector3 diff = delta * correction;
                        bone.headPos += diff;
                        bone.tailPos -= diff;
                    }

                    if (!bone.parent.empty()) {
                        auto parentIt = ragdollBoneIdx.find(bone.parent);
                        if (parentIt != ragdollBoneIdx.end()) {
                            auto& parent = bones[parentIt->second];
                            Vector3 desiredHeadPos = parent.tailPos + bone.parentOffset;
                            Vector3 jointError = desiredHeadPos - bone.headPos;
                            float stiffness = (spineChainBones.count(bone.name) > 0)
                                ? spineJointStiffness
                                : parentJointStiffness;
                            Vector3 correction = jointError * stiffness;
                            bone.headPos += correction;
                            parent.tailPos -= correction * (1.0f - stiffness);
                        }
                    }

                    if (!bone.restDir.isZero()) {
                        Vector3 currentDir = (bone.tailPos - bone.headPos).normalized();
                        if (!currentDir.isZero()) {
                            double dot = std::max(-1.0, std::min(1.0, Vector3::dotProduct(currentDir, bone.restDir)));
                            double angle = std::acos(dot);
                            if (angle > maxJointAngleRad) {
                                Vector3 axis = Vector3::crossProduct(currentDir, bone.restDir);
                                double axisLen = axis.length();
                                Vector3 restrictedDir;
                                if (axisLen > 1e-6) {
                                    axis /= axisLen;
                                    double rotAngle = angle - maxJointAngleRad;
                                    double c = std::cos(rotAngle), s = std::sin(rotAngle);
                                    restrictedDir = currentDir * c
                                        + Vector3::crossProduct(axis, currentDir) * s
                                        + axis * Vector3::dotProduct(axis, currentDir) * (1.0 - c);
                                    restrictedDir = restrictedDir.normalized();
                                } else {
                                    restrictedDir = bone.restDir;
                                }
                                bone.tailPos = bone.headPos + restrictedDir * bone.restLength;
                            }
                        }
                    }
                }
            }

            for (auto& bone : bones) {
                double headDot = Vector3::dotProduct(bone.headPos, gravityDir);
                double headLimit = groundLevel - bone.radius;
                if (headDot > headLimit)
                    bone.headPos -= gravityDir * (headDot - headLimit);

                double tailDot = Vector3::dotProduct(bone.tailPos, gravityDir);
                double tailLimit = groundLevel - bone.radius;
                if (tailDot > tailLimit)
                    bone.tailPos -= gravityDir * (tailDot - tailLimit);
            }

            for (size_t i = 0; i < bones.size(); ++i) {
                bones[i].headVel = (bones[i].headPos - savedHead[i]) / dt;
                bones[i].tailVel = (bones[i].tailPos - savedTail[i]) / dt;
            }

            for (auto& bone : bones) {
                double headDot = Vector3::dotProduct(bone.headPos, gravityDir);
                if (headDot >= groundLevel - bone.radius - 1e-4) {
                    double vDot = Vector3::dotProduct(bone.headVel, gravityDir);
                    if (vDot > 0.0)
                        bone.headVel -= gravityDir * (vDot * (1.0 + groundBounce));
                }
                double tailDot = Vector3::dotProduct(bone.tailPos, gravityDir);
                if (tailDot >= groundLevel - bone.radius - 1e-4) {
                    double vDot = Vector3::dotProduct(bone.tailVel, gravityDir);
                    if (vDot > 0.0)
                        bone.tailVel -= gravityDir * (vDot * (1.0 + groundBounce));
                }
                bone.headVel *= damping;
                bone.tailVel *= damping;
            }

            std::map<std::string, Matrix4x4> boneWorldTransforms;
            for (const auto& bone : bones) {
                boneWorldTransforms[bone.name] = animation::buildBoneWorldTransform(bone.headPos, bone.tailPos);
            }

            if (capeSim.active)
                capeSim.step(boneWorldTransforms["Chest"], dt, boneWorldTransforms);

            auto& frameData = animationClip.frames[frame];
            frameData.time = static_cast<float>(tNormalized) * durationSeconds;
            frameData.boneWorldTransforms = boneWorldTransforms;
            for (const auto& pair : boneWorldTransforms) {
                auto invIt = inverseBindMatrices.find(pair.first);
                if (invIt != inverseBindMatrices.end()) {
                    Matrix4x4 skinMat = pair.second;
                    skinMat *= invIt->second;
                    frameData.boneSkinMatrices[pair.first] = skinMat;
                }
            }
        }

        return true;
    }

} // namespace bird

} // namespace dust3d
