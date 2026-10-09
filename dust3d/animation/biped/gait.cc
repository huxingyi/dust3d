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

// Shared gait engine for the biped walk and run loops.
//
// Everything is locked to the foot contacts, so the parts of the body move the
// way they do in a real gait instead of as unrelated sine waves:
//
//   - One cycle is one step of each foot. The left foot touches down at t = 0,
//     the right at t = 0.5. A foot is on the ground for `stanceFraction` of the
//     cycle; when that is under one half (run) there are two flight phases.
//   - A planted foot slides back under the body at a constant speed, the same
//     speed the clip reports as its movement speed, so a character moved by the
//     clip's speed keeps its feet still on the ground.
//   - The foot rolls: the heel touches down with the toes up, the foot goes flat,
//     the heel lifts and the foot pivots about the toes to push off. The leg is
//     solved with two-bone IK to the ankle; the foot bone is posed on its own.
//   - The swing leaves with the ground speed it had, lifts early (the knee folds,
//     in a run the heel kicks up behind), reaches forward and pulls back slightly
//     before the touch-down.
//   - The hips ride over the standing leg: in a walk they are highest at
//     mid-stance (vaulting over a straight leg), in a run lowest at mid-stance
//     (the leg absorbs the landing) and highest in the flight. They shift over
//     the standing foot, turn so the reaching leg's hip goes forward and drop a
//     little on the side of the swinging leg. How low the hips sit is solved so
//     the legs never snap straight.
//   - The chest turns against the pelvis, the arms swing with the opposite leg
//     with the elbow bending more as the arm comes forward and the forearm
//     trailing, and the head stays level and looks ahead.
//   - Every bone the gait does not drive (jaw, eyelids, held props, ...) rides
//     with its parent; hair and capes are simulated.

