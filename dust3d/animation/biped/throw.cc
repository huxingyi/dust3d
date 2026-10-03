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

// Procedural throw for the biped rig: a spear, a boomerang or a stone hurled at the target.
//
// A one-shot ranged attack for game use: the off-side foot steps forward into a wide stance, the body coils away
// from the target with the throwing arm cocked back (upper arm out to the side, forearm up,
// the hand beside the head and the weapon held level and aimed) while the other arm points
// at the target. Then the hips and chest uncoil, the arm whips over the shoulder (or round
// at shoulder height, for a sidearm boomerang throw), the weapon leaves the hand at the
// release, and the arm follows through across the body before everything returns to rest.
//
// The weapon is skinned to the hand. During the throw the hand turns at the wrist so that the
// weapon points at the target, whatever angle it is held at in the rest pose (weaponPitch).
// The clip starts and ends exactly in the rest pose, so it blends with idle and walk.
//
// Adjustable animation parameters:
//   - throwArm:             0 = left arm (the modelled +X side), 1 = right arm
//   - sidearmFactor:        0 = overarm (spear), 1 = sidearm, at shoulder height (boomerang)
//   - windupFactor:         how far the body coils and the arm cocks back
//   - twistFactor:          how far the hips and chest turn away from the target
//   - leanFactor:           lean back in the wind-up, forward in the follow-through
//   - stepFactor:           how far the front foot steps toward the target (0 = feet planted)
//   - crouchFactor:         how low the thrower crouches (0 = standing, 1 = a low crouch)
//   - offArmPointFactor:    how far the other arm points at the target, then pulls down
//   - weaponPitch:          the held weapon's angle in the rest pose, in degrees from
//                           vertical toward the front (0 = carried upright)
//   - weaponAimFactor:      how fully the wrist turns the weapon onto the target (0 = no turn)
//   - aimHeightFactor:      how high the weapon is aimed (1 = a few degrees up, a long throw)
//   - releaseTimingFactor:  when the weapon leaves the hand (1 = halfway through the clip)

#include <algorithm>
#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/biped/throw.h>
#include <dust3d/animation/common.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>
#include <set>
#include <vector>

namespace dust3d {

namespace biped {

    namespace {

        // Turn unit vector a toward unit vector b by the fraction w, along the great circle.
        Vector3 slerpDirection(const Vector3& a, const Vector3& b, double w)
        {
            w = std::clamp(w, 0.0, 1.0);
            double c = std::clamp(Vector3::dotProduct(a, b), -1.0, 1.0);
            double angle = std::acos(c);
            if (angle < 1e-6)
                return b;
            Vector3 axis = Vector3::crossProduct(a, b);
            if (axis.lengthSquared() < 1e-12) {
                axis = Vector3::crossProduct(a, Vector3(0.0, 1.0, 0.0));
                if (axis.lengthSquared() < 1e-12)
                    axis = Vector3::crossProduct(a, Vector3(1.0, 0.0, 0.0));
            }
            Matrix4x4 m;
            m.rotate(axis.normalized(), angle * w);
            return m.transformVector(a).normalized();
        }

        enum class Ease {
            Smooth, // ease in and out
            In, // accelerate: the whip of the throw
            Out // decelerate: the follow-through
        };

        double ease(double u, Ease e)
        {
            u = std::clamp(u, 0.0, 1.0);
            switch (e) {
            case Ease::In:
                return u * u * u;
            case Ease::Out: {
                double v = 1.0 - u;
                return 1.0 - v * v;
            }
            default:
                return u * u * (3.0 - 2.0 * u);
            }
        }

        struct DirKey {
            double t;
            Vector3 v;
            Ease e; // easing of the segment that ends at this key
        };

        Vector3 keyedDirection(const std::vector<DirKey>& keys, double t)
        {
            if (t <= keys.front().t)
                return keys.front().v;
            for (size_t i = 1; i < keys.size(); ++i) {
                if (t <= keys[i].t) {
                    double span = std::max(1e-6, keys[i].t - keys[i - 1].t);
                    return slerpDirection(keys[i - 1].v, keys[i].v, ease((t - keys[i - 1].t) / span, keys[i].e));
                }
            }
            return keys.back().v;
        }

        struct ScalarKey {
            double t;
            double v;
            Ease e;
        };

