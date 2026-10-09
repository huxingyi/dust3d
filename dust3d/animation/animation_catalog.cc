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

#include <dust3d/animation/animation_catalog.h>
#include <dust3d/animation/biped/clip_catalog.h>
#include <utility>
#include <vector>

namespace dust3d {
namespace animation {

    namespace {

        struct Clip {
            const char* name;
            const char* rig;
            double duration;
            int samples;
            bool loop;
            std::map<std::string, double> parameters;
        };

        // The biped clips in biped/clip_catalog.cc are added to these.
        const std::vector<Clip>& clips()
        {
            static const std::vector<Clip> list = {
                { "BipedBlock", "Biped", 1.5, 45, true },
                { "BipedCast", "Biped", 1.0, 48, false },
                { "BipedChannel", "Biped", 2.0, 64, true },
                { "BipedCombatIdle", "Biped", 2.0, 60, true },
                { "BipedDie", "Biped", 1.3, 40, false },
                { "BipedDodge", "Biped", 0.65, 30, false },
                { "BipedFall", "Biped", 1.0, 30, true },
                { "BipedHop", "Biped", 0.6, 20, true },
                { "BipedHurt", "Biped", 1.0, 36, false },
                { "BipedIdle", "Biped", 4.0, 90, true, { { "breathingAmplitudeFactor", 0.65 }, { "headLookFactor", 0.65 }, { "weightShiftFactor", 0.65 } } },
                { "BipedJump", "Biped", 1.2, 40, false },
                { "BipedJumpStart", "Biped", 0.3, 12, false },
                { "BipedKick", "Biped", 0.8, 24, false },
                { "BipedLand", "Biped", 0.4, 16, false },
                { "BipedRoar", "Biped", 3.0, 120, false },
                { "BipedRun", "Biped", 1.0, 30, true },
                { "BipedSlam", "Biped", 0.9, 48, false },
                { "BipedSlash", "Biped", 0.75, 30, false },
                { "BipedStab", "Biped", 0.7, 48, false },
                { "BipedStrafeLeft", "Biped", 1.0, 30, true },
                { "BipedStrafeRight", "Biped", 1.0, 30, true },
                { "BipedThrow", "Biped", 0.9, 36, false },
                { "BipedTurnLeft", "Biped", 0.8, 32, false },
                { "BipedTurnRight", "Biped", 0.8, 32, false },
                { "BipedWalk", "Biped", 1.0, 30, true },
                { "BipedWalkBackward", "Biped", 1.0, 30, true },
                { "QuadrupedAttack", "Quadruped", 1.2, 40, false },
                { "QuadrupedDie", "Quadruped", 1.4, 42, false },
                { "QuadrupedEat", "Quadruped", 2.0, 40, true },
                { "QuadrupedHurt", "Quadruped", 1.0, 36, false },
                { "QuadrupedIdle", "Quadruped", 4.0, 90, true, { { "breathingAmplitudeFactor", 0.65 }, { "headLookFactor", 0.65 }, { "weightShiftFactor", 0.65 } } },
                { "QuadrupedRoar", "Quadruped", 3.0, 120, false },
                { "QuadrupedRun", "Quadruped", 1.0, 30, true },
                { "QuadrupedWalk", "Quadruped", 1.0, 30, true },
                { "BirdAttack", "Bird", 2.5, 60, false },
                { "BirdDie", "Bird", 1.4, 42, false },
                { "BirdEat", "Bird", 3.0, 60, true },
                { "BirdFly", "Bird", 1.0, 30, true },
                { "BirdGlide", "Bird", 3.0, 60, true },
                { "BirdHurt", "Bird", 0.8, 24, false },
                { "BirdIdle", "Bird", 4.0, 90, true, { { "breathingAmplitudeFactor", 0.65 }, { "headLookFactor", 0.65 }, { "headPeckFactor", 0.35 }, { "weightShiftFactor", 0.65 } } },
                { "BirdRun", "Bird", 1.0, 30, true },
                { "BirdStrike", "Bird", 0.9, 27, false },
                { "BirdWalk", "Bird", 1.0, 30, true },
                { "FishAttack", "Fish", 0.9, 27, false },
                { "FishDie", "Fish", 1.8, 54, false },
                { "FishHurt", "Fish", 0.8, 24, false },
                { "FishIdle", "Fish", 4.0, 90, true, { { "breathingAmplitudeFactor", 0.65 } } },
                { "FishSwim", "Fish", 1.0, 30, true },
                { "InsectAttack", "Insect", 1.0, 30, false },
                { "InsectBite", "Insect", 0.9, 27, false },
                { "InsectDie", "Insect", 1.2, 36, false },
                { "InsectFly", "Insect", 1.0, 30, true },
                { "InsectHurt", "Insect", 0.7, 21, false },
                { "InsectIdle", "Insect", 4.0, 90, true, { { "breathingAmplitudeFactor", 0.65 }, { "legTwitchFactor", 0.35 } } },
                { "InsectRubHands", "Insect", 1.0, 30, true },
                { "InsectWalk", "Insect", 1.0, 30, true },
                { "SnakeDie", "Snake", 1.4, 42, false },
                { "SnakeHurt", "Snake", 0.8, 24, false },
                { "SnakeIdle", "Snake", 4.0, 90, true, { { "breathingAmplitudeFactor", 0.65 }, { "tongueFlickFactor", 0.35 } } },
                { "SnakeSlither", "Snake", 1.0, 30, true },
                { "SnakeStrike", "Snake", 0.9, 30, false },
                { "SpiderAttack", "Spider", 1.0, 36, false },
                { "SpiderDie", "Spider", 1.2, 36, false },
                { "SpiderHurt", "Spider", 0.8, 24, false },
                { "SpiderIdle", "Spider", 4.0, 90, true, { { "abdomenPulseFactor", 1.25 }, { "bodySwayFactor", 1.25 }, { "breathingAmplitudeFactor", 1.25 } } },
                { "SpiderRun", "Spider", 1.0, 30, true },
                { "SpiderWalk", "Spider", 1.0, 30, true },
            };
            return list;
        }

