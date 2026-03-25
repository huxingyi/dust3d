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

#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

bool AnimationGenerator::generateFlyWalkCycle(const RigStructure& rigStructure,
    const std::map<std::string, Matrix4x4>& inverseBindMatrices,
    RigAnimationClip& animationClip,
    int frameCount,
    float durationSeconds)
{
    animationClip = RigAnimationClip();

    if (rigStructure.type != "fly")
        return false;

    if (frameCount <= 0 || durationSeconds <= 0.0f)
        return false;

    if (inverseBindMatrices.empty())
        return false;

    std::map<std::string, Matrix4x4> baseWorldTransforms;
    RigGenerator rigGenerator;
    if (!rigGenerator.computeBoneWorldTransforms(rigStructure, baseWorldTransforms))
        return false;

    const double pi = 3.14159265358979323846;
    const double twoPi = 2.0 * pi;
    const int samples = frameCount;

    animationClip.name = "fly_walk";
    animationClip.durationSeconds = durationSeconds;
    animationClip.frames.reserve(samples);

    auto lookupLegPhase = [&](const std::string& boneName) -> double {
        bool isLeft = boneName.find("Left") != std::string::npos;
        bool isRight = boneName.find("Right") != std::string::npos;

        if (!isLeft && !isRight)
            return 0.0;

        return isRight ? pi : 0.0;
    };

    for (int frameIndex = 0; frameIndex < samples; ++frameIndex) {
        float time = durationSeconds * (float)frameIndex / (float)samples;
        double phase = twoPi * ((double)frameIndex / (double)samples);

        float bodyBob = (float)(std::sin(phase * 2.0) * 0.012);
        float bodyRoll = (float)(std::sin(phase * 2.0 + pi * 0.5) * 0.03);
        float wingPhase = (float)(std::sin(phase * 3.0) * 0.8);

        BoneAnimationFrame frame;
        frame.time = time;

        for (const auto& boneNode : rigStructure.bones) {
            auto itBase = baseWorldTransforms.find(boneNode.name);
            if (itBase == baseWorldTransforms.end())
                continue;

            Matrix4x4 boneTransform = itBase->second;

            if (boneNode.name == "Thorax") {
                Matrix4x4 bodyMotion;
                bodyMotion.translate(Vector3(0.0, bodyBob, 0.0));
                Matrix4x4 bodyRotation;
                bodyRotation.rotate(Quaternion::fromAxisAndAngle(Vector3(0.0, 0.0, 1.0), bodyRoll));
                bodyMotion *= bodyRotation;
                boneTransform *= bodyMotion;
            }

            if (boneNode.name == "LeftWing" || boneNode.name == "RightWing") {
                float sideSign = (boneNode.name == "LeftWing") ? 1.0f : -1.0f;
                Matrix4x4 wingRot;
                wingRot.rotate(Quaternion::fromAxisAndAngle(Vector3(1.0, 0.0, 0.0), sideSign * wingPhase));
                boneTransform *= wingRot;
            }

            if (boneNode.name.find("Coxa") != std::string::npos
                || boneNode.name.find("Femur") != std::string::npos
                || boneNode.name.find("Tibia") != std::string::npos) {

                double phaseOffset = lookupLegPhase(boneNode.name);
                double footPhase = phase + phaseOffset;
                if (boneNode.name.find("Coxa") != std::string::npos) {
                    float coxaAngle = (float)(std::sin(footPhase) * 0.28);
                    Matrix4x4 coxaRot;
                    coxaRot.rotate(Quaternion::fromAxisAndAngle(Vector3(0.0, 1.0, 0.0), coxaAngle));
                    boneTransform *= coxaRot;
                } else if (boneNode.name.find("Femur") != std::string::npos) {
                    float femurAngle = (float)(std::sin(footPhase + pi * 0.45) * 0.22);
                    Matrix4x4 femurRot;
                    femurRot.rotate(Quaternion::fromAxisAndAngle(Vector3(1.0, 0.0, 0.0), femurAngle));
                    boneTransform *= femurRot;
                } else {
                    float tibiaAngle = (float)(std::sin(footPhase + pi * 0.8) * 0.18);
                    Matrix4x4 tibiaRot;
                    tibiaRot.rotate(Quaternion::fromAxisAndAngle(Vector3(1.0, 0.0, 0.0), tibiaAngle));
                    boneTransform *= tibiaRot;
                }
            }

            frame.boneWorldTransforms.emplace(boneNode.name, boneTransform);
            auto inverseIt = inverseBindMatrices.find(boneNode.name);
            if (inverseIt != inverseBindMatrices.end()) {
                Matrix4x4 skinTransform = boneTransform;
                skinTransform *= inverseIt->second;
                frame.boneSkinMatrices.emplace(boneNode.name, skinTransform);
            }
        }

        animationClip.frames.push_back(std::move(frame));
    }

    return true;
}

} // namespace dust3d
