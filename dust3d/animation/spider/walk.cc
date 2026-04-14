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

// Procedural walk animation for eight-legged spider rig.
// Uses alternating tetrapod gait: two groups of four legs alternate
// swing and stance phases, similar to real spider locomotion.

#include <array>
#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/common.h>
#include <dust3d/animation/spider/walk.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace spider {

    namespace {

        struct LegDef {
            const char* coxaName;
            const char* femurName;
            const char* tibiaName;
        };

        struct LegRest {
            Vector3 coxaPos, coxaEnd;
            Vector3 femurEnd;
            Vector3 tibiaEnd;
            Vector3 restStickDir;
            Vector3 restCoxaToFemurVec;
        };

    } // anonymous namespace

    bool walk(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        int frameCount = static_cast<int>(parameters.getValue("frameCount", 30.0));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 1.0));

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

        // Verify required bones from rig_spider.xml
        static const char* requiredBones[] = {
            "Root", "Cephalothorax", "Head", "Abdomen",
            "FrontLeftCoxa", "FrontLeftFemur", "FrontLeftTibia",
            "FrontRightCoxa", "FrontRightFemur", "FrontRightTibia",
            "MidFrontLeftCoxa", "MidFrontLeftFemur", "MidFrontLeftTibia",
            "MidFrontRightCoxa", "MidFrontRightFemur", "MidFrontRightTibia",
            "MidBackLeftCoxa", "MidBackLeftFemur", "MidBackLeftTibia",
            "MidBackRightCoxa", "MidBackRightFemur", "MidBackRightTibia",
            "BackLeftCoxa", "BackLeftFemur", "BackLeftTibia",
            "BackRightCoxa", "BackRightFemur", "BackRightTibia"
        };
        for (const char* name : requiredBones) {
            if (boneIdx.find(name) == boneIdx.end())
                return false;
        }

        // ===================================================================
        // 1. Determine body facing from bone positions
        // ===================================================================
        Vector3 bodyVector = getBonePos("Head") - getBoneEnd("Abdomen");
        if (bodyVector.isZero())
            return false;

        Vector3 forward = bodyVector.normalized();
        Vector3 worldUp(0.0, 1.0, 0.0);
        Vector3 right = Vector3::crossProduct(forward, worldUp);
        if (right.lengthSquared() < 1e-8)
            right = Vector3::crossProduct(forward, Vector3(0.0, 0.0, 1.0));
        right.normalize();
        Vector3 up = Vector3::crossProduct(right, forward).normalized();

        // ===================================================================
        // 2. Define eight legs with alternating tetrapod gait groups
        // ===================================================================
        // Alternating tetrapod gait – the standard spider locomotion pattern:
        //   Group A: FrontLeft, MidFrontRight, MidBackLeft, BackRight
        //   Group B: FrontRight, MidFrontLeft, MidBackRight, BackLeft
        //
        // Phase offsets per leg (front-to-back ripple pattern).
        // Front legs lead, back legs trail within each gait group.
        // Left/right alternation is handled by the gait group assignment,
        // so both sides of the same row get the SAME offset to preserve
        // the front-to-back wave without disrupting the alternation.
        static const double basePhaseOffsets[8] = {
            // FrontLeft(A), FrontRight(B)
            0.15, 0.15,
            // MidFrontLeft(B), MidFrontRight(A)
            0.05, 0.05,
            // MidBackLeft(A), MidBackRight(B)
            -0.05, -0.05,
            // BackLeft(B), BackRight(A)
            -0.15, -0.15
        };

        // Per-leg step length multiplier: real spiders reach their front
        // legs much further forward than back legs.  These ratios produce
        // the characteristic long-reach front stride where the leg is
        // nearly fully stretched at the peak of the swing.
        static const double legStepScale[8] = {
            // FrontLeft, FrontRight
            2.5, 2.5,
            // MidFrontLeft, MidFrontRight
            1.5, 1.5,
            // MidBackLeft, MidBackRight
            0.9, 0.9,
            // BackLeft, BackRight
            0.7, 0.7
        };

        struct LegRuntime {
            LegDef def;
            int gaitGroup;
            int sideSign;
            double phaseOffset;
        };
        std::array<LegRuntime, 8> legs = { {
            { { "FrontLeftCoxa", "FrontLeftFemur", "FrontLeftTibia" }, 0, +1, 0.0 },
            { { "FrontRightCoxa", "FrontRightFemur", "FrontRightTibia" }, 1, -1, 0.0 },
            { { "MidFrontLeftCoxa", "MidFrontLeftFemur", "MidFrontLeftTibia" }, 1, +1, 0.0 },
            { { "MidFrontRightCoxa", "MidFrontRightFemur", "MidFrontRightTibia" }, 0, -1, 0.0 },
            { { "MidBackLeftCoxa", "MidBackLeftFemur", "MidBackLeftTibia" }, 0, +1, 0.0 },
            { { "MidBackRightCoxa", "MidBackRightFemur", "MidBackRightTibia" }, 1, -1, 0.0 },
            { { "BackLeftCoxa", "BackLeftFemur", "BackLeftTibia" }, 1, +1, 0.0 },
            { { "BackRightCoxa", "BackRightFemur", "BackRightTibia" }, 0, -1, 0.0 },
        } };
        for (size_t i = 0; i < 8; ++i)
            legs[i].phaseOffset = basePhaseOffsets[i];

        // Gather rest-pose joint positions for every leg
        std::array<LegRest, 8> legRest;
        for (size_t i = 0; i < 8; ++i) {
            legRest[i].coxaPos = getBonePos(legs[i].def.coxaName);
            legRest[i].coxaEnd = getBoneEnd(legs[i].def.coxaName);
            legRest[i].femurEnd = getBoneEnd(legs[i].def.femurName);
            legRest[i].tibiaEnd = getBoneEnd(legs[i].def.tibiaName);
            Vector3 chordVec = legRest[i].tibiaEnd - legRest[i].coxaEnd;
            legRest[i].restStickDir = chordVec.isZero() ? Vector3(1, 0, 0) : chordVec.normalized();
            legRest[i].restCoxaToFemurVec = legRest[i].femurEnd - legRest[i].coxaEnd;
        }

        // ===================================================================
        // 3. Compute walking parameters from body geometry
        // ===================================================================
        double stepLengthFactor = parameters.getValue("stepLengthFactor", 1.0);
        double stepHeightFactor = parameters.getValue("stepHeightFactor", 1.0);
        double bodyBobFactor = parameters.getValue("bodyBobFactor", 1.0);
        double legSpreadFactor = parameters.getValue("legSpreadFactor", 1.0);
        double abdomenSwayFactor = parameters.getValue("abdomenSwayFactor", 1.0);
        double pedipalpSwayFactor = parameters.getValue("pedipalpSwayFactor", 1.0);
        double bodyYawFactor = parameters.getValue("bodyYawFactor", 1.0);

        double bodyLength = bodyVector.length();
        double stepLength = bodyLength * 0.22 * stepLengthFactor;
        double stepHeight = bodyLength * 0.08 * stepHeightFactor;
        double bodyBobAmp = bodyLength * 0.015 * bodyBobFactor;
        double abdomenSwayAmp = 0.03 * abdomenSwayFactor;
        double pedipalpSwayAmp = 0.05 * pedipalpSwayFactor;
        double bodyYawAmp = 0.015 * bodyYawFactor;

        // Ground level
        double minUpProj = Vector3::dotProduct(legRest[0].tibiaEnd, up);
        for (size_t i = 1; i < 8; ++i) {
            double proj = Vector3::dotProduct(legRest[i].tibiaEnd, up);
            if (proj < minUpProj)
                minUpProj = proj;
        }

        // Foot home positions
        std::array<Vector3, 8> footHome;
        for (size_t i = 0; i < 8; ++i) {
            double proj = Vector3::dotProduct(legRest[i].tibiaEnd, up);
            footHome[i] = legRest[i].tibiaEnd - up * (proj - minUpProj);
            // Apply leg spread factor: scale lateral (right-axis) component away from center
            double rightProj = Vector3::dotProduct(footHome[i], right);
            footHome[i] = footHome[i] + right * (rightProj * (legSpreadFactor - 1.0));

            // Slightly bias front legs forward and back legs rearward so the gait
            // looks more purposeful and the body feels balanced over the stance legs.
            if (i == 0 || i == 1 || i == 2 || i == 3)
                footHome[i] = footHome[i] + forward * (bodyLength * 0.02);
            else
                footHome[i] = footHome[i] - forward * (bodyLength * 0.02);
        }

        // ===================================================================
        // 4. Generate animation frames
        // ===================================================================
        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        double gaitSpeedFactor = parameters.getValue("gaitSpeedFactor", 1.0);
        const double cycles = std::max(1.0, std::round(gaitSpeedFactor));

        // ----- Spring-damper state for secondary dynamics -----
        // These track current values and velocities that are integrated
        // across frames, producing organic overshoot and settling instead
        // of rigid sine-wave motion.
        double dt = durationSeconds / static_cast<double>(frameCount);
        double springStiffness = parameters.getValue("springStiffness", 120.0);
        double springDamping = parameters.getValue("springDamping", 12.0);

        double sdBodyBob = 0.0, sdBodyBobVel = 0.0;
        double sdBodyPitch = 0.0, sdBodyPitchVel = 0.0;
        double sdBodyRoll = 0.0, sdBodyRollVel = 0.0;
        double sdBodyYaw = 0.0, sdBodyYawVel = 0.0;
        double sdAbdomenYaw = 0.0, sdAbdomenYawVel = 0.0;
        double sdPalpLeft = 0.0, sdPalpLeftVel = 0.0;
        double sdPalpRight = 0.0, sdPalpRightVel = 0.0;

        // ----- Foot locking state -----
        // When a leg transitions from swing to stance, its world-space foot
        // position is recorded. During the entire stance phase the foot stays
        // pinned at that position, eliminating sliding.
        std::array<Vector3, 8> plantedFootPos;
        std::array<bool, 8> wasSwinging;
        for (size_t i = 0; i < 8; ++i) {
            plantedFootPos[i] = footHome[i];
            wasSwinging[i] = false;
        }

        for (int frame = 0; frame < frameCount; ++frame) {
            double tNormalized = static_cast<double>(frame) / static_cast<double>(frameCount);
            double t = fmod(tNormalized * cycles, 1.0);

            // Body oscillation — spring-damper targets
            double targetBodyBob = bodyBobAmp * std::sin(t * 4.0 * Math::Pi);
            double targetBodyPitch = 0.015 * bodyBobFactor * std::sin(t * 2.0 * Math::Pi);
            double targetBodyRoll = 0.01 * bodyBobFactor * std::cos(t * 2.0 * Math::Pi);
            double targetBodyYaw = bodyYawAmp * std::sin(t * 2.0 * Math::Pi);

            // Step spring-damper for each body DOF
            auto springStep = [&](double& cur, double& vel, double target) {
                double accel = -springStiffness * (cur - target) - springDamping * vel;
                vel += accel * dt;
                cur += vel * dt;
            };
            springStep(sdBodyBob, sdBodyBobVel, targetBodyBob);
            springStep(sdBodyPitch, sdBodyPitchVel, targetBodyPitch);
            springStep(sdBodyRoll, sdBodyRollVel, targetBodyRoll);
            springStep(sdBodyYaw, sdBodyYawVel, targetBodyYaw);

            double bodyBob = sdBodyBob;
            double bodyPitch = sdBodyPitch;
            double bodyRoll = sdBodyRoll;
            double bodyYaw = sdBodyYaw;

            Matrix4x4 bodyTransform;
            bodyTransform.translate(up * bodyBob);
            bodyTransform.rotate(forward, bodyRoll);
            bodyTransform.rotate(right, bodyPitch);
            bodyTransform.rotate(up, bodyYaw);

            // -------------------------------------------------------
            // 4a. Compute foot target for each leg
            // -------------------------------------------------------
            std::array<Vector3, 8> footTarget;

            for (size_t i = 0; i < 8; ++i) {
                int group = legs[i].gaitGroup;
                bool isSwing;
                double legPhase;

                double phase = std::fmod(t + legs[i].phaseOffset + 1.0, 1.0);
                if (group == 0) {
                    if (phase < 0.5) {
                        isSwing = true;
                        legPhase = phase / 0.5;
                    } else {
                        isSwing = false;
                        legPhase = (phase - 0.5) / 0.5;
                    }
                } else {
                    if (phase < 0.5) {
                        isSwing = false;
                        legPhase = phase / 0.5;
                    } else {
                        isSwing = true;
                        legPhase = (phase - 0.5) / 0.5;
                    }
                }

                Vector3 footFront = footHome[i] + forward * stepLength * legStepScale[i];
                Vector3 footBack = footHome[i] - forward * stepLength * legStepScale[i];

                // Front legs also lift higher for a more dramatic reaching arc
                double legLift = stepHeight * (legStepScale[i] > 1.0 ? legStepScale[i] : 1.0);

                if (isSwing) {
                    double smoothSwing = animation::smootherstep(legPhase);
                    Vector3 groundPos = footBack + (footFront - footBack) * smoothSwing;
                    double lift = legLift * std::sin(smoothSwing * Math::Pi);
                    double lateralSwing = stepLength * 0.15 * legs[i].sideSign * std::sin(smoothSwing * Math::Pi);
                    footTarget[i] = groundPos + up * lift + right * lateralSwing;
                    wasSwinging[i] = true;
                } else {
                    // Foot locking: record foot position at the moment of landing
                    if (wasSwinging[i]) {
                        // Capture the actual ground position where the swing arc ended,
                        // not a precomputed position — this keeps the foot exactly
                        // where it visually touched down.
                        Vector3 landingPos = footBack + (footFront - footBack) * 1.0; // end of swing
                        // Apply the same lateral offset at landing (swing peak is at mid-swing,
                        // so at legPhase=1.0 lateral is ~0, landing is near footFront)
                        plantedFootPos[i] = landingPos;
                        wasSwinging[i] = false;
                    }
                    // Slight backward drift during stance so the foot isn't perfectly
                    // frozen — real spider feet shift subtly as the body advances.
                    // This small motion (10% of step length) prevents the rigid
                    // locked-foot look while still eliminating visible sliding.
                    double stanceDrift = stepLength * 0.1 * animation::smoothstep(legPhase);
                    footTarget[i] = plantedFootPos[i] - forward * stanceDrift;
                }
            }

            // -------------------------------------------------------
            // 4b. Body bone transforms
            // -------------------------------------------------------
            std::map<std::string, Matrix4x4> boneWorldTransforms;

            auto computeBodyBone = [&](const std::string& name, const Matrix4x4& transform) {
                Vector3 pos = getBonePos(name);
                Vector3 end = getBoneEnd(name);
                Vector3 newPos = transform.transformPoint(pos);
                Vector3 newEnd = transform.transformPoint(end);
                boneWorldTransforms[name] = animation::buildBoneWorldTransform(newPos, newEnd);
            };

            computeBodyBone("Root", bodyTransform);
            computeBodyBone("Cephalothorax", bodyTransform);
            computeBodyBone("Head", bodyTransform);

            // Abdomen sways opposite to the body yaw — spring-damper for trailing lag
            double targetAbdomenYaw = -abdomenSwayAmp * std::sin(t * 2.0 * Math::Pi);
            springStep(sdAbdomenYaw, sdAbdomenYawVel, targetAbdomenYaw);
            Matrix4x4 abdomenTransform = bodyTransform;
            abdomenTransform.rotate(up, sdAbdomenYaw);
            computeBodyBone("Abdomen", abdomenTransform);

            // Pedipalps: slight alternating sway synchronized with walk
            // Pedipalps — spring-damper for organic sway
            {
                double targetLeft = pedipalpSwayAmp * std::sin(t * 4.0 * Math::Pi);
                double targetRight = -pedipalpSwayAmp * std::sin(t * 4.0 * Math::Pi);
                springStep(sdPalpLeft, sdPalpLeftVel, targetLeft);
                springStep(sdPalpRight, sdPalpRightVel, targetRight);

                double palpAngles[2] = { sdPalpLeft, sdPalpRight };
                const char* palpNames[2] = { "LeftPedipalp", "RightPedipalp" };
                for (int p = 0; p < 2; ++p) {
                    if (boneIdx.count(palpNames[p]) == 0)
                        continue;
                    Matrix4x4 palpTransform = bodyTransform;
                    palpTransform.rotate(up, palpAngles[p]);
                    computeBodyBone(palpNames[p], palpTransform);
                }
            }

            // -------------------------------------------------------
            // 4c. Leg IK
            // -------------------------------------------------------

            for (size_t i = 0; i < 8; ++i) {
                Vector3 hipPos = bodyTransform.transformPoint(legRest[i].coxaPos);
                Vector3 coxaEnd = bodyTransform.transformPoint(legRest[i].coxaEnd);
                Vector3 tibiaEnd = bodyTransform.transformPoint(legRest[i].tibiaEnd);

                std::vector<Vector3> chain = { hipPos, coxaEnd, tibiaEnd };

                // Pole vector: spider legs bend upward
                Vector3 poleVector = coxaEnd + up * 0.5;
                // Use very low softness (0.02) so legs can nearly fully
                // straighten at maximum reach, matching real spider gait
                // where the front legs extend almost straight when reaching.
                animation::solveTwoBoneIk(chain, footTarget[i], poleVector, 0.02);

                // Reconstruct femur-tibia junction
                Vector3 newStickDir = (chain[2] - chain[1]);
                if (newStickDir.isZero())
                    newStickDir = legRest[i].restStickDir;
                else
                    newStickDir.normalize();
                Quaternion stickRot = Quaternion::rotationTo(legRest[i].restStickDir, newStickDir);
                Matrix4x4 stickRotMat;
                stickRotMat.rotate(stickRot);
                Vector3 femurEnd = chain[1] + stickRotMat.transformVector(legRest[i].restCoxaToFemurVec);

                boneWorldTransforms[legs[i].def.coxaName] = animation::buildBoneWorldTransform(chain[0], chain[1]);
                boneWorldTransforms[legs[i].def.femurName] = animation::buildBoneWorldTransform(chain[1], femurEnd);
                boneWorldTransforms[legs[i].def.tibiaName] = animation::buildBoneWorldTransform(femurEnd, chain[2]);
            }

            // -------------------------------------------------------
            // 4d. Skin matrices
            // -------------------------------------------------------
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

} // namespace spider

} // namespace dust3d
