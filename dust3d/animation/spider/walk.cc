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

        struct LegDefWithGait : public LegDef {
            int gaitGroup;
            constexpr LegDefWithGait(LegDef base, int g)
                : LegDef(base)
                , gaitGroup(g)
            {
            }
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
        static const std::array<LegDefWithGait, 8> legs = { {
            { { "FrontLeftCoxa", "FrontLeftFemur", "FrontLeftTibia" }, 0 },
            { { "FrontRightCoxa", "FrontRightFemur", "FrontRightTibia" }, 1 },
            { { "MidFrontLeftCoxa", "MidFrontLeftFemur", "MidFrontLeftTibia" }, 1 },
            { { "MidFrontRightCoxa", "MidFrontRightFemur", "MidFrontRightTibia" }, 0 },
            { { "MidBackLeftCoxa", "MidBackLeftFemur", "MidBackLeftTibia" }, 0 },
            { { "MidBackRightCoxa", "MidBackRightFemur", "MidBackRightTibia" }, 1 },
            { { "BackLeftCoxa", "BackLeftFemur", "BackLeftTibia" }, 1 },
            { { "BackRightCoxa", "BackRightFemur", "BackRightTibia" }, 0 },
        } };

        // Gather rest-pose joint positions for every leg
        std::array<LegRest, 8> legRest;
        for (size_t i = 0; i < 8; ++i) {
            legRest[i].coxaPos = getBonePos(legs[i].coxaName);
            legRest[i].coxaEnd = getBoneEnd(legs[i].coxaName);
            legRest[i].femurEnd = getBoneEnd(legs[i].femurName);
            legRest[i].tibiaEnd = getBoneEnd(legs[i].tibiaName);
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
        double stepLength = bodyLength * 0.15 * stepLengthFactor;
        double stepHeight = bodyLength * 0.06 * stepHeightFactor;
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
        }

        // ===================================================================
        // 4. Generate animation frames
        // ===================================================================
        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        double gaitSpeedFactor = parameters.getValue("gaitSpeedFactor", 1.0);
        const double cycles = std::max(1.0, std::round(gaitSpeedFactor));

        for (int frame = 0; frame < frameCount; ++frame) {
            double tNormalized = static_cast<double>(frame) / static_cast<double>(frameCount);
            double t = fmod(tNormalized * cycles, 1.0);

            // Body oscillation
            double bodyBob = bodyBobAmp * std::sin(t * 4.0 * Math::Pi);
            double bodyPitch = 0.015 * bodyBobFactor * std::sin(t * 2.0 * Math::Pi);
            double bodyYaw = bodyYawAmp * std::sin(t * 2.0 * Math::Pi);

            Matrix4x4 bodyTransform;
            bodyTransform.translate(up * bodyBob);
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

                if (group == 0) {
                    if (t < 0.5) {
                        isSwing = true;
                        legPhase = t / 0.5;
                    } else {
                        isSwing = false;
                        legPhase = (t - 0.5) / 0.5;
                    }
                } else {
                    if (t < 0.5) {
                        isSwing = false;
                        legPhase = t / 0.5;
                    } else {
                        isSwing = true;
                        legPhase = (t - 0.5) / 0.5;
                    }
                }

                Vector3 footFront = footHome[i] + forward * stepLength;
                Vector3 footBack = footHome[i] - forward * stepLength;

                if (isSwing) {
                    double smoothSwing = animation::smootherstep(legPhase);
                    Vector3 groundPos = footBack + (footFront - footBack) * smoothSwing;
                    double lift = stepHeight * std::sin(smoothSwing * Math::Pi);
                    footTarget[i] = groundPos + up * lift;
                } else {
                    footTarget[i] = footFront + (footBack - footFront) * animation::smoothstep(legPhase);
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

            // Abdomen sways opposite to the body yaw for a natural trailing effect
            Matrix4x4 abdomenTransform = bodyTransform;
            double abdomenYaw = -abdomenSwayAmp * std::sin(t * 2.0 * Math::Pi);
            abdomenTransform.rotate(up, abdomenYaw);
            computeBodyBone("Abdomen", abdomenTransform);

            // Pedipalps: slight alternating sway synchronized with walk
            for (const char* palpName : { "LeftPedipalp", "RightPedipalp" }) {
                if (boneIdx.count(palpName) == 0)
                    continue;
                Matrix4x4 palpTransform = bodyTransform;
                double sign = (std::string(palpName) == "LeftPedipalp") ? 1.0 : -1.0;
                double palpAngle = pedipalpSwayAmp * sign * std::sin(t * 4.0 * Math::Pi);
                palpTransform.rotate(up, palpAngle);
                computeBodyBone(palpName, palpTransform);
            }

            // -------------------------------------------------------
            // 4c. Leg IK
            // -------------------------------------------------------
            bool useFabrikIk = parameters.getBool("useFabrikIk", true);
            bool planeStabilization = parameters.getBool("planeStabilization", true);

            for (size_t i = 0; i < 8; ++i) {
                Vector3 hipPos = bodyTransform.transformPoint(legRest[i].coxaPos);
                Vector3 coxaEnd = bodyTransform.transformPoint(legRest[i].coxaEnd);
                Vector3 tibiaEnd = bodyTransform.transformPoint(legRest[i].tibiaEnd);

                std::vector<Vector3> chain = { hipPos, coxaEnd, tibiaEnd };

                Vector3 preferPlane = Vector3::crossProduct(chain[1] - chain[0], chain[2] - chain[1]);
                if (preferPlane.lengthSquared() < 1e-10)
                    preferPlane = Vector3::crossProduct(chain[1] - chain[0], up);

                if (useFabrikIk) {
                    Vector3 plane = planeStabilization ? preferPlane : Vector3();
                    animation::solveFabrikIk(chain, footTarget[i], 15, plane);
                } else {
                    animation::solveCcdIk(chain, footTarget[i], 15);
                }

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

                boneWorldTransforms[legs[i].coxaName] = animation::buildBoneWorldTransform(chain[0], chain[1]);
                boneWorldTransforms[legs[i].femurName] = animation::buildBoneWorldTransform(chain[1], femurEnd);
                boneWorldTransforms[legs[i].tibiaName] = animation::buildBoneWorldTransform(femurEnd, chain[2]);
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