        double keyedScalar(const std::vector<ScalarKey>& keys, double t)
        {
            if (t <= keys.front().t)
                return keys.front().v;
            for (size_t i = 1; i < keys.size(); ++i) {
                if (t <= keys[i].t) {
                    double span = std::max(1e-6, keys[i].t - keys[i - 1].t);
                    double u = ease((t - keys[i - 1].t) / span, keys[i].e);
                    return keys[i - 1].v + (keys[i].v - keys[i - 1].v) * u;
                }
            }
            return keys.back().v;
        }

        Vector3 dir(const Vector3& f, double a, const Vector3& o, double b, const Vector3& u, double c)
        {
            return (f * a + o * b + u * c).normalized();
        }

        // The rotation whose columns are c0, c1, c2 (the images of x, y and z), as a quaternion.
        Quaternion quaternionFromColumns(const Vector3& c0, const Vector3& c1, const Vector3& c2)
        {
            double r00 = c0.x(), r10 = c0.y(), r20 = c0.z();
            double r01 = c1.x(), r11 = c1.y(), r21 = c1.z();
            double r02 = c2.x(), r12 = c2.y(), r22 = c2.z();
            double trace = r00 + r11 + r22;
            if (trace > 0.0) {
                double s = std::sqrt(trace + 1.0) * 2.0;
                return Quaternion(0.25 * s, (r21 - r12) / s, (r02 - r20) / s, (r10 - r01) / s).normalized();
            }
            if (r00 > r11 && r00 > r22) {
                double s = std::sqrt(1.0 + r00 - r11 - r22) * 2.0;
                return Quaternion((r21 - r12) / s, 0.25 * s, (r01 + r10) / s, (r02 + r20) / s).normalized();
            }
            if (r11 > r22) {
                double s = std::sqrt(1.0 + r11 - r00 - r22) * 2.0;
                return Quaternion((r02 - r20) / s, (r01 + r10) / s, 0.25 * s, (r12 + r21) / s).normalized();
            }
            double s = std::sqrt(1.0 + r22 - r00 - r11) * 2.0;
            return Quaternion((r10 - r01) / s, (r02 + r20) / s, (r12 + r21) / s, 0.25 * s).normalized();
        }

        // The rotation that takes the frame (axis0, side0) onto (axis1, side1): the weapon's
        // length onto the aim, and its flat side onto a side that never lines up with the aim,
        // so the hand's roll is the same from frame to frame.
        Quaternion frameRotation(const Vector3& axis0, const Vector3& side0, const Vector3& axis1, const Vector3& side1)
        {
            Vector3 a0 = axis0.normalized();
            Vector3 s0 = (side0 - a0 * Vector3::dotProduct(side0, a0)).normalized();
            Vector3 n0 = Vector3::crossProduct(a0, s0);
            Vector3 a1 = axis1.normalized();
            Vector3 s1 = (side1 - a1 * Vector3::dotProduct(side1, a1)).normalized();
            Vector3 n1 = Vector3::crossProduct(a1, s1);
            auto column = [&](int j) {
                return a1 * a0[j] + s1 * s0[j] + n1 * n0[j];
            };
            return quaternionFromColumns(column(0), column(1), column(2));
        }

    } // namespace