#include <algorithm>
#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/biped/gait.h>
#include <dust3d/animation/biped/pose.h>
#include <dust3d/animation/common.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace biped {

    namespace {

        double fract(double x)
        {
            return x - std::floor(x);
        }

        Matrix4x4 translation(const Vector3& v)
        {
            Matrix4x4 m;
            m.translate(v);
            return m;
        }

        // Product a * b (b applied first).
        Matrix4x4 then(const Matrix4x4& a, const Matrix4x4& b)
        {
            Matrix4x4 m = a;
            m *= b;
            return m;
        }

        double signOf(double v)
        {
            return v >= 0.0 ? 1.0 : -1.0;
        }

    } // namespace

    bool locomote(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters,
        const GaitStyle& style)
    {
        using namespace animation;

        int frameCount = std::max(2, static_cast<int>(parameters.getValue("frameCount", 30.0)));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 1.0));
        if (durationSeconds <= 0.0f)
            durationSeconds = 1.0f;

        auto boneIdx = buildBoneIndexMap(rigStructure);
        auto bonePos = [&](const std::string& name) -> Vector3 {
            return getBonePos(rigStructure, boneIdx, name);
        };
        auto boneEnd = [&](const std::string& name) -> Vector3 {
            return getBoneEnd(rigStructure, boneIdx, name);
        };

        static const char* requiredBones[] = {
            "Root", "Hips", "Spine", "Chest", "Neck", "Head",
            "LeftUpperLeg", "LeftLowerLeg", "LeftFoot",
            "RightUpperLeg", "RightLowerLeg", "RightFoot",
            "LeftShoulder", "LeftUpperArm", "LeftLowerArm", "LeftHand",
            "RightShoulder", "RightUpperArm", "RightLowerArm", "RightHand"
        };
        if (!validateRequiredBones(boneIdx, requiredBones, sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;

        // ===================================================================
        // Frame of the body
        // ===================================================================
        const Vector3 up(0.0, 1.0, 0.0);
        Vector3 hipsPos = bonePos("Hips");
        // Forward: from the hips toward the toes (the same direction the clip's movement uses).
        Vector3 toesMid = (boneEnd("LeftFoot") + boneEnd("RightFoot")) * 0.5;
        Vector3 forward(toesMid.x() - hipsPos.x(), 0.0, toesMid.z() - hipsPos.z());
        if (forward.lengthSquared() < 1e-10)
            forward = Vector3(0.0, 0.0, 1.0);
        forward.normalize();
        // The creature's right; a positive turn about it lifts the front (pitches back).
        Vector3 side = Vector3::crossProduct(forward, up).normalized();
        Matrix4x4 travelRotation;
        travelRotation.rotate(up, parameters.getValue("travelAngleDegrees", 0.0) * Math::Pi / 180.0);
        Vector3 travel = travelRotation.transformVector(forward);

        // ===================================================================
        // Legs
        // ===================================================================
        struct Leg {
            std::string upper, lower, foot;
            double phase; // when in the cycle this foot touches down
            Vector3 hipRest, ankleRest, toeRest, heelPivotRest, toePivotRest;
            double reach; // farthest the IK may put the ankle from the hip
        };
        double groundY = restGroundHeight(rigStructure);
        auto makeLeg = [&](const char* prefix, double phase) {
            Leg leg;
            leg.upper = std::string(prefix) + "UpperLeg";
            leg.lower = std::string(prefix) + "LowerLeg";
            leg.foot = std::string(prefix) + "Foot";
            leg.phase = phase;
            leg.hipRest = bonePos(leg.upper);
            leg.ankleRest = boneEnd(leg.lower);
            leg.toeRest = boneEnd(leg.foot);
            // The sole reaches a little past the bones at both ends: pivot there, so neither
            // the heel nor the toes dip into the ground as the foot rolls.
            Vector3 footAlong(leg.toeRest.x() - leg.ankleRest.x(), 0.0, leg.toeRest.z() - leg.ankleRest.z());
            leg.heelPivotRest = Vector3(leg.ankleRest.x(), groundY, leg.ankleRest.z()) - footAlong * 0.1;
            leg.toePivotRest = Vector3(leg.toeRest.x(), groundY, leg.toeRest.z()) + footAlong * 0.1;
            double chain = (bonePos(leg.lower) - leg.hipRest).length() + (leg.ankleRest - bonePos(leg.lower)).length();
            double restDistance = (leg.ankleRest - leg.hipRest).length();
            // Never quite straight (no knee pop); a leg modelled bent keeps most of its bend.
            leg.reach = std::min(0.993 * chain, std::max(restDistance + 0.01 * chain, 0.9 * chain));
            return leg;
        };
        Leg legs[2] = { makeLeg("Left", 0.0), makeLeg("Right", 0.5) };

        double legLength = 0.5 * ((legs[0].ankleRest - legs[0].hipRest).length() + (legs[1].ankleRest - legs[1].hipRest).length());
        if (legLength < 1e-6)
            return false;

        // Signs that make the motions independent of how the rig is laid out.
        Vector3 leftHipOffset = legs[0].hipRest - hipsPos;
        // Toward the left leg.
        Vector3 toLeft = Vector3::dotProduct(legs[0].hipRest - legs[1].hipRest, side) >= 0.0 ? side : -side;
        // A positive yaw brings the left hip forward when yawSign is +1.
        double yawSign = signOf(Vector3::dotProduct(Vector3::crossProduct(up, leftHipOffset), forward));
        // A positive roll about forward lifts the left hip when listSign is +1.
        double listSign = signOf(Vector3::dotProduct(Vector3::crossProduct(forward, toLeft), up));

        const double beta = std::clamp(style.stanceFraction, 0.15, 0.85);
        const double span = style.footSpan * legLength;
        const double lift = style.liftHeight * legLength;
        const double liftGamma = std::log(0.5) / std::log(std::clamp(style.liftPeakAt, 0.15, 0.85));

        // Foot placement for one leg at its own phase u in [0, 1) (0 = touch-down).
        // Returns the rigid motion of the foot from its rest pose; the ankle target is
        // that motion applied to the rest ankle.
        auto footMotion = [&](const Leg& leg, double legT) -> Matrix4x4 {
            double x = 0.0;
            double y = 0.0;
            double pitch = 0.0;
            if (legT < beta) {
                double s = legT / beta;
                x = span * (style.landingFraction - s);
                if (s < style.heelRollEnd)
                    pitch = style.heelStrikePitch * (1.0 - smoothstep(s / style.heelRollEnd));
                else if (s > style.heelRiseStart) {
                    double r = (s - style.heelRiseStart) / (1.0 - style.heelRiseStart);
                    pitch = -style.toeOffPitch * r * r * (3.0 - 2.0 * r) * (0.35 + 0.65 * r);
                }
            } else {
                double u = (legT - beta) / (1.0 - beta);
                double p0 = span * (style.landingFraction - 1.0);
                // Ground speed in this parameterisation, kept partly at both ends.
                double m = -style.swingRetraction * span * (1.0 - beta) / beta;
                x = p0 + span * (3.0 * u * u - 2.0 * u * u * u) + m * (u - 3.0 * u * u + 2.0 * u * u * u);
                y = lift * std::sin(Math::Pi * std::pow(u, liftGamma));
                // The toes come up through the swing, ready for the heel strike.
                double w = smoothstep(u / 0.7);
                pitch = -style.toeOffPitch * (1.0 - w) + style.heelStrikePitch * w;
            }
            Vector3 offset = travel * x + up * y;
            Vector3 pivot = (pitch >= 0.0 ? leg.heelPivotRest : leg.toePivotRest) + offset;
            return then(rotationAbout(pivot, side, pitch), translation(offset));
        };

        // ===================================================================
        // Timing
        // ===================================================================
        const double cycles = std::max(1.0, std::round(parameters.getValue("gaitSpeedFactor", 1.0)));
        const double cycleSeconds = durationSeconds / cycles;
        animation::addGaitMarkers(animationClip, cycles, durationSeconds, "LeftFoot", 0.0, beta);
        animation::addGaitMarkers(animationClip, cycles, durationSeconds, "RightFoot", -0.5, beta - 0.5);

        // Pelvis motion at cycle phase t. The height comes from heightAt (zero while it is
        // being solved).
        std::vector<double> heightTable;
        auto heightAt = [&](double t) -> double {
            if (heightTable.empty())
                return 0.0;
            double x = fract(t) * heightTable.size();
            size_t i = static_cast<size_t>(x) % heightTable.size();
            size_t j = (i + 1) % heightTable.size();
            double f = x - std::floor(x);
            return heightTable[i] * (1.0 - f) + heightTable[j] * f;
        };
        auto pelvisAt = [&](double t) -> Matrix4x4 {
            double midStance = 0.5 * beta;
            double sway = style.sway * legLength * std::cos(2.0 * Math::Pi * (t - midStance - 0.04));
            double yaw = yawSign * style.yaw * std::cos(2.0 * Math::Pi * t);
            double list = listSign * style.list * std::cos(2.0 * Math::Pi * (t - 0.4 * beta));
            double lean = style.lean + style.leanPulse * std::cos(4.0 * Math::Pi * (t - beta * 0.8));
            Matrix4x4 m = translation(up * heightAt(t) + toLeft * sway);
            m *= rotationAbout(hipsPos, up, yaw);
            m *= rotationAbout(hipsPos, forward, list);
            m *= rotationAbout(hipsPos, side, -lean);
            return m;
        };

        // Hip height through the cycle. First, the highest the hips can be at each moment with
        // no leg stretched past its reach; in a walk that is the inverted pendulum of the
        // standing leg (high at mid-stance, low with both feet down).
        {
            const int samples = 240;
            std::vector<double> highest(samples);
            for (int i = 0; i < samples; ++i) {
                double t = static_cast<double>(i) / samples;
                Matrix4x4 pelvis = pelvisAt(t);
                double h = 1e9;
                for (const auto& leg : legs) {
                    Vector3 hip = pelvis.transformPoint(leg.hipRest);
                    Vector3 ankle = footMotion(leg, fract(t - leg.phase)).transformPoint(leg.ankleRest);
                    Vector3 d = hip - ankle;
                    double vertical = Vector3::dotProduct(d, up);
                    Vector3 horizontal = d - up * vertical;
                    double h2 = horizontal.lengthSquared();
                    double r2 = leg.reach * leg.reach;
                    h = std::min(h, (h2 < r2 ? std::sqrt(r2 - h2) : 0.0) - vertical);
                }
                highest[i] = h;
            }
            std::vector<double> target(samples);
            if (style.running) {
                // Down as the leg takes the landing at mid-stance, up in the flight.
                for (int i = 0; i < samples; ++i) {
                    double t = static_cast<double>(i) / samples;
                    target[i] = -style.bob * legLength * std::cos(4.0 * Math::Pi * (t - 0.5 * beta));
                }
            } else {
                // Ride the pendulum, rounded where the legs change over, plus any cartoon bounce.
                // A running minimum followed by a running mean of the same width stays under
                // the limit everywhere.
                const int halfWindow = std::max(1, samples / 40);
                std::vector<double> eroded(samples);
                for (int i = 0; i < samples; ++i) {
                    double m = highest[i];
                    for (int k = -halfWindow; k <= halfWindow; ++k)
                        m = std::min(m, highest[(i + k + samples) % samples]);
                    eroded[i] = m;
                }
                std::vector<double> smooth(samples);
                for (int i = 0; i < samples; ++i) {
                    double sum = 0.0;
                    for (int k = -halfWindow; k <= halfWindow; ++k)
                        sum += eroded[(i + k + samples) % samples];
                    smooth[i] = sum / (2 * halfWindow + 1);
                }
                double lowest = *std::min_element(smooth.begin(), smooth.end());
                for (int i = 0; i < samples; ++i) {
                    double t = static_cast<double>(i) / samples;
                    target[i] = lowest + std::clamp(style.pendulum, 0.0, 1.0) * (smooth[i] - lowest)
                        + style.bob * legLength * std::cos(4.0 * Math::Pi * (t - 0.5 * beta));
                }
            }
            // Lower it until the legs fit everywhere, then add the extra knee bend.
            double over = -1e9;
            for (int i = 0; i < samples; ++i)
                over = std::max(over, target[i] - highest[i]);
            for (int i = 0; i < samples; ++i)
                target[i] -= over + style.extraCrouch * legLength;
            heightTable = target;
        }

        // ===================================================================
        // Bone hierarchy (for the bones the gait does not drive)
        // ===================================================================
        std::map<std::string, Matrix4x4> rest = restBoneWorldTransforms(rigStructure);
        std::vector<std::pair<int, std::string>> byDepth;
        for (const auto& bone : rigStructure.bones) {
            int depth = 0;
            for (std::string p = bone.parent; !p.empty() && depth < 64; ++depth) {
                auto it = boneIdx.find(p);
                if (it == boneIdx.end())
                    break;
                p = rigStructure.bones[it->second].parent;
            }
            byDepth.push_back({ depth, bone.name });
        }
        std::stable_sort(byDepth.begin(), byDepth.end(),
            [](const std::pair<int, std::string>& a, const std::pair<int, std::string>& b) { return a.first < b.first; });
        std::map<std::string, std::string> parentOf;
        for (const auto& bone : rigStructure.bones)
            parentOf[bone.name] = bone.parent;

        // Arm rest data
        struct Arm {
            std::string shoulder, upper, lower, hand;
            double phase; // when this arm is furthest forward (with the opposite leg)
            double restElbow;
        };
        auto makeArm = [&](const char* prefix, double phase) {
            Arm arm;
            arm.shoulder = std::string(prefix) + "Shoulder";
            arm.upper = std::string(prefix) + "UpperArm";
            arm.lower = std::string(prefix) + "LowerArm";
            arm.hand = std::string(prefix) + "Hand";
            arm.phase = phase;
            Vector3 a = boneEnd(arm.upper) - bonePos(arm.upper);
            Vector3 b = boneEnd(arm.lower) - bonePos(arm.lower);
            a = relaxedArm(rigStructure, boneIdx, arm.upper, parameters).transformVector(a);
            b = relaxedForearm(rigStructure, boneIdx, prefix, parameters).transformVector(b);
            arm.restElbow = (a.isZero() || b.isZero()) ? 0.0 : Vector3::angle(a, b);
            return arm;
        };
        // The right arm swings forward with the left leg, the left arm with the right leg.
        Arm arms[2] = { makeArm("Left", 0.5), makeArm("Right", 0.0) };

        static const char* tailBones[] = { "TailBase", "TailMid", "TailTip" };

        // Hair and cape
        const std::vector<std::string> hairBoneNames = { "HairBack1", "HairBack2", "HairBack3" };
        HairChainSimulator hairSim;
        if (boneIdx.count("HairBack1"))
            hairSim.initialize(rigStructure, boneIdx, hairBoneNames, rest["Head"], style.hairStiffness, style.hairDamping, 1.0);
        CapeGridSimulator capeSim;
        if (boneIdx.count("CenterCape1"))
            capeSim.initialize(rigStructure, boneIdx, rest["Chest"], 0.08, 0.85, 1.2, 0.15);
        double dt = durationSeconds / frameCount;

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);
        // A planted foot moves back at the ground speed: moving the character at this speed
        // keeps it still on the ground.
        animationClip.movementSpeed = static_cast<float>(span / (beta * cycleSeconds));
        animationClip.movementDirectionX = static_cast<float>(travel.x());
        animationClip.movementDirectionZ = static_cast<float>(travel.z());

        // Pass 0 settles the hair and cape so the loop is seamless; pass 1 records.
        for (int pass = 0; pass < 2; ++pass) {
            for (int frame = 0; frame < frameCount; ++frame) {
                double tNormalized = static_cast<double>(frame) / static_cast<double>(frameCount);
                double t = fract(tNormalized * cycles);
                double cyc = 2.0 * Math::Pi * t;

                std::map<std::string, Matrix4x4> layers;
                std::map<std::string, Matrix4x4> world;
                auto apply = [&](const std::string& name, const Matrix4x4& layer) {
                    if (!boneIdx.count(name))
                        return;
                    layers[name] = layer;
                    world[name] = then(layer, rest[name]);
                };

                // ---------------------------------------------------------
                // Pelvis, spine, head
                // ---------------------------------------------------------
                Matrix4x4 pelvis = pelvisAt(t);
                apply("Root", pelvis);
                apply("Hips", pelvis);

                double pelvisYaw = yawSign * style.yaw * std::cos(cyc);
                double pelvisList = listSign * style.list * std::cos(2.0 * Math::Pi * (t - 0.4 * beta));
                // The thorax turns against the pelvis (with the arms), the spine takes the list
                // out so the shoulders stay level.
                double counter = style.spineCounter;
                Vector3 spineJoint = pelvis.transformPoint(bonePos("Spine"));
                Matrix4x4 spine = rotationAbout(spineJoint, up, -pelvisYaw * 0.8 * counter);
                spine *= rotationAbout(spineJoint, forward, -pelvisList * 0.7);
                spine *= pelvis;
                apply("Spine", spine);
                Vector3 chestJoint = spine.transformPoint(bonePos("Chest"));
                Matrix4x4 chest = rotationAbout(chestJoint, up, -pelvisYaw * 0.9 * counter);
                chest *= rotationAbout(chestJoint, forward, -pelvisList * 0.3);
                // Running: the chest rocks a little with each push-off.
                if (style.running)
                    chest *= rotationAbout(chestJoint, side, -0.02 * std::cos(4.0 * Math::Pi * (t - beta)));
                chest *= spine;
                apply("Chest", chest);
                double chestYaw = pelvisYaw * (1.0 - 1.7 * counter);

                // Head: undo most of the turn and of the lean so it looks ahead, nod against the bob.
                double hs = std::clamp(style.headStabilize, 0.0, 1.0);
                double bobPhase = std::cos(4.0 * Math::Pi * (t - 0.5 * beta));
                double nod = (style.running ? 1.0 : -1.0) * bobPhase * 0.025 * hs;
                Vector3 neckJoint = chest.transformPoint(bonePos("Neck"));
                Matrix4x4 neck = rotationAbout(neckJoint, up, -chestYaw * hs * 0.5);
                neck *= rotationAbout(neckJoint, side, style.lean * hs * 0.4 + nod * 0.5);
                neck *= chest;
                apply("Neck", neck);
                Vector3 headJoint = neck.transformPoint(bonePos("Head"));
                Matrix4x4 head = rotationAbout(headJoint, up, -chestYaw * hs * 0.5);
                head *= rotationAbout(headJoint, side, style.lean * hs * 0.4 + nod * 0.5);
                head *= neck;
                apply("Head", head);

                // ---------------------------------------------------------
                // Tail: rides the pelvis, sways against it, the wave travels to the tip
                // ---------------------------------------------------------
                {
                    Matrix4x4 parent = pelvis;
                    int ti = 0;
                    for (const char* name : tailBones) {
                        if (!boneIdx.count(name))
                            continue;
                        Vector3 joint = parent.transformPoint(bonePos(name));
                        double attenuation = 1.0 + 0.35 * ti;
                        double sway = style.tailSway * attenuation * std::sin(cyc - 0.9 * ti + 0.4) - pelvisYaw * 0.6;
                        double bounce = style.tailBounce * attenuation * std::cos(4.0 * Math::Pi * (t - 0.5 * beta) - 0.9 * (ti + 1));
                        Matrix4x4 m = rotationAbout(joint, up, sway);
                        m *= rotationAbout(joint, side, bounce);
                        m *= parent;
                        apply(name, m);
                        parent = m;
                        ++ti;
                    }
                }

                // ---------------------------------------------------------
                // Legs
                // ---------------------------------------------------------
                for (const auto& leg : legs) {
                    Matrix4x4 motion = footMotion(leg, fract(t - leg.phase));
                    Vector3 ankleTarget = motion.transformPoint(leg.ankleRest);
                    // The knee points forward (and a little out with the pelvis turn).
                    Vector3 bend = pelvis.transformVector(forward);
                    poseTwoBoneLeg(rigStructure, boneIdx, leg.upper, leg.lower, leg.foot, pelvis, ankleTarget, bend, false, world);
                    // The foot takes its own roll; if the leg fell short, it follows the ankle.
                    Matrix4x4 shin = world[leg.lower];
                    Vector3 ankle = shin.transformPoint(Vector3(0.0, 0.0, (leg.ankleRest - bonePos(leg.lower)).length()));
                    Matrix4x4 footLayer = then(translation(ankle - ankleTarget), motion);
                    layers[leg.foot] = footLayer;
                    world[leg.foot] = then(footLayer, rest[leg.foot]);
                    layers[leg.upper] = then(world[leg.upper], rest[leg.upper].inverted());
                    layers[leg.lower] = then(world[leg.lower], rest[leg.lower].inverted());
                }

                // ---------------------------------------------------------
                // Arms: swing with the opposite leg, elbows bend as the arm comes forward
                // ---------------------------------------------------------
                for (const auto& arm : arms) {
                    double armPhase = 2.0 * Math::Pi * (t - arm.phase - style.armLag);
                    double swing = style.armForwardBias + style.armSwing * std::cos(armPhase);
                    double forwardness = 0.5 + 0.5 * std::cos(armPhase - 2.0 * Math::Pi * style.forearmLag);

                    // Shoulder: rides the chest and rolls forward a touch with the arm.
                    Vector3 shoulderJoint = chest.transformPoint(bonePos(arm.shoulder));
                    Vector3 outward = chest.transformVector(boneEnd(arm.shoulder) - bonePos(arm.shoulder));
                    Matrix4x4 shoulder;
                    Vector3 shrugAxis = Vector3::crossProduct(outward, forward);
                    if (shrugAxis.lengthSquared() > 1e-12)
                        shoulder = rotationAbout(shoulderJoint, shrugAxis.normalized(), style.shoulderShrug * std::cos(armPhase));
                    shoulder *= chest;
                    apply(arm.shoulder, shoulder);

                    // Upper arm swings in the plane of its own direction and forward, so A-pose
                    // and T-pose rigs both work.
                    Vector3 joint = shoulder.transformPoint(bonePos(arm.upper));
                    Matrix4x4 neutral = composePose(shoulder, relaxedArm(rigStructure, boneIdx, arm.upper, parameters));
                    Vector3 dir = neutral.transformVector(boneEnd(arm.upper) - bonePos(arm.upper));
                    Vector3 bodyForward = chest.transformVector(forward);
                    Vector3 axis = Vector3::crossProduct(dir, bodyForward);
                    if (axis.lengthSquared() < 1e-12)
                        axis = chest.transformVector(side) * -1.0;
                    axis.normalize();
                    Matrix4x4 upper = then(rotationAbout(joint, axis, swing), neutral);
                    apply(arm.upper, upper);

                    // Forearm: bends toward the front, more when the arm is forward. The bend is
                    // a target angle, so arms modelled bent are not folded further than asked.
                    Vector3 elbow = upper.transformPoint(bonePos(arm.lower));
                    Vector3 upperDir = upper.transformVector(boneEnd(arm.upper) - bonePos(arm.upper));
                    Vector3 elbowAxis = Vector3::crossProduct(upperDir, bodyForward);
                    Matrix4x4 lower = upper;
                    lower *= relaxedArm(rigStructure, boneIdx, arm.upper, parameters).inverted();
                    lower *= relaxedForearm(rigStructure, boneIdx, arm.upper.substr(0, arm.upper.size() - 8), parameters);
                    if (elbowAxis.lengthSquared() > 1e-12) {
                        double targetBend = style.elbowBend + style.elbowSwingBend * forwardness;
                        double add = std::max(0.0, targetBend - arm.restElbow);
                        lower = then(rotationAbout(elbow, elbowAxis.normalized(), add), lower);
                    }
                    apply(arm.lower, lower);

                    // Hand: trails the forearm a little.
                    Vector3 wrist = lower.transformPoint(bonePos(arm.hand));
                    Vector3 forearmDir = lower.transformVector(boneEnd(arm.lower) - bonePos(arm.lower));
                    Vector3 wristAxis = Vector3::crossProduct(forearmDir, bodyForward);
                    Matrix4x4 hand = lower;
                    if (wristAxis.lengthSquared() > 1e-12)
                        hand = then(rotationAbout(wrist, wristAxis.normalized(), -0.12 * style.armSwing * std::sin(armPhase - 2.0 * Math::Pi * style.forearmLag)), lower);
                    apply(arm.hand, hand);
                }

                // ---------------------------------------------------------
                // Every other bone rides with its parent
                // ---------------------------------------------------------
                for (const auto& entry : byDepth) {
                    const std::string& name = entry.second;
                    if (layers.count(name))
                        continue;
                    Matrix4x4 layer;
                    auto p = layers.find(parentOf[name]);
                    if (p != layers.end())
                        layer = p->second;
                    else
                        layer = pelvis;
                    layers[name] = layer;
                    world[name] = then(layer, rest[name]);
                }

                if (hairSim.active)
                    hairSim.step(world["Head"], dt, world);
                if (capeSim.active)
                    capeSim.step(world["Chest"], dt, world);

                if (pass == 1) {
                    auto& animFrame = animationClip.frames[frame];
                    animFrame.time = static_cast<float>(tNormalized) * durationSeconds;
                    animFrame.boneWorldTransforms = world;
                    finishFrame(animFrame, inverseBindMatrices);
                }
            }
        }

        return true;
    }

} // namespace biped

} // namespace dust3d
