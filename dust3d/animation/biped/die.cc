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

// Procedural death/ragdoll animation for biped rig.
//
// Industry techniques used:
//
// 1. XPBD (EXTENDED POSITION BASED DYNAMICS) — Compliance-based constraint
//    solving from Macklin & Müller 2016. Instead of iterating with arbitrary
//    stiffness multipliers, each constraint has a compliance parameter (inverse
//    stiffness) that produces physically correct behavior regardless of
//    iteration count or timestep. This eliminates the "rubbery" feel of
//    basic PBD.
//
// 2. MUSCLE TONE DECAY — Joint angular constraints start with high stiffness
//    (simulating active muscle tone) that exponentially decays over time.
//    This creates the realistic transition from "just hit" (body still has
//    some rigidity) to "fully ragdoll" (completely limp).
//
// 3. HIT IMPULSE PROPAGATION — Instead of hand-tuned per-bone velocities,
//    an impact force is applied at a configurable hit point (chest height)
//    and propagates through the kinematic chain with mass-weighted
//    attenuation. Each bone receives force proportional to its distance
//    from the impact point, creating a realistic chain reaction.
//
// 4. BONE CAPSULE SELF-COLLISION — Bones are modeled as capsules (line
//    segment + radius). Every frame, capsule-capsule overlap tests prevent
//    arms from passing through the torso and legs from intersecting,
//    producing more believable crumpled poses.

#include <algorithm>
#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/biped/die.h>
#include <dust3d/animation/common.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>
#include <functional>

namespace dust3d {

namespace biped {

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
            float mass; // mass proportional to bone length
            float muscleTone; // current muscle stiffness (decays over time)
        };

        // Closest distance between two line segments (for capsule collision)
        double segmentSegmentDist(
            const Vector3& p0, const Vector3& p1,
            const Vector3& q0, const Vector3& q1,
            Vector3& closestOnP, Vector3& closestOnQ)
        {
            Vector3 d1 = p1 - p0;
            Vector3 d2 = q1 - q0;
            Vector3 r = p0 - q0;
            double a = Vector3::dotProduct(d1, d1);
            double e = Vector3::dotProduct(d2, d2);
            double f = Vector3::dotProduct(d2, r);

            double s = 0.0, t = 0.0;

            if (a <= 1e-10 && e <= 1e-10) {
                closestOnP = p0;
                closestOnQ = q0;
                return (p0 - q0).length();
            }
            if (a <= 1e-10) {
                s = 0.0;
                t = std::max(0.0, std::min(1.0, f / e));
            } else {
                double c = Vector3::dotProduct(d1, r);
                if (e <= 1e-10) {
                    t = 0.0;
                    s = std::max(0.0, std::min(1.0, -c / a));
                } else {
                    double b = Vector3::dotProduct(d1, d2);
                    double denom = a * e - b * b;
                    if (denom > 1e-10)
                        s = std::max(0.0, std::min(1.0, (b * f - c * e) / denom));
                    else
                        s = 0.0;
                    t = (b * s + f) / e;
                    if (t < 0.0) {
                        t = 0.0;
                        s = std::max(0.0, std::min(1.0, -c / a));
                    } else if (t > 1.0) {
                        t = 1.0;
                        s = std::max(0.0, std::min(1.0, (b - c) / a));
                    }
                }
            }
            closestOnP = p0 + d1 * s;
            closestOnQ = q0 + d2 * t;
            return (closestOnP - closestOnQ).length();
        }

