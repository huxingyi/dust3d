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

#include <dust3d/animation/biped/gait.h>
#include <dust3d/animation/biped/gait_variant.h>
namespace dust3d {
namespace biped {
    bool gaitMotion(const RigStructure& rig, const std::map<std::string, Matrix4x4>& inverse,
        RigAnimationClip& clip, const AnimationParams& params, GaitKind kind)
    {
        auto style = kind == GaitKind::Sprint ? runStyle(params) : walkStyle(params);
        if (kind == GaitKind::Sprint) {
            style.stanceFraction = 0.28;
            style.footSpan *= 1.2;
            style.liftHeight *= 1.15;
            style.lean *= 1.5;
            style.armSwing *= 1.25;
            style.bob *= 1.4;
        } else {
            style.stanceFraction = 0.78;
            style.footSpan *= 0.45;
            style.liftHeight *= 0.45;
            style.extraCrouch += 0.28;
            style.lean = 0.2;
            style.armSwing *= 0.25;
            style.elbowBend = 0.7;
            style.heelStrikePitch = style.toeOffPitch = 0;
            style.sway *= 0.5;
            style.hairStiffness = 0.22;
            style.hairDamping = 0.97;
        }
        return locomote(rig, inverse, clip, params, style);
    }
} // namespace biped
} // namespace dust3d
