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

// Procedural ragdoll death animation for quadruped rig.
//
// Uses position-based dynamics (PBD) to simulate a ragdoll collapse:
//   - Bones fall under gravity with capsule-based ground collision
//   - Length, joint, and angular constraints keep the skeleton coherent
//   - Initial impulse causes the body to collapse sideways
//
// Tunable parameters allow animating different quadruped animals dying:
//   - collapseSpeed:     how quickly the animal drops (initial downward velocity)
//   - legSpreadFactor:   how much legs splay outward during collapse
//   - rollIntensity:     sideways roll impulse on body/spine bones
//   - lengthStiffness:   bone length preservation (PBD constraint)
//   - parentStiffness:   parent-child joint adherence
//   - maxJointAngleDeg:  angular limit per joint
//   - damping:           velocity damping per frame
//   - groundBounce:      restitution coefficient on ground contact

#include <algorithm>
#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/common.h>
#include <dust3d/animation/quadruped/die.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>
#include <functional>

namespace dust3d {

namespace quadruped {

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
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 1.0));
        std::map<std::string, size_t> boneIdx;
        for (size_t i = 0; i < rigStructure.bones.size(); ++i)
            boneIdx[rigStructure.bones[i].name] = i;

        static const char* requiredBones[] = {
            "Pelvis", "Spine", "Chest", "Neck", "Head",
            "FrontLeftUpperLeg", "FrontLeftLowerLeg", "FrontLeftFoot",
            "FrontRightUpperLeg", "FrontRightLowerLeg", "FrontRightFoot",
            "BackLeftUpperLeg", "BackLeftLowerLeg", "BackLeftFoot",
            "BackRightUpperLeg", "BackRightLowerLeg", "BackRightFoot"
        };

        if (!animation::validateRequiredBones(boneIdx, requiredBones, sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;

        // Build ragdoll bone state from rig structure
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

        // Build name-to-index map over ragdoll bone array
        std::map<std::string, size_t> ragdollBoneIdx;
        for (size_t i = 0; i < bones.size(); ++i)
            ragdollBoneIdx[bones[i].name] = i;

        // Topological sort so parents are always processed before children
        {
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
        }

        // Capture parent offsets
        for (auto& bone : bones) {
            if (!bone.parent.empty()) {
                auto parentIt = ragdollBoneIdx.find(bone.parent);
                if (parentIt != ragdollBoneIdx.end()) {
                    const auto& parent = bones[parentIt->second];
                    bone.parentOffset = bone.headPos - parent.tailPos;
                }
            }
        }

        // Compute gravity direction from rest pose: average direction from spine
        // center toward all foot tips defines "down".
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

        // Forward direction (head → tail along spine)
        Vector3 forwardDir(0.0, 0.0, 1.0);
        {
            auto headIt = ragdollBoneIdx.find("Head");
            auto pelvisIt = ragdollBoneIdx.find("Pelvis");
            if (headIt != ragdollBoneIdx.end() && pelvisIt != ragdollBoneIdx.end()) {
                Vector3 headPos = bones[headIt->second].headPos;
                Vector3 pelvisPos = bones[pelvisIt->second].headPos;
                Vector3 dir = headPos - pelvisPos;
                if (dir.length() > 1e-6)
                    forwardDir = dir.normalized();
            }
        }

        // Lateral axis
        Vector3 sideDir = Vector3::crossProduct(forwardDir, gravityDir);
        if (sideDir.length() < 1e-6) {
            Vector3 arbitrary = (std::abs(gravityDir.x()) < 0.9) ? Vector3(1.0, 0.0, 0.0) : Vector3(0.0, 1.0, 0.0);
            sideDir = Vector3::crossProduct(arbitrary, gravityDir);
        }
        sideDir = sideDir.normalized();
        Vector3 upDir = -gravityDir;

        // Ground level: furthest foot tip along gravity direction
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

        // User-tunable parameters for controlling different animal death styles
        float collapseSpeed = static_cast<float>(parameters.getValue("collapseSpeed", 1.0));
        float legSpreadFactor = static_cast<float>(parameters.getValue("legSpreadFactor", 1.0));
        float rollIntensity = static_cast<float>(parameters.getValue("rollIntensity", 1.0));
        float lengthStiffness = static_cast<float>(parameters.getValue("lengthStiffness", 0.9));
        float parentJointStiffness = static_cast<float>(parameters.getValue("parentStiffness", 0.8));
        float maxJointAngleDeg = static_cast<float>(parameters.getValue("maxJointAngleDeg", 120.0));
        double maxJointAngleRad = maxJointAngleDeg * (Math::Pi / 180.0);
        float damping = static_cast<float>(parameters.getValue("damping", 0.95));
        float groundBounce = static_cast<float>(parameters.getValue("groundBounce", 0.22));

        // Apply initial death impulse: the quadruped collapses sideways,
        // legs buckle outward, head and tail droop.
        {
            const double bodyRollVel = 1.5 * rollIntensity;
            const double bodyDropVel = 0.5 * collapseSpeed;

            // Spine chain collapses sideways
            for (const char* bodyBone : { "Pelvis", "Spine", "Chest", "Neck", "Head" }) {
                auto it = ragdollBoneIdx.find(bodyBone);
                if (it != ragdollBoneIdx.end()) {
                    auto& b = bones[it->second];
                    b.headVel = sideDir * bodyRollVel + gravityDir * bodyDropVel;
                    b.tailVel = sideDir * bodyRollVel + gravityDir * bodyDropVel;
                }
            }

            // Head drops forward and down
            {
                auto it = ragdollBoneIdx.find("Head");
                if (it != ragdollBoneIdx.end()) {
                    auto& b = bones[it->second];
                    b.tailVel += gravityDir * (1.0 * collapseSpeed) + forwardDir * 0.3;
                }
            }

            // Jaw drops open
            {
                auto it = ragdollBoneIdx.find("Jaw");
                if (it != ragdollBoneIdx.end()) {
                    auto& b = bones[it->second];
                    b.tailVel += gravityDir * (1.5 * collapseSpeed);
                }
            }

            // Tail curls/drops
            for (const char* tailBone : { "TailBase", "TailMid", "TailTip" }) {
                auto it = ragdollBoneIdx.find(tailBone);
                if (it != ragdollBoneIdx.end()) {
                    auto& b = bones[it->second];
                    b.headVel += gravityDir * (0.3 * collapseSpeed);
                    b.tailVel += gravityDir * (0.6 * collapseSpeed) + sideDir * (0.3 * rollIntensity);
                }
            }

            // Legs splay outward and buckle
            for (auto& b : bones) {
                bool isLeg = (b.name.find("UpperLeg") != std::string::npos
                    || b.name.find("LowerLeg") != std::string::npos
                    || b.name.find("Foot") != std::string::npos);
                if (!isLeg)
                    continue;
                double outerSide = (b.name.find("Left") != std::string::npos) ? 1.0 : -1.0;
                double legOutwardVel = 0.8 * legSpreadFactor;
                double legDropVel = 0.3 * collapseSpeed;

                // Lower legs and feet splay more than upper legs
                if (b.name.find("LowerLeg") != std::string::npos || b.name.find("Foot") != std::string::npos) {
                    legOutwardVel *= 1.5;
                    legDropVel *= 1.2;
                }

                b.headVel += sideDir * (outerSide * legOutwardVel * 0.5) + gravityDir * legDropVel;
                b.tailVel += sideDir * (outerSide * legOutwardVel) + gravityDir * (legDropVel * 1.3);
            }
        }

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.clear();
        animationClip.frames.resize(frameCount);

        double dt = durationSeconds / std::max(1, frameCount);
        Vector3 gravity = gravityDir * 9.80;

        const size_t constraintIterations = 4;

        for (int frame = 0; frame < frameCount; ++frame) {
            // One-shot clip: t spans [0, 1]
            double tNormalized = static_cast<double>(frame) / static_cast<double>(std::max(1, frameCount - 1));

            // Save positions
            std::vector<Vector3> savedHead(bones.size()), savedTail(bones.size());
            for (size_t i = 0; i < bones.size(); ++i) {
                savedHead[i] = bones[i].headPos;
                savedTail[i] = bones[i].tailPos;
            }

            // Integrate gravity
            for (auto& bone : bones) {
                bone.headVel += gravity * dt;
                bone.tailVel += gravity * dt;
                bone.headPos += bone.headVel * dt;
                bone.tailPos += bone.tailVel * dt;
            }

            // PBD constraint solve
            for (size_t iter = 0; iter < constraintIterations; ++iter) {
                for (auto& bone : bones) {
                    // Length constraint
                    Vector3 delta = bone.tailPos - bone.headPos;
                    double len = delta.length();
                    if (len > 1e-6) {
                        double correction = (len - bone.restLength) / len * 0.5 * lengthStiffness;
                        Vector3 diff = delta * correction;
                        bone.headPos += diff;
                        bone.tailPos -= diff;
                    }

                    // Joint constraint
                    if (!bone.parent.empty()) {
                        auto parentIt = ragdollBoneIdx.find(bone.parent);
                        if (parentIt != ragdollBoneIdx.end()) {
                            auto& parent = bones[parentIt->second];
                            Vector3 desiredHeadPos = parent.tailPos + bone.parentOffset;
                            Vector3 jointError = desiredHeadPos - bone.headPos;
                            Vector3 correction = jointError * parentJointStiffness;
                            bone.headPos += correction;
                            parent.tailPos -= correction;
                        }
                    }

                    // Angular constraint
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

            // Ground collision using capsule radius
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

            // Derive velocities from position changes (PBD)
            for (size_t i = 0; i < bones.size(); ++i) {
                bones[i].headVel = (bones[i].headPos - savedHead[i]) / dt;
                bones[i].tailVel = (bones[i].tailPos - savedTail[i]) / dt;
            }

            // Velocity bounce and damping
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

            // Build bone world transforms and skin matrices
            std::map<std::string, Matrix4x4> boneWorldTransforms;
            for (const auto& bone : bones) {
                boneWorldTransforms[bone.name] = animation::buildBoneWorldTransform(bone.headPos, bone.tailPos);
            }

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

} // namespace quadruped

} // namespace dust3d
