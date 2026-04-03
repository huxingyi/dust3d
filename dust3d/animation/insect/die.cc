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
#include <dust3d/animation/insect/common.h>
#include <dust3d/animation/insect/die.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace insect {

    namespace {

        struct BoneRagdollInfo {
            std::string name;
            std::string parent;
            Vector3 headPos;
            Vector3 tailPos;
            Vector3 headVel;
            Vector3 tailVel;
            Vector3 restDir;
            float restLength;
            float radius;
        };

    } // anonymous namespace

    bool die(const RigStructure& rigStructure,
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
            "Head", "Thorax", "Abdomen",
            "LeftWing", "RightWing",
            "FrontLeftCoxa", "FrontLeftFemur", "FrontLeftTibia",
            "FrontRightCoxa", "FrontRightFemur", "FrontRightTibia",
            "MiddleLeftCoxa", "MiddleLeftFemur", "MiddleLeftTibia",
            "MiddleRightCoxa", "MiddleRightFemur", "MiddleRightTibia",
            "BackLeftCoxa", "BackLeftFemur", "BackLeftTibia",
            "BackRightCoxa", "BackRightFemur", "BackRightTibia"
        };

        if (!insect::validateRequiredBones(boneIdx, requiredBones, sizeof(requiredBones) / sizeof(requiredBones[0])))
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
            info.radius = b.capsuleRadius;
            bones.push_back(info);
        }

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.clear();
        animationClip.frames.resize(frameCount);

        double dt = durationSeconds / std::max(1, frameCount);
        Vector3 gravity(0.0, -9.80, 0.0);

        float lengthStiffness = static_cast<float>(parameters.getValue("insectDieLengthStiffness", 0.9));
        float parentJointStiffness = static_cast<float>(parameters.getValue("insectDieParentStiffness", 0.8));
        float maxJointAngleDeg = static_cast<float>(parameters.getValue("insectDieMaxJointAngleDeg", 120.0));
        float maxJointAngleRad = maxJointAngleDeg * (3.14159265f / 180.0f);
        float damping = static_cast<float>(parameters.getValue("insectDieDamping", 0.95));
        float groundY = static_cast<float>(parameters.getValue("insectDieGroundY", 0.0));
        float groundBounce = static_cast<float>(parameters.getValue("insectDieGroundBounce", 0.22));

        for (int frame = 0; frame < frameCount; ++frame) {
            double tNormalized = static_cast<double>(frame) / static_cast<double>(frameCount);

            // Per-bone dynamic update
            for (auto& bone : bones) {
                // Apply gravity and integrate endpoint velocities and positions
                bone.headVel += gravity * dt;
                bone.tailVel += gravity * dt;
                bone.headPos += bone.headVel * dt;
                bone.tailPos += bone.tailVel * dt;
            }

            // Constraint solve (length maintenance + hierarchical joint and angular constraints)
            for (size_t iter = 0; iter < 2; ++iter) {
                for (auto& bone : bones) {
                    // length constraint
                    Vector3 delta = bone.tailPos - bone.headPos;
                    float len = delta.length();
                    if (len > 1e-6f) {
                        float correction = (len - bone.restLength) / len * 0.5f * lengthStiffness;
                        Vector3 diff = delta * correction;
                        bone.headPos += diff;
                        bone.tailPos -= diff;
                        bone.headVel += diff / dt;
                        bone.tailVel -= diff / dt;
                    }

                    // parent-child positional constraint (IK-like joint stiffness)
                    if (!bone.parent.empty()) {
                        auto parentIt = std::find_if(bones.begin(), bones.end(),
                            [&](const BoneRagdollInfo& p) { return p.name == bone.parent; });
                        if (parentIt != bones.end()) {
                            Vector3 jointError = parentIt->tailPos - bone.headPos;
                            Vector3 correction = jointError * parentJointStiffness;
                            bone.headPos += correction;
                            parentIt->tailPos -= correction;
                            bone.headVel += correction / dt;
                            parentIt->tailVel -= correction / dt;
                        }
                    }

                    // angular constraint relative to rest direction (help preserve insect limb posture)
                    if (!bone.restDir.isZero()) {
                        Vector3 currentDir = (bone.tailPos - bone.headPos).normalized();
                        if (!currentDir.isZero()) {
                            float dot = std::max(-1.0f, std::min(1.0f, static_cast<float>(Vector3::dotProduct(currentDir, bone.restDir))));
                            float angle = std::acos(dot);
                            if (angle > maxJointAngleRad && angle > 1e-6f) {
                                float t = maxJointAngleRad / angle;
                                Vector3 restrictedDir = (bone.restDir * (1.0f - t) + currentDir * t).normalized();
                                Vector3 oldTail = bone.tailPos;
                                bone.tailPos = bone.headPos + restrictedDir * bone.restLength;
                                bone.tailVel = (bone.tailPos - oldTail) / dt * 0.5f + bone.tailVel * 0.5f;
                            }
                        }
                    }
                }
            }

            // Collision with ground for collision capsules
            for (auto& bone : bones) {
                for (Vector3* p : { &bone.headPos, &bone.tailPos }) {
                    if (p->y() < groundY + bone.radius) {
                        p->setY(groundY + bone.radius);
                        // bounce and friction
                        if (&bone.headPos == p)
                            bone.headVel.setY(-bone.headVel.y() * groundBounce);
                        else
                            bone.tailVel.setY(-bone.tailVel.y() * groundBounce);
                    }
                }

                bone.headVel *= damping;
                bone.tailVel *= damping;
            }

            std::map<std::string, Matrix4x4> boneWorldTransforms;
            for (const auto& bone : bones) {
                boneWorldTransforms[bone.name] = insect::buildBoneWorldTransform(bone.headPos, bone.tailPos);
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

} // namespace insect

} // namespace dust3d
