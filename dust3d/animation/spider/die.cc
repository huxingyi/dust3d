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

// Procedural death for the spider rig (spiders, scorpions and crabs).
// See arthropod_die.h.
//
// Adjustable animation parameters:
//   - collapseSpeedFactor:  how fast the body goes down (> 1 = sooner)
//   - legCurlFactor:        1 = the legs curl in and up (the death curl), 0 = they splay flat
//   - flipOver:             0 = stays upright, 1 = rolls onto its back
//   - twitchFactor:         how much the legs twitch as it dies
//   - legSpreadFactor:      how far splayed legs slide out
//   - groundBounce:         how much the body rebounds when it hits the ground

#include <dust3d/animation/arthropod_die.h>
#include <dust3d/animation/spider/die.h>

namespace dust3d {

namespace spider {

    bool die(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        static const std::vector<animation::ArthropodLeg> legs = {
            { "FrontLeftCoxa", "FrontLeftFemur", "FrontLeftTibia" },
            { "FrontRightCoxa", "FrontRightFemur", "FrontRightTibia" },
            { "MidFrontLeftCoxa", "MidFrontLeftFemur", "MidFrontLeftTibia" },
            { "MidFrontRightCoxa", "MidFrontRightFemur", "MidFrontRightTibia" },
            { "MidBackLeftCoxa", "MidBackLeftFemur", "MidBackLeftTibia" },
            { "MidBackRightCoxa", "MidBackRightFemur", "MidBackRightTibia" },
            { "BackLeftCoxa", "BackLeftFemur", "BackLeftTibia" },
            { "BackRightCoxa", "BackRightFemur", "BackRightTibia" }
        };
        return animation::arthropodDeath(rigStructure, inverseBindMatrices, animationClip, parameters,
            { "Cephalothorax" }, "Head", "Abdomen", legs);
    }

} // namespace spider

} // namespace dust3d
