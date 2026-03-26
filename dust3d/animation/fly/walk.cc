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
#include <utility>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/fly/walk.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace fly {

bool walk(const RigStructure& rigStructure,
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
    const int samples = frameCount;

    animationClip.name = "fly_walk";
    animationClip.durationSeconds = durationSeconds;
    animationClip.frames.reserve(samples);

    auto lookupLegPhase = [&](const std::string& boneName) -> double {
        bool isTripodA = boneName.find("FrontLeft") != std::string::npos
            || boneName.find("MiddleRight") != std::string::npos
            || boneName.find("BackLeft") != std::string::npos;
        bool isTripodB = boneName.find("FrontRight") != std::string::npos
            || boneName.find("MiddleLeft") != std::string::npos
            || boneName.find("BackRight") != std::string::npos;

        if (isTripodA)
            return 0.0;
        if (isTripodB)
            return pi;
        return 0.0;
    };

    auto makePivotRotation = [&](const Vector3& pivot, const Vector3& axis, float angle) {
        Matrix4x4 transform;
        transform.translate(pivot);
        transform.rotate(Quaternion::fromAxisAndAngle(axis, angle));
        transform.translate(-pivot);
        return transform;
    };

    auto findBone = [&](const std::string& name) -> const RigNode* {
        for (const auto& bone : rigStructure.bones) {
            if (bone.name == name)
                return &bone;
        }
        return nullptr;
    };

    const RigNode* thoraxNode = findBone("Thorax");
    Vector3 bodyAxis(0.0, 0.0, 1.0);
    Vector3 bodyUp(0.0, 1.0, 0.0);

    auto boneVector = [&](const RigNode* node) -> Vector3 {
        if (!node)
            return Vector3();
        return Vector3(node->endX - node->posX,
            node->endY - node->posY,
            node->endZ - node->posZ);
    };

    const RigNode* leftWingNode = findBone("LeftWing");
    const RigNode* rightWingNode = findBone("RightWing");

    if (thoraxNode) {
        Vector3 thoraxDir = boneVector(thoraxNode);
        if (!thoraxDir.isZero())
            bodyAxis = thoraxDir.normalized();

        if (leftWingNode && rightWingNode) {
            Vector3 wingSpan(rightWingNode->posX - leftWingNode->posX,
                rightWingNode->posY - leftWingNode->posY,
                rightWingNode->posZ - leftWingNode->posZ);
            if (!wingSpan.isZero()) {
                bodyUp = Vector3::crossProduct(bodyAxis, wingSpan).normalized();
                if (bodyUp.isZero())
                    bodyUp = Vector3(0.0, 1.0, 0.0);
            }
        }
    }

    Vector3 bodyRight = Vector3::crossProduct(bodyUp, bodyAxis).normalized();
    if (bodyRight.isZero())
        bodyRight = Vector3(1.0, 0.0, 0.0);
    else
        bodyUp = Vector3::crossProduct(bodyAxis, bodyRight).normalized();

    std::map<std::string, std::vector<std::string>> childBones;
    for (const auto& boneNode : rigStructure.bones) {
        if (!boneNode.parent.empty())
            childBones[boneNode.parent].push_back(boneNode.name);
    }

    auto gatherDescendants = [&](const std::string& rootBoneName) {
        std::vector<std::string> descendants;
        std::vector<std::string> stack = { rootBoneName };
        while (!stack.empty()) {
            std::string current = stack.back();
            stack.pop_back();
            descendants.push_back(current);
            auto childIt = childBones.find(current);
            if (childIt == childBones.end())
                continue;
            for (const auto& childName : childIt->second)
                stack.push_back(childName);
        }
        return descendants;
    };

    struct LegChain {
        std::string coxa;
        std::string femur;
        std::string tibia;
        float femurLength = 0.0f;
        float tibiaLength = 0.0f;
        Vector3 restFootPos;
        double phaseOffset = 0.0;
        float stepLength = 0.10f;
        float stepHeight = 0.035f;
        float stanceDrop = 0.012f;
        float dutyFactor = 0.68f;
        float coxaSwing = 0.18f;
        std::vector<std::string> coxaAffectedBones;
        std::vector<std::string> femurAffectedBones;
        std::vector<std::string> tibiaAffectedBones;
    };

    std::vector<LegChain> legChains;
    std::map<std::string, const RigNode*> boneNodeMap;
    for (const auto& boneNode : rigStructure.bones)
        boneNodeMap.emplace(boneNode.name, &boneNode);

    auto findChildBone = [&](const std::string& parentName, const std::string& contain) -> const RigNode* {
        for (const auto& boneNode : rigStructure.bones) {
            if (boneNode.parent == parentName
                && boneNode.name.find(contain) != std::string::npos)
                return &boneNode;
        }
        return nullptr;
    };

    for (const auto& boneNode : rigStructure.bones) {
        if (boneNode.name.find("Coxa") == std::string::npos)
            continue;

        const RigNode* femurNode = findChildBone(boneNode.name, "Femur");
        if (!femurNode)
            continue;

        const RigNode* tibiaNode = findChildBone(femurNode->name, "Tibia");
        if (!tibiaNode)
            continue;

        LegChain chain;
        chain.coxa = boneNode.name;
        chain.femur = femurNode->name;
        chain.tibia = tibiaNode->name;
        chain.femurLength = boneVector(femurNode).length();
        chain.tibiaLength = boneVector(tibiaNode).length();
        chain.phaseOffset = lookupLegPhase(chain.coxa);

        if (chain.coxa.find("Front") != std::string::npos) {
            chain.stepLength = 0.08f;
            chain.stepHeight = 0.040f;
            chain.coxaSwing = 0.20f;
        } else if (chain.coxa.find("Middle") != std::string::npos) {
            chain.stepLength = 0.11f;
            chain.stepHeight = 0.032f;
            chain.coxaSwing = 0.16f;
        } else if (chain.coxa.find("Back") != std::string::npos) {
            chain.stepLength = 0.09f;
            chain.stepHeight = 0.028f;
            chain.coxaSwing = 0.14f;
        }

        chain.coxaAffectedBones = gatherDescendants(chain.coxa);
        chain.femurAffectedBones = gatherDescendants(chain.femur);
        chain.tibiaAffectedBones = gatherDescendants(chain.tibia);

        auto tibiaIt = baseWorldTransforms.find(chain.tibia);
        if (tibiaIt != baseWorldTransforms.end())
            chain.restFootPos = tibiaIt->second.transformPoint(Vector3(0.0, 0.0, chain.tibiaLength));

        legChains.push_back(chain);
    }

    const std::vector<std::string> thoraxAffectedBones = gatherDescendants("Thorax");

    auto clampAngle = [&](double value, double minVal, double maxVal) {
        if (value < minVal) return minVal;
        if (value > maxVal) return maxVal;
        return value;
    };

    auto signedAngleBetween = [&](const Vector3& from, const Vector3& to, const Vector3& normal) {
        Vector3 fromNorm = from.normalized();
        Vector3 toNorm = to.normalized();
        double cosAngle = Vector3::dotProduct(fromNorm, toNorm);
        cosAngle = clampAngle(cosAngle, -1.0, 1.0);
        double angle = std::acos(cosAngle);
        Vector3 cross = Vector3::crossProduct(fromNorm, toNorm);
        if (Vector3::dotProduct(cross, normal) < 0)
            angle = -angle;
        return (float)angle;
    };

    auto solveTwoBoneIK = [&](const Vector3& root,
            const Vector3& joint,
            const Vector3& end,
            const Vector3& target,
            const Vector3& axis,
            float femurLength,
            float tibiaLength,
            float& outShoulder,
            float& outKnee) -> bool {
        Vector3 toJoint = joint - root;
        Vector3 toEnd = end - joint;
        Vector3 toTarget = target - root;

        if (toJoint.isZero() || toEnd.isZero() || toTarget.isZero())
            return false;

        float targetDist = (float)toTarget.length();
        float maxReach = femurLength + tibiaLength;
        float minReach = std::fabs(femurLength - tibiaLength);
        if (targetDist < 1e-5f)
            return false;

        float clampedDist = std::min(maxReach, std::max(minReach + 1e-5f, targetDist));

        float cosKnee = (femurLength * femurLength + tibiaLength * tibiaLength - clampedDist * clampedDist) /
            (2.0f * femurLength * tibiaLength);
        cosKnee = clampAngle(cosKnee, -1.0, 1.0);

        float desiredKneeAngle = (float)std::acos(cosKnee);

        float currentKneeAngle = (float)Vector3::angle(toJoint, toEnd);
        outKnee = desiredKneeAngle - currentKneeAngle;

        Vector3 fromDir = toJoint.normalized();
        Vector3 targetDir = toTarget.normalized();
        Vector3 axisNorm = axis;
        if (axisNorm.isZero())
            axisNorm = bodyRight;
        else
            axisNorm.normalize();

        outShoulder = signedAngleBetween(fromDir, targetDir, axisNorm);
        return true;
    };

    auto applyWorldTransform = [](const Matrix4x4& transform,
            const std::vector<std::string>& affectedBones,
            std::map<std::string, Matrix4x4>& boneWorldTransforms) {
        for (const auto& boneName : affectedBones) {
            auto it = boneWorldTransforms.find(boneName);
            if (it == boneWorldTransforms.end())
                continue;
            Matrix4x4 updated = transform;
            updated *= it->second;
            it->second = updated;
        }
    };

    auto smoothstep = [](float value) {
        return value * value * (3.0f - 2.0f * value);
    };

    struct GaitSample {
        float strideOffset = 0.0f;
        float verticalOffset = 0.0f;
        float coxaAngle = 0.0f;
    };

    auto sampleTripodGait = [&](const LegChain& chain, double framePhase) {
        double cycle = (framePhase + chain.phaseOffset) / (2.0 * pi);
        cycle -= std::floor(cycle);

        GaitSample sample;
        if (cycle < chain.dutyFactor) {
            float stanceT = (float)(cycle / chain.dutyFactor);
            float strideT = -1.0f + 2.0f * stanceT;
            sample.strideOffset = strideT * chain.stepLength * 0.5f;
            sample.verticalOffset = -chain.stanceDrop;
            sample.coxaAngle = -strideT * chain.coxaSwing;
        } else {
            float swingT = (float)((cycle - chain.dutyFactor) / (1.0 - chain.dutyFactor));
            float easedSwing = smoothstep(swingT);
            float strideT = 1.0f - 2.0f * easedSwing;
            sample.strideOffset = strideT * chain.stepLength * 0.5f;
            sample.verticalOffset = std::sin((float)(pi * swingT)) * chain.stepHeight;
            sample.coxaAngle = -strideT * chain.coxaSwing * 0.85f;
        }

        return sample;
    };

    for (int frameIndex = 0; frameIndex < samples; ++frameIndex) {
        float time = durationSeconds * (float)frameIndex / (float)samples;
        double phase = 2.0 * pi * ((double)frameIndex / (double)samples);

        float bodyBob = (float)(std::sin(phase * 2.0) * 0.006);
        float bodyRoll = (float)(std::sin(phase) * 0.025);
        float wingPhase = (float)(std::sin(phase * 3.0) * 0.8);

        BoneAnimationFrame frame;
        frame.time = time;
        frame.boneWorldTransforms = baseWorldTransforms;

        if (!thoraxAffectedBones.empty()) {
            Matrix4x4 bodyLift;
            bodyLift.translate(bodyUp * bodyBob);
            applyWorldTransform(bodyLift, thoraxAffectedBones, frame.boneWorldTransforms);

            auto thoraxIt = frame.boneWorldTransforms.find("Thorax");
            if (thoraxIt != frame.boneWorldTransforms.end()) {
                Vector3 thoraxPivot = thoraxIt->second.transformPoint(Vector3(0.0, 0.0, 0.0));
                Matrix4x4 thoraxRoll = makePivotRotation(thoraxPivot, bodyAxis, bodyRoll);
                applyWorldTransform(thoraxRoll, thoraxAffectedBones, frame.boneWorldTransforms);
            }
        }

        for (const auto& boneNode : rigStructure.bones) {
            if (boneNode.name == "LeftWing" || boneNode.name == "RightWing") {
                auto wingIt = frame.boneWorldTransforms.find(boneNode.name);
                if (wingIt == frame.boneWorldTransforms.end())
                    continue;
                float sideSign = (boneNode.name == "LeftWing") ? 1.0f : -1.0f;
                Vector3 wingPivot = wingIt->second.transformPoint(Vector3(0.0, 0.0, 0.0));
                Matrix4x4 wingRot = makePivotRotation(wingPivot, bodyRight, sideSign * wingPhase);
                applyWorldTransform(wingRot, { boneNode.name }, frame.boneWorldTransforms);
            }
        }

        std::map<std::string, GaitSample> gaitSamples;
        for (const auto& chain : legChains) {
            GaitSample gait = sampleTripodGait(chain, phase);
            gaitSamples.emplace(chain.coxa, gait);

            auto itCoxa = frame.boneWorldTransforms.find(chain.coxa);
            if (itCoxa == frame.boneWorldTransforms.end())
                continue;

            Vector3 coxaPivot = itCoxa->second.transformPoint(Vector3(0.0, 0.0, 0.0));
            Matrix4x4 coxaRot = makePivotRotation(coxaPivot, bodyUp, gait.coxaAngle);
            applyWorldTransform(coxaRot, chain.coxaAffectedBones, frame.boneWorldTransforms);
        }

        for (const auto& chain : legChains) {
            auto itCoxa = frame.boneWorldTransforms.find(chain.coxa);
            auto itFemur = frame.boneWorldTransforms.find(chain.femur);
            auto itTibia = frame.boneWorldTransforms.find(chain.tibia);
            if (itCoxa == frame.boneWorldTransforms.end()
                || itFemur == frame.boneWorldTransforms.end()
                || itTibia == frame.boneWorldTransforms.end())
                continue;

            Vector3 rootPos = itCoxa->second.transformPoint(Vector3(0.0, 0.0, 0.0));
            Vector3 jointPos = itFemur->second.transformPoint(Vector3(0.0, 0.0, 0.0));
            Vector3 endPos = itTibia->second.transformPoint(Vector3(0.0, 0.0, chain.tibiaLength));

            const auto gaitIt = gaitSamples.find(chain.coxa);
            if (gaitIt == gaitSamples.end())
                continue;
            const GaitSample& gait = gaitIt->second;

            Vector3 desiredFoot = chain.restFootPos
                + bodyAxis * gait.strideOffset
                + bodyUp * gait.verticalOffset;

            Vector3 legAxis = Vector3::crossProduct(bodyAxis, (jointPos - rootPos));
            if (legAxis.isZero())
                legAxis = bodyRight;
            else
                legAxis.normalize();

            float shoulderDelta = 0.0f;
            float kneeDelta = 0.0f;
            if (solveTwoBoneIK(rootPos, jointPos, endPos, desiredFoot, legAxis,
                                chain.femurLength, chain.tibiaLength,
                                shoulderDelta, kneeDelta)) {
                Matrix4x4 femurRot = makePivotRotation(jointPos, legAxis, shoulderDelta);
                applyWorldTransform(femurRot, chain.femurAffectedBones, frame.boneWorldTransforms);

                auto tibiaUpdatedIt = frame.boneWorldTransforms.find(chain.tibia);
                if (tibiaUpdatedIt == frame.boneWorldTransforms.end())
                    continue;
                Vector3 tibiaPivot = tibiaUpdatedIt->second.transformPoint(Vector3(0.0, 0.0, 0.0));
                Matrix4x4 tibiaRot = makePivotRotation(tibiaPivot, legAxis, kneeDelta);
                applyWorldTransform(tibiaRot, chain.tibiaAffectedBones, frame.boneWorldTransforms);
            }
        }

        frame.boneSkinMatrices.clear();
        for (const auto& pair : frame.boneWorldTransforms) {
            auto inverseIt = inverseBindMatrices.find(pair.first);
            if (inverseIt == inverseBindMatrices.end())
                continue;
            Matrix4x4 skinTransform = pair.second;
            skinTransform *= inverseIt->second;
            frame.boneSkinMatrices.emplace(pair.first, skinTransform);
        }

        animationClip.frames.push_back(std::move(frame));
    }

    return true;
}

} // namespace fly

} // namespace dust3d
