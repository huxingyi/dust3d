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

// Shared authored death for many-legged rigs (spiders, scorpions, crabs, insects).
//
// One-shot, ends lying still. A spasm, then the body drops to the ground under
// gravity, the feet sliding out, and bounces once; with flipOver it rolls over
// onto its back like a dead beetle. The legs draw in toward the belly into the
// death curl (legCurlFactor 1; on its back they fold up over the belly) or lie
// splayed out flat (legCurlFactor 0, e.g. a mechanical walker), with a few fading
// twitches. The abdomen sags and tips over to one side (a scorpion's tail falls
// over). Every bone is kept on or above the ground.

#ifndef DUST3D_ANIMATION_ARTHROPOD_DIE_H_
#define DUST3D_ANIMATION_ARTHROPOD_DIE_H_

#include <algorithm>
#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/common.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <string>
#include <vector>

namespace dust3d {

namespace animation {

    struct ArthropodLeg {
        std::string coxa;
        std::string femur;
        std::string tibia;
    };

    // bodyBones: the rigid body (first one = the front segment used for facing, e.g.
    // Cephalothorax / Thorax); headBone: the head; rearBone: the abdomen.
    inline bool arthropodDeath(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters,
        const std::vector<std::string>& bodyBones,
        const std::string& headBone,
        const std::string& rearBone,
        const std::vector<ArthropodLeg>& legs)
    {
        int frameCount = std::max(2, static_cast<int>(parameters.getValue("frameCount", 36)));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 1.2));

        auto boneIdx = buildBoneIndexMap(rigStructure);
        for (const auto& name : bodyBones)
            if (!boneIdx.count(name))
                return false;
        if (!boneIdx.count(headBone) || !boneIdx.count(rearBone))
            return false;
        for (const auto& leg : legs)
            if (!boneIdx.count(leg.coxa) || !boneIdx.count(leg.femur) || !boneIdx.count(leg.tibia))
                return false;
        auto bonePos = [&](const std::string& name) -> Vector3 {
            return getBonePos(rigStructure, boneIdx, name);
        };
        auto boneEnd = [&](const std::string& name) -> Vector3 {
            return getBoneEnd(rigStructure, boneIdx, name);
        };

        double speed = std::max(0.3, parameters.getValue("collapseSpeedFactor", 1.0));
        double legCurl = std::clamp(parameters.getValue("legCurlFactor", 1.0), 0.0, 1.5);
        double flipOver = std::clamp(parameters.getValue("flipOver", 0.0), 0.0, 1.0);
        double twitch = parameters.getValue("twitchFactor", 1.0);
        double legSpread = parameters.getValue("legSpreadFactor", 1.0);
        double bounce = 0.35 * std::clamp(parameters.getValue("groundBounce", 0.22), 0.0, 1.0);

        Vector3 up(0.0, 1.0, 0.0);
        Vector3 axis = boneEnd(headBone) - boneEnd(rearBone);
        Vector3 forward(axis.x(), 0.0, axis.z());
        if (forward.lengthSquared() < 1e-10)
            forward = Vector3(0.0, 0.0, 1.0);
        forward.normalize();
        Vector3 right = Vector3::crossProduct(forward, up).normalized();
        double bodyLength = std::max(1e-4, axis.length());

        double groundY = restGroundHeight(rigStructure);
        Vector3 centre = bonePos(bodyBones[0]);
        double bodyRadius = 0.0;
        for (const auto& name : bodyBones)
            bodyRadius = std::max(bodyRadius, static_cast<double>(rigStructure.bones[boneIdx[name]].capsuleRadius));
        bodyRadius = std::max(bodyRadius, static_cast<double>(rigStructure.bones[boneIdx[rearBone]].capsuleRadius));
        if (bodyRadius < 1e-4)
            bodyRadius = 0.15 * bodyLength;
        double standHeight = centre.y() - groundY;
        double lyingHeight = std::min(standHeight, 1.05 * bodyRadius);

        struct LegRest {
            Vector3 hip, tip;
            double length;
        };
        std::vector<LegRest> legRest;
        for (const auto& leg : legs) {
            LegRest r;
            r.hip = bonePos(leg.coxa);
            r.tip = boneEnd(leg.tibia);
            r.length = (boneEnd(leg.femur) - bonePos(leg.femur)).length() + (boneEnd(leg.tibia) - bonePos(leg.tibia)).length();
            legRest.push_back(r);
        }

        std::map<std::string, Matrix4x4> rest = restBoneWorldTransforms(rigStructure);