        // Names from earlier versions.
        const std::pair<const char*, const char*> g_aliases[] = {
            { "BirdForward", "BirdFly" },
            { "FishForward", "FishSwim" },
            { "InsectForward", "InsectFly" },
            { "SnakeForward", "SnakeSlither" },
        };

        // A gait travelling at an angle to the facing direction (degrees, left
        // positive) takes shorter steps.
        struct Direction {
            const char* suffix;
            double angle;
            double stepLength;
            bool diagonal;
        };
        const Direction g_directions[] = {
            { "ForwardLeft", 45, 0.7, true },
            { "ForwardRight", -45, 0.7, true },
            { "Left", 90, 0.32, false },
            { "Right", -90, 0.32, false },
            { "BackwardLeft", 135, 0.55, true },
            { "BackwardRight", -135, 0.55, true },
            { "Backward", 180, 0.55, false },
        };

        struct DirectionalGait {
            const char* generator;
            // BipedWalk has its own strafe and backward clips.
            bool diagonalsOnly;
        };
        const DirectionalGait g_directionalGaits[] = {
            { "BipedWalk", true },
            { "BipedRun", false },
            { "QuadrupedWalk", false },
            { "QuadrupedRun", false },
            { "BirdWalk", false },
            { "BirdRun", false },
            { "InsectWalk", false },
            { "SpiderWalk", false },
            { "SpiderRun", false },
        };

    }

    const std::map<std::string, ClipInfo>& clipCatalog()
    {
        static const std::map<std::string, ClipInfo> catalog = [] {
            std::map<std::string, ClipInfo> result;
            for (const auto& clip : clips())
                result[clip.name] = { clip.rig, clip.name, false, clip.duration, clip.samples, clip.loop, clip.parameters };
            for (const auto& clip : biped::additionalClips())
                result[clip.type] = { "Biped", clip.type, false, clip.duration, clip.samples, clip.loop, {} };
            for (const auto& alias : g_aliases) {
                ClipInfo info = result.at(alias.second);
                info.alias = true;
                result[alias.first] = info;
            }
            for (const auto& gait : g_directionalGaits) {
                for (const auto& direction : g_directions) {
                    if (gait.diagonalsOnly && !direction.diagonal)
                        continue;
                    ClipInfo info = result.at(gait.generator);
                    info.parameters["travelAngleDegrees"] = direction.angle;
                    info.parameters["stepLengthFactor"] = direction.stepLength;
                    if (info.rig == "Biped") {
                        // No heel-to-toe roll or forward lean when not heading forward.
                        info.parameters["footRollFactor"] = 0.0;
                        info.parameters["leanForwardFactor"] = 0.0;
                    }
                    result[std::string(gait.generator) + direction.suffix] = info;
                }
            }
            return result;
        }();
        return catalog;
    }

