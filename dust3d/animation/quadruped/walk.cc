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

// Procedural walk animation for four-legged quadruped rig.
//
// Supports all quadruped creatures (horse, dog, cat, lizard, etc.) through
// adjustable animation parameters:
//   - stepLengthFactor:       stride extent scaled by body length
//   - stepHeightFactor:       foot lift height during swing phase
//   - bodyBobFactor:          vertical oscillation amplitude of the torso
//   - gaitSpeedFactor:        number of gait cycles per clip (rounded to integer)
//   - spineFlexFactor:        lateral spine undulation amplitude
//   - tailSwayFactor:         tail lateral sway amplitude
//   - useFabrikIk:            use FABRIK IK solver (vs CCD)
//   - planeStabilization:     constrain legs to their rest-pose plane
//
// The default gait is a lateral walk (same-side legs move nearly together with
// a phase offset).  Adjusting parameters allows approximating:
//   - Walk (default): deliberate, all four feet contact ground at some point
//   - Trot: diagonal pairs move together (increase gaitSpeedFactor)
//   - Various body proportions via step length/height scaling

#include <array>
#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/common.h>
#include <dust3d/animation/quadruped/walk.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace quadruped {

    // Describes a single quadruped leg chain: upper leg, lower leg, foot
    struct LegDef {
        const char* upperLegName;
        const char* lowerLegName;
        const char* footName;
    };

    struct LegDefWithPhase : public LegDef {
        double phaseOffset; // 0.0–1.0 offset within the gait cycle
        constexpr LegDefWithPhase(LegDef base, double phase)
            : LegDef(base)
            , phaseOffset(phase)
        {
        }
    };

    // Cached rest-pose geometry for a leg
    struct LegRest {
        Vector3 upperLegPos, upperLegEnd;
        Vector3 lowerLegEnd;
        Vector3 footEnd; // end-effector (hoof/paw tip)
        Vector3 restStickDir; // normalized(footEnd - upperLegEnd)
        Vector3 restUpperToLowerVec; // (lowerLegEnd - upperLegEnd) un-normalized
    };

    bool walk(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = static_cast<int>(parameters.getValue("frameCount", 30.0));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 1.0));

        // ----- bone lookup -----
        auto boneIdx = buildBoneIndexMap(rigStructure);

        auto bonePos = [&](const std::string& name) -> Vector3 {
            return getBonePos(rigStructure, boneIdx, name);
        };
        auto boneEnd = [&](const std::string& name) -> Vector3 {
            return getBoneEnd(rigStructure, boneIdx, name);
        };

        // ----- verify required bones -----
        static const char* requiredBones[] = {
            "Root", "Pelvis", "Spine", "Chest", "Neck", "Head",
            "FrontLeftUpperLeg", "FrontLeftLowerLeg", "FrontLeftFoot",
            "FrontRightUpperLeg", "FrontRightLowerLeg", "FrontRightFoot",
            "BackLeftUpperLeg", "BackLeftLowerLeg", "BackLeftFoot",
            "BackRightUpperLeg", "BackRightLowerLeg", "BackRightFoot"
        };
        if (!validateRequiredBones(boneIdx, requiredBones, sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;

        // ===================================================================
        // 1. Body orientation from spine
        // ===================================================================
        Vector3 bodyVector = boneEnd("Chest") - bonePos("Pelvis");
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
        // 2. Define four legs with walk-cycle phase offsets
        // ===================================================================
        // Lateral sequence walk: FrontLeft(0), BackRight(0.25),
        //   FrontRight(0.5), BackLeft(0.75)
        // This is the most common natural quadruped walk pattern.
        static const std::array<LegDefWithPhase, 4> legs = { {
            { { "FrontLeftUpperLeg", "FrontLeftLowerLeg", "FrontLeftFoot" }, 0.0 },
            { { "BackRightUpperLeg", "BackRightLowerLeg", "BackRightFoot" }, 0.25 },
            { { "FrontRightUpperLeg", "FrontRightLowerLeg", "FrontRightFoot" }, 0.5 },
            { { "BackLeftUpperLeg", "BackLeftLowerLeg", "BackLeftFoot" }, 0.75 },
        } };

        // Gather rest-pose data
        std::array<LegRest, 4> legRest;
        for (size_t i = 0; i < 4; ++i) {
            legRest[i].upperLegPos = bonePos(legs[i].upperLegName);
            legRest[i].upperLegEnd = boneEnd(legs[i].upperLegName);
            legRest[i].lowerLegEnd = boneEnd(legs[i].lowerLegName);
            legRest[i].footEnd = boneEnd(legs[i].footName);
            Vector3 chordVec = legRest[i].footEnd - legRest[i].upperLegEnd;
            legRest[i].restStickDir = chordVec.isZero() ? Vector3(0, -1, 0) : chordVec.normalized();
            legRest[i].restUpperToLowerVec = legRest[i].lowerLegEnd - legRest[i].upperLegEnd;
        }

        // ===================================================================
        // 3. Walking parameters from body geometry
        // ===================================================================
        double stepLengthFactor = parameters.getValue("stepLengthFactor", 1.0);
        double stepHeightFactor = parameters.getValue("stepHeightFactor", 1.0);
        double bodyBobFactor = parameters.getValue("bodyBobFactor", 1.0);
        double spineFlexFactor = parameters.getValue("spineFlexFactor", 1.0);
        double tailSwayFactor = parameters.getValue("tailSwayFactor", 1.0);

        double bodyLength = bodyVector.length();
        double stepLength = bodyLength * 0.25 * stepLengthFactor;
        double stepHeight = bodyLength * 0.10 * stepHeightFactor;
        double bodyBobAmp = bodyLength * 0.015 * bodyBobFactor;
        double spineFlexAmp = 0.03 * spineFlexFactor;
        double tailSwayAmp = 0.15 * tailSwayFactor;

        // Ground level: lowest foot tip projected onto up axis
        double minUpProj = Vector3::dotProduct(legRest[0].footEnd, up);
        for (size_t i = 1; i < 4; ++i) {
            double proj = Vector3::dotProduct(legRest[i].footEnd, up);
            if (proj < minUpProj)
                minUpProj = proj;
        }

        // Foot home positions at ground level
        std::array<Vector3, 4> footHome;
        for (size_t i = 0; i < 4; ++i) {
            double proj = Vector3::dotProduct(legRest[i].footEnd, up);
            footHome[i] = legRest[i].footEnd - up * (proj - minUpProj);
        }

        // ===================================================================
        // 4. Generate animation frames
        // ===================================================================
        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        double gaitSpeedFactor = parameters.getValue("gaitSpeedFactor", 1.0);
        const double cycles = std::max(1.0, std::round(gaitSpeedFactor));

        // Swing duty factor: fraction of the cycle each leg is in swing phase
        // For a walk, each leg swings for ~25% of the cycle
        const double swingDuty = 0.25;

        bool useFabrikIk = parameters.getBool("useFabrikIk", true);
        bool planeStabilization = parameters.getBool("planeStabilization", true);

        for (int frame = 0; frame < frameCount; ++frame) {
            double tNormalized = static_cast<double>(frame) / static_cast<double>(frameCount);
            double t = fmod(tNormalized * cycles, 1.0);

            // -- body oscillation --
            // In a walk, the body bobs twice per cycle (once per diagonal pair)
            double bodyBob = bodyBobAmp * std::sin(t * 4.0 * Math::Pi);
            double bodyPitch = 0.015 * bodyBobFactor * std::sin(t * 4.0 * Math::Pi);
            // Lateral sway: body shifts toward the supporting side
            double bodySway = bodyBobAmp * 0.5 * std::sin(t * 2.0 * Math::Pi);

            Matrix4x4 bodyTransform;
            bodyTransform.translate(up * bodyBob + right * bodySway);
            bodyTransform.rotate(right, bodyPitch);

            // Spine lateral flex
            double spineBend = spineFlexAmp * std::sin(t * 2.0 * Math::Pi);

            // -------------------------------------------------------
            // 4a. Compute foot target for each leg
            // -------------------------------------------------------
            std::array<Vector3, 4> footTarget;

            for (size_t i = 0; i < 4; ++i) {
                // Each leg's phase within the cycle
                double legT = fmod(t + legs[i].phaseOffset, 1.0);

                Vector3 footFront = footHome[i] + forward * stepLength;
                Vector3 footBack = footHome[i] - forward * stepLength;

                if (legT < swingDuty) {
                    // Swing phase: arc from back to front
                    double legPhase = legT / swingDuty;
                    double smoothSwing = smootherstep(legPhase);
                    Vector3 groundPos = footBack + (footFront - footBack) * smoothSwing;
                    double lift = stepHeight * std::sin(smoothSwing * Math::Pi);
                    footTarget[i] = groundPos + up * lift;
                } else {
                    // Stance phase: foot on ground, sliding front to back
                    double legPhase = (legT - swingDuty) / (1.0 - swingDuty);
                    footTarget[i] = footFront + (footBack - footFront) * smoothstep(legPhase);
                }
            }

            // -------------------------------------------------------
            // 4b. Body and spine bone transforms
            // -------------------------------------------------------
            std::map<std::string, Matrix4x4> boneWorldTransforms;

            auto computeBodyBone = [&](const std::string& name, double flexAngle = 0.0) {
                Vector3 pos = bonePos(name);
                Vector3 end = boneEnd(name);
                Vector3 newPos = bodyTransform.transformPoint(pos);
                Vector3 newEnd = bodyTransform.transformPoint(end);
                if (std::abs(flexAngle) > 1e-6) {
                    // Apply lateral flex around the up axis
                    Matrix4x4 flexMat;
                    flexMat.rotate(up, flexAngle);
                    Vector3 offset = newEnd - newPos;
                    newEnd = newPos + flexMat.transformVector(offset);
                }
                boneWorldTransforms[name] = buildBoneWorldTransform(newPos, newEnd);
            };

            computeBodyBone("Root");
            computeBodyBone("Pelvis", -spineBend * 0.3);
            computeBodyBone("Spine", spineBend);
            computeBodyBone("Chest", spineBend * 0.3);
            computeBodyBone("Neck");
            computeBodyBone("Head");

            // Optional bones
            if (boneIdx.count("Jaw"))
                computeBodyBone("Jaw");

            // Tail: sway with a traveling wave
            double tailPhase = t * 2.0 * Math::Pi;
            static const char* tailBones[] = { "TailBase", "TailMid", "TailTip" };
            for (int ti = 0; ti < 3; ++ti) {
                if (boneIdx.count(tailBones[ti]) == 0)
                    continue;
                double tailAngle = tailSwayAmp * std::sin(tailPhase - ti * 0.8);
                computeBodyBone(tailBones[ti], tailAngle);
            }

            // -------------------------------------------------------
            // 4c. Leg IK
            // -------------------------------------------------------
            for (size_t i = 0; i < 4; ++i) {
                Vector3 hipPos = bodyTransform.transformPoint(legRest[i].upperLegPos);
                Vector3 upperLegEnd = bodyTransform.transformPoint(legRest[i].upperLegEnd);
                Vector3 footEnd = bodyTransform.transformPoint(legRest[i].footEnd);

                // 3-joint chain: [hip, upper-leg-end, foot-tip]
                // treating lower-leg+foot as one rigid segment for IK
                std::vector<Vector3> chain = { hipPos, upperLegEnd, footEnd };

                Vector3 preferPlane = Vector3::crossProduct(chain[1] - chain[0], chain[2] - chain[1]);
                if (preferPlane.lengthSquared() < 1e-10)
                    preferPlane = Vector3::crossProduct(chain[1] - chain[0], up);

                if (useFabrikIk) {
                    Vector3 plane = planeStabilization ? preferPlane : Vector3();
                    solveFabrikIk(chain, footTarget[i], 15, plane);
                } else {
                    solveCcdIk(chain, footTarget[i], 15);
                }

                // Stabilize knee: project it onto the leg's sagittal plane so
                // the upper leg swings forward/backward naturally but doesn't
                // drift laterally.  The plane is defined by the hip position
                // and the rest-pose leg direction, with the "right" vector as
                // the plane normal (lateral axis).
                Vector3 legPlaneNormal = right; // lateral axis
                Vector3 kneeOffset = chain[1] - hipPos;
                double lateralDrift = Vector3::dotProduct(kneeOffset, legPlaneNormal)
                    - Vector3::dotProduct(upperLegEnd - hipPos, legPlaneNormal);
                chain[1] = chain[1] - legPlaneNormal * lateralDrift;

                // Re-enforce segment lengths after lateral correction
                double len0 = (legRest[i].upperLegEnd - legRest[i].upperLegPos).length();
                double len1 = (legRest[i].footEnd - legRest[i].upperLegEnd).length();
                Vector3 dir0 = chain[1] - chain[0];
                if (!dir0.isZero())
                    chain[1] = chain[0] + dir0.normalized() * len0;
                Vector3 dir1 = chain[2] - chain[1];
                if (!dir1.isZero())
                    chain[2] = chain[1] + dir1.normalized() * len1;

                // Reconstruct lower-leg position from IK result
                Vector3 newStickDir = (chain[2] - chain[1]);
                if (newStickDir.isZero())
                    newStickDir = legRest[i].restStickDir;
                else
                    newStickDir.normalize();
                Quaternion stickRot = Quaternion::rotationTo(legRest[i].restStickDir, newStickDir);
                Matrix4x4 stickRotMat;
                stickRotMat.rotate(stickRot);
                Vector3 lowerLegEnd = chain[1] + stickRotMat.transformVector(legRest[i].restUpperToLowerVec);

                boneWorldTransforms[legs[i].upperLegName] = buildBoneWorldTransform(chain[0], chain[1]);
                boneWorldTransforms[legs[i].lowerLegName] = buildBoneWorldTransform(chain[1], lowerLegEnd);
                boneWorldTransforms[legs[i].footName] = buildBoneWorldTransform(lowerLegEnd, chain[2]);
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

} // namespace quadruped

} // namespace dust3d
