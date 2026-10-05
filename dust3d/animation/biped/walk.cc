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

// Procedural walk loop for the biped rig.
//
// Designed for all bipedal creatures: humans, biped robots, anthropomorphic
// animals, dinosaurs, penguins and any game character that walks on two legs
// with a biped bone structure. The motion comes from the shared gait engine
// (gait.cc): the feet roll heel to toe and stay planted at the clip's movement
// speed, the hips vault over the standing leg (highest at mid-stance), shift
// and turn with the steps, the chest turns against the pelvis, the arms swing
// with the opposite leg and the head stays level.
//
// Adjustable animation parameters:
//   - stepLengthFactor:    stride length relative to leg length
//   - stepHeightFactor:    foot lift in the swing
//   - bodyBobFactor:       up and down travel of the hips
//   - gaitSpeedFactor:     number of gait cycles per clip
//   - armSwingFactor:      arm swing (0 = arms still)
//   - hipSwayFactor:       hips shift over the standing foot and drop on the swinging side
//   - hipRotateFactor:     pelvis turn with each step
//   - spineFlexFactor:     chest turning against the pelvis
//   - headBobFactor:       how much the head keeps level and looks ahead
//   - kneeBendFactor:      extra knee bend (a softer, lower walk)
//   - footRollFactor:      heel strike and toe push-off
//   - leanForwardFactor:   forward lean of the upper body
//   - bouncinessFactor:    extra vertical bounce (cartoon style)
//   - forearmPhaseOffset:  how far the forearm trails the upper arm (0.5 = natural)
//   - tailSwayFactor:      tail sway

#include <algorithm>
#include <dust3d/animation/biped/gait.h>
#include <dust3d/animation/biped/walk.h>

namespace dust3d {

namespace biped {

    bool walk(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        auto p = [&](const char* name, double defaultValue = 1.0) { return parameters.getValue(name, defaultValue); };

        GaitStyle s;
        s.running = false;
        s.stanceFraction = 0.6;
        s.footSpan = 0.75 * p("stepLengthFactor");
        s.landingFraction = 0.55;
        s.liftHeight = 0.1 * p("stepHeightFactor");
        s.liftPeakAt = 0.35;
        s.swingRetraction = 0.8;
        s.heelStrikePitch = 0.32 * p("footRollFactor");
        s.toeOffPitch = 0.6 * p("footRollFactor");
        s.heelRollEnd = 0.15;
        s.heelRiseStart = 0.55;

        s.pendulum = std::clamp(p("bodyBobFactor") * std::min(1.0, p("bouncinessFactor")), 0.0, 1.0);
        s.bob = 0.012 * p("bodyBobFactor") * std::max(0.0, p("bouncinessFactor") - 1.0);
        s.extraCrouch = 0.005 * p("kneeBendFactor");
        s.sway = 0.035 * p("hipSwayFactor");
        s.list = 0.045 * p("hipSwayFactor");
        s.yaw = 0.07 * p("hipRotateFactor");
        s.lean = 0.04 * p("leanForwardFactor");
        s.leanPulse = 0.0;
        s.spineCounter = std::clamp(p("spineFlexFactor"), 0.0, 1.5);
        s.headStabilize = std::clamp(0.8 * p("headBobFactor"), 0.0, 1.0);

        s.armSwing = 0.3 * p("armSwingFactor");
        s.armForwardBias = 0.03;
        s.elbowBend = 0.25;
        s.elbowSwingBend = 0.3 * std::min(2.0, p("armSwingFactor"));
        s.armLag = 0.03;
        s.forearmLag = 0.16 * std::clamp(p("forearmPhaseOffset", 0.5), 0.0, 1.0);
        s.shoulderShrug = 0.04 * p("armSwingFactor");

        s.tailSway = 0.08 * p("tailSwayFactor");
        s.tailBounce = 0.02 * p("bouncinessFactor");
        s.hairStiffness = 0.10;
        s.hairDamping = 0.88;

        return locomote(rigStructure, inverseBindMatrices, animationClip, parameters, s);
    }

} // namespace biped

} // namespace dust3d
