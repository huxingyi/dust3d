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

#include <dust3d/base/string.h>
#include <dust3d/rig/rig_generator.h>
#include <map>
#include <utility>

namespace dust3d {

struct BoneAnimationFrame {
    float time = 0.0f;
    std::map<std::string, Matrix4x4> boneWorldTransforms;
    std::map<std::string, Matrix4x4> boneSkinMatrices;
};

struct RigAnimationEvent {
    std::string name;
    float time = 0;
    std::string bone;
};

struct RigAnimationClip {
    std::string name;
    float durationSeconds = 1.0f;
    std::vector<BoneAnimationFrame> frames;
    bool loop = false; // A terminal key at durationSeconds closes looping clips.
    std::string entryPose;
    std::string exitPose;
    std::vector<RigAnimationEvent> events; // Seconds from clip start, exported with the clip.
    std::string rootMotion = "inPlace"; // Controller supplies world travel.
    float rootYawDegrees = 0.0f; // Turn clips end with this rotation baked into Root.
    float movementSpeed = 0.0f;
    float movementDirectionX = 0.0f;
    float movementDirectionZ = 0.0f;
};

struct AnimationParams {
    std::map<std::string, std::string> values;

    double getValue(const std::string& name, double defaultValue) const
    {
        auto it = values.find(name);
        if (it == values.end())
            return defaultValue;

        try {
            return String::toDouble(it->second);
        } catch (...) {
            return defaultValue;
        }
    }

    bool getBool(const std::string& name, bool defaultValue) const
    {
        auto it = values.find(name);
        if (it == values.end())
            return defaultValue;
        return dust3d::String::isTrue(it->second);
    }

    void setValue(const std::string& name, double value)
    {
        values[name] = String::fromDouble(value);
    }

    void setBool(const std::string& name, bool value)
    {
        values[name] = value ? "true" : "false";
    }
};

class AnimationGenerator {
public:
    AnimationGenerator() = default;
    ~AnimationGenerator() = default;

    static std::pair<double, int> defaultTiming(const std::string& animationType);

    static bool generate(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const std::string& animationName,
        const AnimationParams& parameters = AnimationParams());
};

}

#endif
