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

// Procedural run loop for the biped rig.
//
// Designed for all bipedal creatures: humans, biped robots, anthropomorphic
// animals, dinosaurs, ostriches, penguins and any game character that runs on
// two legs with a biped bone structure. The motion comes from the shared gait
// engine (gait.cc). Compared to the walk, a run has:
//   - a short ground contact (~35% of the cycle per foot) and two flight phases
//   - the hips lowest at mid-stance (the leg absorbs the landing) and highest in flight
//   - a mid-foot landing close under the body and a strong toe push-off
//   - the heel kicking up behind and the knee driving forward in the swing
//   - a stronger forward lean, bent elbows (~75 degrees) and a bigger arm drive
//
// Adjustable animation parameters:
//   - stepLengthFactor:       stride length relative to leg length
//   - stepHeightFactor:       heel kick-up in the swing
//   - bodyBobFactor:          up and down travel of the hips
//   - gaitSpeedFactor:        number of gait cycles per clip
//   - armSwingFactor:         arm drive (0 = arms still)
//   - hipSwayFactor:          hips shift over the standing foot
//   - hipRotateFactor:        pelvis turn with each stride
//   - spineFlexFactor:        chest turning against the pelvis
//   - headBobFactor:          how much the head keeps level and looks ahead
//   - kneeBendFactor:         knee bend as the leg takes the landing
//   - footRollFactor:         toe push-off
//   - leanForwardFactor:      forward lean of the upper body
//   - bouncinessFactor:       extra vertical bounce (cartoon style)
//   - forearmPhaseOffset:     how far the forearm trails the upper arm (0.5 = natural)
//   - suspensionFactor:       how high the body floats in the flight phase
//   - strideFrequencyFactor:  higher = quicker ground contacts, longer flights
//   - tailSwayFactor:         tail sway (optional bones)

#include <algorithm>
#include <dust3d/animation/biped/gait.h>
#include <dust3d/animation/biped/run.h>

namespace dust3d {

namespace biped {

    bool run(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        auto p = [&](const char* name, double defaultValue = 1.0) { return parameters.getValue(name, defaultValue); };

        GaitStyle s;
        s.running = true;
        s.stanceFraction = std::clamp(0.36 / std::max(0.1, p("strideFrequencyFactor")), 0.22, 0.48);
        s.footSpan = 0.85 * p("stepLengthFactor");
        s.landingFraction = 0.4;
        s.liftHeight = 0.3 * p("stepHeightFactor");
        s.liftPeakAt = 0.38;
        s.swingRetraction = 0.7;
        s.heelStrikePitch = 0.08 * p("footRollFactor");
        s.toeOffPitch = 0.55 * p("footRollFactor");
        s.heelRollEnd = 0.12;
        s.heelRiseStart = 0.45;

        s.bob = 0.02 * p("bodyBobFactor") * p("bouncinessFactor") + 0.01 * p("suspensionFactor");
        s.extraCrouch = 0.02 * p("kneeBendFactor");
        s.sway = 0.012 * p("hipSwayFactor");
        s.list = 0.03 * p("hipSwayFactor");
        s.yaw = 0.1 * p("hipRotateFactor");
        s.lean = 0.15 * p("leanForwardFactor");
        s.leanPulse = 0.03 * p("leanForwardFactor");
        s.spineCounter = std::clamp(1.1 * p("spineFlexFactor"), 0.0, 1.5);
        s.headStabilize = std::clamp(0.85 * p("headBobFactor"), 0.0, 1.0);

        s.armSwing = 0.55 * p("armSwingFactor");
        s.armForwardBias = 0.12;
        s.elbowBend = 1.2;
        s.elbowSwingBend = 0.35;
        s.armLag = 0.02;
        s.forearmLag = 0.12 * std::clamp(p("forearmPhaseOffset", 0.5), 0.0, 1.0);
        s.shoulderShrug = 0.07 * p("armSwingFactor");

        s.tailSway = 0.1 * p("tailSwayFactor");
        s.tailBounce = 0.05 * p("bouncinessFactor");
        s.hairStiffness = 0.12;
        s.hairDamping = 0.84;

        return locomote(rigStructure, inverseBindMatrices, animationClip, parameters, s);
    }

} // namespace biped

} // namespace dust3d
