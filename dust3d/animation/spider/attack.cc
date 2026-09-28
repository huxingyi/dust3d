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

// Procedural strike animation for spider rig.
//
// A one-shot attack for game use: the creature rears back on its rear legs,
// lifts its front legs and pedipalps, curls the abdomen up (a scorpion's
// tail), then lunges forward and slams the front legs down, and settles back
// into its rest stance. The clip starts and ends in the rest pose, so it can
// also be played back to back. Works for spiders, crabs and scorpions.
//
// Adjustable animation parameters:
//   - lungeDistanceFactor:  how far the body lunges forward on the strike
//   - rearHeightFactor:     how high the front of the body rears up
//   - frontLegRaiseFactor:  how high the front legs are lifted
//   - pedipalpStrikeFactor: pedipalp swing (claws for crabs and scorpions)
//   - abdomenCurlFactor:    abdomen curl over the back (scorpion tail strike)
//   - strikeTimingFactor:   when the strike lands (1 = 45% into the clip)

#include <algorithm>
#include <array>
#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/common.h>
#include <dust3d/animation/spider/attack.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace spider {

    bool attack(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = static_cast<int>(parameters.getValue("frameCount", 36));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 1.0));

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

        double lungeDistanceFactor = parameters.getValue("lungeDistanceFactor", 1.0);
        double rearHeightFactor = parameters.getValue("rearHeightFactor", 1.0);
        double frontLegRaiseFactor = parameters.getValue("frontLegRaiseFactor", 1.0);
        double pedipalpStrikeFactor = parameters.getValue("pedipalpStrikeFactor", 1.0);
        double abdomenCurlFactor = parameters.getValue("abdomenCurlFactor", 1.0);
        double strikeTimingFactor = parameters.getValue("strikeTimingFactor", 1.0);

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

        // The body pitches about the rear of the cephalothorax, so the back legs stay planted.
        Vector3 pivot = bonePos("Abdomen");
        pivot = pivot - upDir * (Vector3::dotProduct(pivot, upDir) - minUpProj);

        auto smooth = [](double a, double b, double x) -> double {
            if (x <= a)
                return 0.0;
            if (x >= b)
                return 1.0;
            double p = (x - a) / (b - a);
            return p * p * (3.0 - 2.0 * p);
        };

        // Phases: windup (rear back), strike (lunge and slam), recover (settle to rest).
        double strikeAt = std::clamp(0.45 * strikeTimingFactor, 0.2, 0.8);
        double windupEnd = strikeAt - 0.12;

        double rearAngle = 0.35 * rearHeightFactor; // rad, head up
        double slamAngle = 0.12 * rearHeightFactor; // rad, head down at impact
        double lungeDist = bodySize * 0.28 * lungeDistanceFactor;
        double backShift = bodySize * 0.08 * lungeDistanceFactor;
        double legRaise = bodySize * 0.45 * frontLegRaiseFactor;

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            // [0, 1): the clip starts and ends at rest, so frame 0 repeats cleanly.
            double t = static_cast<double>(frame) / static_cast<double>(frameCount);

            double windup = smooth(0.0, windupEnd, t) * (1.0 - smooth(windupEnd, strikeAt, t));
            double strike = smooth(windupEnd, strikeAt, t) * (1.0 - smooth(strikeAt + 0.08, 1.0, t));

            double pitch = -rearAngle * windup + slamAngle * strike;
            double shift = -backShift * windup + lungeDist * strike;

            Matrix4x4 bodyTransform;
            bodyTransform.translate(pivot + forward * shift);
            bodyTransform.rotate(right, pitch);
            bodyTransform.translate(Vector3() - pivot);

            std::map<std::string, Matrix4x4> boneWorldTransforms;

            auto computeBone = [&](const std::string& name, double extraPitch = 0.0, double extraYaw = 0.0) {
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
            computeBone("Head", 0.15 * strike - 0.1 * windup);

            // The abdomen curls up over the back during the windup and whips forward on the strike.
            // Its bone points backwards, so a positive pitch about `right` lifts its end.
            double curl = abdomenCurlFactor * (0.9 * windup + 0.5 * strike);
            computeBone("Abdomen", curl);

            for (const char* palp : { "LeftPedipalp", "RightPedipalp" }) {
                if (!boneIdx.count(palp))
                    continue;
                double side = (palp[0] == 'L') ? 1.0 : -1.0;
                double raise = -0.6 * pedipalpStrikeFactor * windup;
                double snap = 0.5 * pedipalpStrikeFactor * strike;
                computeBone(palp, raise + snap, -side * 0.35 * pedipalpStrikeFactor * strike);
            }

            for (size_t i = 0; i < legCount; ++i) {
                Vector3 footTarget = footHome[i];
                if (legs[i].front) {
                    // Raised high and forward in the windup, slammed down ahead on the strike.
                    footTarget = footHome[i] + upDir * (legRaise * windup)
                        + forward * (bodySize * 0.12 * windup + lungeDist * 1.2 * strike);
                } else {
                    footTarget = footHome[i] + forward * (lungeDist * 0.25 * strike);
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
            animFrame.time = static_cast<float>(t) * durationSeconds;
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
