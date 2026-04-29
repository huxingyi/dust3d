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

#ifndef DUST3D_ANIMATION_FISH_SWIM_H_
#define DUST3D_ANIMATION_FISH_SWIM_H_

#include <dust3d/animation/animation_generator.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace fish {

    /**
     * Generate swim animation for fish rigs.
     *
     * Creates a natural undulating swimming motion with customizable parameters
     * for different fish species characteristics. The animation generates a
     * traveling sinusoidal wave from head to tail, with appropriate fin movements.
     *
     * Parameters exposed through AnimationParams:
     *
     * Swimming Behavior:
     *   swimSpeed (1.0): Overall animation speed multiplier
     *   swimFrequency (2.0): Tail beat frequency multiplier
     *
     * Body Motion:
     *   spineAmplitude (0.15): Spine side-to-side undulation amplitude in radians
     *   waveLength (1.0): Undulation wavelength factor
     *   tailAmplitudeRatio (1.5): Tail amplitude vs body amplitude ratio
     *   bodyBob (0.02): Vertical bobbing amplitude
     *   bodyRoll (0.05): Subtle body roll amplitude in radians
     *   forwardThrust (0.08): Forward surge amplitude
     *
     * Fin Motion:
     *   pectoralFlapPower (0.4): Pectoral fin flap amplitude
     *   pelvicFlapPower (0.25): Pelvic fin flap amplitude
     *   dorsalSwayPower (0.2): Dorsal fin sway amplitude
     *   ventralSwayPower (0.2): Ventral fin sway amplitude
     *   pectoralPhaseOffset (0.0): Pectoral fin timing offset
     *   pelvicPhaseOffset (0.5): Pelvic fin timing offset
     *
     * Required bones: Root, Head, BodyFront, BodyMid, BodyRear, TailStart, TailEnd
     * Optional bones: Dorsal/Ventral/Pectoral/Pelvic fins
     */
    bool swim(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters = AnimationParams());

} // namespace fish

} // namespace dust3d

#endif // DUST3D_ANIMATION_FISH_SWIM_H_
