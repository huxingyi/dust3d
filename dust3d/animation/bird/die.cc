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

// Procedural death for the bird rig.
//
// One-shot, ends lying still. On the ground (airborne = 0) the blow jolts the
// bird, its legs fold and it sinks onto its breast, the head going down, then it
// flops over onto its side and lands with a small bounce: the wings drape, the
// legs lie out half bent and the neck and head lie along the ground. In the air
// (airborne = 1) it gives a few failing wing beats, goes limp and falls nose down
// with the wings trailing up, hits the ground breast first and lies with the
// wings spread out flat and the head turned aside (the game drops the model from
// its flying height to land with the impact, at 0.62 of the clip). Every bone is
// kept on or above the ground (using the rig's capsule radii).
//
// Adjustable animation parameters:
//   - collapseSpeedFactor:  how fast the body goes down (> 1 = sooner)
//   - fallSide:             which side it falls onto (1 = its right, -1 = its left)
//   - rollIntensityFactor:  how far it rolls (1 = onto its side; in the air a slight tilt)
//   - wingFlapFactor:       how hard the wings beat before going limp
//   - wingSpreadFactor:     how far the wings splay open lying down
//   - headDropFactor:       how far the neck and head drop
//   - airborne:             0 = standing on the ground, 1 = flying
//   - groundBounce:         how much the body rebounds when it hits the ground

