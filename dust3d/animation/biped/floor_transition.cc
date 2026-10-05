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

#include <dust3d/animation/biped/floor_transition.h>
#include <dust3d/base/math.h>
#include <initializer_list>
namespace dust3d {
namespace biped {
    namespace {
        // These floor transitions keep ankle targets in world space: rotating
        // the pelvis must not drag a supporting foot around with the torso.
        PoseSample basePose()
        {
            PoseSample p;
            p.floorContact = true;
            p.worldKnees = true;
            p.footFollowWeight = 0;
            for (int i = 0; i < 2; ++i) {
                p.arms[i].targetHand = true;
                p.arms[i].targetGround = true;
                p.arms[i].handTarget = Vector3(i == 0 ? 0.28 : -0.28, 0.07, -0.12);
                p.arms[i].handDirection = Vector3(0, 0, 1);
            }
            return p;
        }
        PoseSample seatedFloor()
        {
            PoseSample p = basePose();
            p.hips = Vector3(0, -0.82, -0.18);
            p.pitch = -0.18;
            p.chestPitch = 0.35;
            p.headPitch = 0.12;
            for (int i = 0; i < 2; ++i) {
                p.ankle[i] = Vector3(i == 0 ? 0.06 : -0.06, 0.04, 0.55);
                p.kneeDirection[i] = Vector3(0, 1, 0.3);
                p.arms[i].weight = 1;
            }
            return p;
        }
        PoseSample halfKneel()
        {
            PoseSample p = basePose();
            p.hips = Vector3(0.02, -0.46, 0.05);
            p.pitch = 1.3;
            p.chestPitch = 0.1;
            p.headPitch = -0.55;
            p.ankle[0] = Vector3(0.08, 0, 0.42);
            p.ankle[1] = Vector3(-0.08, 0.06, -0.38);
            p.kneeDirection[1] = Vector3(0, 0, 1);
            p.arms[1].handTarget = Vector3(-0.28, 0.07, 0.62);
            p.arms[1].weight = 1;
            p.arms[0].targetHand = false;
            p.arms[0].upper = Vector3(0.15, -0.8, 0.45);
            p.arms[0].lower = Vector3(0, -0.7, 0.35);
            p.arms[0].weight = 1;
            return p;
        }
        PoseSample pushUp()
        {
            PoseSample p = halfKneel();
            p.hips = Vector3(0.03, -0.23, 0.22);
            p.pitch = 0.22;
            p.chestPitch = 0.12;
            p.ankle[1] = Vector3(-0.05, 0.12, -0.25);
            p.kneeDirection[1] = Vector3(0, 0, 1);
            p.arms[1].weight = 0;
            return p;
        }
        PoseSample blend(const PoseSample& a, const PoseSample& b, double t)
        {
            PoseSample p = a;
            auto v = [t](const Vector3& x, const Vector3& y) { return x + (y - x) * t; };
            auto f = [t](double x, double y) { return x + (y - x) * t; };
            p.hips = v(a.hips, b.hips);
            p.footFollowWeight = f(a.footFollowWeight, b.footFollowWeight);
            p.pitch = f(a.pitch, b.pitch);
            p.roll = f(a.roll, b.roll);
            p.yaw = f(a.yaw, b.yaw);
            p.chestPitch = f(a.chestPitch, b.chestPitch);
            p.chestYaw = f(a.chestYaw, b.chestYaw);
            p.headPitch = f(a.headPitch, b.headPitch);
            p.headYaw = f(a.headYaw, b.headYaw);
            for (int i = 0; i < 2; ++i) {
                p.ankle[i] = v(a.ankle[i], b.ankle[i]);
                p.kneeDirection[i] = v(a.kneeDirection[i], b.kneeDirection[i]);
                p.arms[i].upper = v(a.arms[i].upper, b.arms[i].upper);
                p.arms[i].lower = v(a.arms[i].lower, b.arms[i].lower);
                p.arms[i].weight = f(a.arms[i].weight, b.arms[i].weight);
                p.arms[i].handTarget = v(a.arms[i].handTarget, b.arms[i].handTarget);
                p.arms[i].handDirection = v(a.arms[i].handDirection, b.arms[i].handDirection);
                // Switch contact mode only while the hand is released. Keep
                // the support target fixed throughout a planted interval.
                p.arms[i].targetHand = a.arms[i].targetHand || b.arms[i].targetHand;
                double contact = f(a.arms[i].targetHand ? a.arms[i].weight * a.arms[i].targetWeight : 0,
                    b.arms[i].targetHand ? b.arms[i].weight * b.arms[i].targetWeight : 0);
                p.arms[i].targetWeight = p.arms[i].weight > 0 ? contact / p.arms[i].weight : 0;
                p.arms[i].targetGround = b.arms[i].targetGround;
            }
            return p;
        }
        struct Key {
            double time;
            PoseSample pose;
        };
        PoseSample sequence(double t, std::initializer_list<Key> keys)
        {
            auto previous = keys.begin();
            for (auto next = previous + 1; next != keys.end(); ++next) {
                if (t <= next->time)
                    return blend(previous->pose, next->pose, easePose((t - previous->time) / (next->time - previous->time)));
                previous = next;
            }
            return previous->pose;
        }
    }
    PoseSample floorRestPose(bool sleeping)
    {
        PoseSample p = basePose();
        p.hips = Vector3(0, -0.86, -0.18);
        p.pitch = -Math::Pi * 0.5;
        p.footFollowWeight = 1;
        // Roll about the character's length into a side sleeping posture.
        p.roll = sleeping ? Math::Pi * 0.5 : 0;
        p.chestPitch = sleeping ? 0.1 : 0;
        p.headPitch = sleeping ? 0.08 : 0;
        for (int i = 0; i < 2; ++i) {
            double sign = i == 0 ? 1 : -1;
            p.ankle[i] = sleeping ? Vector3(-0.28, i == 0 ? 0.16 : 0.04, 0.45)
                                  : Vector3(sign * 0.06, 0.05, 0.72);
            p.kneeDirection[i] = sleeping ? Vector3(-1, 0.15, 0) : Vector3(0, 1, 0.2);
            p.arms[i].targetHand = false;
            p.arms[i].upper = sleeping ? Vector3(sign * 0.12, -0.5, 0.7) : Vector3(sign * 0.35, -1, 0);
            p.arms[i].lower = sleeping ? Vector3(0, 0.5, 0.7) : Vector3(0, -1, 0.1);
            p.arms[i].weight = 1;
        }
        return p;
    }
    PoseSample floorTransitionPose(double t, FloorTransition transition, double intensity)
    {
        PoseSample standing = basePose();
        PoseSample sit = seatedFloor(), kneel = halfKneel(), push = pushUp();
        PoseSample sleep = floorRestPose(true), down = floorRestPose(false);
        PoseSample reposition = sit;
        reposition.hips = Vector3(0, -0.76, 0.02);
        reposition.chestPitch = 0.5;
        reposition.arms[1].weight = 0; // Lift the palm before planting it ahead.
        PoseSample reach = reposition;
        reach.arms[1].handTarget = kneel.arms[1].handTarget;
        reach.ankle[0] = kneel.ankle[0] + Vector3(0, 0.12, 0);
        reach.ankle[1] = Vector3(-0.08, 0.18, -0.12);
        PoseSample upright = basePose();
        upright.hips = Vector3(0, 0, 0.2);
        upright.ankle[0] = kneel.ankle[0];
        PoseSample step = basePose();
        step.hips = Vector3(0, 0, 0.1);
        step.ankle[0] = Vector3(0.04, 0.1, 0.2);
        // The standing boundary uses the same foot orientation as relaxed idle.
        if (t <= 0 && (transition == FloorTransition::LieDown || transition == FloorTransition::Knockdown))
            return standing;
        if (t >= 1 && (transition == FloorTransition::WakeUp || transition == FloorTransition::GetUp))
            return standing;
        switch (transition) {
        case FloorTransition::LieDown: {
            PoseSample lower = crouchedPose(1);
            lower.floorContact = true;
            lower.worldKnees = true;
            lower.footFollowWeight = 0;
            lower.hips = Vector3(0, -0.48, -0.12);
            PoseSample lean = sit;
            lean.pitch = -0.85;
            lean.hips = Vector3(0, -0.85, -0.18);
            lean.chestPitch = 0.2;
            lean.arms[0].weight = 0;
            lean.arms[1].handTarget = Vector3(-0.28, 0.07, -0.12);
            return sequence(t, { { 0, standing }, { .24, lower }, { .5, sit }, { .72, lean }, { .86, down }, { 1, sleep } });
        }
        case FloorTransition::WakeUp: {
            PoseSample alert = sleep;
            alert.headPitch += 0.16;
            alert.chestPitch += 0.08;
            PoseSample elbow = sit;
            elbow.pitch = -0.65;
            elbow.chestPitch = 0.25;
            return sequence(t, { { 0, sleep }, { .12, alert }, { .22, down }, { .34, elbow }, { .46, sit }, { .55, reposition }, { .61, reach }, { .72, kneel }, { .75, kneel }, { .86, push }, { .92, upright }, { .96, step }, { 1, standing } });
        }
        case FloorTransition::Knockdown: {
            PoseSample recoil = standing;
            recoil.hips = Vector3(0, -0.14, -0.1);
            recoil.pitch = -0.2 * intensity;
            recoil.chestPitch = -0.15 * intensity;
            for (auto& arm : recoil.arms) {
                arm.targetHand = false;
                arm.upper = Vector3(0, -0.4, 0.6);
                arm.lower = Vector3(0, 0.6, 0.5);
                arm.weight = 1;
            }
            PoseSample fall = sit;
            fall.hips = Vector3(0, -0.72, -0.25);
            fall.pitch = -0.65;
            fall.chestPitch = 0.3;
            for (int i = 0; i < 2; ++i) {
                fall.arms[i] = recoil.arms[i];
                fall.ankle[i] = Vector3(i == 0 ? 0.06 : -0.06, 0.12, 0.4);
            }
            PoseSample impact = down;
            impact.chestPitch = 0.22;
            impact.headPitch = 0.18;
            for (auto& ankle : impact.ankle)
                ankle += Vector3(0, 0.2, -0.18);
            return sequence(t, { { 0, standing }, { .16, recoil }, { .48, fall }, { .7, impact }, { .88, down }, { 1, down } });
        }
        case FloorTransition::GetUp: {
            PoseSample curl = down;
            curl.chestPitch = 0.55;
            curl.headPitch = 0.22;
            curl.ankle[0] = Vector3(0.06, 0.04, 0.42);
            curl.ankle[1] = Vector3(-0.06, 0.04, 0.42);
            return sequence(t, { { 0, down }, { .14, curl }, { .34, sit }, { .42, reposition }, { .49, reach }, { .6, kneel }, { .64, kneel }, { .8, push }, { .9, upright }, { .95, step }, { 1, standing } });
        }
        }
        return standing;
    }
}
}
