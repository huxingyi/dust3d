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

// Procedural bite (ground attack) animation for the insect rig.
//
// A one-shot attack for insects on the ground: ants, termites, beetles,
// mantises. The insect rears back on its back legs with the front legs and
// head raised, then lunges forward and snaps its head down (the jaws), while
// the abdomen curls forward under the body (an ant's or wasp's sting), and
// settles back into its stance. The middle and back feet stay planted and the
// front feet land ahead and walk back in, so no foot slides. It needs no wing
// bones. The clip starts and ends exactly in the rest pose, so it blends with
// the idle and walk loops. (InsectAttack is the flying dive.)
//
// Adjustable animation parameters:
//   - lungeDistanceFactor:  how far the body lunges forward
//   - rearHeightFactor:     how high the head end rears up before the bite
//   - frontLegRaiseFactor:  how high the front legs lift
//   - headSnapFactor:       how hard the head (jaws) snaps down on the bite
//   - abdomenCurlFactor:    how far the abdomen curls forward to sting
//   - strikeTimingFactor:   when the bite lands (1 = 45% into the clip)

#include <algorithm>
#include <array>
#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/insect/bite.h>
#include <dust3d/animation/insect/common.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace insect {

    bool bite(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = std::max(2, static_cast<int>(parameters.getValue("frameCount", 27)));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 0.9));

        auto boneIdx = buildBoneIndexMap(rigStructure);
        auto bonePos = [&](const std::string& name) -> Vector3 {
            return getBonePos(rigStructure, boneIdx, name);
        };
        auto boneEnd = [&](const std::string& name) -> Vector3 {
            return getBoneEnd(rigStructure, boneIdx, name);
        };

        static const char* requiredBones[] = {
            "Head", "Thorax", "Abdomen",
            "FrontLeftCoxa", "FrontLeftFemur", "FrontLeftTibia",
            "FrontRightCoxa", "FrontRightFemur", "FrontRightTibia",
            "MiddleLeftCoxa", "MiddleLeftFemur", "MiddleLeftTibia",
            "MiddleRightCoxa", "MiddleRightFemur", "MiddleRightTibia",
            "BackLeftCoxa", "BackLeftFemur", "BackLeftTibia",
            "BackRightCoxa", "BackRightFemur", "BackRightTibia"
        };
        if (!validateRequiredBones(boneIdx, requiredBones, sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;

        Vector3 bodyVector = bonePos("Thorax") - boneEnd("Abdomen");
        double bodyLength = bodyVector.length();
        bodyVector.setY(0.0);
        if (bodyVector.lengthSquared() < 1e-10 || bodyLength < 1e-6)
            return false;
        Vector3 forward = bodyVector.normalized();
        Vector3 up(0.0, 1.0, 0.0);
        // The creature's right; a positive rotation about it lifts the head end.
        Vector3 right = Vector3::crossProduct(forward, up).normalized();

        double lungeDistanceFactor = parameters.getValue("lungeDistanceFactor", 1.0);
        double rearHeightFactor = parameters.getValue("rearHeightFactor", 1.0);
        double frontLegRaiseFactor = parameters.getValue("frontLegRaiseFactor", 1.0);
        double headSnapFactor = parameters.getValue("headSnapFactor", 1.0);
        double abdomenCurlFactor = parameters.getValue("abdomenCurlFactor", 1.0);
        double tStrike = std::clamp(0.45 * parameters.getValue("strikeTimingFactor", 1.0), 0.25, 0.75);
        animationClip.events.push_back({ "hit", static_cast<float>(tStrike * durationSeconds), "Head" });

        struct Leg {
            const char* coxa;
            const char* femur;
            const char* tibia;
            bool front;
        };
        static const std::array<Leg, 6> legs = { {
            { "FrontLeftCoxa", "FrontLeftFemur", "FrontLeftTibia", true },
            { "FrontRightCoxa", "FrontRightFemur", "FrontRightTibia", true },
            { "MiddleLeftCoxa", "MiddleLeftFemur", "MiddleLeftTibia", false },
            { "MiddleRightCoxa", "MiddleRightFemur", "MiddleRightTibia", false },
            { "BackLeftCoxa", "BackLeftFemur", "BackLeftTibia", false },
            { "BackRightCoxa", "BackRightFemur", "BackRightTibia", false },
        } };
        std::array<LegRest, 6> legRest;
        double groundY = boneEnd(legs[0].tibia).y();
        for (size_t i = 0; i < legs.size(); ++i) {
            legRest[i].coxaPos = bonePos(legs[i].coxa);
            legRest[i].coxaEnd = boneEnd(legs[i].coxa);
            legRest[i].femurEnd = boneEnd(legs[i].femur);
            legRest[i].tibiaEnd = boneEnd(legs[i].tibia);
            Vector3 chord = legRest[i].tibiaEnd - legRest[i].coxaEnd;
            legRest[i].restStickDir = chord.isZero() ? Vector3(1, 0, 0) : chord.normalized();
            legRest[i].restCoxaToFemurVec = legRest[i].femurEnd - legRest[i].coxaEnd;
            groundY = std::min(groundY, legRest[i].tibiaEnd.y());
        }
        std::array<Vector3, 6> footHome;
        for (size_t i = 0; i < legs.size(); ++i) {
            footHome[i] = legRest[i].tibiaEnd;
            footHome[i].setY(groundY);
        }

        double rearAngle = 0.32 * rearHeightFactor;
        double biteAngle = 0.12 * rearHeightFactor;
        double backShift = bodyLength * 0.08 * lungeDistanceFactor;
        double lunge = bodyLength * 0.3 * lungeDistanceFactor;
        double legRaise = bodyLength * 0.35 * frontLegRaiseFactor;
        double headSnap = 0.45 * headSnapFactor;
        double abdomenCurl = 0.55 * abdomenCurlFactor;

        // The body pitches about the back feet, so they stay planted while the front rears.
        Vector3 pivot = (legRest[4].tibiaEnd + legRest[5].tibiaEnd) * 0.5;
        pivot.setY(groundY);

        std::map<std::string, Matrix4x4> rest = restBoneWorldTransforms(rigStructure);

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            double t = static_cast<double>(frame) / static_cast<double>(frameCount - 1);
            double windupEnd = tStrike - 0.12;
            // Rear back, then strike, then settle (each exactly 0 at both ends of the clip).
            double windup = smoothstep(t / windupEnd) * (1.0 - smoothstep((t - windupEnd) / (tStrike - windupEnd)));
            double strike = smoothstep((t - windupEnd) / (tStrike - windupEnd))
                * (1.0 - smoothstep((t - (tStrike + 0.1)) / std::max(0.1, 0.95 - (tStrike + 0.1))));
            // The head snap is sharper than the body: it peaks right on the hit.
            double snapX = (t - (tStrike - 0.05)) / 0.2;
            double snap = (snapX > 0.0 && snapX < 1.0) ? std::sin(Math::Pi * snapX) : 0.0;

            Matrix4x4 body;
            body.translate(pivot + forward * (-backShift * windup + lunge * strike));
            // A positive rotation about `right` lifts the head end.
            body.rotate(right, rearAngle * windup - biteAngle * strike);
            body.translate(Vector3() - pivot);

            std::map<std::string, Matrix4x4> world = rest;
            for (const char* name : { "Root", "Thorax", "LeftWing", "RightWing" }) {
                if (boneIdx.count(name)) {
                    Matrix4x4 m = body;
                    m *= rest[name];
                    world[name] = m;
                }
            }
            {
                Vector3 neck = body.transformPoint(bonePos("Head"));
                Matrix4x4 headLayer = rotationAbout(neck, right, 0.15 * windup - headSnap * snap);
                headLayer *= body;
                headLayer *= rest["Head"];
                world["Head"] = headLayer;

                Vector3 waist = body.transformPoint(bonePos("Abdomen"));
                // The abdomen points backwards: a nose-up rotation swings its tip down and forward.
                Matrix4x4 abdomenLayer = rotationAbout(waist, right, abdomenCurl * (0.35 * windup + strike));
                abdomenLayer *= body;
                abdomenLayer *= rest["Abdomen"];
                world["Abdomen"] = abdomenLayer;
            }

            for (size_t i = 0; i < legs.size(); ++i) {
                Vector3 footTarget = footHome[i];
                if (legs[i].front) {
                    // Raised in the windup, slammed down ahead on the strike, walked back in.
                    footTarget += up * (legRaise * windup) + forward * (bodyLength * 0.1 * windup + lunge * 1.1 * strike);
                } else if (i < 4) {
                    // Middle feet shuffle forward with the lunge (lift, move, plant).
                    double shuffle = strike;
                    double liftX = (t - (windupEnd - 0.02)) / 0.16;
                    double lift = (liftX > 0.0 && liftX < 1.0) ? std::sin(Math::Pi * liftX) : 0.0;
                    double backX = (t - 0.7) / 0.16;
                    double liftBack = (backX > 0.0 && backX < 1.0) ? std::sin(Math::Pi * backX) : 0.0;
                    footTarget += forward * (lunge * 0.6 * shuffle) + up * (bodyLength * 0.06 * (lift + liftBack));
                }
                Vector3 hip = body.transformPoint(legRest[i].coxaPos);
                Vector3 coxaEnd = body.transformPoint(legRest[i].coxaEnd);
                Vector3 tibiaEnd = body.transformPoint(legRest[i].tibiaEnd);
                std::vector<Vector3> chain = { hip, coxaEnd, tibiaEnd };
                solveTwoBoneIk(chain, footTarget, coxaEnd + up * 0.5, 0.02);
                Vector3 stickDir = chain[2] - chain[1];
                stickDir = stickDir.isZero() ? legRest[i].restStickDir : stickDir.normalized();
                Matrix4x4 stickRot;
                stickRot.rotate(Quaternion::rotationTo(legRest[i].restStickDir, stickDir));
                Vector3 femurEnd = chain[1] + stickRot.transformVector(legRest[i].restCoxaToFemurVec);
                world[legs[i].coxa] = buildBoneWorldTransform(chain[0], chain[1]);
                world[legs[i].femur] = buildBoneWorldTransform(chain[1], femurEnd);
                world[legs[i].tibia] = buildBoneWorldTransform(femurEnd, chain[2]);
            }

            auto& animFrame = animationClip.frames[frame];
            animFrame.time = static_cast<float>(frame) / static_cast<float>(frameCount) * durationSeconds;
            animFrame.boneWorldTransforms = world;
            finishFrame(animFrame, inverseBindMatrices);
        }
        return true;
    }

} // namespace insect

} // namespace dust3d
