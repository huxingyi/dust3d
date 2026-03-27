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

#ifndef DUST3D_ANIMATION_ANIMATION_GENERATOR_H_
#define DUST3D_ANIMATION_ANIMATION_GENERATOR_H_

#include <map>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

struct BoneAnimationFrame {
    float time = 0.0f;
    std::map<std::string, Matrix4x4> boneWorldTransforms;
    std::map<std::string, Matrix4x4> boneSkinMatrices;
};

struct RigAnimationClip {
    std::string name;
    float durationSeconds = 1.0f;
    std::vector<BoneAnimationFrame> frames;
};

struct AnimationParams {
    std::map<std::string, double> values;

    double getValue(const std::string& name, double defaultValue) const
    {
        auto it = values.find(name);
        if (it == values.end())
            return defaultValue;
        return it->second;
    }

    bool getBool(const std::string& name, bool defaultValue) const
    {
        return getValue(name, defaultValue ? 1.0 : 0.0) != 0.0;
    }

    void setValue(const std::string& name, double value)
    {
        values[name] = value;
    }

    void setBool(const std::string& name, bool value)
    {
        values[name] = value ? 1.0 : 0.0;
    }
};

class AnimationGenerator {
public:
    AnimationGenerator() = default;
    ~AnimationGenerator() = default;

    static bool generate(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const std::string& animationName,
        int frameCount = 30,
        float durationSeconds = 1.0f,
        const AnimationParams& parameters = AnimationParams());
};

}

#endif
