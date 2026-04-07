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

// Procedural run animation for four-legged quadruped rig.
//
// Compared to the walk, a run features:
//   - Diagonal pair gait (trot/canter): diagonal legs move in phase
//   - An aerial/suspension phase where all feet leave the ground
//   - Greater forward lean of the torso
//   - Longer strides and higher foot lifts
//   - More pronounced body pitch oscillation
//
// Adjustable animation parameters:
//   - stepLengthFactor:       stride extent scaled by body length
//   - stepHeightFactor:       foot lift height during swing phase
//   - bodyBobFactor:          vertical oscillation amplitude of the torso
//   - gaitSpeedFactor:        number of gait cycles per clip (rounded to integer)
//   - spineFlexFactor:        lateral spine undulation amplitude
//   - tailSwayFactor:         tail lateral sway amplitude
//   - suspensionFactor:       aerial phase height (all feet off ground)
//   - forwardLeanFactor:      forward pitch of the torso during run
//   - strideFrequencyFactor:  controls stride tempo (higher = more rapid turnover)
//   - boundFactor:            0=trot (diagonal pairs), 1=bound (front/back pairs)

#include <array>
#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/common.h>
#include <dust3d/animation/quadruped/run.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace quadruped {

    struct RunLegDef {
        const char* upperLegName;
        const char* lowerLegName;
        const char* footName;
    };

    struct RunLegDefWithPhase : public RunLegDef {
        double phaseOffset;
        constexpr RunLegDefWithPhase(RunLegDef base, double phase)
            : RunLegDef(base)
            , phaseOffset(phase)
        {
        }
    };

    struct RunLegRest {
        Vector3 upperLegPos, upperLegEnd;
        Vector3 lowerLegEnd;
        Vector3 footEnd;
        Vector3 restStickDir;
        Vector3 restUpperToLowerVec;
    };

    bool run(const RigStructure& rigStructure,
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
        // 2. Define four legs with run-cycle phase offsets
        // ===================================================================
        // Phase offsets are interpolated between:
        //   Trot (boundFactor=0): diagonal pairs (FL+BR=0, FR+BL=0.5)
        //   Bound (boundFactor=1): front/back pairs (FL+FR=0, BL+BR=0.5)
        // Animals like rabbits/cheetahs use a bound; dogs/horses often trot.
        double boundFactor = parameters.getValue("boundFactor", 0.0);
        boundFactor = std::max(0.0, std::min(1.0, boundFactor));

        // Trot phases:  FL=0, BR=0,   FR=0.5, BL=0.5
        // Bound phases: FL=0, FR=0,   BL=0.5, BR=0.5
        //   BR: trot=0.0, bound=0.5  => lerp 0.0 + boundFactor * 0.5
        //   FR: trot=0.5, bound=0.0  => lerp 0.5 - boundFactor * 0.5
        //   BL: trot=0.5, bound=0.5  => stays 0.5
        const std::array<RunLegDefWithPhase, 4> legs = { {
            { { "FrontLeftUpperLeg", "FrontLeftLowerLeg", "FrontLeftFoot" }, 0.0 },
            { { "BackRightUpperLeg", "BackRightLowerLeg", "BackRightFoot" }, 0.0 + boundFactor * 0.5 },
            { { "FrontRightUpperLeg", "FrontRightLowerLeg", "FrontRightFoot" }, 0.5 - boundFactor * 0.5 },
            { { "BackLeftUpperLeg", "BackLeftLowerLeg", "BackLeftFoot" }, 0.5 },
        } };

        // Gather rest-pose data
        std::array<RunLegRest, 4> legRest;
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
        // 3. Running parameters from body geometry
        // ===================================================================
        double stepLengthFactor = parameters.getValue("stepLengthFactor", 1.0);
        double stepHeightFactor = parameters.getValue("stepHeightFactor", 1.0);
        double bodyBobFactor = parameters.getValue("bodyBobFactor", 1.0);
        double spineFlexFactor = parameters.getValue("spineFlexFactor", 1.0);
        double tailSwayFactor = parameters.getValue("tailSwayFactor", 1.0);
        double suspensionFactor = parameters.getValue("suspensionFactor", 1.0);
        double forwardLeanFactor = parameters.getValue("forwardLeanFactor", 1.0);
        double strideFrequencyFactor = parameters.getValue("strideFrequencyFactor", 1.0);

        double bodyLength = bodyVector.length();
        // Run has longer strides than walk
        double stepLength = bodyLength * 0.40 * stepLengthFactor;
        // Higher foot lift during run
        double stepHeight = bodyLength * 0.18 * stepHeightFactor;
        // More pronounced body bob
        double bodyBobAmp = bodyLength * 0.025 * bodyBobFactor;
        double spineFlexAmp = 0.05 * spineFlexFactor;
        double tailSwayAmp = 0.20 * tailSwayFactor;
        // Suspension: whole body lifts during aerial phase
        double suspensionAmp = bodyLength * 0.04 * suspensionFactor;
        // Forward lean angle in radians
        double forwardLeanAngle = 0.06 * forwardLeanFactor;

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

        // Swing duty factor: in a run/trot each leg swings for ~40% of cycle
        // (shorter stance phase than walk)
        const double swingDuty = 0.40 * strideFrequencyFactor;
        const double clampedSwingDuty = std::min(swingDuty, 0.50);

        for (int frame = 0; frame < frameCount; ++frame) {
            double tNormalized = static_cast<double>(frame) / static_cast<double>(frameCount);
            double t = fmod(tNormalized * cycles, 1.0);

            // -- body oscillation --
            // Trot: body bobs twice per cycle (4π), each diagonal pair causes one bob
            // Bound: body bobs once per cycle (2π), front pair then back pair
            double trotBob = std::sin(t * 4.0 * Math::Pi);
            double boundBob = std::sin(t * 2.0 * Math::Pi);
            double bodyBob = bodyBobAmp * (trotBob * (1.0 - boundFactor) + boundBob * boundFactor);

            // Suspension phase: trot has two peaks, bound has two distinct aerial phases
            // In a bound/gallop there's a "gathered" suspension (legs tucked under)
            // and an "extended" suspension (legs stretched out)
            double trotSuspension = std::max(0.0, std::sin(t * 4.0 * Math::Pi));
            // Bound suspension is stronger and follows the single-cycle rhythm
            double boundSuspension = std::max(0.0, std::sin(t * 2.0 * Math::Pi - 0.5 * Math::Pi));
            double suspensionScale = 1.0 + boundFactor * 0.8; // bound has more airtime
            double suspension = suspensionAmp * suspensionScale
                * (trotSuspension * (1.0 - boundFactor) + boundSuspension * boundFactor);

            // Body pitch: trot is subtle; bound has dramatic nose-down on landing,
            // nose-up on launch — the whole body rocks with the gait
            double trotPitch = 0.025 * bodyBobFactor * std::sin(t * 4.0 * Math::Pi);
            double boundPitch = 0.06 * bodyBobFactor * std::sin(t * 2.0 * Math::Pi);
            double bodyPitch = trotPitch * (1.0 - boundFactor) + boundPitch * boundFactor;

            // Lateral sway: present in trot, vanishes in bound (symmetric gait)
            double bodySway = bodyBobAmp * 0.3 * std::sin(t * 2.0 * Math::Pi) * (1.0 - boundFactor);

            Matrix4x4 bodyTransform;
            bodyTransform.translate(up * (bodyBob + suspension) + right * bodySway);
            // Forward lean: constant tilt plus dynamic pitch
            bodyTransform.rotate(right, forwardLeanAngle + bodyPitch);

            // Spine flex:
            // Trot: mostly lateral undulation
            // Bound: dramatic sagittal flex — spine bunches when back legs drive,
            //   extends when front legs reach. This is the signature cheetah motion.
            double spineBend = spineFlexAmp * std::sin(t * 2.0 * Math::Pi) * (1.0 - boundFactor * 0.7);
            double trotSpineExtend = spineFlexAmp * 0.5 * std::sin(t * 4.0 * Math::Pi);
            double boundSpineExtend = spineFlexAmp * 3.0 * std::sin(t * 2.0 * Math::Pi);
            double spineExtend = trotSpineExtend * (1.0 - boundFactor) + boundSpineExtend * boundFactor;

            // -------------------------------------------------------
            // 4a. Compute foot target for each leg
            // -------------------------------------------------------
            std::array<Vector3, 4> footTarget;

            for (size_t i = 0; i < 4; ++i) {
                double legT = fmod(t + legs[i].phaseOffset, 1.0);

                Vector3 footFront = footHome[i] + forward * stepLength;
                Vector3 footBack = footHome[i] - forward * stepLength;

                if (legT < clampedSwingDuty) {
                    // Swing phase: arc from back to front
                    double legPhase = legT / clampedSwingDuty;
                    double smoothSwing = smootherstep(legPhase);
                    Vector3 groundPos = footBack + (footFront - footBack) * smoothSwing;
                    double lift = stepHeight * std::sin(smoothSwing * Math::Pi);
                    footTarget[i] = groundPos + up * lift;
                } else {
                    // Stance phase: foot on ground, sliding front to back
                    double legPhase = (legT - clampedSwingDuty) / (1.0 - clampedSwingDuty);
                    footTarget[i] = footFront + (footBack - footFront) * smoothstep(legPhase);
                }
            }

            // -------------------------------------------------------
            // 4b. Body and spine bone transforms
            // -------------------------------------------------------
            std::map<std::string, Matrix4x4> boneWorldTransforms;

            auto computeBodyBone = [&](const std::string& name, double flexAngle = 0.0, double sagittalAngle = 0.0) {
                Vector3 pos = bonePos(name);
                Vector3 end = boneEnd(name);
                Vector3 newPos = bodyTransform.transformPoint(pos);
                Vector3 newEnd = bodyTransform.transformPoint(end);
                if (std::abs(flexAngle) > 1e-6) {
                    Matrix4x4 flexMat;
                    flexMat.rotate(up, flexAngle);
                    Vector3 offset = newEnd - newPos;
                    newEnd = newPos + flexMat.transformVector(offset);
                }
                if (std::abs(sagittalAngle) > 1e-6) {
                    Matrix4x4 sagMat;
                    sagMat.rotate(right, sagittalAngle);
                    Vector3 offset = newEnd - newPos;
                    newEnd = newPos + sagMat.transformVector(offset);
                }
                boneWorldTransforms[name] = buildBoneWorldTransform(newPos, newEnd);
            };

            computeBodyBone("Root");
            // In bound mode, pelvis and chest rotate more in opposition
            // creating the characteristic cheetah spine coil/uncoil
            double pelvisSagittal = -spineExtend * (0.5 + boundFactor * 0.5);
            double chestSagittal = spineExtend * (0.5 + boundFactor * 0.5);
            computeBodyBone("Pelvis", -spineBend * 0.3, pelvisSagittal);
            computeBodyBone("Spine", spineBend, spineExtend);
            computeBodyBone("Chest", spineBend * 0.3, chestSagittal);
            computeBodyBone("Neck");
            computeBodyBone("Head");

            if (boneIdx.count("Jaw"))
                computeBodyBone("Jaw");

            // Tail: sway with a traveling wave, more vigorous during run
            double tailPhase = t * 2.0 * Math::Pi;
            static const char* tailBones[] = { "TailBase", "TailMid", "TailTip" };
            for (int ti = 0; ti < 3; ++ti) {
                if (boneIdx.count(tailBones[ti]) == 0)
                    continue;
                double tailAngle = tailSwayAmp * std::sin(tailPhase - ti * 0.8);
                // Tail also bounces vertically during run
                double tailBounce = spineExtend * 0.3 * (ti + 1);
                computeBodyBone(tailBones[ti], tailAngle, tailBounce);
            }

            // -------------------------------------------------------
            // 4c. Leg IK
            // -------------------------------------------------------
            for (size_t i = 0; i < 4; ++i) {
                Vector3 hipPos = bodyTransform.transformPoint(legRest[i].upperLegPos);
                Vector3 upperLegEnd = bodyTransform.transformPoint(legRest[i].upperLegEnd);
                Vector3 footEnd = bodyTransform.transformPoint(legRest[i].footEnd);

                std::vector<Vector3> chain = { hipPos, upperLegEnd, footEnd };

                // Use rest-pose knee position shifted forward/backward to control bend.
                // The rest-pose knee already has the correct lateral placement.
                bool isFrontLeg = (i == 0 || i == 2);
                Vector3 poleVector = upperLegEnd + forward * (isFrontLeg ? -1.0 : 1.0);
                solveTwoBoneIk(chain, footTarget[i], poleVector);

                // Preserve rest-pose lateral position of the knee so it doesn't
                // collapse inward. Project the drift along the right axis and
                // snap it back to the rest-pose value.
                double restLateral = Vector3::dotProduct(upperLegEnd, right);
                double solvedLateral = Vector3::dotProduct(chain[1], right);
                chain[1] = chain[1] + right * (restLateral - solvedLateral);

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
