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

#include <algorithm>
#include <dust3d/animation/biped/strafe_left.h>
#include <dust3d/animation/biped/walk.h>
namespace dust3d {
namespace biped {
    bool strafeLeft(const RigStructure& rig, const std::map<std::string, Matrix4x4>& inverse,
        RigAnimationClip& clip, const AnimationParams& params)
    {
        AnimationParams p = params;
        p.setValue("travelAngleDegrees", 90);
        // Short steps keep the feet facing the target without crossing the knees.
        p.setValue("stepLengthFactor", std::clamp(params.getValue("stepLengthFactor", 1.0), 0.0, 1.5) * 0.32);
        p.setValue("footRollFactor", 0.0);
        p.setValue("leanForwardFactor", 0.0);
        return walk(rig, inverse, clip, p);
    }
} // namespace biped
} // namespace dust3d
