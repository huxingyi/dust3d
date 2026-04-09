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
#include <dust3d/animation/biped/run.h>
#include <dust3d/animation/biped/walk.h>
#include <dust3d/animation/bird/die.h>
#include <dust3d/animation/bird/forward.h>
#include <dust3d/animation/bird/idle.h>
#include <dust3d/animation/fish/die.h>
#include <dust3d/animation/fish/forward.h>
#include <dust3d/animation/fish/idle.h>
#include <dust3d/animation/insect/attack.h>
#include <dust3d/animation/insect/die.h>
#include <dust3d/animation/insect/forward.h>
#include <dust3d/animation/insect/idle.h>
#include <dust3d/animation/insect/rub_hands.h>
#include <dust3d/animation/insect/walk.h>
#include <dust3d/animation/quadruped/die.h>
#include <dust3d/animation/quadruped/idle.h>
#include <dust3d/animation/quadruped/run.h>
#include <dust3d/animation/quadruped/walk.h>
#include <dust3d/animation/snake/die.h>
#include <dust3d/animation/snake/forward.h>
#include <dust3d/animation/snake/idle.h>
#include <dust3d/animation/spider/die.h>
#include <dust3d/animation/spider/idle.h>
#include <dust3d/animation/spider/walk.h>

namespace dust3d {

bool AnimationGenerator::generate(const RigStructure& rigStructure,
    const std::map<std::string, Matrix4x4>& inverseBindMatrices,
    RigAnimationClip& animationClip,
    const std::string& animationType,
    const AnimationParams& parameters)
{
    if (animationType == "InsectWalk")
        return insect::walk(rigStructure, inverseBindMatrices, animationClip, parameters);
    if (animationType == "InsectIdle")
        return insect::idle(rigStructure, inverseBindMatrices, animationClip, parameters);
    if (animationType == "InsectRubHands")
        return insect::rubHands(rigStructure, inverseBindMatrices, animationClip, parameters);
    if (animationType == "InsectForward")
        return insect::forward(rigStructure, inverseBindMatrices, animationClip, parameters);
    if (animationType == "InsectAttack")
        return insect::attack(rigStructure, inverseBindMatrices, animationClip, parameters);
    if (animationType == "InsectDie")
        return insect::die(rigStructure, inverseBindMatrices, animationClip, parameters);
    if (animationType == "BirdForward")
        return bird::forward(rigStructure, inverseBindMatrices, animationClip, parameters);
    if (animationType == "BirdIdle")
        return bird::idle(rigStructure, inverseBindMatrices, animationClip, parameters);
    if (animationType == "FishForward")
        return fish::forward(rigStructure, inverseBindMatrices, animationClip, parameters);
    if (animationType == "FishIdle")
        return fish::idle(rigStructure, inverseBindMatrices, animationClip, parameters);
    if (animationType == "FishDie")
        return fish::die(rigStructure, inverseBindMatrices, animationClip, parameters);
    if (animationType == "SnakeForward")
        return snake::forward(rigStructure, inverseBindMatrices, animationClip, parameters);
    if (animationType == "SnakeIdle")
        return snake::idle(rigStructure, inverseBindMatrices, animationClip, parameters);
    if (animationType == "SnakeDie")
        return snake::die(rigStructure, inverseBindMatrices, animationClip, parameters);
    if (animationType == "BipedWalk")
        return biped::walk(rigStructure, inverseBindMatrices, animationClip, parameters);
    if (animationType == "BipedIdle")
        return biped::idle(rigStructure, inverseBindMatrices, animationClip, parameters);
    if (animationType == "BipedRun")
        return biped::run(rigStructure, inverseBindMatrices, animationClip, parameters);
    if (animationType == "BipedJump")
        return biped::jump(rigStructure, inverseBindMatrices, animationClip, parameters);
    if (animationType == "BipedDie")
        return biped::die(rigStructure, inverseBindMatrices, animationClip, parameters);
    if (animationType == "QuadrupedWalk")
        return quadruped::walk(rigStructure, inverseBindMatrices, animationClip, parameters);
    if (animationType == "QuadrupedIdle")
        return quadruped::idle(rigStructure, inverseBindMatrices, animationClip, parameters);
    if (animationType == "QuadrupedRun")
        return quadruped::run(rigStructure, inverseBindMatrices, animationClip, parameters);
    if (animationType == "QuadrupedDie")
        return quadruped::die(rigStructure, inverseBindMatrices, animationClip, parameters);
    if (animationType == "BirdDie")
        return bird::die(rigStructure, inverseBindMatrices, animationClip, parameters);
    if (animationType == "SpiderDie")
        return spider::die(rigStructure, inverseBindMatrices, animationClip, parameters);
    if (animationType == "SpiderIdle")
        return spider::idle(rigStructure, inverseBindMatrices, animationClip, parameters);
    if (animationType == "SpiderWalk")
        return spider::walk(rigStructure, inverseBindMatrices, animationClip, parameters);

    // Add future rig type + animationName mappings here.
    return false;
}

} // namespace dust3d
