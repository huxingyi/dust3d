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

#ifndef DUST3D_ANIMATION_FISH_DIE_H_
#define DUST3D_ANIMATION_FISH_DIE_H_

#include <dust3d/animation/animation_generator.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace fish {

    /**
     * Generate a fish death animation for fish rigs.
     *
     * Simulates a dramatic death sequence: a violent impact jerk, rapid
     * body thrashing that decays, and a slow roll to upside-down (belly-up),
     * ending with the fish floating motionless on its back.
     *
     * Animation phases:
     *   Impact     (0–20%): sudden lateral hit jerk + high-frequency thrash
     *   Rolling   (20–80%): decaying thrash while body rolls 180° around its spine axis
     *   Settled   (80–100%): body fully belly-up, minor residual drift
     *
     * Parameters exposed through AnimationParams:
     *
     *   fishDieHitIntensity (1.0): Amplitude of the initial impact jerk (scales with body size)
     *   fishDieHitFrequency (8.0): Frequency of the initial thrash oscillation (cycles)
     *   fishDieFlipSpeed    (1.0): Speed multiplier for the body-roll flip
     *   fishDieFinFlop      (1.0): Amplitude of fin flopping during death
     *   fishDieSpinDecay    (4.0): Exponential decay rate for thrash amplitude
     *
     * Required bones: Root, Head, BodyFront, BodyMid, BodyRear, TailStart, TailEnd
     * Optional bones: Dorsal/Ventral/Pectoral/Pelvic fins
     */
    bool die(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters = AnimationParams());

} // namespace fish

} // namespace dust3d

#endif // DUST3D_ANIMATION_FISH_DIE_H_
