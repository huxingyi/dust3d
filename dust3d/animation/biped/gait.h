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

#ifndef DUST3D_ANIMATION_BIPED_GAIT_H_
#define DUST3D_ANIMATION_BIPED_GAIT_H_

#include <dust3d/animation/animation_generator.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace biped {

    // The shape of a two-legged gait. Lengths are in leg lengths (hip to ankle),
    // angles in radians, timings in fractions of one gait cycle (one step of each
    // foot). BipedWalk and BipedRun fill one of these from their parameters.
    struct GaitStyle {
        bool running = false;

        // Feet
        double stanceFraction = 0.6; // share of the cycle a foot is on the ground (walk ~0.6, run ~0.35)
        double footSpan = 0.75; // how far the ankle travels under the body while on the ground
        double landingFraction = 0.55; // share of footSpan in front of the hip at touch-down
        double liftHeight = 0.1; // peak ankle lift in the swing
        double liftPeakAt = 0.35; // when in the swing the lift peaks
        double swingRetraction = 0.8; // 0..1: how much of the ground speed the foot keeps at lift-off and touch-down
        double heelStrikePitch = 0.3; // toe-up angle as the heel touches down
        double toeOffPitch = 0.6; // toe-down angle as the toes push off
        double heelRollEnd = 0.15; // share of the stance by which the foot is flat
        double heelRiseStart = 0.55; // share of the stance at which the heel starts to lift

        // Pelvis and body
        double pendulum = 0.9; // walk: 0..1, how much the hips rise over the standing leg
        double bob = 0.0; // extra vertical travel of the hips each step (run: all of it)
        double extraCrouch = 0.01; // extra knee bend on top of what the stride needs
        double sway = 0.035; // side shift over the standing foot
        double yaw = 0.07; // pelvis turn: the hip of the leg reaching forward goes forward
        double list = 0.05; // pelvis drop on the side of the swinging leg
        double lean = 0.05; // forward lean of the upper body
        double leanPulse = 0.0; // extra lean each push-off
        double spineCounter = 1.0; // 0..1.5: the chest turns against the pelvis
        double headStabilize = 0.8; // 0..1: the head keeps level and looks ahead

        // Arms
        double armSwing = 0.3; // upper arm swing either side
        double armForwardBias = 0.03; // arms carried a little forward
        double elbowBend = 0.3; // elbow bend with the arm at the back
        double elbowSwingBend = 0.35; // extra elbow bend with the arm at the front
        double armLag = 0.03; // the arms trail the legs
        double forearmLag = 0.08; // the forearm trails the upper arm
        double shoulderShrug = 0.04; // the shoulder rolls forward with the arm

        // Extras
        double tailSway = 0.08;
        double tailBounce = 0.0;
        double hairStiffness = 0.10;
        double hairDamping = 0.88;
    };

    bool locomote(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters,
        const GaitStyle& style);

} // namespace biped

} // namespace dust3d

#endif
