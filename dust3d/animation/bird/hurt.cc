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

// Procedural hurt (hit reaction) animation for the bird rig.
//
// A one-shot clip for game use, for birds on the ground and in the air.
//
// On the ground (airborne = 0) the body is knocked back and away from the
// blow and sags on its legs while both feet stay planted (two-bone IK), the
// chest rears up, the neck whips back and then overshoots forward, the wings
// flare out from the body and the tail flicks up.
//
// In the air (airborne = 1) the same reaction is layered on top of the flying
// wing beat (BirdFly, which also takes its own parameters: stepHeightFactor
// for the flap size, gaitSpeedFactor for the beats per clip), and the bird
// drops a little altitude and flails its wings instead of planting its feet.
// That way a flyer keeps flapping while it is hit, instead of freezing.
//
// After a short hit-stop the body settles back with one small overshoot. On
// the ground the clip starts and ends exactly in the rest pose; in the air it
// starts and ends on the flying cycle.
//
// Adjustable animation parameters:
//   - recoilFactor:     how far the body is knocked back (and sideways)
//   - hitDirection:     where the blow comes from (-1 = left, 0 = front, 1 = right)
//   - neckWhipFactor:   how far the neck and head whip back
//   - wingFlareFactor:  how far the wings flare out (flail in the air)
//   - tailFlickFactor:  how far the tail flicks up
//   - crouchFactor:     how far the legs give under the blow (ground)
//   - airborne:         0 = standing on the ground, 1 = flying
//   - recoverySpeed:    how quickly the body settles back (> 1 = sooner)