    const std::set<std::string>& standardBones(const std::string& rig)
    {
        // The bones of application/resources/rig_*.xml, and the fish's optional Jaw.
        static const std::map<std::string, std::set<std::string>> bones = {
            { "Biped", { "Root", "Hips", "Spine", "Chest", "Neck", "Head", "HairBack1", "HairBack2", "HairBack3", "LeftCape1", "LeftCape2", "LeftCape3", "CenterCape1", "CenterCape2", "CenterCape3", "RightCape1", "RightCape2", "RightCape3", "LeftShoulder", "LeftUpperArm", "LeftLowerArm", "LeftHand", "RightShoulder", "RightUpperArm", "RightLowerArm", "RightHand", "LeftUpperLeg", "LeftLowerLeg", "LeftFoot", "RightUpperLeg", "RightLowerLeg", "RightFoot", "TailBase", "TailMid", "TailTip" } },
            { "Bird", { "Root", "Pelvis", "Spine", "Chest", "Neck", "Head", "Beak", "TailBase", "TailFeathers", "LeftCape1", "LeftCape2", "LeftCape3", "CenterCape1", "CenterCape2", "CenterCape3", "RightCape1", "RightCape2", "RightCape3", "LeftWingShoulder", "LeftWingElbow", "LeftWingHand", "RightWingShoulder", "RightWingElbow", "RightWingHand", "LeftUpperLeg", "LeftLowerLeg", "LeftFoot", "RightUpperLeg", "RightLowerLeg", "RightFoot" } },
            { "Fish", { "Root", "Head", "Jaw", "BodyFront", "BodyMid", "BodyRear", "TailStart", "TailEnd", "DorsalFinFront", "DorsalFinMid", "DorsalFinRear", "VentralFinFront", "VentralFinMid", "VentralFinRear", "LeftPectoralFin", "RightPectoralFin", "LeftPelvicFin", "RightPelvicFin" } },
            { "Insect", { "Root", "Head", "Thorax", "Abdomen", "FrontLeftCoxa", "FrontLeftFemur", "FrontLeftTibia", "FrontRightCoxa", "FrontRightFemur", "FrontRightTibia", "MiddleLeftCoxa", "MiddleLeftFemur", "MiddleLeftTibia", "MiddleRightCoxa", "MiddleRightFemur", "MiddleRightTibia", "BackLeftCoxa", "BackLeftFemur", "BackLeftTibia", "BackRightCoxa", "BackRightFemur", "BackRightTibia", "LeftWing", "RightWing" } },
            { "Quadruped", { "Root", "Pelvis", "Spine", "Chest", "Neck", "Head", "Jaw", "TailBase", "TailMid", "TailTip", "FrontLeftUpperLeg", "FrontLeftLowerLeg", "FrontLeftFoot", "FrontRightUpperLeg", "FrontRightLowerLeg", "FrontRightFoot", "BackLeftUpperLeg", "BackLeftLowerLeg", "BackLeftFoot", "BackRightUpperLeg", "BackRightLowerLeg", "BackRightFoot" } },
            { "Snake", { "Root", "Head", "Jaw", "Spine1", "Spine2", "Spine3", "Spine4", "Spine5", "Spine6", "Tail1", "Tail2", "Tail3", "Tail4", "TailTip" } },
            { "Spider", { "Root", "Cephalothorax", "Head", "Abdomen", "LeftPedipalp", "RightPedipalp", "FrontLeftCoxa", "FrontLeftFemur", "FrontLeftTibia", "FrontRightCoxa", "FrontRightFemur", "FrontRightTibia", "MidFrontLeftCoxa", "MidFrontLeftFemur", "MidFrontLeftTibia", "MidFrontRightCoxa", "MidFrontRightFemur", "MidFrontRightTibia", "MidBackLeftCoxa", "MidBackLeftFemur", "MidBackLeftTibia", "MidBackRightCoxa", "MidBackRightFemur", "MidBackRightTibia", "BackLeftCoxa", "BackLeftFemur", "BackLeftTibia", "BackRightCoxa", "BackRightFemur", "BackRightTibia" } },
        };
        static const std::set<std::string> empty;
        auto found = bones.find(rig);
        return found == bones.end() ? empty : found->second;
    }

} // namespace animation
} // namespace dust3d
