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

#include <algorithm>
#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/biped/block.h>
#include <dust3d/animation/biped/cast.h>
#include <dust3d/animation/biped/channel.h>
#include <dust3d/animation/biped/clip_catalog.h>
#include <dust3d/animation/biped/combat_idle.h>
#include <dust3d/animation/biped/die.h>
#include <dust3d/animation/biped/dodge.h>
#include <dust3d/animation/biped/fall.h>
#include <dust3d/animation/biped/hop.h>
#include <dust3d/animation/biped/hurt.h>
#include <dust3d/animation/biped/idle.h>
#include <dust3d/animation/biped/jump.h>
#include <dust3d/animation/biped/jump_start.h>
#include <dust3d/animation/biped/kick.h>
#include <dust3d/animation/biped/land.h>
#include <dust3d/animation/biped/pose.h>
#include <dust3d/animation/biped/roar.h>
#include <dust3d/animation/biped/run.h>
#include <dust3d/animation/biped/slam.h>
#include <dust3d/animation/biped/slash.h>
#include <dust3d/animation/biped/stab.h>
#include <dust3d/animation/biped/strafe_left.h>
#include <dust3d/animation/biped/strafe_right.h>
#include <dust3d/animation/biped/throw.h>
#include <dust3d/animation/biped/turn_left.h>
#include <dust3d/animation/biped/turn_right.h>
#include <dust3d/animation/biped/walk.h>
#include <dust3d/animation/biped/walk_backward.h>
#include <dust3d/animation/bird/attack.h>
#include <dust3d/animation/bird/die.h>
#include <dust3d/animation/bird/eat.h>
#include <dust3d/animation/bird/fly.h>
#include <dust3d/animation/bird/glide.h>
#include <dust3d/animation/bird/hurt.h>
#include <dust3d/animation/bird/idle.h>
#include <dust3d/animation/bird/run.h>
#include <dust3d/animation/bird/strike.h>
#include <dust3d/animation/bird/walk.h>
#include <dust3d/animation/common.h>
#include <dust3d/animation/fish/attack.h>
#include <dust3d/animation/fish/die.h>
#include <dust3d/animation/fish/hurt.h>
#include <dust3d/animation/fish/idle.h>
#include <dust3d/animation/fish/swim.h>
#include <dust3d/animation/insect/attack.h>
#include <dust3d/animation/insect/bite.h>
#include <dust3d/animation/insect/die.h>
#include <dust3d/animation/insect/fly.h>
#include <dust3d/animation/insect/hurt.h>
#include <dust3d/animation/insect/idle.h>
#include <dust3d/animation/insect/rub_hands.h>
#include <dust3d/animation/insect/walk.h>
#include <dust3d/animation/quadruped/attack.h>
#include <dust3d/animation/quadruped/die.h>
#include <dust3d/animation/quadruped/eat.h>
#include <dust3d/animation/quadruped/hurt.h>
#include <dust3d/animation/quadruped/idle.h>
#include <dust3d/animation/quadruped/roar.h>
#include <dust3d/animation/quadruped/run.h>
#include <dust3d/animation/quadruped/walk.h>
#include <dust3d/animation/snake/die.h>
#include <dust3d/animation/snake/hurt.h>
#include <dust3d/animation/snake/idle.h>
#include <dust3d/animation/snake/slither.h>
#include <dust3d/animation/snake/strike.h>
#include <dust3d/animation/spider/attack.h>
#include <dust3d/animation/spider/die.h>
#include <dust3d/animation/spider/hurt.h>
#include <dust3d/animation/spider/idle.h>
#include <dust3d/animation/spider/run.h>
#include <dust3d/animation/spider/walk.h>