        // Timeline (normalised; collapseSpeedFactor scales it): a spasm, the drop (lands at
        // 0.36), the roll onto its back (0.14 to 0.56), the legs curling in from 0.1, then
        // fading twitches.
        auto drop = [&](double x) { return fallEnvelope(x - 0.04, 0.32, bounce, 0.1); };
        // A many-legged body tumbles over, flailing, rather than toppling like a rigid block:
        // the roll eases in and out.
        auto roll = [&](double x) { return smootherstep((x - 0.14) / 0.42); };
        const double legsMoveFrom = 0.1;
        double fallSide = 1.0;
        auto bodyAt = [&](double x) {
            double spasm = x < 0.1 ? std::pow(std::sin(Math::Pi * x / 0.1), 2.0) : 0.0;
            double dropped = drop(x);
            double rolled = roll(x) * flipOver;
            double height = standHeight - (standHeight - lyingHeight) * dropped;
            // Rolling over it rests on its side for a moment: a little higher.
            height += 0.3 * bodyRadius * std::sin(Math::Pi * std::clamp(rolled, 0.0, 1.0));
            Matrix4x4 m;
            m.translate(centre + up * (height - standHeight + 0.15 * bodyRadius * spasm));
            m.rotate(forward, fallSide * Math::Pi * rolled);
            m.rotate(right, -0.1 * dropped * (1.0 - flipOver) + 0.08 * spasm);
            m.translate(Vector3() - centre);
            return m;
        };

        // Each leg's tip in the body's own frame: where it was when the legs started to move
        // (planted, sliding out as the body dropped) and the limp pose (curled in toward the
        // belly, or splayed out flat). Blending between them in the body's frame carries the
        // legs round with the body as it rolls over, so they never sweep across it.
        std::vector<Vector3> tipFrom(legs.size()), tipTo(legs.size());
        {
            Matrix4x4 body = bodyAt(legsMoveFrom);
            Matrix4x4 toLocal = body.inverted();
            for (size_t i = 0; i < legs.size(); ++i) {
                const LegRest& r = legRest[i];
                Vector3 out = r.tip - centre;
                out.setY(0.0);
                out = out.lengthSquared() > 1e-12 ? out.normalized() : forward;
                Vector3 planted = r.tip + out * (0.15 * r.length * drop(legsMoveFrom));
                tipFrom[i] = toLocal.transformPoint(planted);
                // Limp: curled in toward the belly (under the body, or up over it on its back),
                // or splayed flat; both in the rest frame.
                // Curled: folded up close to the body, the foot just out from the hip toward the
                // belly (the knee then sticks out to the side).
                Vector3 femurStart = bonePos(legs[i].femur);
                Vector3 curled = femurStart + out * (0.25 * r.length) + (Vector3() - up) * (0.2 * r.length);
                Vector3 splayed = r.tip + out * (0.25 * r.length * legSpread) - up * (standHeight - lyingHeight);
                double c = std::min(1.0, legCurl);
                tipTo[i] = splayed + (curled - splayed) * c;
            }
        }

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            double t = static_cast<double>(frame) / static_cast<double>(frameCount - 1);
            double ts = t * speed;
            double dropped = drop(ts);
            double curl = smoothstep((ts - legsMoveFrom) / 0.45);
            double limp = smoothstep((ts - 0.3) / 0.4);
            Matrix4x4 body = bodyAt(ts);

            std::map<std::string, Matrix4x4> world = rest;
            std::map<std::string, Matrix4x4> layers;
            auto apply = [&](const std::string& name, const Matrix4x4& layer) {
                if (!boneIdx.count(name))
                    return;
                layers[name] = layer;
                Matrix4x4 m = layer;
                m *= rest[name];
                world[name] = m;
            };
            apply("Root", Matrix4x4());
            for (const auto& name : bodyBones)
                apply(name, body);
            apply(headBone, body);
            // The abdomen sags and tips over to one side as it goes limp (a scorpion's tail,
            // raised over its back, falls over onto the ground).
            Vector3 waist = body.transformPoint(bonePos(rearBone));
            Vector3 rearDir = body.transformVector(boneEnd(rearBone) - bonePos(rearBone));
            Vector3 sagAxis = Vector3::crossProduct(rearDir, up);
            Matrix4x4 abdomen;
            if (rearDir.lengthSquared() > 1e-12)
                abdomen = rotationAbout(waist, body.transformVector(forward), -fallSide * 1.0 * limp * (1.0 - flipOver));
            if (sagAxis.lengthSquared() > 1e-12)
                abdomen *= rotationAbout(waist, sagAxis.normalized(), -0.2 * curl * (1.0 - flipOver));
            abdomen *= body;
            apply(rearBone, abdomen);

