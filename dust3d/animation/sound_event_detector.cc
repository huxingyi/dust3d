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
#include <dust3d/animation/sound_event_detector.h>
#include <dust3d/base/matrix4x4.h>

namespace dust3d {

float SoundEventDetector::getBoneY(const BoneAnimationFrame& frame, const std::string& boneName)
{
    auto it = frame.boneWorldTransforms.find(boneName);
    if (it == frame.boneWorldTransforms.end())
        return 0.0f;
    // Matrix4x4 is a flat double[16] array, column-major
    // Translation Y is at index M31 = 13
    return static_cast<float>(it->second.constData()[Matrix4x4::M31]);
}

std::vector<SoundEvent> SoundEventDetector::detectFootContacts(
    const RigAnimationClip& clip,
    const std::vector<std::string>& footBones)
{
    std::vector<SoundEvent> events;

    if (clip.frames.size() < 3)
        return events;

    for (const auto& boneName : footBones) {
        // Find the minimum Y across all frames to establish ground level
        float minY = 1e10f;
        float maxY = -1e10f;
        for (const auto& frame : clip.frames) {
            float y = getBoneY(frame, boneName);
            if (y < minY)
                minY = y;
            if (y > maxY)
                maxY = y;
        }

        float range = maxY - minY;
        if (range < 1e-6f)
            continue;

        // Ground contact threshold: within 15% of minimum
        float threshold = minY + range * 0.15f;

        // Detect downward zero-crossings of (y - threshold)
        // i.e., frames where foot goes from above threshold to at/below
        bool wasAbove = getBoneY(clip.frames[0], boneName) > threshold;

        for (size_t i = 1; i < clip.frames.size(); ++i) {
            float y = getBoneY(clip.frames[i], boneName);
            bool isAbove = y > threshold;

            if (wasAbove && !isAbove) {
                // Contact event detected
                SoundEvent event;
                event.timeSeconds = clip.frames[i].time;
                event.boneName = boneName;

                // Intensity based on downward velocity
                float prevY = getBoneY(clip.frames[i - 1], boneName);
                float dt = clip.frames[i].time - clip.frames[i - 1].time;
                if (dt > 0.0f) {
                    float velocity = (prevY - y) / dt;
                    // Normalize velocity to 0-1 range, higher velocity = louder
                    event.intensity = std::min(1.0f, std::max(0.2f, velocity / (range * 10.0f)));
                } else {
                    event.intensity = 0.5f;
                }

                events.push_back(event);
            }
            wasAbove = isAbove;
        }
    }

    // Sort by time
    std::sort(events.begin(), events.end(), [](const SoundEvent& a, const SoundEvent& b) {
        return a.timeSeconds < b.timeSeconds;
    });

    return events;
}

std::vector<SoundEvent> SoundEventDetector::detectBodyImpact(
    const RigAnimationClip& clip,
    const std::string& rootBone)
{
    std::vector<SoundEvent> events;

    if (clip.frames.size() < 3)
        return events;

    // Detect sudden downward acceleration of the root bone (body collapse/impact)
    std::vector<float> yPositions;
    yPositions.reserve(clip.frames.size());
    for (const auto& frame : clip.frames) {
        yPositions.push_back(getBoneY(frame, rootBone));
    }

    float maxY = *std::max_element(yPositions.begin(), yPositions.end());
    float minY = *std::min_element(yPositions.begin(), yPositions.end());
    float range = maxY - minY;
    if (range < 1e-6f)
        return events;

    // Look for frames where second derivative is large (sudden deceleration = impact)
    for (size_t i = 2; i < clip.frames.size(); ++i) {
        float dt1 = clip.frames[i].time - clip.frames[i - 1].time;
        float dt0 = clip.frames[i - 1].time - clip.frames[i - 2].time;
        if (dt1 < 1e-6f || dt0 < 1e-6f)
            continue;

        float v1 = (yPositions[i] - yPositions[i - 1]) / dt1;
        float v0 = (yPositions[i - 1] - yPositions[i - 2]) / dt0;
        float accel = (v1 - v0) / ((dt0 + dt1) * 0.5f);

        // Large upward acceleration after downward motion = ground impact
        if (accel > range * 5.0f && v0 < 0.0f) {
            SoundEvent event;
            event.timeSeconds = clip.frames[i].time;
            event.boneName = rootBone;
            event.intensity = std::min(1.0f, std::max(0.3f, std::abs(v0) / (range * 5.0f)));
            events.push_back(event);
        }
    }

    return events;
}

std::vector<SoundEvent> SoundEventDetector::detect(
    const RigAnimationClip& clip,
    const std::string& animationType)
{
    if (clip.frames.empty())
        return {};

    // Biped locomotion: detect foot contacts
    if (animationType == "BipedWalk" || animationType == "BipedRun") {
        return detectFootContacts(clip, { "leftFoot", "rightFoot", "LeftFoot", "RightFoot", "leftToe", "rightToe", "LeftToe", "RightToe" });
    }

    // Biped jump: detect landing impact
    if (animationType == "BipedJump") {
        auto footEvents = detectFootContacts(clip, { "leftFoot", "rightFoot", "LeftFoot", "RightFoot" });
        auto bodyEvents = detectBodyImpact(clip, "hip");
        footEvents.insert(footEvents.end(), bodyEvents.begin(), bodyEvents.end());
        std::sort(footEvents.begin(), footEvents.end(), [](const SoundEvent& a, const SoundEvent& b) {
            return a.timeSeconds < b.timeSeconds;
        });
        return footEvents;
    }

    // Biped/Quadruped die: detect body impact
    if (animationType == "BipedDie" || animationType == "QuadrupedDie" || animationType == "BirdDie" || animationType == "InsectDie" || animationType == "SpiderDie") {
        auto events = detectBodyImpact(clip, "hip");
        if (events.empty()) {
            // Try root bone alternatives
            for (const auto& name : { "Hip", "root", "Root", "pelvis", "Pelvis", "body", "Body" }) {
                events = detectBodyImpact(clip, name);
                if (!events.empty())
                    break;
            }
        }
        return events;
    }

    // Quadruped locomotion
    if (animationType == "QuadrupedWalk" || animationType == "QuadrupedRun") {
        return detectFootContacts(clip, { "FrontLeftFoot", "FrontRightFoot", "BackLeftFoot", "BackRightFoot", "leftFrontFoot", "rightFrontFoot", "leftBackFoot", "rightBackFoot", "LeftFrontFoot", "RightFrontFoot", "LeftBackFoot", "RightBackFoot", "leftFrontToe", "rightFrontToe", "leftBackToe", "rightBackToe" });
    }

    // Insect/Spider walk: detect leg contacts
    if (animationType == "InsectWalk" || animationType == "InsectForward" || animationType == "SpiderWalk") {
        // Collect all bones that might be leg tips
        std::vector<std::string> legBones;
        if (!clip.frames.empty()) {
            for (const auto& pair : clip.frames[0].boneWorldTransforms) {
                const auto& name = pair.first;
                // Match leg tip bones by common naming patterns
                if (name.find("Foot") != std::string::npos || name.find("foot") != std::string::npos || name.find("Tip") != std::string::npos || name.find("tip") != std::string::npos || name.find("Tarsus") != std::string::npos || name.find("tarsus") != std::string::npos || name.find("Tibia") != std::string::npos || name.find("tibia") != std::string::npos) {
                    legBones.push_back(name);
                }
            }
        }
        if (!legBones.empty()) {
            return detectFootContacts(clip, legBones);
        }
    }

    // Bird forward: detect foot contacts
    if (animationType == "BirdForward") {
        return detectFootContacts(clip, { "leftFoot", "rightFoot", "LeftFoot", "RightFoot" });
    }

    // Fish: underwater body/tail movement — track lateral tail undulation
    if (animationType == "FishForward" || animationType == "FishIdle") {
        std::vector<SoundEvent> events;
        std::vector<std::string> bodyBones = {
            "BodyFront", "BodyMid", "BodyRear", "TailStart", "TailEnd"
        };

        // Find which bones exist
        std::vector<std::string> foundBones;
        if (!clip.frames.empty()) {
            for (const auto& name : bodyBones) {
                if (clip.frames[0].boneWorldTransforms.find(name) != clip.frames[0].boneWorldTransforms.end()) {
                    foundBones.push_back(name);
                }
            }
        }

        if (foundBones.empty() || clip.frames.size() < 2) {
            // Fallback: periodic underwater whooshes
            float interval = clip.durationSeconds / std::max(3.0f, clip.durationSeconds * 4.0f);
            for (float t = 0.0f; t < clip.durationSeconds; t += interval) {
                SoundEvent event;
                event.timeSeconds = t;
                event.intensity = 0.2f + 0.15f * sinf(t * 12.56f / clip.durationSeconds);
                event.boneName = "body";
                event.isUnderwater = true;
                events.push_back(event);
            }
            return events;
        }

        // Track lateral velocity of body/tail bones
        float timePerFrame = clip.durationSeconds / std::max(1, static_cast<int>(clip.frames.size()) - 1);
        for (size_t fi = 1; fi < clip.frames.size(); ++fi) {
            float frameTime = fi * timePerFrame;
            float totalSpeed = 0.0f;
            std::string peakBone = "TailEnd";
            float peakSpeed = 0.0f;

            for (const auto& boneName : foundBones) {
                auto currIt = clip.frames[fi].boneWorldTransforms.find(boneName);
                auto prevIt = clip.frames[fi - 1].boneWorldTransforms.find(boneName);
                if (currIt == clip.frames[fi].boneWorldTransforms.end() || prevIt == clip.frames[fi - 1].boneWorldTransforms.end())
                    continue;
                float dx = static_cast<float>(currIt->second.constData()[Matrix4x4::M30] - prevIt->second.constData()[Matrix4x4::M30]);
                float dy = static_cast<float>(currIt->second.constData()[Matrix4x4::M31] - prevIt->second.constData()[Matrix4x4::M31]);
                float dz = static_cast<float>(currIt->second.constData()[Matrix4x4::M32] - prevIt->second.constData()[Matrix4x4::M32]);
                float speed = sqrtf(dx * dx + dy * dy + dz * dz) / std::max(timePerFrame, 0.001f);
                totalSpeed += speed;
                if (speed > peakSpeed) {
                    peakSpeed = speed;
                    peakBone = boneName;
                }
            }

            float avgSpeed = totalSpeed / foundBones.size();
            // Lower intensity range — underwater sounds are softer
            float intensity = std::min(0.45f, avgSpeed * 0.6f) + 0.05f;
            if (intensity > 0.08f) {
                SoundEvent event;
                event.timeSeconds = frameTime;
                event.intensity = intensity;
                event.boneName = peakBone;
                event.isUnderwater = true;
                events.push_back(event);
            }
        }
        return events;
    }

    // FishDie: thrashing in water
    if (animationType == "FishDie") {
        auto events = detectBodyImpact(clip, "BodyMid");
        for (auto& e : events) {
            e.isUnderwater = true;
        }
        return events;
    }

    // Snake: continuous ground friction from lateral undulation
    // Track lateral (X) displacement of spine segments to detect slither pulses
    if (animationType == "SnakeForward" || animationType == "SnakeIdle") {
        std::vector<SoundEvent> events;
        std::vector<std::string> spineBones = {
            "Spine1", "Spine2", "Spine3", "Spine4", "Spine5", "Spine6",
            "Tail1", "Tail2", "Tail3", "Tail4"
        };

        // Find which spine bones exist in the clip
        std::vector<std::string> foundBones;
        if (!clip.frames.empty()) {
            for (const auto& name : spineBones) {
                if (clip.frames[0].boneWorldTransforms.find(name) != clip.frames[0].boneWorldTransforms.end()) {
                    foundBones.push_back(name);
                }
            }
        }

        if (foundBones.empty() || clip.frames.empty()) {
            // Fallback: generate periodic friction events
            float interval = clip.durationSeconds / std::max(4.0f, clip.durationSeconds * 5.0f);
            for (float t = 0.0f; t < clip.durationSeconds; t += interval) {
                SoundEvent event;
                event.timeSeconds = t;
                event.intensity = 0.15f + 0.1f * sinf(t * 12.56f / clip.durationSeconds);
                event.boneName = "Spine3";
                events.push_back(event);
            }
            return events;
        }

        // Measure lateral velocity of each spine bone across frames to detect slither peaks
        float timePerFrame = clip.durationSeconds / std::max(1, static_cast<int>(clip.frames.size()) - 1);
        for (size_t fi = 1; fi < clip.frames.size(); ++fi) {
            float frameTime = fi * timePerFrame;
            float totalLateralSpeed = 0.0f;
            std::string peakBone = "Spine3";
            float peakSpeed = 0.0f;

            for (const auto& boneName : foundBones) {
                auto currIt = clip.frames[fi].boneWorldTransforms.find(boneName);
                auto prevIt = clip.frames[fi - 1].boneWorldTransforms.find(boneName);
                if (currIt == clip.frames[fi].boneWorldTransforms.end() || prevIt == clip.frames[fi - 1].boneWorldTransforms.end())
                    continue;
                // X position is at index M30 = 12, Z at index M32 = 14 in column-major 4x4
                float dx = static_cast<float>(currIt->second.constData()[Matrix4x4::M30] - prevIt->second.constData()[Matrix4x4::M30]);
                float dz = static_cast<float>(currIt->second.constData()[Matrix4x4::M32] - prevIt->second.constData()[Matrix4x4::M32]);
                float lateralSpeed = sqrtf(dx * dx + dz * dz) / std::max(timePerFrame, 0.001f);
                totalLateralSpeed += lateralSpeed;
                if (lateralSpeed > peakSpeed) {
                    peakSpeed = lateralSpeed;
                    peakBone = boneName;
                }
            }

            // Normalize to a reasonable intensity range
            float avgSpeed = totalLateralSpeed / foundBones.size();
            // Continuous friction: always some sound when moving, peaks at fast lateral motion
            float intensity = std::min(0.5f, avgSpeed * 0.8f) + 0.08f;
            if (intensity > 0.1f) {
                SoundEvent event;
                event.timeSeconds = frameTime;
                event.intensity = intensity;
                event.boneName = peakBone;
                events.push_back(event);
            }
        }
        return events;
    }

    // SnakeDie: body impact
    if (animationType == "SnakeDie") {
        return detectBodyImpact(clip, "Spine3");
    }

    // InsectAttack: head strike
    if (animationType == "InsectAttack") {
        return detectBodyImpact(clip, "Head");
    }

    // InsectRubHands: detect front leg motion
    if (animationType == "InsectRubHands") {
        return detectFootContacts(clip, { "FrontLeftTibia", "FrontRightTibia", "FrontLeftFemur", "FrontRightFemur" });
    }

    return {};
}

} // namespace dust3d
