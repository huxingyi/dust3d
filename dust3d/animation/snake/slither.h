#ifndef DUST3D_ANIMATION_SNAKE_SLITHER_H_
#define DUST3D_ANIMATION_SNAKE_SLITHER_H_

#include <dust3d/animation/animation_generator.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace snake {

    /**
 * Generate slither animation for snake rigs.
 *
 * The snake moves in a treadmill-like locomotion cycle: the body wave travels
 * from tail to head while the global head position remains locked in world
 * space. This creates the impression of forward motion through a traveling
 * sine wave along a flexible backbone.
 *
 * Parameters exposed through AnimationParams:
 *   waveSpeed (1.0): Overall animation speed multiplier.
 *   waveFrequency (2.0): Number of wave cycles along the body.
 *   waveAmplitude (0.15): Side-to-side wave amplitude in radians.
 *   waveLength (1.0): Normalized wavelength along the body length.
 *   tailAmplitudeRatio (2.5): Tail amplitude multiplier relative to head.
 *   headYawFactor (0.05): Small head yaw rotation in radians.
 *   headPullFactor (0.3): Strength of head-to-tail straightening.
 *   jawAmplitude (0.25): Jaw opening amplitude.
 *   jawFrequency (1.0): Jaw oscillation speed factor.
 *
 * Required bones: Root, Head, Spine1..Spine6, Tail1..TailTip
 * Optional bones: Jaw
 */
    bool slither(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters = AnimationParams());

} // namespace snake

} // namespace dust3d

#endif // DUST3D_ANIMATION_SNAKE_SLITHER_H_