namespace dust3d {

std::pair<double, int> AnimationGenerator::defaultTiming(const std::string& type)
{
    static const std::map<std::string, std::pair<double, int>> bipedTiming = {
        { "BipedWalk", { 1.0, 30 } },
        { "BipedRun", { 1.0, 30 } },
        { "BipedIdle", { 4.0, 90 } },
        { "BipedJump", { 1.2, 40 } },
        { "BipedHop", { 0.6, 20 } },
        { "BipedRoar", { 3.0, 120 } },
        { "BipedHurt", { 1.0, 36 } },
        { "BipedDie", { 1.3, 40 } },
        { "BipedSlam", { 0.9, 48 } },
        { "BipedKick", { 0.8, 24 } },
        { "BipedThrow", { 0.9, 36 } },
        { "BipedStab", { 0.7, 48 } },
        { "BipedCast", { 1.0, 48 } },
        { "BipedChannel", { 2.0, 64 } },
        { "BipedCombatIdle", { 2.0, 60 } },
        { "BipedStrafeLeft", { 1.0, 30 } },
        { "BipedStrafeRight", { 1.0, 30 } },
        { "BipedWalkBackward", { 1.0, 30 } },
        { "BipedTurnLeft", { 0.8, 32 } },
        { "BipedTurnRight", { 0.8, 32 } },
        { "BipedJumpStart", { 0.3, 12 } },
        { "BipedFall", { 1.0, 30 } },
        { "BipedLand", { 0.4, 16 } },
        { "BipedSlash", { 0.75, 30 } },
        { "BipedBlock", { 1.5, 45 } },
        { "BipedDodge", { 0.65, 30 } },
    };
    if (const auto* clip = biped::additionalClip(type))
        return { clip->duration, clip->samples };
    auto found = bipedTiming.find(type);
    return found == bipedTiming.end() ? std::make_pair(3.0, 90) : found->second;
}

bool AnimationGenerator::generate(const RigStructure& rigStructure,
    const std::map<std::string, Matrix4x4>& inverseBindMatrices,
    RigAnimationClip& animationClip,
    const std::string& animationType,
    const AnimationParams& inputParameters)
{
    animationClip.frames.clear();
    animationClip.movementSpeed = 0.0f;
    animationClip.movementDirectionX = 0.0f;
    animationClip.movementDirectionZ = 0.0f;
    animationClip.loop = false;
    animationClip.entryPose.clear();
    animationClip.exitPose.clear();
    animationClip.rootYawDegrees = 0.0f;
    animationClip.events.clear();
    animationClip.rootMotion = "inPlace";
    AnimationParams parameters = inputParameters;
    const bool isBiped = animationType.compare(0, 5, "Biped") == 0;
    if (isBiped) {
        // Reject invalid input before conversion to integer sizes or simulation steps.
        for (const auto& value : parameters.values) {
            if (!std::isfinite(parameters.getValue(value.first, 0.0)))
                return false;
        }
        if (parameters.values.count("frameCount"))
            parameters.setValue("frameCount", std::clamp(parameters.getValue("frameCount", 30.0), 2.0, 4999.0));
        if (parameters.values.count("durationSeconds")
            && (parameters.getValue("durationSeconds", 1.0) <= 0.0
                || !std::isfinite(static_cast<float>(parameters.getValue("durationSeconds", 1.0)))))
            return false;
    }
    animationClip.loop = isBiped && (animationType == "BipedIdle" || animationType == "BipedWalk" || animationType == "BipedRun" || animationType == "BipedHop" || animationType == "BipedChannel" || animationType == "BipedCombatIdle" || animationType == "BipedBlock" || animationType == "BipedFall" || animationType == "BipedStrafeLeft" || animationType == "BipedStrafeRight" || animationType == "BipedWalkBackward");
    const auto* definition = isBiped ? biped::additionalClip(animationType) : nullptr;
    if (definition)
        animationClip.loop = definition->loop;
    if (isBiped) {
        auto timing = defaultTiming(animationType);
        if (!parameters.values.count("durationSeconds"))
            parameters.setValue("durationSeconds", timing.first);
        if (!parameters.values.count("frameCount"))
            parameters.setValue("frameCount", timing.second);
        animationClip.entryPose = "relaxed";
        animationClip.exitPose = "relaxed";
        if (animationClip.loop)
            animationClip.entryPose = animationClip.exitPose = "cycle";
        if (animationType == "BipedJumpStart")
            animationClip.exitPose = "airborne";
        if (animationType == "BipedFall")
            animationClip.entryPose = animationClip.exitPose = "airborne";
        if (animationType == "BipedLand")
            animationClip.entryPose = "airborne";
        if (animationType == "BipedTurnLeft" || animationType == "BipedTurnRight") {
            animationClip.exitPose = "turned";
            animationClip.rootMotion = "bakedRotation";
            animationClip.rootYawDegrees = std::clamp(parameters.getValue("turnAngleDegrees", 90.0), 15.0, 120.0)
                * (animationType == "BipedTurnLeft" ? 1.0 : -1.0);
        }
        if (animationType == "BipedDie")
            animationClip.exitPose = "dead";
        if (definition) {
            animationClip.entryPose = definition->entryPose;
            animationClip.exitPose = definition->exitPose;
        }
    }
    bool result = false;
    // Locomotion clips that know their true ground speed (the speed at which their planted
    // feet slide back) report it themselves; the rest get an estimate below.

    if (animationType == "InsectWalk")
        result = insect::walk(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "InsectIdle")
        result = insect::idle(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "InsectRubHands")
        result = insect::rubHands(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "InsectForward" || animationType == "InsectFly")
        result = insect::fly(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "InsectAttack")
        result = insect::attack(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "InsectDie")
        result = insect::die(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BirdForward" || animationType == "BirdFly")
        result = bird::fly(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BirdGlide")
        result = bird::glide(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BirdIdle")
        result = bird::idle(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BirdAttack")
        result = bird::attack(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BirdWalk")
        result = bird::walk(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BirdRun")
        result = bird::run(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "FishForward" || animationType == "FishSwim")
        result = fish::swim(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "FishIdle")
        result = fish::idle(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "FishDie")
        result = fish::die(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "SnakeForward" || animationType == "SnakeSlither")
        result = snake::slither(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "SnakeIdle")
        result = snake::idle(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "SnakeDie")
        result = snake::die(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "SnakeStrike")
        result = snake::strike(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BipedWalk")
        result = biped::walk(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BipedIdle")
        result = biped::idle(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BipedRun")
        result = biped::run(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BipedJump")
        result = biped::jump(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BipedHop")
        result = biped::hop(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BipedRoar")
        result = biped::roar(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BipedHurt")
        result = biped::hurt(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BipedDie")
        result = biped::die(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BipedSlam")
        result = biped::slam(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BipedKick")
        result = biped::kick(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BipedThrow")
        result = biped::hurl(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BipedStab")
        result = biped::stab(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BipedCast")
        result = biped::cast(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BipedChannel")
        result = biped::channel(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BipedCombatIdle")
        result = biped::combatIdle(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BipedBlock")
        result = biped::block(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BipedFall")
        result = biped::fall(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BipedJumpStart")
        result = biped::jumpStart(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BipedLand")
        result = biped::land(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BipedSlash")
        result = biped::slash(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BipedDodge")
        result = biped::dodge(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BipedTurnLeft")
        result = biped::turnLeft(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BipedTurnRight")
        result = biped::turnRight(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BipedStrafeLeft")
        result = biped::strafeLeft(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BipedStrafeRight")
        result = biped::strafeRight(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BipedWalkBackward")
        result = biped::walkBackward(rigStructure, inverseBindMatrices, animationClip, parameters);
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
    else if (animationType == "QuadrupedRoar")
        result = quadruped::roar(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "QuadrupedDie")
        result = quadruped::die(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BirdEat")
        result = bird::eat(rigStructure, inverseBindMatrices, animationClip, parameters);
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
    else if (animationType == "SpiderAttack")
        result = spider::attack(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "SpiderHurt")
        result = spider::hurt(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "SnakeHurt")
        result = snake::hurt(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BirdStrike")
        result = bird::strike(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "BirdHurt")
        result = bird::hurt(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "InsectHurt")
        result = insect::hurt(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "InsectBite")
        result = insect::bite(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "FishAttack")
        result = fish::attack(rigStructure, inverseBindMatrices, animationClip, parameters);
    else if (animationType == "FishHurt")
        result = fish::hurt(rigStructure, inverseBindMatrices, animationClip, parameters);

    if (definition)
        result = definition->generate(rigStructure, inverseBindMatrices, animationClip, parameters);
    if (!result)
        return false;

    // Correct directional rolls before inheriting accessory transforms.
    if (isBiped) {
        animation::referenceRollsToRest(rigStructure, inverseBindMatrices, animationClip);
        const auto rest = animation::restBoneWorldTransforms(rigStructure);
        const auto drape = biped::neutralSecondary(rigStructure, animation::buildBoneIndexMap(rigStructure), rest);
        for (auto& frame : animationClip.frames) {
            if (!definition && animationType != "BipedIdle" && animationType != "BipedWalk" && animationType != "BipedRun"
                && animationType != "BipedJump" && animationType != "BipedRoar" && animationType != "BipedStab"
                && animationType != "BipedCombatIdle" && animationType != "BipedBlock" && animationType != "BipedFall"
                && animationType != "BipedJumpStart" && animationType != "BipedLand" && animationType != "BipedSlash"
                && animationType != "BipedDodge" && animationType != "BipedTurnLeft" && animationType != "BipedTurnRight"
                && animationType != "BipedStrafeLeft" && animationType != "BipedStrafeRight" && animationType != "BipedWalkBackward")
                biped::relaxUndirectedArms(rigStructure, parameters, frame);
            double t = frame.time / animationClip.durationSeconds;
            // Entry/exit states retain the same stationary drape when clips are
            // split; free movement loops keep simulated secondary motion throughout.
            double boundary = 0.0;
            if (!animationClip.loop && animationType != "BipedDie")
                boundary = std::max(1.0 - biped::easePose(t / 0.12), biped::easePose((t - 0.78) / 0.22));
            else if ((animationType == "BipedFall" || animationType == "BipedChannel") || (definition && animationClip.entryPose != "cycle" && animationClip.entryPose != "work" && animationClip.entryPose != "swimming"))
                boundary = std::max(1.0 - biped::easePose(t / 0.12), biped::easePose((t - 0.78) / 0.22));
            biped::applySecondaryNeutral(rigStructure, rest, drape, frame, boundary);
            animation::inheritUndrivenBones(rigStructure, inverseBindMatrices, frame);
        }
    }

    // Post-process: apply eyelid blink if eyelid bones exist
    {
        auto boneIdx = animation::buildBoneIndexMap(rigStructure);
        bool hasEyelids = (boneIdx.count("LeftUpperEyelid") && boneIdx.count("LeftLowerEyelid"))
            || (boneIdx.count("RightUpperEyelid") && boneIdx.count("RightLowerEyelid"));
        if (hasEyelids) {
            for (auto& frame : animationClip.frames) {
                float tNormalized = (animationClip.durationSeconds > 0.0f)
                    ? frame.time / animationClip.durationSeconds
                    : 0.0f;
                animation::applyEyelidBlink(rigStructure, boneIdx, inverseBindMatrices,
                    frame.boneWorldTransforms, frame.boneSkinMatrices, tNormalized);
            }
        }
    }

    if (!isBiped)
        animation::referenceRollsToRest(rigStructure, inverseBindMatrices, animationClip);

    // Post-process: tails never pass through the ground (where the feet stand in the rest
    // pose). Only for rigs that stand on feet: fish swim, and snake clips keep their bodies
    // on the ground themselves.
    bool standsOnFeet = false;
    for (const auto& bone : rigStructure.bones)
        standsOnFeet = standsOnFeet || bone.name.find("Foot") != std::string::npos || bone.name.find("Tibia") != std::string::npos;
    if (standsOnFeet && animationType.find("BipedSwim") != 0) {
        auto boneIdx = animation::buildBoneIndexMap(rigStructure);
        double groundY = animation::restGroundHeight(rigStructure);
        for (auto& frame : animationClip.frames)
            animation::keepTailsAboveGround(rigStructure, boneIdx, inverseBindMatrices, frame, groundY);
    }

    // Determine movement speed and direction based on animation type
    float speedFactor = 0.0f;
    if (animationType.find("Run") != std::string::npos)
        speedFactor = 2.0f;
    else if (animationType.find("Walk") != std::string::npos)
        speedFactor = 1.0f;
    else if (animationType.find("Forward") != std::string::npos
        || animationType.find("Fly") != std::string::npos
        || animationType.find("Swim") != std::string::npos
        || animationType.find("Slither") != std::string::npos)
        speedFactor = 1.0f;
    else if (animationType.find("Glide") != std::string::npos)
        speedFactor = 1.2f;

    if (!definition && speedFactor > 0.0f && animationClip.movementSpeed <= 0.0f) {
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

    if (animationClip.loop && !animationClip.frames.empty()) {
        // glTF derives duration from the last key. Include the wrap interval explicitly,
        // with identical transforms after blink, accessory and ground corrections.
        BoneAnimationFrame seam = animationClip.frames.front();
        seam.time = animationClip.durationSeconds;
        animationClip.frames.push_back(std::move(seam));
    }

    return true;
}

} // namespace dust3d
