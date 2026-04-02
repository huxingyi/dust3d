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
            info.radius = b.capsuleRadius;
            bones.push_back(info);
        }

        animationClip.name = "flyDie";
        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.clear();
        animationClip.frames.resize(frameCount);

        double dt = durationSeconds / std::max(1, frameCount);
        Vector3 gravity(0.0, -9.80, 0.0);
        float damping = 0.95f;
        float groundY = 0.0f;

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

            // Constraint solve (length maintenance + parent heads should follow parent tails)
            for (size_t iter = 0; iter < 2; ++iter) {
                for (auto& bone : bones) {
                    Vector3 delta = bone.tailPos - bone.headPos;
                    float len = delta.length();
                    if (len > 1e-6f) {
                        float correction = (len - bone.restLength) / len * 0.5f;
                        Vector3 diff = delta * correction;
                        bone.headPos += diff;
                        bone.tailPos -= diff;
                        bone.headVel += diff / dt;
                        bone.tailVel -= diff / dt;
                    }

                    if (!bone.parent.empty()) {
                        auto parentIt = std::find_if(bones.begin(), bones.end(),
                            [&](const BoneRagdollInfo& p) { return p.name == bone.parent; });
                        if (parentIt != bones.end()) {
                            Vector3 jointError = parentIt->tailPos - bone.headPos;
                            Vector3 half = jointError * 0.5f;
                            bone.headPos += half;
                            parentIt->tailPos -= half;
                            bone.headVel += half / dt;
                            parentIt->tailVel -= half / dt;
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
                            bone.headVel.setY(-bone.headVel.y() * 0.22f);
                        else
                            bone.tailVel.setY(-bone.tailVel.y() * 0.22f);
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