            for (size_t i = 0; i < legs.size(); ++i) {
                const LegRest& r = legRest[i];
                Vector3 target;
                if (ts <= legsMoveFrom) {
                    Vector3 out = r.tip - centre;
                    out.setY(0.0);
                    out = out.lengthSquared() > 1e-12 ? out.normalized() : forward;
                    target = r.tip + out * (0.15 * r.length * dropped);
                } else {
                    // Rolling over, the legs it rolls onto fold in against its side (most halfway
                    // over): their sideways reach, which would point down into the ground, is
                    // drawn in until the foot sits just below the hip, toward the belly.
                    double side = Vector3::dotProduct(r.hip - centre, right) >= 0.0 ? 1.0 : -1.0;
                    double over = std::sin(Math::Pi * std::clamp(roll(ts), 0.0, 1.0) * flipOver);
                    Vector3 tipLocal = tipFrom[i] + (tipTo[i] - tipFrom[i]) * curl;
                    Vector3 lateral = right * Vector3::dotProduct(tipLocal - bonePos(legs[i].femur), right);
                    Vector3 tuck = (side == fallSide) ? (lateral * -0.8 + (Vector3() - up) * (0.15 * r.length)) * over : Vector3();
                    target = body.transformPoint(tipFrom[i] + (tipTo[i] - tipFrom[i]) * curl + tuck);
                    target.setY(std::max(target.y(), groundY + rigStructure.bones[boneIdx[legs[i].tibia]].capsuleRadius * curl));
                }
                // A few fading twitches, legs out of step: each leg kicks out and draws back in
                // along its reach (so its knee keeps bending the same way).
                double tw = twitch * 0.12 * r.length * std::sin(2.0 * Math::Pi * (5.0 * ts + 0.37 * static_cast<double>(i)))
                    * std::max(0.0, std::sin(Math::Pi * std::clamp((ts - 0.3) / 0.6, 0.0, 1.0)));
                Vector3 reach = target - body.transformPoint(bonePos(legs[i].femur));
                if (reach.lengthSquared() > 1e-12)
                    target += reach.normalized() * tw;
                // The femur-tibia joint points out to the leg's side of the body (judged by its
                // foot, which always stands out to one side even where the hip is near the
                // middle), turning with the body as it rolls over: it never lines up with the leg
                // and flips.
                double legSide = Vector3::dotProduct(r.tip - centre, right) >= 0.0 ? 1.0 : -1.0;
                Vector3 outward = body.transformVector(right * legSide).normalized();
                // The coxa stays with the body; femur and tibia fold at the knee.
                Vector3 femurStart = body.transformPoint(bonePos(legs[i].femur));
                std::vector<Vector3> chain = { femurStart, body.transformPoint(bonePos(legs[i].tibia)), body.transformPoint(r.tip) };
                // As it goes limp the knee turns to point along the body (front legs forward,
                // the others back): horizontal however far it has rolled, so a leg it rolls onto
                // folds alongside it instead of kneeling into the ground.
                double foreAft = Vector3::dotProduct(r.hip - centre, forward) > 0.0 ? 1.0 : -1.0;
                double alongBody = smoothstep((ts - legsMoveFrom) / 0.15);
                Vector3 kneeDir = outward * (1.0 - alongBody) + body.transformVector(forward) * (foreAft * alongBody);
                solveTwoBoneIk(chain, target, chain[1] + kneeDir * r.length, 0.02);
                // Bones turned from where the body carries them (they keep their twist).
                auto carried = [&](const std::string& name, const Vector3& newStart, const Vector3& newEnd) {
                    Matrix4x4 carriedRest = body;
                    carriedRest *= rest[name];
                    return boneFromRest(carriedRest, body.transformPoint(bonePos(name)), body.transformPoint(boneEnd(name)), newStart, newEnd);
                };
                Matrix4x4 coxaWorld = body;
                coxaWorld *= rest[legs[i].coxa];
                world[legs[i].coxa] = coxaWorld;
                world[legs[i].femur] = carried(legs[i].femur, chain[0], chain[1]);
                world[legs[i].tibia] = carried(legs[i].tibia, chain[1], chain[2]);
                layers[legs[i].coxa] = Matrix4x4();
            }

            // Anything else (pedipalps, wings, antennae, spikes) follows its nearest moved ancestor.
            for (const auto& bone : rigStructure.bones) {
                if (layers.count(bone.name))
                    continue;
                bool isLeg = false;
                for (const auto& leg : legs)
                    isLeg = isLeg || bone.name == leg.femur || bone.name == leg.tibia;
                if (isLeg)
                    continue;
                std::string parent = bone.parent;
                while (!parent.empty() && !layers.count(parent)) {
                    auto it = boneIdx.find(parent);
                    parent = it == boneIdx.end() ? std::string() : rigStructure.bones[it->second].parent;
                }
                if (!parent.empty())
                    apply(bone.name, layers[parent]);
            }

            auto& animFrame = animationClip.frames[frame];
            animFrame.time = static_cast<float>(frame) / static_cast<float>(frameCount) * durationSeconds;
            animFrame.boneWorldTransforms = world;
            finishFrame(animFrame, inverseBindMatrices);
            // Everything but the body is kept above the ground (the legs rarely need it: their
            // feet are aimed above it and their knees point along the body).
            std::vector<std::string> keepRigid = bodyBones;
            keepRigid.push_back("Root");
            keepBonesAboveGround(rigStructure, boneIdx, inverseBindMatrices, animFrame, groundY, [&](const std::string& name) { return std::find(keepRigid.begin(), keepRigid.end(), name) == keepRigid.end(); }, 0.6, true);
        }
        return true;
    }

} // namespace animation

} // namespace dust3d

#endif
