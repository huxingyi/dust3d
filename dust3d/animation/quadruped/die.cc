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

// Procedural death for the quadruped rig.
//
// One-shot, ends lying still. The blow jolts the body, the legs fold and it drops
// onto its chest (hind end first), chin to the ground, then it flops over onto its
// side and lands with a small bounce. Lying, the legs are half bent: the lower
// ones lie on the ground, the upper ones rest on top of them, a little staggered.
// The neck and head lie along the ground, the jaw falls open and the tail lies
// limp. Every bone is kept on or above the ground (using the rig's capsule radii),
// so nothing sinks into the floor whatever the body shape.
//
// Adjustable animation parameters:
//   - collapseSpeedFactor:  how fast the body goes down (> 1 = sooner)
//   - fallSide:             which side it falls onto (1 = its right, -1 = its left)
//   - rollIntensityFactor:  how far it rolls (1 = onto its side, 2 = onto its back)
//   - legBuckleFactor:      how far the legs fold before it rolls over
//   - headDropFactor:       how far the neck and head drop
//   - legSpreadFactor:      how far the legs reach out lying down
//   - groundBounce:         how much the body rebounds when it hits the ground

#include <algorithm>
#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/common.h>
#include <dust3d/animation/quadruped/die.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace quadruped {

    bool die(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = std::max(2, static_cast<int>(parameters.getValue("frameCount", 42)));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 1.4));

        auto boneIdx = buildBoneIndexMap(rigStructure);
        auto bonePos = [&](const std::string& name) -> Vector3 {
            return getBonePos(rigStructure, boneIdx, name);
        };
        auto boneEnd = [&](const std::string& name) -> Vector3 {
            return getBoneEnd(rigStructure, boneIdx, name);
        };

        static const char* requiredBones[] = {
            "Root", "Pelvis", "Spine", "Chest", "Neck", "Head",
            "FrontLeftUpperLeg", "FrontLeftLowerLeg", "FrontLeftFoot",
            "FrontRightUpperLeg", "FrontRightLowerLeg", "FrontRightFoot",
            "BackLeftUpperLeg", "BackLeftLowerLeg", "BackLeftFoot",
            "BackRightUpperLeg", "BackRightLowerLeg", "BackRightFoot"
        };
        if (!validateRequiredBones(boneIdx, requiredBones, sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;

        double speed = std::max(0.3, parameters.getValue("collapseSpeedFactor", 1.0));
        double fallSide = parameters.getValue("fallSide", 1.0) >= 0.0 ? 1.0 : -1.0;
        double rollIntensity = parameters.getValue("rollIntensityFactor", 1.0);
        double legBuckle = parameters.getValue("legBuckleFactor", 1.0);
        double headDrop = parameters.getValue("headDropFactor", 1.0);
        double legSpread = parameters.getValue("legSpreadFactor", 1.0);
        double bounce = 0.35 * std::clamp(parameters.getValue("groundBounce", 0.22), 0.0, 1.0);

        Vector3 up(0.0, 1.0, 0.0);
        Vector3 spine = bonePos("Chest") - bonePos("Pelvis");
        Vector3 forward(spine.x(), 0.0, spine.z());
        if (forward.lengthSquared() < 1e-10)
            forward = Vector3(0.0, 0.0, 1.0);
        forward.normalize();
        Vector3 right = Vector3::crossProduct(forward, up).normalized();

        double groundY = restGroundHeight(rigStructure);
        Vector3 centre = (bonePos("Pelvis") + boneEnd("Chest")) * 0.5;
        double bodyRadius = 0.0;
        for (const char* name : { "Pelvis", "Spine", "Chest" })
            bodyRadius = std::max(bodyRadius, static_cast<double>(rigStructure.bones[boneIdx[name]].capsuleRadius));
        if (bodyRadius < 1e-4)
            bodyRadius = 0.18 * spine.length();
        // Lying on its side the body rests on its half-width where that is larger than its
        // radius (wide, flat lizards): the hips and shoulders are at the sides.
        double halfWidth = 0.0;
        for (const char* name : { "FrontLeftUpperLeg", "FrontRightUpperLeg", "BackLeftUpperLeg", "BackRightUpperLeg" }) {
            double legRadius = rigStructure.bones[boneIdx[name]].capsuleRadius;
            halfWidth = std::max(halfWidth, std::abs(Vector3::dotProduct(bonePos(name) - centre, right)) + legRadius);
        }
        double standHeight = centre.y() - groundY;
        // On its side it rests on the wider of radius and half-width; on its back on its radius.
        double finalRoll = 0.5 * Math::Pi * rollIntensity;
        double lyingHeight = std::min(standHeight,
            1.1 * (std::abs(std::cos(finalRoll)) * bodyRadius + std::abs(std::sin(finalRoll)) * std::max(bodyRadius, halfWidth)));

        struct Leg {
            const char* upper;
            const char* lower;
            const char* foot;
        };
        static const Leg legs[] = {
            { "FrontLeftUpperLeg", "FrontLeftLowerLeg", "FrontLeftFoot" },
            { "FrontRightUpperLeg", "FrontRightLowerLeg", "FrontRightFoot" },
            { "BackLeftUpperLeg", "BackLeftLowerLeg", "BackLeftFoot" },
            { "BackRightUpperLeg", "BackRightLowerLeg", "BackRightFoot" },
        };

        std::map<std::string, Matrix4x4> rest = restBoneWorldTransforms(rigStructure);

        double endsOnSide = smoothstep((0.85 - std::abs(std::cos(finalRoll))) / 0.5);
        // The legs buckle and the body drops to about half its height (a low animal, such as a
        // lizard, right onto its belly); the roll takes it the rest of the way down. Folding
        // the legs flat under the body would need a different fold for every kind of leg.
        double sternalHeight = std::min(standHeight, std::max(1.05 * bodyRadius, 0.5 * standHeight));
        Vector3 pelvisPos = bonePos("Pelvis");
        Vector3 chestPos = boneEnd("Chest");
        double spineLength = std::max(1e-4, (chestPos - pelvisPos).length());

        // Timeline (normalised; collapseSpeedFactor scales it): the blow at 0, the hind end
        // then the front drop onto the chest under gravity (landing at 0.34 and 0.38), the head
        // goes down, then the body flops over onto its side (landing at 0.7) and settles.
        auto sinkRear = [&](double x) { return fallEnvelope(x - 0.04, 0.3, 0.3 * bounce, 0.08); };
        auto sinkFront = [&](double x) { return fallEnvelope(x - 0.08, 0.3, 0.3 * bounce, 0.08); };
        const double rollStart = 0.42;
        const double rollLand = 0.7;
        auto roll = [&](double x) { return fallEnvelope(x - rollStart, rollLand - rollStart, bounce, 0.12); };

        // The body at time x: drops as the legs buckle (pitching as one end lands before the
        // other), then rolls over about the ground onto its side (or back).
        auto bodyAt = [&](double x) {
            double blow = x < 0.12 ? std::pow(std::sin(Math::Pi * x / 0.12), 2.0) : 0.0;
            double rearDrop = (standHeight - sternalHeight) * legBuckle * sinkRear(x);
            double frontDrop = (standHeight - sternalHeight) * legBuckle * sinkFront(x);
            double rolledAmount = roll(x);
            double sternalDrop = std::min(0.5 * (rearDrop + frontDrop), standHeight - sternalHeight);
            double pitch = std::atan2(frontDrop - rearDrop, spineLength);
            // Rolling, it rests on whatever is underneath at that angle: its back (radius)
            // or its side (the wider of radius and shoulder/hip half-width).
            double angle = 0.5 * Math::Pi * rollIntensity * rolledAmount;
            double support = 1.05 * (std::abs(std::cos(angle)) * bodyRadius + std::abs(std::sin(angle)) * std::max(bodyRadius, halfWidth));
            double h = std::max(standHeight - sternalDrop - (sternalHeight - lyingHeight) * rolledAmount, lyingHeight);
            h = std::max(h, std::min(standHeight, support));
            Matrix4x4 m;
            m.translate(centre + up * (h - standHeight) + right * (fallSide * bodyRadius * 0.6 * rolledAmount));
            // A positive rotation about `forward` lowers the right side; the blow first jolts
            // it the other way.
            m.rotate(forward, fallSide * (0.5 * Math::Pi * rollIntensity * rolledAmount - 0.07 * blow));
            m.rotate(right, -pitch);
            m.translate(Vector3() - centre);
            return m;
        };

        // Legs. While the body drops the feet stay on the ground (sliding a little, sphinx-like:
        // the front feet forward, the hind feet forward and out) and the legs bend at the knee
        // the way they do at rest. From when it starts to roll they move, in the body's own
        // frame, from where they were then to the lying pose (half bent, the front legs
        // forward, the hind legs back, the upper pair drooping onto the lower one; curled up on
        // its back), carried round with the body, so they never sweep through the hip and flip.
        struct LegFrame {
            Vector3 knee;
            Vector3 ankle;
        };
        const double legsMoveFrom = rollStart - 0.04;
        auto legInfo = [&](const Leg& leg, double& side, bool& front, double& legLength, double& legRadius) {
            Vector3 hipRest = bonePos(leg.upper);
            side = Vector3::dotProduct(hipRest - centre, right) >= 0.0 ? 1.0 : -1.0;
            front = Vector3::dotProduct(hipRest - centre, forward) >= 0.0;
            legLength = (bonePos(leg.lower) - hipRest).length() + (boneEnd(leg.lower) - bonePos(leg.lower)).length();
            legRadius = rigStructure.bones[boneIdx[leg.upper]].capsuleRadius;
        };
        auto bendHintFor = [&](const Leg& leg, bool front, double legLength) {
            Vector3 hipRest = bonePos(leg.upper);
            Vector3 ankleRest = boneEnd(leg.lower);
            double restBend = Vector3::dotProduct(bonePos(leg.lower) - (hipRest + ankleRest) * 0.5, forward);
            return std::abs(restBend) > 0.03 * legLength ? (restBend > 0.0 ? 1.0 : -1.0) : (front ? -1.0 : 1.0);
        };
        std::map<std::string, Vector3> kneeHistory;
        auto plantedPose = [&](const Leg& leg, double x, const Matrix4x4& body, std::map<std::string, Matrix4x4>& out) {
            double side, legLength, legRadius;
            bool front;
            legInfo(leg, side, front, legLength, legRadius);
            double sunk = legBuckle * (front ? sinkFront(x) : sinkRear(x));
            Vector3 target = boneEnd(leg.lower) + (front ? forward * (0.3 * legLength) : forward * (0.18 * legLength) + right * (side * 0.1 * legLength)) * sunk;
            Vector3 hint = body.transformVector(forward) * bendHintFor(leg, front, legLength);
            poseTwoBoneLeg(rigStructure, boneIdx, leg.upper, leg.lower, leg.foot, body, target, hint, true, out);
        };
        // The lying pose in the rest frame, by joint angles.
        auto lyingPose = [&](const Leg& leg) {
            double side, legLength, legRadius;
            bool front;
            legInfo(leg, side, front, legLength, legRadius);
            bool upperSide = side != fallSide;
            double backCurl = 1.0 - endsOnSide;
            Vector3 hipRest = bonePos(leg.upper);
            Vector3 kneeRest = bonePos(leg.lower);
            Vector3 ankleRest = boneEnd(leg.lower);
            double swing = legSpread * (front ? 0.45 - 0.2 * backCurl : -0.35 + 0.25 * backCurl)
                + (upperSide ? (front ? 0.15 : -0.12) : 0.0);
            double spread = legSpread * 0.3 * backCurl;
            double droop = (upperSide ? 0.3 : 0.0) * endsOnSide;
            double kneeBend = 0.45 + 0.4 * backCurl;
            Vector3 shinDir = (ankleRest - kneeRest).normalized();
            Vector3 kneePoint = kneeRest - (hipRest + ankleRest) * 0.5;
            kneePoint = kneePoint - shinDir * Vector3::dotProduct(kneePoint, shinDir);
            Vector3 kneeDir = kneePoint.length() > 0.03 * legLength ? kneePoint.normalized() : forward * bendHintFor(leg, front, legLength);
            // Turning the shin about cross(kneeDir, shin) moves the foot away from the knee's
            // side: the joint bends further.
            Vector3 hinge = Vector3::crossProduct(kneeDir, shinDir);
            hinge = hinge.lengthSquared() > 1e-12 ? hinge.normalized() : right;
            Matrix4x4 thigh = rotationAbout(hipRest, forward, -fallSide * droop);
            thigh *= rotationAbout(hipRest, forward, -side * spread);
            thigh *= rotationAbout(hipRest, right, swing);
            Vector3 knee = thigh.transformPoint(kneeRest);
            Matrix4x4 shin = rotationAbout(knee, thigh.transformVector(hinge), kneeBend);
            shin *= thigh;
            return LegFrame { knee, shin.transformPoint(ankleRest) };
        };
        std::map<std::string, LegFrame> legFrom, legTo;
        {
            Matrix4x4 body = bodyAt(legsMoveFrom);
            Matrix4x4 toLocal = body.inverted();
            std::map<std::string, Matrix4x4> posed = rest;
            for (const auto& leg : legs) {
                plantedPose(leg, legsMoveFrom, body, posed);
                legFrom[leg.upper] = LegFrame { toLocal.transformPoint(posed[leg.lower].transformPoint(Vector3())),
                    toLocal.transformPoint(posed[leg.lower].transformPoint(Vector3(0.0, 0.0, (boneEnd(leg.lower) - bonePos(leg.lower)).length()))) };
                legTo[leg.upper] = lyingPose(leg);
            }
        }

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);
        std::map<std::string, Vector3> groundDirections;

        for (int frame = 0; frame < frameCount; ++frame) {
            double t = static_cast<double>(frame) / static_cast<double>(frameCount - 1);
            double x = t * speed;
            double blow = x < 0.12 ? std::pow(std::sin(Math::Pi * x / 0.12), 2.0) : 0.0;
            double headDown = smoothstep((x - 0.1) / 0.32);
            double legsOut = smoothstep((x - rollStart + 0.04) / (rollLand - rollStart + 0.12));
            double limp = smoothstep((x - 0.55) / 0.35);
            double landWobble = impactWobble(x - rollLand - 0.03, 0.18, 0.08);

            Matrix4x4 body = bodyAt(x);

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
            for (const char* name : { "Pelvis", "Spine", "Chest" })
                apply(name, body);

            // Neck and head: the head jerks up with the blow, sinks to the ground as the body
            // goes down, is carried over by the roll and flops onto the ground after it.
            Vector3 neckJoint = body.transformPoint(bonePos("Neck"));
            Vector3 neckDir = body.transformVector(boneEnd("Neck") - bonePos("Neck"));
            Vector3 bodyForward = body.transformVector(forward);
            Vector3 alongGround = Vector3(bodyForward.x(), 0.0, bodyForward.z());
            if (alongGround.lengthSquared() < 1e-12)
                alongGround = forward;
            alongGround = alongGround.normalized() - up * 0.3;
            Matrix4x4 neck = turnAbout(neckJoint, neckDir, alongGround, headDrop * headDown);
            neck *= rotationAbout(neckJoint, body.transformVector(right), 0.25 * blow);
            neck *= body;
            Matrix4x4 flop = rotationAbout(neckJoint, bodyForward, -fallSide * 0.25 * landWobble * headDrop);
            flop *= neck;
            neck = flop;
            apply("Neck", neck);
            Vector3 headJoint = neck.transformPoint(bonePos("Head"));
            Vector3 headDir = neck.transformVector(boneEnd("Head") - bonePos("Head"));
            Vector3 headFlat = Vector3(headDir.x(), 0.0, headDir.z());
            if (headFlat.lengthSquared() < 1e-12)
                headFlat = alongGround;
            Matrix4x4 head = turnAbout(headJoint, headDir, headFlat.normalized() - up * 0.15, headDrop * headDown);
            head *= neck;
            apply("Head", head);
            if (boneIdx.count("Jaw")) {
                Matrix4x4 jaw = rotationAbout(head.transformPoint(bonePos("Jaw")), head.transformVector(right), -0.35 * limp);
                jaw *= head;
                apply("Jaw", jaw);
            }

            // Tail: goes limp with the body (the ground clamp lays it on the ground).
            for (const char* name : { "TailBase", "TailMid", "TailTip" })
                apply(name, body);

            // Legs (see legFrom / legTo above).
            for (const auto& leg : legs) {
                if (x <= legsMoveFrom) {
                    plantedPose(leg, x, body, world);
                } else {
                    double side, legLength, legRadius;
                    bool front;
                    legInfo(leg, side, front, legLength, legRadius);
                    const LegFrame& from = legFrom[leg.upper];
                    const LegFrame& to = legTo[leg.upper];
                    // Rolling over onto its back, the legs it rolls over tuck in against the body
                    // (most halfway over) instead of propping it up.
                    double rollShare = std::clamp(roll(x), 0.0, 1.0);
                    Vector3 tuck = (side == fallSide)
                        ? right * (-side * 0.35 * legLength * (1.0 - endsOnSide) * std::sin(Math::Pi * rollShare))
                        : Vector3();
                    Vector3 knee = body.transformPoint(from.knee + (to.knee - from.knee) * legsOut + tuck * 0.5);
                    Vector3 target = body.transformPoint(from.ankle + (to.ankle - from.ankle) * legsOut + tuck);
                    // Feet the body rolls onto are pushed along the ground, not into it.
                    target.setY(std::max(target.y(), groundY + legRadius * legsOut));
                    Vector3 hip = body.transformPoint(bonePos(leg.upper));
                    Vector3 hint = knee - (hip + target) * 0.5;
                    if (hint.lengthSquared() < 1e-12)
                        hint = body.transformVector(forward) * bendHintFor(leg, front, legLength);
                    poseTwoBoneLeg(rigStructure, boneIdx, leg.upper, leg.lower, leg.foot, body, target, hint.normalized(), true, world, &kneeHistory[leg.upper]);
                }
            }

            // Anything else (ears, spikes, eyelids...) follows its nearest moved ancestor.
            for (const auto& bone : rigStructure.bones) {
                if (layers.count(bone.name) || bone.name == "Root" || bone.name.find("Leg") != std::string::npos
                    || bone.name.find("Foot") != std::string::npos)
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
            // Nothing below the floor: legs, neck, head and tail rest on it.
            keepBonesAboveGround(
                rigStructure, boneIdx, inverseBindMatrices, animFrame, groundY, [](const std::string& name) { return name != "Root" && name != "Pelvis" && name != "Spine" && name != "Chest"; }, 0.8, true, &groundDirections);
        }
        return true;
    }

} // namespace quadruped

} // namespace dust3d
