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
#include <functional>

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
        const AnimationParams& parameters)
    {
        int frameCount = static_cast<int>(parameters.getValue("frameCount", 30.0));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 1.0));
        std::map<std::string, size_t> boneIdx;
        for (size_t i = 0; i < rigStructure.bones.size(); ++i)
            boneIdx[rigStructure.bones[i].name] = i;

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

        // Build name-to-index map over the ragdoll bone array for O(1) lookup
        std::map<std::string, size_t> ragdollBoneIdx;
        for (size_t i = 0; i < bones.size(); ++i)
            ragdollBoneIdx[bones[i].name] = i;

        // Topological sort so parents are always processed before their children,
        // which lets joint constraints propagate correctly in a single pass.
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

        // Compute gravity direction from the rest pose: the average unit vector from
        // the thorax toward every leg-tip (Tibia tail) defines "down" for this model.
        Vector3 gravityDir(0.0, -1.0, 0.0);
        {
            Vector3 thoraxPos;
            auto tit = ragdollBoneIdx.find("Thorax");
            if (tit != ragdollBoneIdx.end())
                thoraxPos = (bones[tit->second].headPos + bones[tit->second].tailPos) * 0.5;

            Vector3 legDirSum(0.0, 0.0, 0.0);
            int legCount = 0;
            for (const auto& b : bones) {
                if (b.name.find("Tibia") == std::string::npos)
                    continue;
                Vector3 dir = b.tailPos - thoraxPos;
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

        // Compute the insect's forward direction (head→abdomen axis).
        Vector3 forwardDir(0.0, 0.0, -1.0);
        {
            auto headIt = ragdollBoneIdx.find("Head");
            auto abdIt = ragdollBoneIdx.find("Abdomen");
            if (headIt != ragdollBoneIdx.end() && abdIt != ragdollBoneIdx.end()) {
                Vector3 headPos = bones[headIt->second].headPos;
                Vector3 abdPos = bones[abdIt->second].tailPos;
                Vector3 dir = headPos - abdPos;
                if (dir.length() > 1e-6)
                    forwardDir = dir.normalized();
            }
        }

        // Lateral axis (left-to-right relative to forward and gravity).
        // For Left bones the outward direction is +sideDir; for Right it is -sideDir.
        Vector3 sideDir = Vector3::crossProduct(forwardDir, gravityDir);
        if (sideDir.length() < 1e-6) {
            // Forward is collinear with gravity – pick any perpendicular.
            Vector3 arbitrary = (std::abs(gravityDir.x()) < 0.9) ? Vector3(1.0, 0.0, 0.0) : Vector3(0.0, 1.0, 0.0);
            sideDir = Vector3::crossProduct(arbitrary, gravityDir);
        }
        sideDir = sideDir.normalized();
        Vector3 upDir = -gravityDir;

        // Ground level: the furthest point along gravityDir across all leg tips
        // in the rest pose defines the floor plane.
        double groundLevel = 0.0;
        {
            double maxDot = -1e18;
            for (const auto& b : bones) {
                if (b.name.find("Tibia") == std::string::npos)
                    continue;
                double d = Vector3::dotProduct(b.tailPos, gravityDir);
                if (d > maxDot)
                    maxDot = d;
            }
            if (maxDot > -1e17)
                groundLevel = maxDot;
        }

        // Apply initial death impulse so the insect doesn't just fall straight down.
        // Body rolls sideways, wings flare up, and legs splay outward.
        // All directions are expressed in the model's own rest-pose frame.
        {
            const double bodyRollVel = 1.5;
            const double bodyDropVel = 0.3;
            for (const char* bodyBone : { "Thorax", "Head", "Abdomen" }) {
                auto it = ragdollBoneIdx.find(bodyBone);
                if (it != ragdollBoneIdx.end()) {
                    auto& b = bones[it->second];
                    // Roll sideways, slight initial push into gravity.
                    b.headVel = sideDir * bodyRollVel + gravityDir * bodyDropVel;
                    b.tailVel = sideDir * bodyRollVel + gravityDir * bodyDropVel;
                }
            }
            for (const char* wing : { "LeftWing", "RightWing" }) {
                auto it = ragdollBoneIdx.find(wing);
                if (it != ragdollBoneIdx.end()) {
                    auto& b = bones[it->second];
                    // Left wing spreads in +sideDir, right wing in -sideDir.
                    double outerSide = (b.name.find("Left") != std::string::npos) ? 1.0 : -1.0;
                    b.headVel = sideDir * (outerSide * 0.3) + upDir * 1.5;
                    b.tailVel = sideDir * (outerSide * 0.8) + upDir * 2.5;
                }
            }
            for (auto& b : bones) {
                bool isLeg = (b.name.find("Coxa") != std::string::npos || b.name.find("Femur") != std::string::npos || b.name.find("Tibia") != std::string::npos);
                if (!isLeg)
                    continue;
                double outerSide = (b.name.find("Left") != std::string::npos) ? 1.0 : -1.0;
                b.headVel = sideDir * (outerSide * 0.5) + upDir * 0.2;
                b.tailVel = sideDir * (outerSide * 1.2) + upDir * 0.5;
            }
        }

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.clear();
        animationClip.frames.resize(frameCount);

        double dt = durationSeconds / std::max(1, frameCount);
        Vector3 gravity = gravityDir * 9.80;

        float lengthStiffness = static_cast<float>(parameters.getValue("lengthStiffness", 0.9));
        float parentJointStiffness = static_cast<float>(parameters.getValue("parentStiffness", 0.8));
        float maxJointAngleDeg = static_cast<float>(parameters.getValue("maxJointAngleDeg", 120.0));
        double maxJointAngleRad = maxJointAngleDeg * (Math::Pi / 180.0);
        float damping = static_cast<float>(parameters.getValue("damping", 0.95));
        float groundBounce = static_cast<float>(parameters.getValue("groundBounce", 0.22));

        // Precomputed per-bone iteration count (more iterations = stiffer constraints).
        const size_t constraintIterations = 4;

        for (int frame = 0; frame < frameCount; ++frame) {
            // Die is a one-shot clip (non-loopable): tNormalized must reach exactly 1.0
            // on the last frame so the insect fully settles.  Loopable clips use
            // frameCount as the divisor (frames span [0, 1)), but here we use
            // frameCount - 1 so frames span [0, 1].
            double tNormalized = static_cast<double>(frame) / static_cast<double>(std::max(1, frameCount - 1));

            // --- Step 1: Save positions at the start of this timestep ---
            std::vector<Vector3> savedHead(bones.size()), savedTail(bones.size());
            for (size_t i = 0; i < bones.size(); ++i) {
                savedHead[i] = bones[i].headPos;
                savedTail[i] = bones[i].tailPos;
            }

            // --- Step 2: Integrate gravity into velocity and predict new positions ---
            for (auto& bone : bones) {
                bone.headVel += gravity * dt;
                bone.tailVel += gravity * dt;
                bone.headPos += bone.headVel * dt;
                bone.tailPos += bone.tailVel * dt;
            }

            // --- Step 3: Position-based constraint solve (no velocity modifications here) ---
            for (size_t iter = 0; iter < constraintIterations; ++iter) {
                // Bones are in topological order so parent joints are resolved first.
                for (auto& bone : bones) {
                    // Length constraint: keep bone length near its rest length.
                    Vector3 delta = bone.tailPos - bone.headPos;
                    double len = delta.length();
                    if (len > 1e-6) {
                        double correction = (len - bone.restLength) / len * 0.5 * lengthStiffness;
                        Vector3 diff = delta * correction;
                        bone.headPos += diff;
                        bone.tailPos -= diff;
                    }

                    // Joint constraint: keep this bone's head coincident with its parent's tail.
                    if (!bone.parent.empty()) {
                        auto parentIt = ragdollBoneIdx.find(bone.parent);
                        if (parentIt != ragdollBoneIdx.end()) {
                            auto& parent = bones[parentIt->second];
                            Vector3 jointError = parent.tailPos - bone.headPos;
                            Vector3 correction = jointError * parentJointStiffness;
                            bone.headPos += correction;
                            parent.tailPos -= correction;
                        }
                    }

                    // Angular constraint: clamp bone direction to within maxJointAngleRad
                    // of its rest direction using Rodrigues' rotation for an exact result.
                    if (!bone.restDir.isZero()) {
                        Vector3 currentDir = (bone.tailPos - bone.headPos).normalized();
                        if (!currentDir.isZero()) {
                            double dot = std::max(-1.0, std::min(1.0, Vector3::dotProduct(currentDir, bone.restDir)));
                            double angle = std::acos(dot);
                            if (angle > maxJointAngleRad) {
                                // Rotate currentDir toward restDir by (angle - maxJointAngleRad).
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

            // --- Step 4: Ground collision (position clamp along gravity axis) ---
            // A position is "below ground" when its projection onto gravityDir exceeds
            // (groundLevel - radius).  Push it back along the anti-gravity direction.
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

            // --- Step 5: Derive velocities from final position changes (PBD style) ---
            for (size_t i = 0; i < bones.size(); ++i) {
                bones[i].headVel = (bones[i].headPos - savedHead[i]) / dt;
                bones[i].tailVel = (bones[i].tailPos - savedTail[i]) / dt;
            }

            // --- Step 6: Velocity-level bounce and damping ---
            // Reflect the velocity component along gravityDir when a point is on the
            // ground and still moving toward it (positive component along gravityDir).
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
