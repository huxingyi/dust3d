#ifndef DUST3D_ANIMATION_INSECT_DIE_H_
#define DUST3D_ANIMATION_INSECT_DIE_H_

#include <dust3d/animation/animation_generator.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace insect {

    bool die(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters = AnimationParams());

} // namespace insect

} // namespace dust3d

#endif // DUST3D_ANIMATION_INSECT_DIE_H_