#include <algorithm>
#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/bird/die.h>
#include <dust3d/animation/common.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace bird {

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
            "LeftWingShoulder", "LeftWingElbow", "LeftWingHand",
            "RightWingShoulder", "RightWingElbow", "RightWingHand",
            "LeftUpperLeg", "LeftLowerLeg", "LeftFoot",
            "RightUpperLeg", "RightLowerLeg", "RightFoot"
        };
        if (!validateRequiredBones(boneIdx, requiredBones, sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;

        double speed = std::max(0.3, parameters.getValue("collapseSpeedFactor", 1.0));
        double fallSide = parameters.getValue("fallSide", 1.0) >= 0.0 ? 1.0 : -1.0;
        double rollIntensity = parameters.getValue("rollIntensityFactor", 1.0);
        double wingFlap = parameters.getValue("wingFlapFactor", 1.0);
        double wingSpread = parameters.getValue("wingSpreadFactor", 1.0);
        double headDrop = parameters.getValue("headDropFactor", 1.0);
        bool airborne = parameters.getValue("airborne", 0.0) > 0.5;
        double bounce = 0.35 * std::clamp(parameters.getValue("groundBounce", 0.22), 0.0, 1.0);

        Vector3 up(0.0, 1.0, 0.0);
        Vector3 spine = boneEnd("Chest") - bonePos("Pelvis");
        Vector3 forward(spine.x(), 0.0, spine.z());
        if (forward.lengthSquared() < 1e-10)
            forward = Vector3(0.0, 0.0, 1.0);
        forward.normalize();
        Vector3 right = Vector3::crossProduct(forward, up).normalized();
        double bodyLength = std::max(1e-4, spine.length());

        double groundY = restGroundHeight(rigStructure);
        Vector3 centre = (bonePos("Pelvis") + boneEnd("Chest")) * 0.5;
        double bodyRadius = 0.0;
        for (const char* name : { "Pelvis", "Spine", "Chest" })
            bodyRadius = std::max(bodyRadius, static_cast<double>(rigStructure.bones[boneIdx[name]].capsuleRadius));
        if (bodyRadius < 1e-4)
            bodyRadius = 0.3 * bodyLength;
        double standHeight = centre.y() - groundY;
        // In the air it lands breast first and only tilts a little; on the ground it rolls over.
        double finalRoll = 0.5 * Math::Pi * rollIntensity * (airborne ? 0.22 : 1.0);
        double sternalHeight = std::min(standHeight, 1.05 * bodyRadius);
        double lyingHeight = std::min(standHeight, 1.1 * bodyRadius);
        double legLength = (boneEnd("RightUpperLeg") - bonePos("RightUpperLeg")).length()
            + (boneEnd("RightLowerLeg") - bonePos("RightLowerLeg")).length();

        // Timeline (normalised; collapseSpeedFactor scales it).
        // Ground: blow, legs fold onto the breast (lands 0.36), flop over (lands 0.7).
        // Air: failing wing beats to 0.2, limp fall, breast hits the ground at 0.62.
        const double sinkLand = 0.36;
        const double rollStart = airborne ? 0.2 : 0.42;
        const double land = airborne ? 0.62 : 0.7;
        auto sink = [&](double x) { return fallEnvelope(x - 0.05, sinkLand - 0.05, 0.3 * bounce, 0.08); };
        auto drop = [&](double x) { return fallEnvelope(x - rollStart, land - rollStart, bounce, 0.12); };
        const double dropRatePeak = 2.0 / (land - rollStart);

        std::map<std::string, Matrix4x4> rest = restBoneWorldTransforms(rigStructure);
        double wingFacing[2] = { 0.0, 0.0 };
        Quaternion wingTrail[2], wingSettle[2];

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);
        std::map<std::string, Vector3> groundDirections;

        for (int frame = 0; frame < frameCount; ++frame) {
            double t = static_cast<double>(frame) / static_cast<double>(frameCount - 1);
            double x = t * speed;
            double blow = x < 0.12 ? std::pow(std::sin(Math::Pi * x / 0.12), 2.0) : 0.0;
            double sunk = airborne ? 0.0 : sink(x);
            double dropped = drop(x);
            // How fast it is falling; after the impact the trailing wings and head ease back.
            double dropRate = x < land ? normalisedRate(drop, x, dropRatePeak) : 1.0 - smoothstep((x - land) / 0.12);
            double headDown = airborne ? smoothstep((x - 0.08) / 0.3) : smoothstep((x - 0.1) / 0.32);
            double limp = smoothstep((x - land + 0.08) / 0.3);
            double legsOut = smoothstep((x - rollStart + 0.04) / (land - rollStart + 0.12));
            double landWobble = impactWobble(x - land - 0.02, 0.16, 0.07);
            // Failing wing beats: two or three, weaker each time, gone by the limp fall.
            double beatsFade = airborne ? std::max(0.0, 1.0 - x / 0.24) : 0.0;
            double flutter = wingFlap * 0.55 * beatsFade * std::sin(2.0 * Math::Pi * x / 0.1);

            double height = standHeight - (standHeight - sternalHeight) * sunk - (sternalHeight - lyingHeight) * dropped;
            if (airborne)
                height = standHeight - (standHeight - lyingHeight) * dropped;
            // Falling from the air the nose goes down, and levels as the breast lands.
            double pitch = airborne ? 0.5 * dropRate - 0.06 * dropped : 0.18 * sunk * (1.0 - dropped);
            Matrix4x4 body;
            body.translate(centre + up * (height - standHeight) + right * (fallSide * bodyRadius * 0.5 * std::abs(std::sin(finalRoll)) * dropped));
            // A positive rotation about `forward` lowers the right side; the blow jolts it the
            // other way first.
            body.rotate(forward, fallSide * (finalRoll * dropped - 0.08 * blow));
            body.rotate(right, -pitch);
            body.translate(Vector3() - centre);

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
            for (const char* name : { "Pelvis", "Spine", "Chest", "TailBase", "TailFeathers" })
                apply(name, body);

            // Neck and head: thrown up by the blow, sag, then lie along the ground, the head
            // turned aside; they flop down a moment after the body lands.
            Vector3 bodyForward = body.transformVector(forward);
            Vector3 neckJoint = body.transformPoint(bonePos("Neck"));
            Vector3 neckDir = body.transformVector(boneEnd("Neck") - bonePos("Neck"));
            Vector3 flat(bodyForward.x(), 0.0, bodyForward.z());
            if (flat.lengthSquared() < 1e-12)
                flat = forward;
            flat = flat.normalized() + right * (fallSide * (airborne ? 0.6 : 0.2) * limp);
            Matrix4x4 neck = turnAbout(neckJoint, neckDir, flat.normalized() - up * 0.3, headDrop * headDown);
            neck *= rotationAbout(neckJoint, body.transformVector(right), 0.3 * blow - 0.35 * dropRate);
            neck *= body;
            Matrix4x4 flop = rotationAbout(neckJoint, bodyForward, -fallSide * 0.25 * landWobble * headDrop);
            flop *= neck;
            neck = flop;
            apply("Neck", neck);
            Vector3 headJoint = neck.transformPoint(bonePos("Head"));
            Matrix4x4 headRoll = rotationAbout(headJoint, neck.transformVector(boneEnd("Head") - bonePos("Head")), fallSide * 0.9 * headDrop * limp);
            headRoll *= neck;
            apply("Head", headRoll);
            apply("Beak", headRoll);

            // Wings: beat weakly, trail up while falling, then lie out (spread flat on the ground
            // for a flyer, draped for a ground bird), slapping down just after the body lands.
            for (const char* side : { "Left", "Right" }) {
                std::string shoulderName = std::string(side) + "WingShoulder";
                std::string handName = std::string(side) + "WingHand";
                Vector3 shoulder = body.transformPoint(bonePos(shoulderName));
                Vector3 restWing = boneEnd(handName) - bonePos(shoulderName);
                double wingSide = Vector3::dotProduct(restWing, right) >= 0.0 ? 1.0 : -1.0;
                bool lower = wingSide == fallSide;
                Matrix4x4 flap = rotationAbout(shoulder, bodyForward, -wingSide * flutter);
                flap *= body;
                Vector3 wingDir = flap.transformVector(restWing);
                // The contact side is fixed in world space. Projecting the rolling
                // body's right axis flips its sign as the corpse passes 90 degrees.
                Vector3 outward = right * wingSide;
                Vector3 back(-bodyForward.x(), 0.0, -bodyForward.z());
                back = back.lengthSquared() > 1e-12 ? back.normalized() : forward * -1.0;
                Vector3 trailing = outward * 0.55 + up * 0.85;
                Vector3 lying = airborne
                    ? outward * (0.7 + 0.3 * wingSpread) + back * (lower ? 0.25 : 0.45) - up * 0.05
                    : outward * (0.4 + 0.6 * wingSpread) + back * 0.3 - up * 0.6;
                Matrix4x4 trail = turnAbout(shoulder, wingDir, trailing, (airborne ? 0.8 : 0.3) * dropRate, &wingTrail[lower ? 1 : 0]);
                Vector3 trailedDir = trail.transformVector(wingDir);
                double settle = std::clamp(limp + 0.35 * landWobble, 0.0, 1.0);
                Matrix4x4 wing = turnAbout(shoulder, trailedDir, lying, settle, &wingSettle[lower ? 1 : 0]);
                wing *= trail;
                wing *= flap;
                // Lying spread out, the wing's flat face lies on the ground.
                if (airborne) {
                    Matrix4x4 flatten = rollToFaceUp(wing, shoulder, restWing, up, settle, &wingFacing[lower ? 1 : 0]);
                    flatten *= wing;
                    wing = flatten;
                }
                for (const char* part : { "WingShoulder", "WingElbow", "WingHand" })
                    apply(std::string(side) + part, wing);
            }

            // Legs: planted and folding as it sinks (on the ground) or tucked back (in the air),
            // then lying out behind the belly, half bent, the upper one resting on the lower.
            Vector3 belly = body.transformVector(Vector3() - up).normalized();
            for (const char* side : { "Left", "Right" }) {
                std::string upper = std::string(side) + "UpperLeg";
                std::string lower = std::string(side) + "LowerLeg";
                std::string foot = std::string(side) + "Foot";
                Vector3 ankleRest = boneEnd(lower);
                double legRadius = rigStructure.bones[boneIdx[upper]].capsuleRadius;
                double legSide = Vector3::dotProduct(bonePos(upper) - centre, right) >= 0.0 ? 1.0 : -1.0;
                bool upperSide = legSide != fallSide;
                Vector3 hip = body.transformPoint(bonePos(upper));
                Vector3 tucked = hip - bodyForward * (0.55 * legLength) + belly * (0.25 * legLength);
                Vector3 lying = hip + belly * (0.4 * legLength) - bodyForward * ((upperSide ? 0.62 : 0.5) * legLength);
                double onSide = smoothstep((0.85 - std::abs(belly.y())) / 0.5);
                double restingHeight = groundY + (upperSide ? 2.4 : 1.0) * legRadius;
                lying.setY(lying.y() + (restingHeight - lying.y()) * onSide);
                Vector3 start = airborne ? body.transformPoint(ankleRest) : ankleRest;
                Vector3 mid = airborne ? start + (tucked - start) * smoothstep(x / 0.25) : start;
                Vector3 target = mid + (lying - mid) * legsOut;
                // Never aimed into the ground (the leg would swing through straight down).
                target.setY(std::max(target.y(), groundY + legRadius));
                // A folded leg keeps at least half its length between hip and ankle: folded
                // tighter the knee has no clear way to point and can flip over.
                Vector3 reachVector = target - hip;
                if (reachVector.length() < 0.5 * legLength && reachVector.lengthSquared() > 1e-12)
                    target = hip + reachVector.normalized() * (0.5 * legLength);
                // The joint between the two leg bones bends the way it does at rest (backwards
                // in birds), as an explicit direction.
                double restBend = Vector3::dotProduct(bonePos(lower) - (bonePos(upper) + ankleRest) * 0.5, forward);
                double bendSign = std::abs(restBend) > 0.03 * legLength ? (restBend > 0.0 ? 1.0 : -1.0) : -1.0;
                poseTwoBoneLeg(rigStructure, boneIdx, upper, lower, foot, body, target, bodyForward * bendSign, true, world);
            }

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
            keepBonesAboveGround(
                rigStructure, boneIdx, inverseBindMatrices, animFrame, groundY, [](const std::string& name) { return name != "Root" && name != "Pelvis" && name != "Spine" && name != "Chest"; }, 0.8, true, &groundDirections);
        }
        return true;
    }

} // namespace bird

} // namespace dust3d
