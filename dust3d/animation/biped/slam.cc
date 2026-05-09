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
// Procedural overhead slam for biped rig.
// Both arms rise overhead and drive down in a two-handed axe blow.
// asymEnvelope windup, explosiveEnvelope strike, bilateral arm cascade
// with right-leads-left asymmetry, spine arch+fold, body drop.

#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/biped/slam.h>
#include <dust3d/animation/common.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace biped {

    bool slam(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& inverseBindMatrices,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;
        // Envelope helpers
        auto asymEnvelope = [](double t, double onset, double peak, double end) -> double {
            if (t < onset)
                return 0.0;
            if (t < peak) {
                double p = (t - onset) / (peak - onset);
                return p * p * (3.0 - 2.0 * p);
            }
            if (t < end) {
                double p = (t - peak) / (end - peak);
                double inv = 1.0 - p;
                return inv * inv * (3.0 - 2.0 * inv);
            }
            return 0.0;
        };
        auto explosiveEnvelope = [](double t, double onset, double peak, double decay) -> double {
            if (t < onset)
                return 0.0;
            if (t < peak) {
                double p = (t - onset) / (peak - onset);
                return p * p * p;
            }
            return std::exp(-(t - peak) * decay);
        };
        auto tremble = [](double tRad, double seed, double intensity) -> double {
            return intensity * (0.4 * std::sin(tRad * 11.0 + seed * 3.7) + 0.25 * std::sin(tRad * 17.0 + seed * 5.3) + 0.2 * std::sin(tRad * 23.0 + seed * 7.1) + 0.15 * std::sin(tRad * 31.0 + seed * 11.3));
        };
        // Damped oscillation: bone overshoots at impulse then rings back to rest.
        // Models inertial settling to avoid the "freeze-frame" look at impact.
        auto dampedRing = [](double t, double onset, double freq, double decay) -> double {
            if (t < onset)
                return 0.0;
            double dt = t - onset;
            return std::exp(-decay * dt) * std::sin(freq * dt);
        };

        int frameCount = static_cast<int>(parameters.getValue("frameCount", 48));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 0.9));
        double slamForceFactor = parameters.getValue("slamForceFactor", 1.0);
        double windupHeightFactor = parameters.getValue("windupHeightFactor", 1.0);
        double bodyMassFactor = parameters.getValue("bodyMassFactor", 1.0);
        double crouchDepthFactor = parameters.getValue("crouchDepthFactor", 1.0);
        double recoverySpeedFactor = parameters.getValue("recoverySpeedFactor", 1.0);
        double tailWhipFactor = parameters.getValue("tailWhipFactor", 1.0);
        double spineArchFactor = parameters.getValue("spineArchFactor", 1.0);
        double spineFoldFactor = parameters.getValue("spineFoldFactor", 1.0);
        double armSpreadFactor = parameters.getValue("armSpreadFactor", 1.0);
        auto boneIdx = buildBoneIndexMap(rigStructure);
        auto bonePos = [&](const std::string& name) -> Vector3 {
            return getBonePos(rigStructure, boneIdx, name);
        };
        auto boneEnd = [&](const std::string& name) -> Vector3 {
            return getBoneEnd(rigStructure, boneIdx, name);
        };

        static const char* requiredBones[] = {
            "Root", "Hips", "Spine", "Chest", "Neck", "Head",
            "LeftUpperLeg", "LeftLowerLeg", "LeftFoot",
            "RightUpperLeg", "RightLowerLeg", "RightFoot",
            "LeftShoulder", "LeftUpperArm", "LeftLowerArm", "LeftHand",
            "RightShoulder", "RightUpperArm", "RightLowerArm", "RightHand"
        };
        if (!validateRequiredBones(boneIdx, requiredBones,
                sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;

        Vector3 upDir(0.0, 1.0, 0.0);
        Vector3 hipsPos = bonePos("Hips");
        Vector3 avgFootEnd = (boneEnd("LeftFoot") + boneEnd("RightFoot")) * 0.5;
        Vector3 hipsToFoot = avgFootEnd - hipsPos;
        Vector3 forward(hipsToFoot.x(), 0.0, hipsToFoot.z());
        if (forward.lengthSquared() < 1e-8)
            forward = Vector3(0.0, 0.0, 1.0);
        forward.normalize();
        Vector3 right = Vector3::crossProduct(upDir, forward);
        if (right.lengthSquared() < 1e-8)
            right = Vector3(1.0, 0.0, 0.0);
        right.normalize();

        double legLen = ((boneEnd("LeftUpperLeg") - bonePos("LeftUpperLeg")).length()
                            + (boneEnd("LeftLowerLeg") - bonePos("LeftLowerLeg")).length()
                            + (boneEnd("LeftFoot") - bonePos("LeftFoot")).length()
                            + (boneEnd("RightUpperLeg") - bonePos("RightUpperLeg")).length()
                            + (boneEnd("RightLowerLeg") - bonePos("RightLowerLeg")).length()
                            + (boneEnd("RightFoot") - bonePos("RightFoot")).length())
            * 0.5;
        if (legLen < 1e-6)
            legLen = 1.0;

        double massInertia = 0.6 + 0.4 * bodyMassFactor;

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        for (int frame = 0; frame < frameCount; ++frame) {
            double t = static_cast<double>(frame) / static_cast<double>(frameCount);
            std::map<std::string, Matrix4x4> boneWorldTransforms;

            // Pre-anticipation: before arms rise the body dips slightly forward and down.
            // This loading motion telegraphs intent and makes the subsequent rise feel powered.
            double anticipate = asymEnvelope(t, 0.0, 0.05, 0.13);

            // Windup: arms overhead, spine arches back [0.0 - 0.30]
            double windupRaise = asymEnvelope(t, 0.0, 0.14, 0.32);
            // Strike: explosive downward [0.28 - 0.38]
            double strikeBurst = explosiveEnvelope(t, 0.28, 0.38, 5.0 / massInertia) * slamForceFactor;
            // Hit-stop: smooth bell so the freeze-frame eases in and out without snapping
            double hitStopVal = asymEnvelope(t, 0.38, 0.43, 0.53);
            // Follow-through / ground-compress [0.48 - 0.65]
            double followThru = asymEnvelope(t, 0.46, 0.55, 0.68) * recoverySpeedFactor;
            // Recovery [0.68 - 1.0]
            double recover = asymEnvelope(t, 0.68, 0.84, 1.0);

            // Spine: arches BACK during windup, folds FORWARD explosively on strike.
            // dampedRing adds a ~1 Hz forward-then-back oscillation at impact — the spine
            // overshoots its forward fold then rings back to neutral, like a whip crack.
            double impactRing = dampedRing(t, 0.38, 10.0, 13.0) * 0.07 * slamForceFactor * spineFoldFactor;
            double spinePitch = windupRaise * (-0.28) * windupHeightFactor * spineArchFactor
                + strikeBurst * 0.42 * slamForceFactor * spineFoldFactor
                + hitStopVal * 0.36 * slamForceFactor * spineFoldFactor
                + followThru * 0.14 * recoverySpeedFactor
                - recover * 0.50
                + impactRing;

            // Body: anticipation dip → rise during windup → crash on strike.
            // The anticipate dip is subtle (1.5% legLen) but gives the brain a visual cue
            // that weight is being loaded before the arms move.
            double verticalOffset = -anticipate * legLen * 0.015
                + windupRaise * legLen * 0.04 * windupHeightFactor
                - strikeBurst * legLen * 0.12 * crouchDepthFactor
                - hitStopVal * legLen * 0.10 * crouchDepthFactor
                - followThru * legLen * 0.06 * crouchDepthFactor
                + recover * legLen * 0.08;
            double forwardLean = anticipate * legLen * 0.012 // lean into the load
                + strikeBurst * legLen * 0.07 * slamForceFactor / massInertia;

            // Lateral rock: body tilts toward the dominant arm during windup (weight loads
            // right), transfers to left at impact — roll-axis lean that breaks rigid symmetry.
            double lateralRock = windupRaise * 0.032 * armSpreadFactor
                - strikeBurst * 0.018 * slamForceFactor
                - hitStopVal * 0.014;

            Matrix4x4 bodyTransform;
            bodyTransform.translate(upDir * verticalOffset + forward * forwardLean);
            bodyTransform.rotate(forward, lateralRock); // roll axis — lateral tilt
            // FK chain: tracks each bone's animated world-space end position.
            std::map<std::string, Vector3> boneChainEnd;
            auto computeChainBone = [&](const std::string& name, const std::string& parentName,
                                        double extraYaw = 0.0, double extraPitch = 0.0, double extraRoll = 0.0) {
                Vector3 pos = bonePos(name), end = boneEnd(name);
                // Bone direction in body space -- preserves rest-pose bone length.
                Vector3 boneDir = bodyTransform.transformVector(end - pos);
                // Apply extra rotations to the direction only (not to the start pos).
                if (std::abs(extraYaw) > 1e-6 || std::abs(extraPitch) > 1e-6 || std::abs(extraRoll) > 1e-6) {
                    Matrix4x4 r;
                    if (std::abs(extraRoll) > 1e-6)
                        r.rotate(forward, extraRoll);
                    if (std::abs(extraYaw) > 1e-6)
                        r.rotate(upDir, extraYaw);
                    if (std::abs(extraPitch) > 1e-6)
                        r.rotate(right, extraPitch);
                    boneDir = r.transformVector(boneDir);
                }
                // Start: parent's animated end + the rest-pose gap DISTANCE offset
                // along boneDir. This keeps |parent-end to child-begin| equal to the
                // rest-pose distance without locking the gap direction, so the child
                // always continues naturally from the parent with no mesh shrinkage.
                Vector3 newPos;
                if (!parentName.empty() && boneChainEnd.count(parentName) > 0) {
                    double gapDist = (pos - boneEnd(parentName)).length();
                    if (gapDist > 1e-8) {
                        double boneDirLen = boneDir.length();
                        Vector3 gapOffset = (boneDirLen > 1e-8)
                            ? boneDir * (gapDist / boneDirLen)
                            : Vector3();
                        newPos = boneChainEnd[parentName] + gapOffset;
                    } else {
                        newPos = boneChainEnd[parentName];
                    }
                } else {
                    newPos = bodyTransform.transformPoint(pos);
                }
                Vector3 newEnd = newPos + boneDir;
                boneChainEnd[name] = newEnd;
                boneWorldTransforms[name] = buildBoneWorldTransform(newPos, newEnd);
            };

            computeChainBone("Root", "");
            computeChainBone("Hips", "");
            computeChainBone("Spine", "Hips", 0.0, spinePitch * 0.55);
            computeChainBone("Chest", "Spine", 0.0, spinePitch * 0.35);
            computeChainBone("Neck", "Chest", 0.0, -spinePitch * 0.20);
            // Head: drives FORWARD at impact and lags BEHIND on recovery —
            // secondary motion that reads as organic weight.
            double headDrive = strikeBurst * 0.10 * slamForceFactor + hitStopVal * 0.08 * slamForceFactor;
            double headLag = recover * (-0.06); // head slightly behind on the way back
            computeChainBone("Head", "Neck", 0.0, -spinePitch * 0.25 + headDrive + headLag);

            // Tail: reacts to spine arc
            {
                static const char* tailBones[] = { "TailBase", "TailMid", "TailTip" };
                Vector3 prevEnd;
                bool hasPrev = false;
                for (int ti = 0; ti < 3; ++ti) {
                    if (boneIdx.count(tailBones[ti]) == 0)
                        continue;
                    double delay = ti * 0.05;
                    double tb = tailWhipFactor * (windupRaise * (-0.10) * (1.0 + ti * 0.3) + explosiveEnvelope(t - delay, 0.28, 0.38, 3.0 / massInertia) * 0.18 * (1.0 + ti * 0.5));
                    Vector3 pos = bonePos(tailBones[ti]), end = boneEnd(tailBones[ti]);
                    Vector3 newPos = bodyTransform.transformPoint(pos);
                    Vector3 newEnd = bodyTransform.transformPoint(end);
                    if (hasPrev) {
                        newPos = prevEnd;
                        newEnd = newPos + (newEnd - newPos);
                    }
                    if (std::abs(tb) > 1e-6) {
                        Matrix4x4 r;
                        r.rotate(right, tb);
                        newEnd = newPos + r.transformVector(newEnd - newPos);
                    }
                    boneWorldTransforms[tailBones[ti]] = buildBoneWorldTransform(newPos, newEnd);
                    prevEnd = newEnd;
                    hasPrev = true;
                }
            }
            auto computeLeg = [&](const char* ul, const char* ll, const char* f, double lift) {
                Vector3 footStart = bonePos(f) + upDir * lift;
                Vector3 footVec = boneEnd(f) - bonePos(f); // rest-pose foot direction+length
                Vector3 hipJoint = bodyTransform.transformPoint(bonePos(ul));
                double upperLen = (boneEnd(ul) - bonePos(ul)).length();
                double lowerLen = (boneEnd(ll) - bonePos(ll)).length();
                Vector3 midBind = bodyTransform.transformPoint(bonePos(ll));
                Vector3 poleTarget = midBind + forward * (upperLen * 0.5);
                // Init joints with rest-pose lengths so IK never stretches bones
                Vector3 hipToKnee = midBind - hipJoint;
                double hkLen = hipToKnee.length();
                Vector3 kneeInit = (hkLen > 1e-6) ? hipJoint + hipToKnee * (upperLen / hkLen) : hipJoint + upDir * (-upperLen);
                Vector3 kneeToFoot = footStart - kneeInit;
                double kfLen = kneeToFoot.length();
                Vector3 ankleInit = (kfLen > 1e-6) ? kneeInit + kneeToFoot * (lowerLen / kfLen) : kneeInit + upDir * (-lowerLen);
                std::vector<Vector3> joints = { hipJoint, kneeInit, ankleInit };
                solveTwoBoneIk(joints, footStart, poleTarget, 0.05);
                boneWorldTransforms[ul] = buildBoneWorldTransform(joints[0], joints[1]);
                boneWorldTransforms[ll] = buildBoneWorldTransform(joints[1], joints[2]);
                // Foot starts at IK-solved ankle so there is no gap with the lower leg
                boneWorldTransforms[f] = buildBoneWorldTransform(joints[2], joints[2] + footVec);
            };
            computeLeg("LeftUpperLeg", "LeftLowerLeg", "LeftFoot", 0.0);
            computeLeg("RightUpperLeg", "RightLowerLeg", "RightFoot", 0.0);

            // BILATERAL ARMS — right leads left by 0.025s for organic asymmetry
            for (int side = 0; side < 2; ++side) {
                bool isLeft = (side == 0);
                double delay = isLeft ? 0.025 : 0.0;
                double sideSign = isLeft ? -1.0 : 1.0;
                const char* shoulder = isLeft ? "LeftShoulder" : "RightShoulder";
                const char* upper = isLeft ? "LeftUpperArm" : "RightUpperArm";
                const char* lower = isLeft ? "LeftLowerArm" : "RightLowerArm";
                const char* hand = isLeft ? "LeftHand" : "RightHand";

                // Shoulder lifts during windup
                // Shoulder start: chest's FK-propagated end so the arm chain
                // is physically connected to the spine (no gap under spine rotation).
                Vector3 shPos = boneChainEnd.count("Chest") > 0
                    ? boneChainEnd["Chest"]
                    : bodyTransform.transformPoint(bonePos(shoulder));
                Vector3 shEnd = shPos + bodyTransform.transformVector(boneEnd(shoulder) - bonePos(shoulder));
                {
                    double shLift = windupRaise * 0.07 * windupHeightFactor * armSpreadFactor - strikeBurst * 0.05;
                    Matrix4x4 r;
                    r.rotate(right, shLift);
                    shEnd = shPos + r.transformVector(shEnd - shPos);
                }
                boneWorldTransforms[shoulder] = buildBoneWorldTransform(shPos, shEnd);

                Vector3 upperStart = shEnd;
                Vector3 upperDir = bodyTransform.transformVector(boneEnd(upper) - bonePos(upper));

                // Arms pitch to OVERHEAD (-1.55 rad) in windup, slam DOWN (+1.70 rad) at strike
                double upperWindup = asymEnvelope(t - delay, 0.0, 0.14, 0.32) * windupHeightFactor * armSpreadFactor;
                double upperStrike = explosiveEnvelope(t - delay, 0.28, 0.38, 5.0 / massInertia) * slamForceFactor;
                double upperPitch = upperWindup * (-1.55)
                    + upperStrike * 1.70
                    + hitStopVal * 1.55 * slamForceFactor
                    + followThru * 0.14
                    - recover * 1.60;
                double upperYaw = windupRaise * sideSign * 0.14 - strikeBurst * sideSign * 0.08;
                Matrix4x4 r1;
                r1.rotate(right, upperPitch);
                r1.rotate(upDir, upperYaw);
                Vector3 upperEnd = upperStart + r1.transformVector(upperDir);
                boneWorldTransforms[upper] = buildBoneWorldTransform(upperStart, upperEnd);

                // Forearm: slight bend at overhead, extends on strike
                Vector3 lowerDir = bodyTransform.transformVector(boneEnd(lower) - bonePos(lower));
                double forearmBurst = explosiveEnvelope(t - delay - 0.02, 0.28, 0.38, 5.0 / massInertia) * slamForceFactor;
                double elbowBend = windupRaise * 0.16 * windupHeightFactor - forearmBurst * 0.10 + recover * 0.18;
                Matrix4x4 r2;
                r2.rotate(right, upperPitch * 0.55 + elbowBend);
                Vector3 lowerEnd = upperEnd + r2.transformVector(lowerDir);
                boneWorldTransforms[lower] = buildBoneWorldTransform(upperEnd, lowerEnd);

                // Hand: wrist crack at impact.
                // Tremble during hit-stop: micro-noise while the muscles hold against the impulse.
                Vector3 handDir = bodyTransform.transformVector(boneEnd(hand) - bonePos(hand));
                double handBurst = explosiveEnvelope(t - delay - 0.04, 0.28, 0.38, 6.0 / massInertia) * slamForceFactor;
                double tRad = t * Math::Pi * 2.0 * 22.0;
                double armTremble = tremble(tRad, isLeft ? 1.7 : 2.3, hitStopVal * 0.016 * slamForceFactor);
                // Wrist also rings at impact — overshoots forward then settles
                double wristRing = dampedRing(t - delay, 0.38, 14.0, 18.0) * 0.05 * slamForceFactor;
                double wristSnap = handBurst * 0.24 * slamForceFactor - recover * 0.22 + armTremble + wristRing;
                Matrix4x4 r3;
                r3.rotate(right, upperPitch * 0.35 + wristSnap);
                Vector3 handEnd = lowerEnd + r3.transformVector(handDir);
                boneWorldTransforms[hand] = buildBoneWorldTransform(lowerEnd, handEnd);
            }
            auto& animFrame = animationClip.frames[frame];
            animFrame.time = static_cast<float>(t) * durationSeconds;
            animFrame.boneWorldTransforms = boneWorldTransforms;
            for (const auto& pair : boneWorldTransforms) {
                auto invIt = inverseBindMatrices.find(pair.first);
                if (invIt != inverseBindMatrices.end()) {
                    Matrix4x4 skinMat = pair.second;
                    skinMat *= invIt->second;
                    animFrame.boneSkinMatrices[pair.first] = skinMat;
                }
            }
        }
        return true;
    }

} // namespace biped

} // namespace dust3d
