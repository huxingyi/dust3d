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

#ifndef DUST3D_ANIMATION_BIPED_CLIP_CATALOG_H_
#define DUST3D_ANIMATION_BIPED_CLIP_CATALOG_H_
#include <dust3d/animation/animation_generator.h>
namespace dust3d {
namespace biped {
    struct ClipDefinition {
        const char* type;
        double duration;
        int samples;
        bool loop;
        const char* entryPose;
        const char* exitPose;
        bool (*generate)(const RigStructure&, const std::map<std::string, Matrix4x4>&, RigAnimationClip&, const AnimationParams&);
    };
    const std::vector<ClipDefinition>& additionalClips();
    const ClipDefinition* additionalClip(const std::string& type);
} // namespace biped
} // namespace dust3d
#endif