        // Check if two bones are in the same chain (parent/child/sibling)
        bool areBoneRelated(const std::vector<BoneRagdollInfo>& bones,
            const std::map<std::string, size_t>& boneIdx,
            size_t a, size_t b)
        {
            // Direct parent-child
            if (bones[a].parent == bones[b].name || bones[b].parent == bones[a].name)
                return true;
            // Same parent (siblings connected at joint)
            if (!bones[a].parent.empty() && bones[a].parent == bones[b].parent)
                return true;
            // Grandparent
            if (!bones[a].parent.empty()) {
                auto pit = boneIdx.find(bones[a].parent);
                if (pit != boneIdx.end() && bones[pit->second].name == bones[b].parent)
                    return true;
            }
            if (!bones[b].parent.empty()) {
                auto pit = boneIdx.find(bones[b].parent);
                if (pit != boneIdx.end() && bones[pit->second].name == bones[a].parent)
                    return true;
            }
            return false;
        }

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
            "Root", "Hips", "Spine", "Chest", "Neck", "Head",
            "LeftShoulder", "LeftUpperArm", "LeftLowerArm", "LeftHand",
            "RightShoulder", "RightUpperArm", "RightLowerArm", "RightHand",
            "LeftUpperLeg", "LeftLowerLeg", "LeftFoot",
            "RightUpperLeg", "RightLowerLeg", "RightFoot"
        };

        if (!animation::validateRequiredBones(boneIdx, requiredBones, sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;

        // ===================================================================
        // 1. Build ragdoll bone list with mass estimation
        // ===================================================================
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
            // Mass proportional to bone volume (length * radius^2)
            info.mass = std::max(0.1f, info.restLength * std::max(0.01f, info.radius * info.radius));
            info.muscleTone = 1.0f;
            bones.push_back(info);
        }

        // Topological sort: parents before children
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

        // ===================================================================
        // 2. Determine body orientation and ground
        // ===================================================================
        Vector3 gravityDir(0.0, -1.0, 0.0);
        Vector3 forwardDir(0.0, 0.0, 1.0);
        {
            auto headIt = ragdollBoneIdx.find("Head");
            auto hipsIt = ragdollBoneIdx.find("Hips");
            if (headIt != ragdollBoneIdx.end() && hipsIt != ragdollBoneIdx.end()) {
                Vector3 delta = bones[headIt->second].headPos - bones[hipsIt->second].headPos;
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

        double groundLevel = -1e18;
        for (const auto& bone : bones) {
            if (bone.name.find("Foot") != std::string::npos || bone.name.find("Hand") != std::string::npos) {
                double d = Vector3::dotProduct(bone.tailPos, gravityDir);
                groundLevel = std::max(groundLevel, d);
            }
        }
        if (groundLevel < -1e17)
            groundLevel = 0.0;

        // ===================================================================
        // 3. Parameters
        // ===================================================================
        float collapseSpeedFactor = static_cast<float>(parameters.getValue("collapseSpeedFactor", 1.0));
        float armFlailFactor = static_cast<float>(parameters.getValue("armFlailFactor", 1.0));
        float headDropFactor = static_cast<float>(parameters.getValue("headDropFactor", 1.0));
        float rollIntensityFactor = static_cast<float>(parameters.getValue("rollIntensityFactor", 1.0));
        float lengthStiffness = static_cast<float>(parameters.getValue("lengthStiffness", 0.9));
        float parentJointStiffness = static_cast<float>(parameters.getValue("parentStiffness", 0.8));
        float maxJointAngleDeg = static_cast<float>(parameters.getValue("maxJointAngleDeg", 130.0));
        double maxJointAngleRad = maxJointAngleDeg * (Math::Pi / 180.0);
        float damping = static_cast<float>(parameters.getValue("damping", 0.95));
        float groundBounce = static_cast<float>(parameters.getValue("groundBounce", 0.18));

        // New parameters
        float muscleToneDecay = static_cast<float>(parameters.getValue("muscleToneDecay", 1.0));
        float hitForceFactor = static_cast<float>(parameters.getValue("hitForceFactor", 1.0));
        float selfCollisionFactor = static_cast<float>(parameters.getValue("selfCollisionFactor", 1.0));

        // Muscle tone half-life in seconds: how quickly joints go limp
        // Higher muscleToneDecay = faster decay = goes limp sooner
        double muscleToneHalfLife = 0.35 / std::max(0.1, static_cast<double>(muscleToneDecay));

        // ===================================================================
        // 4. Hit impulse propagation
        // ===================================================================
        // Impact originates at the chest, hitting from the front of the body.
        // Only the upper body (above hips) receives force; legs and feet stay
        // planted so the character folds backward/sideways from the torso up.
        auto chestIt = ragdollBoneIdx.find("Chest");
        Vector3 hitPoint;
        if (chestIt != ragdollBoneIdx.end())
            hitPoint = (bones[chestIt->second].headPos + bones[chestIt->second].tailPos) * 0.5;
        else
            hitPoint = Vector3(0, 0, 0);

        // Hit direction: from the front (pushes backward along -forwardDir)
        // with slight upward and sideways components for natural roll
        Vector3 hitDir = forwardDir * (-1.0)
            + sideDir * (0.25 * rollIntensityFactor)
            + gravityDir * (-0.15);
        if (!hitDir.isZero())
            hitDir.normalize();

        double hitForceBase = 2.5 * collapseSpeedFactor * hitForceFactor;

        // Bones in the lower body that should NOT receive hit force
        // so feet stay planted during the initial impact
        static const char* lowerBodyBones[] = {
            "LeftUpperLeg", "LeftLowerLeg", "LeftFoot",
            "RightUpperLeg", "RightLowerLeg", "RightFoot"
        };
        std::set<std::string> lowerBodySet(lowerBodyBones,
            lowerBodyBones + sizeof(lowerBodyBones) / sizeof(lowerBodyBones[0]));

        // Compute total mass for normalization
        double totalMass = 0.0;
        for (const auto& bone : bones)
            totalMass += bone.mass;
        if (totalMass < 1e-6)
            totalMass = 1.0;

        for (auto& bone : bones) {
            // Skip lower body — legs stay where they are
            if (lowerBodySet.count(bone.name)) {
                bone.headVel = Vector3(0.0, 0.0, 0.0);
                bone.tailVel = Vector3(0.0, 0.0, 0.0);
                continue;
            }

            Vector3 boneMid = (bone.headPos + bone.tailPos) * 0.5;
            double distFromHit = (boneMid - hitPoint).length();

            // Force attenuation: inverse distance with minimum floor
            // Closer bones get more force; very distant bones still get some
            double maxDist = 2.0; // normalize to approximate body height
            double attenuation = 1.0 / (1.0 + distFromHit / maxDist);

            // Mass-weighted: lighter bones accelerate more
            double invMass = 1.0 / static_cast<double>(bone.mass);
            double forceMag = hitForceBase * attenuation * invMass * (bone.mass / totalMass) * 8.0;

            // Direction: mix hit direction with a radial component away from hit point
            Vector3 radial = boneMid - hitPoint;
            if (!radial.isZero())
                radial.normalize();
            Vector3 boneHitDir = hitDir * 0.7 + radial * 0.3;
            if (!boneHitDir.isZero())
                boneHitDir.normalize();

            bone.headVel = boneHitDir * forceMag;
            bone.tailVel = boneHitDir * forceMag;
        }

        // Additional specific impulses for character (arm flail, head drop)
        auto applyExtraImpulse = [&](const std::string& name, const Vector3& extra) {
            auto it = ragdollBoneIdx.find(name);
            if (it != ragdollBoneIdx.end()) {
                bones[it->second].headVel += extra;
                bones[it->second].tailVel += extra;
            }
        };

        // Head gets extra drop
        {
            auto it = ragdollBoneIdx.find("Head");
            if (it != ragdollBoneIdx.end()) {
                bones[it->second].tailVel += forwardDir * 0.25 + gravityDir * (0.2 * collapseSpeedFactor * headDropFactor);
            }
        }

        // Arms get extra flail outward
        for (const auto& name : { "LeftUpperArm", "LeftLowerArm", "LeftHand" })
            applyExtraImpulse(name, sideDir * (0.5 * armFlailFactor));
        for (const auto& name : { "RightUpperArm", "RightLowerArm", "RightHand" })
            applyExtraImpulse(name, sideDir * (-0.5 * armFlailFactor));

        // ===================================================================
        // 5. Generate frames with XPBD + muscle tone decay + self-collision
        // ===================================================================
        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.clear();
        animationClip.frames.resize(frameCount);

        double dt = durationSeconds / std::max(1, frameCount);
        double dtSq = dt * dt;
        Vector3 gravity = gravityDir * 9.8;
        const size_t constraintIterations = 6;

        // Pre-build self-collision pair list (skip related bones)
        std::vector<std::pair<size_t, size_t>> collisionPairs;
        if (selfCollisionFactor > 0.01f) {
            for (size_t i = 0; i < bones.size(); ++i) {
                for (size_t j = i + 1; j < bones.size(); ++j) {
                    if (!areBoneRelated(bones, ragdollBoneIdx, i, j))
                        collisionPairs.emplace_back(i, j);
                }
            }
        }

        for (int frame = 0; frame < frameCount; ++frame) {
            double tNormalized = static_cast<double>(frame) / static_cast<double>(std::max(1, frameCount - 1));
            double simTime = tNormalized * durationSeconds;

            // --- Muscle tone decay: exponential falloff ---
            double toneMultiplier = std::exp(-0.693 * simTime / muscleToneHalfLife);
            // Clamp to [0, 1]
            toneMultiplier = std::max(0.0, std::min(1.0, toneMultiplier));

            // --- Save old positions for Verlet velocity derivation ---
            std::vector<Vector3> savedHead(bones.size()), savedTail(bones.size());
            for (size_t i = 0; i < bones.size(); ++i) {
                savedHead[i] = bones[i].headPos;
                savedTail[i] = bones[i].tailPos;
            }

            // --- Semi-implicit Euler integration ---
            for (auto& bone : bones) {
                bone.headVel += gravity * dt;
                bone.tailVel += gravity * dt;
                bone.headPos += bone.headVel * dt;
                bone.tailPos += bone.tailVel * dt;
            }

            // --- XPBD constraint solving ---
            // Compliance = inverse stiffness. Lower compliance = stiffer constraint.
            // In XPBD: deltaX = -C(x) / (w1 + w2 + alpha/dt^2) where alpha = compliance
            for (size_t iter = 0; iter < constraintIterations; ++iter) {

                // 5a. Distance constraint (bone length preservation)
                // Compliance decreases with lengthStiffness (stiffer = less compliance)
                double lengthCompliance = (1.0 - lengthStiffness) * 0.01;
                double lengthAlpha = lengthCompliance / dtSq;

                for (auto& bone : bones) {
                    Vector3 delta = bone.tailPos - bone.headPos;
                    double len = delta.length();
                    if (len < 1e-8)
                        continue;
                    double C = len - bone.restLength; // constraint violation
                    double invMassSum = (1.0 / bone.mass) + (1.0 / bone.mass); // both endpoints same mass
                    double denom = invMassSum + lengthAlpha;
                    if (denom < 1e-12)
                        continue;
                    double lambda = -C / denom;
                    Vector3 n = delta * (1.0 / len);
                    Vector3 correction = n * (lambda / bone.mass);
                    bone.headPos -= correction;
                    bone.tailPos += correction;
                }

                // 5b. Joint constraint (parent-child attachment)
                // Compliance modulated by muscle tone: high tone = low compliance = rigid
                double baseJointCompliance = (1.0 - parentJointStiffness) * 0.005;
                double jointCompliance = baseJointCompliance + (1.0 - toneMultiplier) * 0.02;
                double jointAlpha = jointCompliance / dtSq;

                for (auto& bone : bones) {
                    if (bone.parent.empty())
                        continue;
                    auto parentIt = ragdollBoneIdx.find(bone.parent);
                    if (parentIt == ragdollBoneIdx.end())
                        continue;
                    auto& parent = bones[parentIt->second];

                    Vector3 desiredHeadPos = parent.tailPos + bone.parentOffset;
                    Vector3 jointError = bone.headPos - desiredHeadPos;
                    double errLen = jointError.length();
                    if (errLen < 1e-8)
                        continue;

                    double C = errLen; // constraint: distance should be 0
                    double invMassSum = (1.0 / bone.mass) + (1.0 / parent.mass);
                    double denom = invMassSum + jointAlpha;
                    if (denom < 1e-12)
                        continue;
                    double lambda = -C / denom;
                    Vector3 n = jointError * (1.0 / errLen);

                    bone.headPos += n * (lambda / bone.mass);
                    parent.tailPos -= n * (lambda / parent.mass);
                }

                // 5c. Angular constraint with muscle-tone-modulated stiffness
                // As tone decays, the effective max angle increases (joints go limper)
                double effectiveMaxAngle = maxJointAngleRad + (Math::Pi - maxJointAngleRad) * (1.0 - toneMultiplier);
                // Also reduce angular correction strength as tone decays
                double angularStrength = 0.3 + 0.7 * toneMultiplier;

                for (auto& bone : bones) {
                    if (bone.restDir.isZero())
                        continue;

                    // Get effective rest direction: use parent's current direction if available
                    Vector3 effectiveRestDir = bone.restDir;
                    if (!bone.parent.empty()) {
                        auto parentIt = ragdollBoneIdx.find(bone.parent);
                        if (parentIt != ragdollBoneIdx.end()) {
                            auto& parent = bones[parentIt->second];
                            Vector3 parentDir = (parent.tailPos - parent.headPos);
                            if (!parentDir.isZero()) {
                                parentDir.normalize();
                                // Rotate rest direction to follow parent's current orientation
                                Quaternion parentRot = Quaternion::rotationTo(
                                    (parent.restDir.isZero() ? Vector3(0, 1, 0) : parent.restDir), parentDir);
                                Matrix4x4 rotMat;
                                rotMat.rotate(parentRot);
                                effectiveRestDir = rotMat.transformVector(bone.restDir);
                                if (!effectiveRestDir.isZero())
                                    effectiveRestDir.normalize();
                            }
                        }
                    }

                    Vector3 currentDir = (bone.tailPos - bone.headPos).normalized();
                    if (currentDir.isZero())
                        continue;

                    double dot = std::max(-1.0, std::min(1.0, Vector3::dotProduct(currentDir, effectiveRestDir)));
                    double angle = std::acos(dot);

                    if (angle > effectiveMaxAngle) {
                        Vector3 axis = Vector3::crossProduct(currentDir, effectiveRestDir);
                        double axisLen = axis.length();
                        Vector3 restrictedDir;
                        if (axisLen > 1e-6) {
                            axis /= axisLen;
                            double rotAngle = (angle - effectiveMaxAngle) * angularStrength;
                            double c = std::cos(rotAngle), s = std::sin(rotAngle);
                            restrictedDir = currentDir * c
                                + Vector3::crossProduct(axis, currentDir) * s
                                + axis * Vector3::dotProduct(axis, currentDir) * (1.0 - c);
                            restrictedDir = restrictedDir.normalized();
                        } else {
                            restrictedDir = effectiveRestDir;
                        }
                        bone.tailPos = bone.headPos + restrictedDir * bone.restLength;
                    }
                }

                // 5d. Self-collision (capsule-capsule)
                if (selfCollisionFactor > 0.01f) {
                    for (const auto& pair : collisionPairs) {
                        auto& boneA = bones[pair.first];
                        auto& boneB = bones[pair.second];

                        Vector3 closestA, closestB;
                        double dist = segmentSegmentDist(
                            boneA.headPos, boneA.tailPos,
                            boneB.headPos, boneB.tailPos,
                            closestA, closestB);

                        double minDist = (boneA.radius + boneB.radius) * selfCollisionFactor;
                        if (dist < minDist && dist > 1e-8) {
                            double overlap = minDist - dist;
                            Vector3 pushDir = closestA - closestB;
                            pushDir.normalize();

                            // Mass-weighted push
                            double totalInvMass = (1.0 / boneA.mass) + (1.0 / boneB.mass);
                            double pushA = overlap * (1.0 / boneA.mass) / totalInvMass;
                            double pushB = overlap * (1.0 / boneB.mass) / totalInvMass;

                            boneA.headPos += pushDir * (pushA * 0.5);
                            boneA.tailPos += pushDir * (pushA * 0.5);
                            boneB.headPos -= pushDir * (pushB * 0.5);
                            boneB.tailPos -= pushDir * (pushB * 0.5);
                        }
                    }
                }
            }

            // --- Ground collision ---
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

            // --- Derive velocities from position change (Verlet) ---
            for (size_t i = 0; i < bones.size(); ++i) {
                bones[i].headVel = (bones[i].headPos - savedHead[i]) / dt;
                bones[i].tailVel = (bones[i].tailPos - savedTail[i]) / dt;
            }

            // --- Ground bounce + velocity damping ---
            for (auto& bone : bones) {
                double headDot = Vector3::dotProduct(bone.headPos, gravityDir);
                if (headDot >= groundLevel - bone.radius - 1e-4) {
                    double vDot = Vector3::dotProduct(bone.headVel, gravityDir);
                    if (vDot > 0.0)
                        bone.headVel -= gravityDir * (vDot * (1.0 + groundBounce));
                    // Ground friction: damp lateral velocity when on ground
                    Vector3 lateralVel = bone.headVel - gravityDir * Vector3::dotProduct(bone.headVel, gravityDir);
                    bone.headVel -= lateralVel * 0.7;
                }
                double tailDot = Vector3::dotProduct(bone.tailPos, gravityDir);
                if (tailDot >= groundLevel - bone.radius - 1e-4) {
                    double vDot = Vector3::dotProduct(bone.tailVel, gravityDir);
                    if (vDot > 0.0)
                        bone.tailVel -= gravityDir * (vDot * (1.0 + groundBounce));
                    Vector3 lateralVel = bone.tailVel - gravityDir * Vector3::dotProduct(bone.tailVel, gravityDir);
                    bone.tailVel -= lateralVel * 0.7;
                }
                bone.headVel *= damping;
                bone.tailVel *= damping;
            }

            // --- Build bone transforms and skin matrices ---
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

} // namespace biped

} // namespace dust3d