#include <algorithm>
#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/bird/fly.h>
#include <dust3d/animation/bird/hurt.h>
#include <dust3d/animation/common.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace bird {

    bool hurt(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = std::max(2, static_cast<int>(parameters.getValue("frameCount", 24)));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 0.8));

        auto boneIdx = buildBoneIndexMap(rigStructure);
        auto bonePos = [&](const std::string& name) -> Vector3 {
            return getBonePos(rigStructure, boneIdx, name);
        };
        auto boneEnd = [&](const std::string& name) -> Vector3 {
            return getBoneEnd(rigStructure, boneIdx, name);
        };

        static const char* requiredBones[] = {
            "Root", "Pelvis", "Spine", "Chest", "Neck", "Head",
            "LeftWingShoulder", "LeftWingElbow", "LeftWingHand",
            "RightWingShoulder", "RightWingElbow", "RightWingHand",
            "LeftUpperLeg", "LeftLowerLeg", "LeftFoot",
            "RightUpperLeg", "RightLowerLeg", "RightFoot"
        };
        if (!validateRequiredBones(boneIdx, requiredBones, sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;

        Vector3 bodyVector = boneEnd("Chest") - bonePos("Pelvis");
        bodyVector.setY(0.0);
        if (bodyVector.lengthSquared() < 1e-10)
            return false;
        double bodyLength = (boneEnd("Chest") - bonePos("Pelvis")).length();
        Vector3 forward = bodyVector.normalized();
        Vector3 up(0.0, 1.0, 0.0);
        // A positive rotation about `right` lifts the front (nose up).
        Vector3 right = Vector3::crossProduct(forward, up).normalized();

        double recoilFactor = parameters.getValue("recoilFactor", 1.0);
        double hitDirection = std::clamp(parameters.getValue("hitDirection", 0.0), -1.0, 1.0);
        double neckWhipFactor = parameters.getValue("neckWhipFactor", 1.0);
        double wingFlareFactor = parameters.getValue("wingFlareFactor", 1.0);
        double tailFlickFactor = parameters.getValue("tailFlickFactor", 1.0);
        double crouchFactor = parameters.getValue("crouchFactor", 1.0);
        bool airborne = parameters.getValue("airborne", 0.0) > 0.5;
        double recoverySpeed = parameters.getValue("recoverySpeed", 1.0);

        // In the air: the flying cycle under the reaction, frame for frame.
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

        double side = hitDirection;
        double recoil = bodyLength * 0.35 * recoilFactor * (1.0 - 0.4 * std::abs(side));
        double sideShift = bodyLength * 0.25 * recoilFactor * side;
        double crouch = bodyLength * 0.18 * crouchFactor;
        double drop = bodyLength * 0.5 * recoilFactor;
        double rearAngle = 0.3 * recoilFactor;
        double rollAngle = 0.25 * recoilFactor * side;
        double neckWhip = 0.6 * neckWhipFactor;
        double wingFlare = 0.7 * wingFlareFactor;
        double tailFlick = 0.5 * tailFlickFactor;

        // Pivot: the ground between the feet, or the body centre in the air.
        Vector3 feetMid = (boneEnd("LeftLowerLeg") + boneEnd("RightLowerLeg")) * 0.5;
        Vector3 bodyCentre = (bonePos("Pelvis") + boneEnd("Chest")) * 0.5;
        Vector3 pivot = airborne ? bodyCentre : feetMid;

        static const char* bodyBones[] = { "Root", "Pelvis", "Spine", "Chest" };
        static const char* neckBones[] = { "Neck", "Head", "Beak" };
        static const char* tailBones[] = { "TailBase", "TailFeathers" };
        struct Wing {
            const char* shoulder;
            const char* elbow;
            const char* hand;
        };
        static const Wing wings[] = {
            { "LeftWingShoulder", "LeftWingElbow", "LeftWingHand" },
            { "RightWingShoulder", "RightWingElbow", "RightWingHand" },
        };
        struct Leg {
            const char* upper;
            const char* lower;
            const char* foot;
        };
        static const Leg legs[] = {
            { "LeftUpperLeg", "LeftLowerLeg", "LeftFoot" },
            { "RightUpperLeg", "RightLowerLeg", "RightFoot" },
        };

        double snap = hitSnapFraction(durationSeconds);

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        std::map<std::string, Matrix4x4> rest = restBoneWorldTransforms(rigStructure);

        for (int frame = 0; frame < frameCount; ++frame) {
            // Reaches t = 1 on the last frame.
            double t = static_cast<double>(frame) / static_cast<double>(frameCount - 1);
            double env = hitReactionEnvelope(t, recoverySpeed, snap);
            double lifted = std::max(0.0, env);
            // The neck lags the body by a frame or two, then overshoots forward.
            double neckEnv = hitReactionEnvelope(t - 0.035, recoverySpeed * 1.1, snap);
            double shudder = hitShudder(t);

            std::map<std::string, Matrix4x4> base = airborne ? baseClip.frames[frame].boneWorldTransforms : rest;
            auto baseOf = [&](const std::string& name) -> Matrix4x4 {
                auto it = base.find(name);
                return it != base.end() ? it->second : rest[name];
            };

            Matrix4x4 body;
            body.translate(pivot
                + forward * (-recoil * env)
                - right * (sideShift * env)
                - up * ((airborne ? drop : crouch) * lifted + bodyLength * 0.02 * shudder));
            body.rotate(right, rearAngle * env);
            body.rotate(forward, -rollAngle * env);
            body.translate(Vector3() - pivot);

            std::map<std::string, Matrix4x4> world = base;
            auto layered = [&](const Matrix4x4& layer, const std::string& name) -> Matrix4x4 {
                Matrix4x4 m = layer;
                m *= baseOf(name);
                return m;
            };

            for (const char* name : bodyBones)
                world[name] = layered(body, name);

            // Neck and head: whip back about the neck base (a nose-up rotation), with the body.
            Vector3 neckBase = body.transformPoint(baseOf("Neck").transformPoint(Vector3()));
            // ... and the head turns away from the blow.
            Matrix4x4 neckLayer = rotationAbout(neckBase, up, 0.3 * side * neckEnv);
            neckLayer *= rotationAbout(neckBase, right, neckWhip * neckEnv);
            neckLayer *= body;
            for (const char* name : neckBones) {
                if (boneIdx.count(name))
                    world[name] = layered(neckLayer, name);
            }

            // Tail flicks up (a nose-down rotation lifts a tail that points backwards).
            if (boneIdx.count("TailBase")) {
                Vector3 tailBase = body.transformPoint(baseOf("TailBase").transformPoint(Vector3()));
                Matrix4x4 tailLayer = rotationAbout(tailBase, right, -tailFlick * lifted);
                tailLayer *= body;
                for (const char* name : tailBones) {
                    if (boneIdx.count(name))
                        world[name] = layered(tailLayer, name);
                }
            }

            // Wings flare up and out from the shoulder, and flail a little in the air.
            for (const auto& wing : wings) {
                Vector3 shoulder = body.transformPoint(baseOf(wing.shoulder).transformPoint(Vector3()));
                double wingSide = Vector3::dotProduct(boneEnd(wing.hand) - bonePos(wing.shoulder), right) >= 0.0 ? 1.0 : -1.0;
                double flare = wingFlare * (lifted + (airborne ? 0.35 * shudder : 0.1 * shudder));
                // A positive rotation about `forward` lowers the +right side, so lift with the opposite sign.
                Matrix4x4 wingLayer = rotationAbout(shoulder, forward, -wingSide * flare);
                wingLayer *= body;
                for (const char* name : { wing.shoulder, wing.elbow, wing.hand })
                    world[name] = layered(wingLayer, name);
            }

            for (const auto& leg : legs) {
                if (airborne) {
                    // Legs hang with the body and kick back a little.
                    Vector3 hip = body.transformPoint(baseOf(leg.upper).transformPoint(Vector3()));
                    Matrix4x4 legLayer = rotationAbout(hip, right, -0.35 * lifted);
                    legLayer *= body;
                    for (const char* name : { leg.upper, leg.lower, leg.foot })
                        world[name] = layered(legLayer, name);
                    continue;
                }
                // On the ground both feet stay planted: IK from the moved hip to the rest ankle.
                posePlantedLeg(rigStructure, boneIdx, leg.upper, leg.lower, leg.foot, body, Vector3(), world);
            }

            auto& animFrame = animationClip.frames[frame];
            animFrame.time = static_cast<float>(frame) / static_cast<float>(frameCount) * durationSeconds;
            animFrame.boneWorldTransforms = world;
            finishFrame(animFrame, inverseBindMatrices);
        }
        return true;
    }

} // namespace bird

} // namespace dust3d
