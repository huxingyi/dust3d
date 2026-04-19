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

#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/biped/die.h>
#include <dust3d/animation/biped/idle.h>
#include <dust3d/animation/biped/jump.h>
#include <dust3d/animation/biped/roar.h>
#include <dust3d/animation/biped/run.h>
#include <dust3d/animation/biped/walk.h>
#include <dust3d/animation/bird/attack.h>
#include <dust3d/animation/bird/die.h>
#include <dust3d/animation/bird/forward.h>
#include <dust3d/animation/bird/glide.h>
#include <dust3d/animation/bird/idle.h>
#include <dust3d/animation/bird/walk.h>
#include <dust3d/animation/common.h>
#include <dust3d/animation/fish/die.h>
#include <dust3d/animation/fish/forward.h>
#include <dust3d/animation/fish/idle.h>
#include <dust3d/animation/insect/attack.h>
#include <dust3d/animation/insect/die.h>
#include <dust3d/animation/insect/forward.h>
#include <dust3d/animation/insect/idle.h>
#include <dust3d/animation/insect/rub_hands.h>
#include <dust3d/animation/insect/walk.h>
#include <dust3d/animation/quadruped/attack.h>
#include <dust3d/animation/quadruped/die.h>
#include <dust3d/animation/quadruped/eat.h>
#include <dust3d/animation/quadruped/hurt.h>
#include <dust3d/animation/quadruped/idle.h>
#include <dust3d/animation/quadruped/run.h>
#include <dust3d/animation/quadruped/walk.h>
#include <dust3d/animation/snake/die.h>
#include <dust3d/animation/snake/forward.h>
#include <dust3d/animation/snake/idle.h>
#include <dust3d/animation/spider/die.h>
#include <dust3d/animation/spider/idle.h>
#include <dust3d/animation/spider/run.h>
#include <dust3d/animation/spider/walk.h>

namespace dust3d {

bool AnimationGenerator::generate(const RigStructure& rigStructure,
    const std::map<std::string, Matrix4x4>& inverseBindMatrices,
    RigAnimationClip& animationClip,
    const std::string& animationType,
    const AnimationParams& parameters)
{
    bool result = false;

    if (animationType == "InsectWalk")
        result = insect::walk(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "InsectIdle")
        result = insect::idle(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "InsectRubHands")
        result = insect::rubHands(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "InsectForward")
        result = insect::forward(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "InsectAttack")
        result = insect::attack(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "InsectDie")
        result = insect::die(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BirdForward")
        result = bird::forward(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BirdGlide")
        result = bird::glide(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BirdIdle")
        result = bird::idle(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BirdAttack")
        result = bird::attack(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BirdWalk")
        result = bird::walk(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "FishForward")
        result = fish::forward(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "FishIdle")
        result = fish::idle(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "FishDie")
        result = fish::die(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "SnakeForward")
        result = snake::forward(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "SnakeIdle")
        result = snake::idle(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "SnakeDie")
        result = snake::die(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BipedWalk")
        result = biped::walk(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BipedIdle")
        result = biped::idle(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BipedRun")
        result = biped::run(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BipedJump")
        result = biped::jump(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BipedRoar")
        result = biped::roar(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BipedDie")
        result = biped::die(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "QuadrupedWalk")
        result = quadruped::walk(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "QuadrupedIdle")
        result = quadruped::idle(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "QuadrupedRun")
        result = quadruped::run(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "QuadrupedAttack")
        result = quadruped::attack(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "QuadrupedHurt")
        result = quadruped::hurt(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "QuadrupedEat")
        result = quadruped::eat(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "QuadrupedDie")
        result = quadruped::die(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BirdDie")
        result = bird::die(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "SpiderDie")
        result = spider::die(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "SpiderIdle")
        result = spider::idle(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "SpiderWalk")
        result = spider::walk(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "SpiderRun")
        result = spider::run(rigStructure, inverseBindMatrices, animationClip, parameters);

    if (!result)
        return false;

    // Determine movement speed and direction based on animation type
    float speedFactor = 0.0f;
    if (animationType.find("Run") != std::string::npos)
        speedFactor = 2.0f;
    else if (animationType.find("Walk") != std::string::npos)
        speedFactor = 1.0f;
    else if (animationType.find("Forward") != std::string::npos)
        speedFactor = 1.0f;
    else if (animationType.find("Glide") != std::string::npos)
        speedFactor = 1.2f;
    else if (animationType.find("Jump") != std::string::npos)
        speedFactor = 1.5f;

    if (speedFactor > 0.0f) {
        // Compute forward direction from rig bone positions
        auto boneIdx = animation::buildBoneIndexMap(rigStructure);
        Vector3 forward(0.0f, 0.0f, 1.0f);

        // Try common bone patterns to determine forward direction
        auto tryForward = [&](const std::string& headBone, const std::string& tailBone) -> bool {
            auto headIt = boneIdx.find(headBone);
            auto tailIt = boneIdx.find(tailBone);
            if (headIt == boneIdx.end() || tailIt == boneIdx.end())
                return false;
            const auto& hb = rigStructure.bones[headIt->second];
            const auto& tb = rigStructure.bones[tailIt->second];
            Vector3 dir(hb.posX - tb.posX, 0.0f, hb.posZ - tb.posZ);
            if (dir.lengthSquared() > 1e-8f) {
                forward = dir.normalized();
                return true;
            }
            return false;
        };

        // Biped: use hips-to-feet direction (same as walk animation)
        auto tryBipedForward = [&]() -> bool {
            auto hipsIt = boneIdx.find("Hips");
            auto leftFootIt = boneIdx.find("LeftFoot");
            auto rightFootIt = boneIdx.find("RightFoot");
            if (hipsIt == boneIdx.end() || leftFootIt == boneIdx.end() || rightFootIt == boneIdx.end())
                return false;
            const auto& hips = rigStructure.bones[hipsIt->second];
            const auto& lf = rigStructure.bones[leftFootIt->second];
            const auto& rf = rigStructure.bones[rightFootIt->second];
            float avgEndX = (lf.endX + rf.endX) * 0.5f;
            float avgEndZ = (lf.endZ + rf.endZ) * 0.5f;
            Vector3 dir(avgEndX - hips.posX, 0.0f, avgEndZ - hips.posZ);
            if (dir.lengthSquared() > 1e-8f) {
                forward = dir.normalized();
                return true;
            }
            return false;
        };

        // Spider/Insect: Head -> Abdomen
        if (!tryForward("Head", "Abdomen"))
            // Quadruped/Bird: Head -> Tail or Head -> Hip
            if (!tryForward("Head", "Tail"))
                if (!tryForward("Head", "Hip"))
                    // Biped: Hips -> average foot end position
                    if (!tryBipedForward())
                        // Fish/Snake: Head -> Body
                        tryForward("Head", "Body");

        // Compute body scale for speed normalization
        float bodyScale = 0.0f;
        for (const auto& bone : rigStructure.bones) {
            Vector3 boneStart(bone.posX, bone.posY, bone.posZ);
            Vector3 boneEnd(bone.endX, bone.endY, bone.endZ);
            float len = (boneEnd - boneStart).length();
            if (len > bodyScale)
                bodyScale = len;
        }
        if (bodyScale < 1e-6f)
            bodyScale = 1.0f;

        float speed = speedFactor * bodyScale;
        animationClip.movementSpeed = speed;
        animationClip.movementDirectionX = (float)forward.x();
        animationClip.movementDirectionZ = (float)forward.z();
    }

    return true;
}

} // namespace dust3d
