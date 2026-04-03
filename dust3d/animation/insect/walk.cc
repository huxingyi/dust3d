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

// Procedural walk animation for six-legged fly/insect rig.

#include <array>
#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/insect/common.h>
#include <dust3d/animation/insect/walk.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace insect {

    namespace {

    } // anonymous namespace

    // ---------------------------------------------------------------------------
    // Public entry point
    // ---------------------------------------------------------------------------
    bool walk(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        int frameCount,
        float durationSeconds,
        const AnimationParams& parameters)
    {
        // ----- bone lookup helpers -----
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

        // ----- verify required bones -----
        static const char* requiredBones[] = {
            "Head", "Thorax", "Abdomen",
            "FrontLeftCoxa", "FrontLeftFemur", "FrontLeftTibia",
            "FrontRightCoxa", "FrontRightFemur", "FrontRightTibia",
            "MiddleLeftCoxa", "MiddleLeftFemur", "MiddleLeftTibia",
            "MiddleRightCoxa", "MiddleRightFemur", "MiddleRightTibia",
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
        // Body vector: direction from abdomen tail toward the head.
        Vector3 bodyVector = getBonePos("Thorax") - getBoneEnd("Abdomen");

        if (bodyVector.isZero())
            return false; // degenerate rig

        // Build an orthonormal frame (forward / up / right) that adapts to
        // the fly's orientation.  If the body is mostly horizontal the fly is
        // "facing front" and walks on the XZ plane; if it points steeply
        // downward (or upward) the walk plane changes accordingly.
        Vector3 forward = bodyVector.normalized();

        Vector3 worldUp(0.0, 1.0, 0.0);
        Vector3 right = Vector3::crossProduct(forward, worldUp);
        if (right.lengthSquared() < 1e-8) {
            // forward is nearly parallel to world-Y – fall back to world-Z
            right = Vector3::crossProduct(forward, Vector3(0.0, 0.0, 1.0));
        }
        right.normalize();
        Vector3 up = Vector3::crossProduct(right, forward).normalized();

        // ===================================================================
        // 2. Define six legs with tripod gait groups
        // ===================================================================
        // Tripod gait – the standard insect locomotion pattern:
        //   Group A (swing first half of cycle): FrontLeft, MiddleRight, BackLeft
        //   Group B (swing second half of cycle): FrontRight, MiddleLeft, BackRight
        static const std::array<LegDefWithGait, 6> legs = { {
            { { "FrontLeftCoxa", "FrontLeftFemur", "FrontLeftTibia" }, 0 },
            { { "FrontRightCoxa", "FrontRightFemur", "FrontRightTibia" }, 1 },
            { { "MiddleLeftCoxa", "MiddleLeftFemur", "MiddleLeftTibia" }, 1 },
            { { "MiddleRightCoxa", "MiddleRightFemur", "MiddleRightTibia" }, 0 },
            { { "BackLeftCoxa", "BackLeftFemur", "BackLeftTibia" }, 0 },
            { { "BackRightCoxa", "BackRightFemur", "BackRightTibia" }, 1 },
        } };

        // Gather rest-pose joint positions for every leg
        struct LegRest {
            Vector3 coxaPos, coxaEnd;
            Vector3 femurEnd;
            Vector3 tibiaEnd; // end-effector (foot tip)
            // For rigid femur+tibia reconstruction: rest-pose chord direction (coxaEnd→tibiaEnd)
            // and the femur offset vector in that same space.  Applying the rotation that maps
            // restStickDir → newStickDir to restCoxaToFemurVec gives the new femurEnd offset,
            // preserving both bone lengths and the knee bend angle exactly.
            Vector3 restStickDir; // normalized(tibiaEnd - coxaEnd) in rest pose
            Vector3 restCoxaToFemurVec; // (femurEnd - coxaEnd) in rest pose (not normalized)
        };
        std::array<LegRest, 6> legRest;
        for (size_t i = 0; i < 6; ++i) {
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

        double bodyLength = bodyVector.length();
        double stepLength = bodyLength * 0.20 * stepLengthFactor; // half-stride extent per leg
        double stepHeight = bodyLength * 0.08 * stepHeightFactor; // foot lift during swing
        double bodyBobAmp = bodyLength * 0.02 * bodyBobFactor; // vertical oscillation amplitude

        // Ground level: project all rest foot tips onto the "up" axis and take
        // the minimum projection as the ground reference.
        double minUpProj = Vector3::dotProduct(legRest[0].tibiaEnd, up);
        for (size_t i = 1; i < 6; ++i) {
            double proj = Vector3::dotProduct(legRest[i].tibiaEnd, up);
            if (proj < minUpProj)
                minUpProj = proj;
        }

        // Foot home positions: rest tibia tips projected down to the ground level
        std::array<Vector3, 6> footHome;
        for (size_t i = 0; i < 6; ++i) {
            double proj = Vector3::dotProduct(legRest[i].tibiaEnd, up);
            footHome[i] = legRest[i].tibiaEnd - up * (proj - minUpProj);
        }

        // ===================================================================
        // 4. Generate animation frames
        // ===================================================================
        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        // Round gaitSpeedFactor to the nearest positive integer so the clip always
        // contains a whole number of gait cycles.  A fractional value would leave
        // the last frame mid-cycle, causing a visible jump when the clip loops.
        double gaitSpeedFactor = parameters.getValue("gaitSpeedFactor", 1.0);
        const double cycles = std::max(1.0, std::round(gaitSpeedFactor));

        for (int frame = 0; frame < frameCount; ++frame) {
            double tNormalized = static_cast<double>(frame) / static_cast<double>(frameCount);
            double t = fmod(tNormalized * cycles, 1.0);

            // -- body oscillation (pelvis movement) --
            // Two bobs per cycle (one per tripod group switch)
            double bodyBob = bodyBobAmp * std::sin(t * 4.0 * Math::Pi);
            // Slight pitch oscillation around the "right" axis
            double bodyPitch = 0.02 * bodyBobFactor * std::sin(t * 2.0 * Math::Pi);

            Matrix4x4 bodyTransform;
            bodyTransform.translate(up * bodyBob);
            bodyTransform.rotate(right, bodyPitch);

            // -------------------------------------------------------
            // 4a. Compute foot target for each leg
            // -------------------------------------------------------
            std::array<Vector3, 6> footTarget;

            for (size_t i = 0; i < 6; ++i) {
                int group = legs[i].gaitGroup;
                bool isSwing;
                double legPhase; // 0→1 within the current sub-phase

                // Group 0: swing in [0, 0.5), stance in [0.5, 1.0)
                // Group 1: stance in [0, 0.5), swing in [0.5, 1.0)
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

                // Stride endpoints relative to the foot home position
                Vector3 footFront = footHome[i] + forward * stepLength;
                Vector3 footBack = footHome[i] - forward * stepLength;

                if (isSwing) {
                    // Swing phase: arc from back → front.
                    // smootherstep on horizontal travel: eases out of liftoff and
                    // into touchdown so the foot doesn't snap forward at a constant speed.
                    double smoothSwing = smootherstep(legPhase);
                    Vector3 groundPos = footBack + (footFront - footBack) * smoothSwing;
                    // Lift: sin curve gives zero velocity at liftoff and touchdown,
                    // which looks more natural than the bare quadratic.  Phase is
                    // smoothstepped first so the peak stays centred.
                    double lift = stepHeight * std::sin(smoothSwing * Math::Pi);
                    footTarget[i] = groundPos + up * lift;
                } else {
                    // Stance phase: foot planted on the ground, sliding front → back.
                    // smoothstep removes the abrupt velocity change at the moment the
                    // foot is set down or lifted off.
                    footTarget[i] = footFront + (footBack - footFront) * smoothstep(legPhase);
                }
            }

            // -------------------------------------------------------
            // 4b. Body bone transforms
            // -------------------------------------------------------
            std::map<std::string, Matrix4x4> boneWorldTransforms;

            auto computeBodyBone = [&](const std::string& name) {
                Vector3 pos = getBonePos(name);
                Vector3 end = getBoneEnd(name);
                Vector3 newPos = bodyTransform.transformPoint(pos);
                Vector3 newEnd = bodyTransform.transformPoint(end);
                boneWorldTransforms[name] = insect::buildBoneWorldTransform(newPos, newEnd);
            };

            computeBodyBone("Head");
            computeBodyBone("Thorax");
            computeBodyBone("Abdomen");

            // Wings follow the thorax
            for (const char* wingName : { "LeftWing", "RightWing" }) {
                if (boneIdx.count(wingName) == 0)
                    continue;
                Vector3 pos = getBonePos(wingName);
                Vector3 end = getBoneEnd(wingName);
                Vector3 newPos = bodyTransform.transformPoint(pos);
                Vector3 newEnd = bodyTransform.transformPoint(end);
                boneWorldTransforms[wingName] = insect::buildBoneWorldTransform(newPos, newEnd);
            }

            // -------------------------------------------------------
            // 4c. Leg IK
            //     For each leg: build a 3-joint chain
            //     [coxaPos, coxaEnd, tibiaEnd]
            //     treating femur+tibia as one rigid segment for IK purposes.
            //     The femur-tibia junction is reconstructed afterward by rotating
            //     the rest-pose structure, preserving the original knee bend angle.
            // -------------------------------------------------------
            for (size_t i = 0; i < 6; ++i) {
                Vector3 hipPos = bodyTransform.transformPoint(legRest[i].coxaPos);
                Vector3 coxaEnd = bodyTransform.transformPoint(legRest[i].coxaEnd);
                Vector3 tibiaEnd = bodyTransform.transformPoint(legRest[i].tibiaEnd);

                // Treat femur+tibia as a single rigid segment so no angle opens between
                // them during IK.  The 3-joint chain is [coxa-root, coxa-end, foot-tip].
                std::vector<Vector3> chain = { hipPos, coxaEnd, tibiaEnd };

                Vector3 preferPlane = Vector3::crossProduct(chain[1] - chain[0], chain[2] - chain[1]);
                if (preferPlane.lengthSquared() < 1e-10)
                    preferPlane = Vector3::crossProduct(chain[1] - chain[0], up);
                bool useFabrikIk = parameters.getBool("useFabrikIk", true);
                bool planeStabilization = parameters.getBool("planeStabilization", true);
                if (useFabrikIk) {
                    Vector3 plane = planeStabilization ? preferPlane : Vector3();
                    insect::solveFabrikIk(chain, footTarget[i], 15, plane);
                } else {
                    insect::solveCcdIk(chain, footTarget[i], 15);
                }

                // Reconstruct the femur-tibia junction by rotating the rest-pose femur
                // offset vector (coxaEnd→femurEnd) by the same rotation that maps the
                // rest-pose chord direction to the current stick direction.  This keeps
                // the knee bend angle identical to the rest pose.
                Vector3 newStickDir = (chain[2] - chain[1]);
                if (newStickDir.isZero())
                    newStickDir = legRest[i].restStickDir;
                else
                    newStickDir.normalize();
                Quaternion stickRot = Quaternion::rotationTo(legRest[i].restStickDir, newStickDir);
                Matrix4x4 stickRotMat;
                stickRotMat.rotate(stickRot);
                Vector3 femurEnd = chain[1] + stickRotMat.transformVector(legRest[i].restCoxaToFemurVec);

                // Build per-bone transforms from IK solution
                boneWorldTransforms[legs[i].coxaName] = insect::buildBoneWorldTransform(chain[0], chain[1]);
                boneWorldTransforms[legs[i].femurName] = insect::buildBoneWorldTransform(chain[1], femurEnd);
                boneWorldTransforms[legs[i].tibiaName] = insect::buildBoneWorldTransform(femurEnd, chain[2]);
            }

            // -------------------------------------------------------
            // 4d. Skin matrices = worldTransform × inverseBindMatrix
            // -------------------------------------------------------
            auto& animFrame = animationClip.frames[frame];
            // Use the frame's actual position in the clip, not the wrapped gait
            // phase, so that frame times remain monotonically increasing.
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

} // namespace insect

} // namespace dust3d
