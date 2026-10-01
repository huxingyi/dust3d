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

// Procedural hurt (hit reaction) animation for the insect rig.
//
// A one-shot clip for game use, for insects on the ground and in the air.
//
// On the ground (airborne = 0) the body is knocked back and away from the
// blow, sags and flinches its head up, the abdomen flicks, the wings flash
// open and buzz, the front legs come up to guard and the other legs scrabble,
// each lifting once and replanting where it stood, so no foot slides.
//
// In the air (airborne = 1) the same jolt is layered on top of the flying
// cycle (InsectFly, which also takes its own parameters), the insect drops a
// little and its wing beat stutters instead of stopping.
//
// After a short hit-stop the body settles back with one small overshoot. On
// the ground the clip starts and ends exactly in the rest pose; in the air it
// starts and ends on the flying cycle.
//
// Adjustable animation parameters:
//   - recoilFactor:        how far the body is knocked back (and sideways)
//   - flinchFactor:        how far the head end pitches up and the body rolls
//   - hitDirection:        where the blow comes from (-1 = left, 0 = front, 1 = right)
//   - frontLegGuardFactor: how high the front legs come up (ground)
//   - legScrabbleFactor:   how high the other legs lift as they scrabble (ground)
//   - wingFlareFactor:     how far the wings flash open (and flail in the air)
//   - abdomenFlickFactor:  how far the abdomen flicks up
//   - airborne:            0 = on the ground, 1 = flying
//   - recoverySpeed:       how quickly the body settles back (> 1 = sooner)

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/insect/common.h>
#include <dust3d/animation/insect/fly.h>
#include <dust3d/animation/insect/hurt.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace insect {

    bool hurt(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = std::max(2, static_cast<int>(parameters.getValue("frameCount", 24)));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 0.7));

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

        double recoilFactor = parameters.getValue("recoilFactor", 1.0);
        double flinchFactor = parameters.getValue("flinchFactor", 1.0);
        double hitDirection = std::clamp(parameters.getValue("hitDirection", 0.0), -1.0, 1.0);
        double frontLegGuardFactor = parameters.getValue("frontLegGuardFactor", 1.0);
        double legScrabbleFactor = parameters.getValue("legScrabbleFactor", 1.0);
        double wingFlareFactor = parameters.getValue("wingFlareFactor", 1.0);
        double abdomenFlickFactor = parameters.getValue("abdomenFlickFactor", 1.0);
        bool airborne = parameters.getValue("airborne", 0.0) > 0.5;
        double recoverySpeed = parameters.getValue("recoverySpeed", 1.0);

        RigAnimationClip baseClip;
        if (airborne) {
            AnimationParams flyParams = parameters;
            flyParams.setValue("frameCount", frameCount);
            flyParams.setValue("durationSeconds", durationSeconds);
            if (!fly(rigStructure, inverseBindMatrices, baseClip, flyParams))
                return false;
            if (static_cast<int>(baseClip.frames.size()) != frameCount)
                return false;
            // The flying clip on its own gets its bones' twist from the rest pose afterwards
            // (bones it builds from a direction alone); give the base that twist now, as
            // layered on top it would no longer be recognised.
            animation::referenceRollsToRest(rigStructure, inverseBindMatrices, baseClip);
        }

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

        double side = hitDirection;
        double recoil = bodyLength * 0.16 * recoilFactor * (1.0 - 0.5 * std::abs(side));
        double sideShift = bodyLength * 0.12 * recoilFactor * side;
        double crouch = bodyLength * 0.05 * recoilFactor;
        double drop = bodyLength * 0.35 * recoilFactor;
        double flinchAngle = 0.3 * flinchFactor;
        double rollAngle = 0.22 * flinchFactor * side;
        double guardRaise = bodyLength * 0.25 * frontLegGuardFactor;
        double scrabbleLift = bodyLength * 0.1 * legScrabbleFactor;
        double wingFlare = 0.9 * wingFlareFactor;

        Vector3 bodyCentre = (bonePos("Thorax") + boneEnd("Abdomen")) * 0.5;
        Vector3 pivot = bodyCentre;
        if (!airborne)
            pivot.setY(groundY);

        std::map<std::string, Matrix4x4> rest = restBoneWorldTransforms(rigStructure);

        double snap = hitSnapFraction(durationSeconds);

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            double t = static_cast<double>(frame) / static_cast<double>(frameCount - 1);
            double env = hitReactionEnvelope(t, recoverySpeed, snap);
            double lifted = std::max(0.0, env);
            double shudder = hitShudder(t, 7.0);

            Matrix4x4 body;
            body.translate(pivot
                + forward * (-recoil * env)
                - right * (sideShift * env)
                - up * ((airborne ? drop : crouch) * lifted + bodyLength * 0.015 * shudder));
            body.rotate(right, flinchAngle * env);
            body.rotate(forward, -rollAngle * env);
            body.translate(Vector3() - pivot);

            const std::map<std::string, Matrix4x4>& base = airborne ? baseClip.frames[frame].boneWorldTransforms : rest;
            auto baseOf = [&](const std::string& name) -> Matrix4x4 {
                auto it = base.find(name);
                return it != base.end() ? it->second : rest[name];
            };
            auto layered = [&](const Matrix4x4& layer, const std::string& name) -> Matrix4x4 {
                Matrix4x4 m = layer;
                m *= baseOf(name);
                return m;
            };

            std::map<std::string, Matrix4x4> world = rest;
            for (const auto& pair : base)
                world[pair.first] = layered(body, pair.first);

            // The head nods up a little more than the body; the abdomen flicks up and lashes.
            {
                Vector3 neck = body.transformPoint(baseOf("Head").transformPoint(Vector3()));
                Matrix4x4 headLayer = rotationAbout(neck, up, 0.25 * side * env);
                headLayer *= rotationAbout(neck, right, 0.2 * flinchFactor * env);
                headLayer *= body;
                world["Head"] = layered(headLayer, "Head");

                Vector3 waist = body.transformPoint(baseOf("Abdomen").transformPoint(Vector3()));
                // The abdomen points backwards, so a nose-down rotation lifts its tip.
                Matrix4x4 abdomenLayer = rotationAbout(waist, right, -abdomenFlickFactor * (0.4 * lifted + 0.1 * shudder));
                abdomenLayer *= body;
                world["Abdomen"] = layered(abdomenLayer, "Abdomen");
            }

            // Wings flash open (on the ground) or flail (in the air), with a fast buzz.
            for (const char* wingName : { "LeftWing", "RightWing" }) {
                if (!boneIdx.count(wingName))
                    continue;
                double wingSide = (std::strcmp(wingName, "LeftWing") == 0) ? 1.0 : -1.0;
                Matrix4x4 wingWorld = world[wingName];
                Vector3 hinge = wingWorld.transformPoint(Vector3());
                double buzz = 0.25 * std::sin(2.0 * Math::Pi * 9.0 * t) * lifted;
                double open = wingFlare * (airborne ? 0.4 : 1.0) * lifted + wingFlareFactor * buzz;
                // Opening swings the wing out sideways and up from its hinge.
                Matrix4x4 wingLayer = rotationAbout(hinge, up, wingSide * 0.6 * open);
                wingLayer *= rotationAbout(hinge, forward, -wingSide * 0.5 * open);
                wingLayer *= wingWorld;
                world[wingName] = wingLayer;
            }

            if (!airborne) {
                for (size_t i = 0; i < legs.size(); ++i) {
                    Vector3 footTarget = footHome[i];
                    if (legs[i].front) {
                        footTarget += up * (guardRaise * lifted) + forward * (bodyLength * 0.04 * lifted);
                    } else {
                        double phase = 0.16 + 0.07 * static_cast<double>(i / 2) + ((i % 2) ? 0.04 : 0.0);
                        double x = (t - phase) / 0.13;
                        double lift = (x > 0.0 && x < 1.0) ? std::sin(Math::Pi * x) : 0.0;
                        footTarget += up * (scrabbleLift * lift);
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
