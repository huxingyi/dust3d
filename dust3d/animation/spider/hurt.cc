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

// Procedural hurt (hit reaction) animation for the spider rig.
//
// A one-shot clip for game use. On the impact the body is knocked back and
// away from the blow, sags on its legs and flinches its front up; the front
// legs and pedipalps come up to guard the face, the abdomen (a scorpion's
// tail) flicks, and the other legs scrabble, each lifting once and replanting
// where it stood, so no foot slides. After a short hit-stop the body settles
// back with one small overshoot. The clip starts and ends exactly in the rest
// pose. Works for spiders, crabs and scorpions.
//
// Adjustable animation parameters:
//   - recoilFactor:        how far the body is knocked back (and sideways)
//   - flinchFactor:        how far the front of the body pitches up and rolls
//   - hitDirection:        where the blow comes from (-1 = left, 0 = front, 1 = right)
//   - frontLegGuardFactor: how high the front legs and pedipalps come up
//   - legScrabbleFactor:   how high the other legs lift as they scrabble
//   - abdomenFlickFactor:  how far the abdomen (tail) flicks up
//   - recoverySpeed:       how quickly the body settles back (> 1 = sooner)

#include <algorithm>
#include <array>
#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/common.h>
#include <dust3d/animation/spider/hurt.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace spider {

    bool hurt(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = static_cast<int>(parameters.getValue("frameCount", 24));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 0.8));

        auto boneIdx = buildBoneIndexMap(rigStructure);

        auto bonePos = [&](const std::string& name) -> Vector3 {
            return getBonePos(rigStructure, boneIdx, name);
        };
        auto boneEnd = [&](const std::string& name) -> Vector3 {
            return getBoneEnd(rigStructure, boneIdx, name);
        };

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
        if (!validateRequiredBones(boneIdx, requiredBones, sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;

        double recoilFactor = parameters.getValue("recoilFactor", 1.0);
        double flinchFactor = parameters.getValue("flinchFactor", 1.0);
        double hitDirection = std::clamp(parameters.getValue("hitDirection", 0.0), -1.0, 1.0);
        double frontLegGuardFactor = parameters.getValue("frontLegGuardFactor", 1.0);
        double legScrabbleFactor = parameters.getValue("legScrabbleFactor", 1.0);
        double abdomenFlickFactor = parameters.getValue("abdomenFlickFactor", 1.0);
        double recoverySpeed = parameters.getValue("recoverySpeed", 1.0);

        Vector3 upDir(0.0, 1.0, 0.0);
        Vector3 cephPos = bonePos("Cephalothorax");
        Vector3 headEnd = boneEnd("Head");
        Vector3 forward(headEnd.x() - cephPos.x(), 0.0, headEnd.z() - cephPos.z());
        if (forward.lengthSquared() < 1e-8)
            forward = Vector3(0.0, 0.0, 1.0);
        forward.normalize();
        Vector3 right = Vector3::crossProduct(upDir, forward);
        if (right.lengthSquared() < 1e-8)
            right = Vector3(1.0, 0.0, 0.0);
        right.normalize();

        double bodySize = (headEnd - boneEnd("Abdomen")).length();
        if (bodySize < 1e-6)
            bodySize = 0.5;

        struct LegTriplet {
            const char* coxa;
            const char* femur;
            const char* tibia;
            bool front;
        };
        static const LegTriplet legs[] = {
            { "FrontLeftCoxa", "FrontLeftFemur", "FrontLeftTibia", true },
            { "FrontRightCoxa", "FrontRightFemur", "FrontRightTibia", true },
            { "MidFrontLeftCoxa", "MidFrontLeftFemur", "MidFrontLeftTibia", false },
            { "MidFrontRightCoxa", "MidFrontRightFemur", "MidFrontRightTibia", false },
            { "MidBackLeftCoxa", "MidBackLeftFemur", "MidBackLeftTibia", false },
            { "MidBackRightCoxa", "MidBackRightFemur", "MidBackRightTibia", false },
            { "BackLeftCoxa", "BackLeftFemur", "BackLeftTibia", false },
            { "BackRightCoxa", "BackRightFemur", "BackRightTibia", false },
        };
        static const size_t legCount = sizeof(legs) / sizeof(legs[0]);

        struct LegRest {
            Vector3 coxaPos, coxaEnd;
            Vector3 femurEnd;
            Vector3 tibiaEnd;
            Vector3 restStickDir;
            Vector3 restCoxaToFemurVec;
        };
        std::array<LegRest, 8> legRest;
        for (size_t i = 0; i < legCount; ++i) {
            legRest[i].coxaPos = bonePos(legs[i].coxa);
            legRest[i].coxaEnd = boneEnd(legs[i].coxa);
            legRest[i].femurEnd = boneEnd(legs[i].femur);
            legRest[i].tibiaEnd = boneEnd(legs[i].tibia);
            Vector3 chordVec = legRest[i].tibiaEnd - legRest[i].coxaEnd;
            legRest[i].restStickDir = chordVec.isZero() ? Vector3(1, 0, 0) : chordVec.normalized();
            legRest[i].restCoxaToFemurVec = legRest[i].femurEnd - legRest[i].coxaEnd;
        }

        double minUpProj = Vector3::dotProduct(legRest[0].tibiaEnd, upDir);
        for (size_t i = 1; i < legCount; ++i)
            minUpProj = std::min(minUpProj, Vector3::dotProduct(legRest[i].tibiaEnd, upDir));
        std::array<Vector3, 8> footHome;
        for (size_t i = 0; i < legCount; ++i) {
            double proj = Vector3::dotProduct(legRest[i].tibiaEnd, upDir);
            footHome[i] = legRest[i].tibiaEnd - upDir * (proj - minUpProj);
        }

        // The body rocks about a point on the ground under the waist.
        Vector3 pivot = (bonePos("Abdomen") + bonePos("Cephalothorax")) * 0.5;
        pivot = pivot - upDir * (Vector3::dotProduct(pivot, upDir) - minUpProj);

        // `right` (as in the spider walk and attack clips) is up x forward, which is the
        // creature's left, so a blow from its right pushes along +right.
        double side = -hitDirection;
        double recoil = bodySize * 0.14 * recoilFactor * (1.0 - 0.5 * std::abs(side));
        double sideShift = bodySize * 0.1 * recoilFactor * side;
        double crouch = bodySize * 0.06 * recoilFactor;
        double flinchAngle = 0.28 * flinchFactor;
        double rollAngle = 0.18 * flinchFactor * side;
        double guardRaise = bodySize * 0.3 * frontLegGuardFactor;
        double scrabbleLift = bodySize * 0.12 * legScrabbleFactor;

        double snap = hitSnapFraction(durationSeconds);

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            // Reaches t = 1 on the last frame, which is the rest pose.
            double t = static_cast<double>(frame) / static_cast<double>(std::max(1, frameCount - 1));
            double env = hitReactionEnvelope(t, recoverySpeed, snap);
            double lifted = std::max(0.0, env);
            double shudder = hitShudder(t);

            Matrix4x4 bodyTransform;
            bodyTransform.translate(pivot
                + forward * (-recoil * env)
                - right * (sideShift * env)
                - upDir * (crouch * lifted + bodySize * 0.015 * shudder));
            // Front of the body flinches up (a negative pitch about `right` lifts the head)
            // and the body rolls away from the blow.
            bodyTransform.rotate(right, -flinchAngle * env);
            bodyTransform.rotate(forward, rollAngle * env);
            bodyTransform.translate(Vector3() - pivot);

            std::map<std::string, Matrix4x4> boneWorldTransforms = restBoneWorldTransforms(rigStructure);

            auto computeBone = [&](const std::string& name, double extraPitch = 0.0, double extraYaw = 0.0) {
                if (!boneIdx.count(name))
                    return;
                Vector3 pos = bodyTransform.transformPoint(bonePos(name));
                Vector3 end = bodyTransform.transformPoint(boneEnd(name));
                if (std::abs(extraPitch) > 1e-6 || std::abs(extraYaw) > 1e-6) {
                    Matrix4x4 extraRot;
                    if (std::abs(extraYaw) > 1e-6)
                        extraRot.rotate(upDir, extraYaw);
                    if (std::abs(extraPitch) > 1e-6)
                        extraRot.rotate(right, extraPitch);
                    end = pos + extraRot.transformVector(end - pos);
                }
                boneWorldTransforms[name] = buildBoneWorldTransform(pos, end);
            };

            computeBone("Root");
            computeBone("Cephalothorax");
            computeBone("Head", -0.15 * flinchFactor * env, -0.2 * side * env);
            // The abdomen (a scorpion's tail) flicks up over the back and lashes once.
            computeBone("Abdomen", abdomenFlickFactor * (0.45 * lifted + 0.12 * shudder));

            // Pedipalps (claws) snap in to guard the face.
            for (const char* palp : { "LeftPedipalp", "RightPedipalp" }) {
                double palpSide = (palp[0] == 'L') ? 1.0 : -1.0;
                computeBone(palp, -0.45 * frontLegGuardFactor * lifted, -palpSide * 0.3 * frontLegGuardFactor * lifted);
            }

            for (size_t i = 0; i < legCount; ++i) {
                Vector3 footTarget = footHome[i];
                if (legs[i].front) {
                    // Front legs come up to guard, then plant again where they were.
                    footTarget = footHome[i] + upDir * (guardRaise * lifted) + forward * (bodySize * 0.05 * lifted);
                } else {
                    // The other legs scrabble: each lifts once and replants at home, staggered
                    // front to back and left to right, while the body is still reeling.
                    double phase = 0.16 + 0.06 * static_cast<double>(i / 2) + ((i % 2) ? 0.04 : 0.0);
                    double x = (t - phase) / 0.14;
                    double lift = (x > 0.0 && x < 1.0) ? std::sin(Math::Pi * x) : 0.0;
                    footTarget = footHome[i] + upDir * (scrabbleLift * lift);
                }

                Vector3 hipPos = bodyTransform.transformPoint(legRest[i].coxaPos);
                Vector3 coxaEndPos = bodyTransform.transformPoint(legRest[i].coxaEnd);
                Vector3 tibiaEndPos = bodyTransform.transformPoint(legRest[i].tibiaEnd);

                std::vector<Vector3> chain = { hipPos, coxaEndPos, tibiaEndPos };
                Vector3 poleVector = coxaEndPos + upDir * 0.5;
                animation::solveTwoBoneIk(chain, footTarget, poleVector, 0.02);

                Vector3 newStickDir = (chain[2] - chain[1]);
                if (newStickDir.isZero())
                    newStickDir = legRest[i].restStickDir;
                else
                    newStickDir.normalize();
                Quaternion stickRot = Quaternion::rotationTo(legRest[i].restStickDir, newStickDir);
                Matrix4x4 stickRotMat;
                stickRotMat.rotate(stickRot);
                Vector3 femurEnd = chain[1] + stickRotMat.transformVector(legRest[i].restCoxaToFemurVec);

                boneWorldTransforms[legs[i].coxa] = animation::buildBoneWorldTransform(chain[0], chain[1]);
                boneWorldTransforms[legs[i].femur] = animation::buildBoneWorldTransform(chain[1], femurEnd);
                boneWorldTransforms[legs[i].tibia] = animation::buildBoneWorldTransform(femurEnd, chain[2]);
            }

            auto& animFrame = animationClip.frames[frame];
            animFrame.time = static_cast<float>(frame) / static_cast<float>(frameCount) * durationSeconds;
            animFrame.boneWorldTransforms = boneWorldTransforms;
            finishFrame(animFrame, inverseBindMatrices);
        }

        return true;
    }

} // namespace spider

} // namespace dust3d