    bool hurl(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = std::max(2, static_cast<int>(parameters.getValue("frameCount", 36)));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 0.9));

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
            "LeftUpperArm", "LeftLowerArm", "RightUpperArm", "RightLowerArm"
        };
        if (!validateRequiredBones(boneIdx, requiredBones, sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;

        bool rightArm = parameters.getValue("throwArm", 0.0) > 0.5;
        double sidearm = std::clamp(parameters.getValue("sidearmFactor", 0.0), 0.0, 1.0);
        double windup = std::max(0.0, parameters.getValue("windupFactor", 1.0));
        double twistFactor = std::max(0.0, parameters.getValue("twistFactor", 1.0));
        double leanFactor = std::max(0.0, parameters.getValue("leanFactor", 1.0));
        double stepFactor = std::max(0.0, parameters.getValue("stepFactor", 1.0));
        double crouchFactor = std::clamp(parameters.getValue("crouchFactor", 0.0), 0.0, 1.5);
        double offArmFactor = std::clamp(parameters.getValue("offArmPointFactor", 1.0), 0.0, 1.0);
        double weaponPitch = parameters.getValue("weaponPitch", 0.0) * Math::Pi / 180.0;
        double weaponAim = std::clamp(parameters.getValue("weaponAimFactor", 1.0), 0.0, 1.0);
        double aimHeight = parameters.getValue("aimHeightFactor", 1.0);
        double tr = std::clamp(0.5 * parameters.getValue("releaseTimingFactor", 1.0), 0.32, 0.66);
        double tw = tr - 0.2; // arm cocked
        double th = tr - 0.07; // end of the hold, the whip starts
        double tf = tr + 0.15; // end of the follow-through

        Vector3 up(0.0, 1.0, 0.0);
        Vector3 toes = (boneEnd("LeftFoot") + boneEnd("RightFoot")) * 0.5 - (boneEnd("LeftLowerLeg") + boneEnd("RightLowerLeg")) * 0.5;
        Vector3 forward(toes.x(), 0.0, toes.z());
        if (forward.lengthSquared() < 1e-10)
            forward = Vector3(0.0, 0.0, 1.0);
        forward.normalize();
        // The creature's right: a positive rotation about it lifts the front (leans back).
        Vector3 right = Vector3::crossProduct(forward, up).normalized();

        double legLength = (boneEnd("RightUpperLeg") - bonePos("RightUpperLeg")).length()
            + (boneEnd("RightLowerLeg") - bonePos("RightLowerLeg")).length();
        if (legLength < 1e-6)
            return false;

        const std::string throwSide = rightArm ? "Right" : "Left";
        const std::string offSide = rightArm ? "Left" : "Right";
        auto outwardOf = [&](const std::string& side) {
            Vector3 d = boneEnd(side + "UpperArm") - bonePos("Chest");
            return right * (Vector3::dotProduct(d, right) >= 0.0 ? 1.0 : -1.0);
        };
        Vector3 o = outwardOf(throwSide);
        Vector3 o2 = outwardOf(offSide);
        // Twisting by +angle about up must carry the throwing shoulder backward.
        double twistSign = 1.0;
        {
            Matrix4x4 m;
            m.rotate(up, 0.3);
            if (Vector3::dotProduct(m.transformVector(o), forward) > 0.0)
                twistSign = -1.0;
        }

        auto restDir = [&](const std::string& name) {
            Vector3 d = boneEnd(name) - bonePos(name);
            return d.lengthSquared() > 1e-12 ? d.normalized() : up;
        };
        auto mix = [](const Vector3& a, const Vector3& b, double s) {
            return (a * (1.0 - s) + b * s).normalized();
        };
        auto scaled = [&](const Vector3& rest, const Vector3& target, double factor) {
            return slerpDirection(rest, target, std::min(1.0, factor));
        };

        // Arm directions in the rest frame of the body (they ride on the chest as it turns).
        const Vector3 upperRest = restDir(throwSide + "UpperArm");
        const Vector3 foreRest = restDir(throwSide + "LowerArm");
        const Vector3 upperW = scaled(upperRest, mix(dir(forward, -0.45, o, 0.85, up, 0.08), dir(forward, -0.5, o, 0.85, up, -0.05), sidearm), windup);
        const Vector3 foreW = scaled(foreRest, mix(dir(forward, -0.1, o, -0.4, up, 0.9), dir(forward, -0.75, o, 0.45, up, 0.25), sidearm), windup);
        const Vector3 upperC = scaled(upperRest, mix(dir(forward, -0.6, o, 0.78, up, 0.1), dir(forward, -0.65, o, 0.75, up, 0.0), sidearm), windup);
        const Vector3 foreC = scaled(foreRest, mix(dir(forward, -0.25, o, -0.35, up, 0.9), dir(forward, -0.85, o, 0.35, up, 0.2), sidearm), windup);
        const Vector3 upperR = mix(dir(forward, 0.6, o, 0.2, up, 0.75), dir(forward, 0.9, o, 0.4, up, 0.05), sidearm);
        const Vector3 foreR = mix(dir(forward, 0.85, o, 0.05, up, 0.55), dir(forward, 0.95, o, 0.1, up, 0.05), sidearm);
        const Vector3 upperF = mix(dir(forward, 0.7, o, -0.3, up, -0.6), dir(forward, 0.65, o, -0.6, up, -0.3), sidearm);
        const Vector3 foreF = mix(dir(forward, 0.5, o, -0.45, up, -0.75), dir(forward, 0.4, o, -0.8, up, -0.35), sidearm);
        std::vector<DirKey> upperKeys = { { 0.0, upperRest, Ease::Smooth }, { tw, upperW, Ease::Smooth },
            { th, upperC, Ease::Smooth }, { tr, upperR, Ease::In }, { tf, upperF, Ease::Out }, { 1.0, upperRest, Ease::Smooth } };
        std::vector<DirKey> foreKeys = { { 0.0, foreRest, Ease::Smooth }, { tw, foreW, Ease::Smooth },
            { th, foreC, Ease::Smooth }, { tr, foreR, Ease::In }, { tf, foreF, Ease::Out }, { 1.0, foreRest, Ease::Smooth } };

        const Vector3 offUpperRest = restDir(offSide + "UpperArm");
        const Vector3 offForeRest = restDir(offSide + "LowerArm");
        const Vector3 point = dir(forward, 0.82, o2, 0.3, up, 0.28 - 0.25 * sidearm);
        const Vector3 offUpperP = scaled(offUpperRest, point, offArmFactor);
        const Vector3 offForeP = scaled(offForeRest, point, offArmFactor);
        const Vector3 offUpperD = scaled(offUpperRest, dir(forward, -0.3, o2, 0.4, up, -0.85), offArmFactor);
        const Vector3 offForeD = scaled(offForeRest, dir(forward, -0.1, o2, 0.3, up, -0.95), offArmFactor);
        std::vector<DirKey> offUpperKeys = { { 0.0, offUpperRest, Ease::Smooth }, { tw, offUpperP, Ease::Smooth },
            { th, offUpperP, Ease::Smooth }, { tr, offUpperD, Ease::In }, { tf, offUpperD, Ease::Out }, { 1.0, offUpperRest, Ease::Smooth } };
        std::vector<DirKey> offForeKeys = { { 0.0, offForeRest, Ease::Smooth }, { tw, offForeP, Ease::Smooth },
            { th, offForeP, Ease::Smooth }, { tr, offForeD, Ease::In }, { tf, offForeD, Ease::Out }, { 1.0, offForeRest, Ease::Smooth } };

        // The weapon: its rest direction, and where the wrist points it.
        const Vector3 weaponRest = (up * std::cos(weaponPitch) + forward * std::sin(weaponPitch)).normalized();
        double aimUp = 0.12 * aimHeight;
        const Vector3 aimW = dir(forward, 1.0, o, 0.0, up, aimUp + 0.05);
        const Vector3 aimR = dir(forward, 1.0, o, 0.0, up, aimUp + 0.15);
        const Vector3 aimF = dir(forward, 1.0, o, -0.15, up, -0.5);
        std::vector<ScalarKey> aimKeys = { { 0.0, 0.0, Ease::Smooth }, { std::max(0.06, tw * 0.75), 1.0, Ease::Smooth },
            { tf, 1.0, Ease::Smooth }, { 0.96, 0.0, Ease::Smooth }, { 1.0, 0.0, Ease::Smooth } };
        std::vector<DirKey> weaponKeys = { { 0.0, weaponRest, Ease::Smooth }, { tw, aimW, Ease::Smooth },
            { th, aimW, Ease::Smooth }, { tr, aimR, Ease::In }, { tf, aimF, Ease::Out }, { 1.0, weaponRest, Ease::Smooth } };

        // The body: coil away (twist), lean back, then uncoil and lean into the throw.
        double twist = 0.55 * twistFactor * std::min(1.5, windup) * twistSign;
        std::vector<ScalarKey> twistKeys = { { 0.0, 0.0, Ease::Smooth }, { tw, 0.9 * twist, Ease::Smooth },
            { th, twist, Ease::Smooth }, { tr, -0.45 * twist, Ease::In }, { tf, -0.6 * twist, Ease::Out }, { 1.0, 0.0, Ease::Smooth } };
        std::vector<ScalarKey> leanKeys = { { 0.0, 0.0, Ease::Smooth }, { tw, 0.1 * leanFactor, Ease::Smooth },
            { th, 0.13 * leanFactor, Ease::Smooth }, { tr, -0.16 * leanFactor, Ease::In }, { tf, -0.26 * leanFactor, Ease::Out },
            { 1.0, 0.0, Ease::Smooth } };
        std::vector<ScalarKey> shiftKeys = { { 0.0, 0.0, Ease::Smooth }, { tw, -0.05, Ease::Smooth },
            { th, -0.06, Ease::Smooth }, { tr, 0.04, Ease::In }, { tf, 0.07, Ease::Out }, { 1.0, 0.0, Ease::Smooth } };
        double crouch = legLength * (0.025 + 0.22 * crouchFactor);
        std::vector<ScalarKey> crouchKeys = { { 0.0, 0.0, Ease::Smooth }, { tw, 0.8, Ease::Smooth },
            { th, 1.0, Ease::Smooth }, { tr, 1.0, Ease::Smooth }, { tf, 0.9, Ease::Smooth }, { 1.0, 0.0, Ease::Smooth } };
        // The front foot steps toward the target into a wide stance and back at the end.
        double stepLength = legLength * 0.26 * stepFactor;
        double stepOutEnd = std::max(0.1, tw - 0.02);
        double stepBackStart = std::min(0.9, tf + 0.06);
        std::vector<ScalarKey> stepKeys = { { 0.0, 0.0, Ease::Smooth }, { 0.04, 0.0, Ease::Smooth },
            { stepOutEnd, 1.0, Ease::Smooth }, { stepBackStart, 1.0, Ease::Smooth }, { 0.95, 0.0, Ease::Smooth }, { 1.0, 0.0, Ease::Smooth } };
        auto stepLift = [&](double t) {
            double lift = legLength * 0.07 * std::min(1.0, stepFactor);
            if (t > 0.04 && t < stepOutEnd)
                return lift * std::sin(Math::Pi * (t - 0.04) / (stepOutEnd - 0.04));
            if (t > stepBackStart && t < 0.95)
                return lift * std::sin(Math::Pi * (t - stepBackStart) / (0.95 - stepBackStart));
            return 0.0;
        };

        std::map<std::string, Matrix4x4> rest = restBoneWorldTransforms(rigStructure);

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            double t = static_cast<double>(frame) / static_cast<double>(frameCount - 1);
            double tw_ = keyedScalar(twistKeys, t);
            double lean = keyedScalar(leanKeys, t);
            double shift = keyedScalar(shiftKeys, t) * legLength * std::min(1.5, leanFactor);
            double drop = keyedScalar(crouchKeys, t) * crouch;

            std::map<std::string, Matrix4x4> world = rest;
            std::map<std::string, Matrix4x4> layers;
            std::set<std::string> posed;
            auto apply = [&](const std::string& name, const Matrix4x4& layer) {
                if (!boneIdx.count(name))
                    return;
                layers[name] = layer;
                posed.insert(name);
                Matrix4x4 m = layer;
                m *= rest[name];
                world[name] = m;
            };

            // Hips carry a third of the turn and lean, the spine and chest the rest.
            Vector3 hipsJoint = bonePos("Hips");
            Matrix4x4 hips;
            hips.translate(forward * shift - up * drop);
            hips *= rotationAbout(hipsJoint, up, tw_ * 0.35);
            hips *= rotationAbout(hipsJoint, right, lean * 0.35);
            apply("Root", Matrix4x4());
            apply("Hips", hips);
            Vector3 spineJoint = hips.transformPoint(bonePos("Spine"));
            Matrix4x4 spine = rotationAbout(spineJoint, up, tw_ * 0.3);
            spine *= rotationAbout(spineJoint, right, lean * 0.3);
            spine *= hips;
            apply("Spine", spine);
            Vector3 chestJoint = spine.transformPoint(bonePos("Chest"));
            Matrix4x4 chest = rotationAbout(chestJoint, up, tw_ * 0.35);
            chest *= rotationAbout(chestJoint, right, lean * 0.35);
            chest *= spine;
            apply("Chest", chest);
            // The head keeps its eyes on the target.
            Vector3 neckJoint = chest.transformPoint(bonePos("Neck"));
            Matrix4x4 neck = rotationAbout(neckJoint, up, -tw_ * 0.85);
            neck *= rotationAbout(neckJoint, right, -lean * 0.7);
            neck *= chest;
            apply("Neck", neck);
            apply("Head", neck);

            // Arm directions are keyed in the body's frame while it coils (the arm cocks back
            // with the shoulders) and in the world's from the whip on (the hand drives at the
            // target, whichever way the body still faces).
            double worldAim = smoothstep((t - th) / std::max(1e-3, tr - th));
            auto armFrame = [&](const Vector3& d) {
                return slerpDirection(chest.transformVector(d).normalized(), d, worldAim);
            };
            // Arms: each bone turns from where its parent carried it to the keyed direction.
            auto poseArm = [&](const std::string& side, const std::vector<DirKey>& upperK, const std::vector<DirKey>& foreK,
                               const std::vector<DirKey>* weaponK) {
                std::string shoulder = side + "Shoulder";
                std::string upper = side + "UpperArm";
                std::string lower = side + "LowerArm";
                std::string hand = side + "Hand";
                apply(shoulder, chest);
                Vector3 joint = chest.transformPoint(bonePos(upper));
                Vector3 carried = chest.transformVector(restDir(upper));
                Matrix4x4 arm = turnAbout(joint, carried, armFrame(keyedDirection(upperK, t)), 1.0);
                arm *= chest;
                apply(upper, arm);
                Vector3 elbow = arm.transformPoint(bonePos(lower));
                Vector3 foreCarried = arm.transformVector(restDir(lower));
                Matrix4x4 fore = turnAbout(elbow, foreCarried, armFrame(keyedDirection(foreK, t)), 1.0);
                fore *= arm;
                apply(lower, fore);
                Matrix4x4 handLayer = fore;
                if (weaponK && boneIdx.count(hand)) {
                    // The wrist turns the weapon onto the target (in the world: the body is turned
                    // away from it), its flat side kept to the side, blended in from the hand as
                    // the forearm carries it, and back to that at the end, so the clip starts and
                    // ends at rest and the hand never flips its roll between frames.
                    Vector3 wristRest = bonePos(hand);
                    Vector3 wrist = fore.transformPoint(wristRest);
                    Quaternion carried = quaternionFromColumns(fore.transformVector(Vector3(1.0, 0.0, 0.0)),
                        fore.transformVector(Vector3(0.0, 1.0, 0.0)), fore.transformVector(Vector3(0.0, 0.0, 1.0)));
                    Vector3 aim = keyedDirection(*weaponK, t);
                    Quaternion aimed = frameRotation(weaponRest, o, aim, chest.transformVector(o));
                    Quaternion q = Quaternion::slerp(carried, aimed, keyedScalar(aimKeys, t) * weaponAim);
                    handLayer = Matrix4x4();
                    handLayer.translate(wrist);
                    handLayer.rotate(q);
                    handLayer.translate(Vector3() - wristRest);
                }
                apply(hand, handLayer);
                // Fingers, held props and anything else under the hand ride on it.
                for (const auto& bone : rigStructure.bones) {
                    if (bone.parent == hand && !posed.count(bone.name))
                        apply(bone.name, handLayer);
                }
            };
            poseArm(throwSide, upperKeys, foreKeys, &weaponKeys);
            poseArm(offSide, offUpperKeys, offForeKeys, nullptr);

            // Legs: the throwing side stays planted, the other foot steps forward.
            for (const char* side : { "Left", "Right" }) {
                std::string s(side);
                Vector3 offset;
                if (s == offSide)
                    offset = forward * (stepLength * keyedScalar(stepKeys, t)) + up * stepLift(t);
                posePlantedLeg(rigStructure, boneIdx, s + "UpperLeg", s + "LowerLeg", s + "Foot", hips, offset, world);
                posed.insert(s + "UpperLeg");
                posed.insert(s + "LowerLeg");
                posed.insert(s + "Foot");
            }

            // Everything not posed above (tails, capes, hair, ears, jaws...) rides on its
            // nearest posed ancestor.
            for (int pass = 0; pass < 8; ++pass) {
                bool changed = false;
                for (const auto& bone : rigStructure.bones) {
                    if (posed.count(bone.name) || bone.parent.empty())
                        continue;
                    auto it = layers.find(bone.parent);
                    Matrix4x4 layer;
                    if (it != layers.end()) {
                        layer = it->second;
                    } else if (posed.count(bone.parent)) {
                        // A leg bone: carry by the parent's motion from rest.
                        Matrix4x4 inv = rest[bone.parent];
                        inv = inv.inverted();
                        layer = world[bone.parent];
                        layer *= inv;
                    } else {
                        continue;
                    }
                    apply(bone.name, layer);
                    changed = true;
                }
                if (!changed)
                    break;
            }

            auto& animFrame = animationClip.frames[frame];
            animFrame.time = static_cast<float>(frame) / static_cast<float>(frameCount) * durationSeconds;
            animFrame.boneWorldTransforms = world;
            finishFrame(animFrame, inverseBindMatrices);
        }
        return true;
    }

} // namespace biped

} // namespace dust3d
