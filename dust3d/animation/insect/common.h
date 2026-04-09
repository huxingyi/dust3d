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

// Insect-specific animation structures and inline forwarding to shared utilities.

#ifndef DUST3D_ANIMATION_INSECT_COMMON_H
#define DUST3D_ANIMATION_INSECT_COMMON_H

#include <dust3d/animation/common.h>

namespace dust3d {
namespace insect {

    // =========================================================================
    // INSECT-SPECIFIC STRUCTURES
    // =========================================================================

    // Describes a single leg chain: coxa (hip), femur (thigh), tibia (shin/foot)
    struct LegDef {
        const char* coxaName;
        const char* femurName;
        const char* tibiaName;
    };

    // Optional: leg definition with gait group assignment (for tripod gaits)
    struct LegDefWithGait : public LegDef {
        int gaitGroup; // 0 = Group A, 1 = Group B
        constexpr LegDefWithGait(LegDef base, int g)
            : LegDef(base)
            , gaitGroup(g)
        {
        }
    };

    // Cached rest-pose geometry for a leg, used to reconstruct pose from IK solutions
    struct LegRest {
        Vector3 coxaPos, coxaEnd;
        Vector3 femurEnd;
        Vector3 tibiaEnd; // end-effector (foot tip)
        // For rigid femur+tibia reconstruction: rest-pose chord direction (coxaEnd->tibiaEnd)
        // and the femur offset vector in that same space. Applying the rotation that maps
        // restStickDir -> newStickDir to restCoxaToFemurVec gives the new femurEnd offset,
        // preserving both bone lengths and the knee bend angle exactly.
        Vector3 restStickDir; // normalized(tibiaEnd - coxaEnd) in rest pose
        Vector3 restCoxaToFemurVec; // (femurEnd - coxaEnd) in rest pose (not normalized)
    };

    // =========================================================================
    // FORWARDING TO SHARED UTILITIES (dust3d::animation namespace)
    // =========================================================================

    using animation::buildBoneIndexMap;
    using animation::buildBoneWorldTransform;
    using animation::getBoneEnd;
    using animation::getBonePos;
    using animation::smootherstep;
    using animation::smoothstep;
    using animation::solveTwoBoneIk;
    using animation::validateRequiredBones;

} // namespace insect
} // namespace dust3d

#endif // DUST3D_ANIMATION_INSECT_COMMON_H
