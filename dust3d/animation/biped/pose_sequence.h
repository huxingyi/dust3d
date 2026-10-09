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

#ifndef DUST3D_ANIMATION_BIPED_POSE_SEQUENCE_H_
#define DUST3D_ANIMATION_BIPED_POSE_SEQUENCE_H_
#include <dust3d/animation/biped/pose.h>
#include <functional>
namespace dust3d {
namespace biped {
    // Coordinates are (left, up, forward), in leg lengths. Angles are radians.
    // Arm targets are offsets from the chest, or the head for consumption contact.
    // Knee/ankle offsets are from rest.
    struct ArmPose {
        Vector3 upper = Vector3(0, -1, 0);
        Vector3 lower = Vector3(0, -1, 0);
        double weight = 0.0;
        double targetWeight = 1.0;
        bool targetHand = false;
        bool targetGround = false; // Target is relative to rest hips X/Z and the ground plane.
        bool targetHead = false; // Consume-item contact follows head motion.
        Vector3 handTarget;
        Vector3 handDirection = Vector3(0, 0, 1);
    };
    struct PoseSample {
        Vector3 hips;
        double pitch = 0, yaw = 0, roll = 0;
        double chestPitch = 0, chestYaw = 0, headPitch = 0, headYaw = 0;
        Vector3 ankle[2];
        // Zero selects the rig's rest bend. Explicit poses can supply a pole.
        Vector3 kneeDirection[2] = { Vector3(), Vector3() };
        bool worldKnees = false;
        double footFollowWeight = -1; // Negative retains the freeFeet policy.
        bool freeFeet = false;
        bool floorContact = false;
        ArmPose arms[2];
    };
    using PoseSampler = std::function<PoseSample(double)>;
    bool poseSequence(const RigStructure& rig, const std::map<std::string, Matrix4x4>& inverse,
        RigAnimationClip& clip, const AnimationParams& params, const PoseSampler& sample);
    // Shared exact boundary poses for enter/hold/exit and knockdown/get-up.
    PoseSample crouchedPose(double weight);
    PoseSample seatedPose(double weight, double seatHeight);
    double actionEnvelope(double t, double prepare = 0.2, double recover = 0.7);
    void addClipEvent(RigAnimationClip& clip, const char* name, double phase, const char* bone = "");
} // namespace biped
} // namespace dust3d
#endif
