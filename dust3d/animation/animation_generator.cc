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
#include <dust3d/animation/animation_catalog.h>
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
#include <iterator>

namespace dust3d {

std::pair<double, int> AnimationGenerator::defaultTiming(const std::string& type)
{
    const auto& catalog = animation::clipCatalog();
    auto found = catalog.find(type);
    return found == catalog.end() ? std::make_pair(3.0, 90)
                                  : std::make_pair(found->second.duration, found->second.samples);
}

AnimationParams AnimationGenerator::defaultParameters(const std::string& type)
{
    AnimationParams result;
    auto found = animation::clipCatalog().find(type);
    if (found == animation::clipCatalog().end())
        return result;
    result.setValue("durationSeconds", found->second.duration);
    result.setValue("frameCount", found->second.samples);
    for (const auto& parameter : found->second.parameters)
        result.setValue(parameter.first, parameter.second);
    return result;
}

bool AnimationGenerator::generate(const RigStructure& rigStructure,
    const std::map<std::string, Matrix4x4>& inverseBindMatrices,
    RigAnimationClip& animationClip,
    const std::string& requestedAnimationType,
    const AnimationParams& inputParameters)
{
    animationClip.frames.clear();
    animationClip.animationType = requestedAnimationType;
    animationClip.movementSpeed = 0.0f;
    animationClip.movementDirectionX = 0.0f;
    animationClip.movementDirectionZ = 0.0f;
    animationClip.loop = false;
    animationClip.entryPose.clear();
    animationClip.exitPose.clear();
    animationClip.rootYawDegrees = 0.0f;
    animationClip.events.clear();
    animationClip.rootMotion = "inPlace";
    const auto preset = animation::clipCatalog().find(requestedAnimationType);
    if (preset == animation::clipCatalog().end() || preset->second.rig != rigStructure.type)
        return false;
    std::map<std::string, std::string> parents;
    for (const auto& bone : rigStructure.bones) {
        if (bone.name.empty() || !parents.emplace(bone.name, bone.parent).second)
            return false;
        const auto inverse = inverseBindMatrices.find(bone.name);
        if (inverse == inverseBindMatrices.end())
            return false;
        for (double value : { static_cast<double>(bone.posX), static_cast<double>(bone.posY), static_cast<double>(bone.posZ),
                 static_cast<double>(bone.endX), static_cast<double>(bone.endY), static_cast<double>(bone.endZ) })
            if (!std::isfinite(value))
                return false;
        for (int i = 0; i < 16; ++i)
            if (!std::isfinite(inverse->second.constData()[i]))
                return false;
    }
    for (const auto& bone : rigStructure.bones) {
        std::set<std::string> visited;
        for (std::string name = bone.name; !name.empty(); name = parents.at(name)) {
            if (!parents.count(name) || !visited.insert(name).second)
                return false;
        }
    }
    const std::string& animationType = preset->second.generator;
    AnimationParams parameters = defaultParameters(requestedAnimationType);
    for (const auto& parameter : inputParameters.values)
        parameters.values[parameter.first] = parameter.second;
    const bool isBiped = animationType.compare(0, 5, "Biped") == 0;
    {
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
    double duration = parameters.getValue("durationSeconds", 1.0);
    if (duration < 0.001 || duration > 600.0)
        return false;
    animationClip.loop = preset->second.loop;
    animationClip.entryPose = animationClip.exitPose = animationClip.loop ? "cycle" : "relaxed";
    if (!animationClip.loop && animationType.find("Die") != std::string::npos)
        animationClip.exitPose = "dead";
    if (animationType == "BirdAttack" || animationType == "InsectAttack")
        animationClip.entryPose = animationClip.exitPose = "flying";
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
    if (animationClip.frames.size() < 2 || !std::isfinite(animationClip.durationSeconds)
        || animationClip.durationSeconds <= 0.0f)
        return false;
    for (size_t i = 0; i < animationClip.frames.size(); ++i)
        animationClip.frames[i].time = animationClip.durationSeconds * static_cast<double>(i)
            / (animationClip.loop ? animationClip.frames.size() : animationClip.frames.size() - 1);

    if (isBiped) {
        animation::referenceRollsToRest(rigStructure, inverseBindMatrices, animationClip);
        const auto rest = animation::restBoneWorldTransforms(rigStructure);
        std::map<std::string, Quaternion> secondaryBlendHistory;
        const auto drape = biped::neutralSecondary(rigStructure, animation::buildBoneIndexMap(rigStructure), rest);
        for (auto& frame : animationClip.frames) {
            if (!definition && animationType != "BipedIdle" && animationType != "BipedWalk" && animationType != "BipedRun"
                && animationType != "BipedJump" && animationType != "BipedSlam" && animationType != "BipedRoar" && animationType != "BipedStab"
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
            if (!animationClip.loop) {
                double relaxedBoundary = 0.0;
                if (animationClip.entryPose == "relaxed")
                    relaxedBoundary = 1.0 - biped::easePose(t / 0.12);
                if (animationClip.exitPose == "relaxed")
                    relaxedBoundary = std::max(relaxedBoundary, biped::easePose((t - 0.84) / 0.16));
                biped::settleRelaxedArms(rigStructure, parameters, frame, relaxedBoundary);
            }
            biped::applySecondaryNeutral(rigStructure, rest, drape, frame, boundary, &secondaryBlendHistory);
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
        std::map<std::string, Vector3> groundDirections;
        for (auto& frame : animationClip.frames)
            animation::keepTailsAboveGround(rigStructure, boneIdx, inverseBindMatrices, frame, groundY, &groundDirections);
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

    // Complete optional accessory chains for every rig family before export.
    for (auto& frame : animationClip.frames) {
        const auto& standard = animation::standardBones(rigStructure.type);
        for (const auto& bone : rigStructure.bones) {
            // Procedural templates animate their known anatomy. Additional attachments
            // retain their bind offset from the animated parent, including generators
            // which initialized an entire map with the rest pose.
            bool emptyChain = (bone.name.find("Tail") == 0 || bone.name.find("Hair") == 0 || bone.name.find("Cape") != std::string::npos)
                && (Vector3(bone.endX, bone.endY, bone.endZ) - Vector3(bone.posX, bone.posY, bone.posZ)).lengthSquared() < 1e-18;
            if (emptyChain || (!standard.count(bone.name) && bone.name.find("Tail") != 0 && bone.name.find("Hair") != 0 && bone.name.find("Cape") == std::string::npos && bone.name.find("Eyelid") == std::string::npos))
                frame.boneWorldTransforms.erase(bone.name);
        }
        animation::inheritUndrivenBones(rigStructure, inverseBindMatrices, frame);
    }

    std::sort(animationClip.events.begin(), animationClip.events.end(), [](const RigAnimationEvent& a, const RigAnimationEvent& b) {
        if (a.time != b.time)
            return a.time < b.time;
        if (a.name != b.name)
            return a.name < b.name;
        return a.bone < b.bone;
    });
    if (animationClip.loop && !animationClip.frames.empty()) {
        // glTF derives duration from the last key. Include the wrap interval explicitly,
        // with identical transforms after blink, accessory and ground corrections.
        BoneAnimationFrame seam = animationClip.frames.front();
        seam.time = animationClip.durationSeconds;
        animationClip.frames.push_back(std::move(seam));
    }

    // A generator may pose a template bone this rig was built without.
    for (auto& frame : animationClip.frames) {
        for (auto it = frame.boneWorldTransforms.begin(); it != frame.boneWorldTransforms.end();)
            it = parents.count(it->first) ? std::next(it) : frame.boneWorldTransforms.erase(it);
        for (auto it = frame.boneSkinMatrices.begin(); it != frame.boneSkinMatrices.end();)
            it = parents.count(it->first) ? std::next(it) : frame.boneSkinMatrices.erase(it);
    }

    // Never publish a partial or nonfinite animation, including invalid secondary
    // simulations or malformed bind data supplied by a headless caller.
    for (const auto& frame : animationClip.frames) {
        if (!std::isfinite(frame.time) || frame.boneWorldTransforms.size() != rigStructure.bones.size()
            || frame.boneSkinMatrices.size() != rigStructure.bones.size())
            return false;
        for (const auto& transform : frame.boneSkinMatrices)
            for (int i = 0; i < 16; ++i)
                if (!std::isfinite(transform.second.constData()[i]))
                    return false;
    }

    return true;
}

} // namespace dust3d
