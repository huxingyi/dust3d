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

// Procedural walk animation for six-legged fly/insect rig.
//
// Based on: "Procedural Locomotion of Multi-Legged Characters in Dynamic
// Environments" (Abdul Karim et al., LIRIS-5511 / Computer Animation and
// Virtual Worlds 2012).
//
// Algorithm overview (following the paper):
//   1. Determine the body facing direction from the Thorax→Abdomen bone vector.
//   2. Build an orthonormal frame (forward / up / right) so the walk works
//      regardless of whether the fly faces front, down, etc.
//   3. Use a *tripod gait* pattern (Gait Manager, paper §3.3):
//        Group A = {FrontLeft, MiddleRight, BackLeft}
//        Group B = {FrontRight, MiddleLeft, BackRight}
//      Each group alternates between swing and stance phases.
//   4. Per frame, compute each foot's target position:
//        – Stance phase: foot slides from front to back on the ground plane
//          (paper §3.2 – foot blocked on the ground).
//        – Swing phase: foot follows a parabolic arc from back to front
//          (paper §3.2 / §4 – 3D trajectory construction).
//   5. Body (pelvis) gets a slight vertical bob and pitch oscillation,
//      derived from the feet phases (paper §3.2 – pelvis movement).
//   6. Solve CCD Inverse Kinematics per leg to match each tibia tip to
//      its target (paper §3 – CCD IK, refs [34][35]).
//   7. Store bone world transforms and skin matrices for every frame.

#include <array>
#include <cmath>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/animation/fly/walk.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace fly {

namespace {

// ---------------------------------------------------------------------------
// CCD (Cyclic Coordinate Descent) IK solver  –  paper §3, refs [34][35]
// Iteratively adjusts each joint angle from end-effector back to root so
// that the chain tip converges toward the target position.
// joints: ordered positions [root … end-effector]  (root stays fixed)
// ---------------------------------------------------------------------------
void solveCcdIk(std::vector<Vector3>& joints, const Vector3& target, int maxIterations)
{
    const double threshold2 = 1e-8;

    for (int iter = 0; iter < maxIterations; ++iter) {
        double dist2 = (joints.back() - target).lengthSquared();
        if (dist2 <= threshold2)
            break;

        // Walk from the joint just before the end-effector back to the root
        for (int i = static_cast<int>(joints.size()) - 2; i >= 0; --i) {
            Vector3 toEnd = joints.back() - joints[i];
            Vector3 toTarget = target - joints[i];

            if (toEnd.lengthSquared() < 1e-12 || toTarget.lengthSquared() < 1e-12)
                continue;

            Quaternion rotation = Quaternion::rotationTo(toEnd.normalized(), toTarget.normalized());

            // Apply the rotation to every descendant joint
            Matrix4x4 rotMat;
            rotMat.rotate(rotation);
            for (size_t j = i + 1; j < joints.size(); ++j) {
                Vector3 offset = joints[j] - joints[i];
                joints[j] = joints[i] + rotMat.transformVector(offset);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// FABRIK (Forward And Backward Reaching Inverse Kinematics) solver
// Alternative to CCD for stronger geometric behavior in serial 1D chains.
// Iteratively adjusts joint positions to preserve bone lengths while matching
// end-effector to the target.  Works with arbitrary 3D  joint chains.
// joints: ordered positions [root … end-effector]  (root stays fixed)
// ---------------------------------------------------------------------------
void solveFabrikIk(std::vector<Vector3>& joints, const Vector3& target, int maxIterations, const Vector3& preferredPlaneNormal = Vector3())
{
    const double threshold2 = 1e-8;
    const double damping = 0.75;        // soft iterative damping to avoid overshoot
    const double planeBias = 0.45;      // bias toward preferred leg plane
    size_t n = joints.size();
    if (n < 2)
        return;

    // original segment lengths
    std::vector<double> lengths(n - 1);
    double totalLength = 0.0;
    for (size_t i = 0; i < n - 1; ++i) {
        lengths[i] = (joints[i + 1] - joints[i]).length();
        totalLength += lengths[i];
    }

    Vector3 rootPos = joints[0];
    double distRootTarget = (target - rootPos).length();
    Vector3 planeNorm;
    if (!preferredPlaneNormal.isZero())
        planeNorm = preferredPlaneNormal.normalized();

    if (distRootTarget <= 1e-12) {
        // degenerate target; no strong positioning.
        return;
    }

    if (distRootTarget > totalLength) {
        // target unreachable: stretch fully toward target.
        Vector3 direction = (target - rootPos).normalized();
        joints[0] = rootPos;
        for (size_t i = 1; i < n; ++i) {
            joints[i] = joints[i - 1] + direction * lengths[i - 1];
        }
        return;
    }

    std::vector<Vector3> newPos(joints);

    for (int iter = 0; iter < maxIterations; ++iter) {
        // Backward reaching
        newPos[n - 1] = target;
        for (int i = static_cast<int>(n) - 2; i >= 0; --i) {
            Vector3 dir = newPos[i] - newPos[i + 1];
            double r = dir.length();
            if (r < 1e-12) {
                continue;
            }
            dir *= (1.0 / r);
            Vector3 candidate = newPos[i + 1] + dir * lengths[i];
            newPos[i] = newPos[i] * (1.0 - damping) + candidate * damping;
        }

        // Forward reaching
        newPos[0] = rootPos;
        for (size_t i = 0; i < n - 1; ++i) {
            Vector3 dir = newPos[i + 1] - newPos[i];
            double r = dir.length();
            if (r < 1e-12) {
                continue;
            }
            dir *= (1.0 / r);
            Vector3 candidate = newPos[i] + dir * lengths[i];
            newPos[i + 1] = newPos[i + 1] * (1.0 - damping) + candidate * damping;
        }

        // Keep chain close to preferred leg plane, if one exists
        if (!planeNorm.isZero()) {
            for (size_t i = 1; i + 1 < n; ++i) {
                Vector3 item = newPos[i] - rootPos;
                double distToPlane = Vector3::dotProduct(item, planeNorm);
                Vector3 projected = newPos[i] - planeNorm * distToPlane;
                newPos[i] = newPos[i] * (1.0 - planeBias) + projected * planeBias;
            }

            // Re-apply segment lengths after plane bias
            for (size_t i = 0; i < n - 1; ++i) {
                Vector3 dir = newPos[i + 1] - newPos[i];
                double r = dir.length();
                if (r < 1e-12)
                    continue;
                newPos[i + 1] = newPos[i] + dir * (lengths[i] / r);
            }
        }

        if ((newPos.back() - target).lengthSquared() <= threshold2) {
            break;
        }
    }

    joints = newPos;
}

// ---------------------------------------------------------------------------
// Build a bone world transform from animated start/end positions.
// Matches the convention used by RigGenerator::computeBoneWorldTransforms():
//   Translate to boneStart, then rotate so local +Z aligns with bone direction.
// ---------------------------------------------------------------------------
Matrix4x4 buildBoneWorldTransform(const Vector3& boneStart, const Vector3& boneEnd)
{
    Vector3 dir = boneEnd - boneStart;
    Matrix4x4 transform;
    transform.translate(boneStart);
    if (!dir.isZero()) {
        Quaternion orient = Quaternion::rotationTo(Vector3(0.0, 0.0, 1.0), dir.normalized());
        transform.rotate(orient);
    }
    return transform;
}

struct LegDef {
    const char* coxaName;
    const char* femurName;
    const char* tibiaName;
    int gaitGroup; // 0 = Group A, 1 = Group B
};

} // anonymous namespace

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------
bool walk(const RigStructure& rigStructure,
    const std::map<std::string, Matrix4x4>& inverseBindMatrices,
    RigAnimationClip& animationClip,
    int frameCount,
    float durationSeconds,
    const AnimationParams& parameters)
{
    // ----- bone lookup helpers -----
    std::map<std::string, size_t> boneIdx;
    for (size_t i = 0; i < rigStructure.bones.size(); ++i)
        boneIdx[rigStructure.bones[i].name] = i;

    auto getBonePos = [&](const std::string& name) -> Vector3 {
        auto it = boneIdx.find(name);
        if (it == boneIdx.end())
            return Vector3();
        const auto& b = rigStructure.bones[it->second];
        return Vector3(b.posX, b.posY, b.posZ);
    };

    auto getBoneEnd = [&](const std::string& name) -> Vector3 {
        auto it = boneIdx.find(name);
        if (it == boneIdx.end())
            return Vector3();
        const auto& b = rigStructure.bones[it->second];
        return Vector3(b.endX, b.endY, b.endZ);
    };

    // ----- verify required bones -----
    static const char* requiredBones[] = {
        "Head", "Thorax", "Abdomen",
        "FrontLeftCoxa", "FrontLeftFemur", "FrontLeftTibia",
        "FrontRightCoxa", "FrontRightFemur", "FrontRightTibia",
        "MiddleLeftCoxa", "MiddleLeftFemur", "MiddleLeftTibia",
        "MiddleRightCoxa", "MiddleRightFemur", "MiddleRightTibia",
        "BackLeftCoxa", "BackLeftFemur", "BackLeftTibia",
        "BackRightCoxa", "BackRightFemur", "BackRightTibia"
    };
    for (const char* name : requiredBones) {
        if (boneIdx.find(name) == boneIdx.end())
            return false;
    }

    // ===================================================================
    // 1. Determine body facing from bone positions  (paper §3.2)
    // ===================================================================
    // Body vector: direction from abdomen tail toward the head.
    Vector3 bodyVector = getBonePos("Thorax") - getBoneEnd("Abdomen");

    if (bodyVector.isZero())
        return false; // degenerate rig

    // Build an orthonormal frame (forward / up / right) that adapts to
    // the fly's orientation.  If the body is mostly horizontal the fly is
    // "facing front" and walks on the XZ plane; if it points steeply
    // downward (or upward) the walk plane changes accordingly.
    Vector3 forward = bodyVector.normalized();

    Vector3 worldUp(0.0, 1.0, 0.0);
    Vector3 right = Vector3::crossProduct(forward, worldUp);
    if (right.lengthSquared() < 1e-8) {
        // forward is nearly parallel to world-Y – fall back to world-Z
        right = Vector3::crossProduct(forward, Vector3(0.0, 0.0, 1.0));
    }
    right.normalize();
    Vector3 up = Vector3::crossProduct(right, forward).normalized();

    // ===================================================================
    // 2. Define six legs with tripod gait groups  (paper §3.3)
    // ===================================================================
    // Tripod gait – the standard insect locomotion pattern:
    //   Group A (swing first half of cycle): FrontLeft, MiddleRight, BackLeft
    //   Group B (swing second half of cycle): FrontRight, MiddleLeft, BackRight
    static const std::array<LegDef, 6> legs = {{
        { "FrontLeftCoxa",   "FrontLeftFemur",   "FrontLeftTibia",   0 },
        { "FrontRightCoxa",  "FrontRightFemur",  "FrontRightTibia",  1 },
        { "MiddleLeftCoxa",  "MiddleLeftFemur",  "MiddleLeftTibia",  1 },
        { "MiddleRightCoxa", "MiddleRightFemur", "MiddleRightTibia", 0 },
        { "BackLeftCoxa",    "BackLeftFemur",    "BackLeftTibia",    0 },
        { "BackRightCoxa",   "BackRightFemur",   "BackRightTibia",   1 },
    }};

    // Gather rest-pose joint positions for every leg
    struct LegRest {
        Vector3 coxaPos, coxaEnd;
        Vector3 femurEnd;
        Vector3 tibiaEnd; // end-effector (foot tip)
    };
    std::array<LegRest, 6> legRest;
    for (size_t i = 0; i < 6; ++i) {
        legRest[i].coxaPos = getBonePos(legs[i].coxaName);
        legRest[i].coxaEnd = getBoneEnd(legs[i].coxaName);
        legRest[i].femurEnd = getBoneEnd(legs[i].femurName);
        legRest[i].tibiaEnd = getBoneEnd(legs[i].tibiaName);
    }

    // ===================================================================
    // 3. Compute walking parameters from body geometry  (paper §3.1)
    // ===================================================================
    double bodyLength = bodyVector.length();
    double stepLength = bodyLength * 0.10 * parameters.stepLengthFactor; // half-stride extent per leg
    double stepHeight = bodyLength * 0.03 * parameters.stepHeightFactor; // foot lift during swing
    double bodyBobAmp = bodyLength * 0.005 * parameters.bodyBobFactor; // vertical oscillation amplitude

    // Ground level: project all rest foot tips onto the "up" axis and take
    // the minimum projection as the ground reference.
    double minUpProj = Vector3::dotProduct(legRest[0].tibiaEnd, up);
    for (size_t i = 1; i < 6; ++i) {
        double proj = Vector3::dotProduct(legRest[i].tibiaEnd, up);
        if (proj < minUpProj)
            minUpProj = proj;
    }

    // Foot home positions: rest tibia tips projected down to the ground level
    std::array<Vector3, 6> footHome;
    for (size_t i = 0; i < 6; ++i) {
        double proj = Vector3::dotProduct(legRest[i].tibiaEnd, up);
        footHome[i] = legRest[i].tibiaEnd - up * (proj - minUpProj);
    }

    // ===================================================================
    // 4. Generate animation frames
    // ===================================================================
    animationClip.name = "walk";
    animationClip.durationSeconds = durationSeconds;
    animationClip.frames.resize(frameCount);

    for (int frame = 0; frame < frameCount; ++frame) {
        double t = static_cast<double>(frame) / static_cast<double>(frameCount);
        t = fmod(t * parameters.gaitSpeedFactor, 1.0);

        // -- body oscillation (paper §3.2 – pelvis movement) --
        // Two bobs per cycle (one per tripod group switch)
        double bodyBob = bodyBobAmp * std::sin(t * 4.0 * Math::Pi);
        // Slight pitch oscillation around the "right" axis
        double bodyPitch = 0.02 * std::sin(t * 2.0 * Math::Pi);

        Matrix4x4 bodyTransform;
        bodyTransform.translate(up * bodyBob);
        bodyTransform.rotate(right, bodyPitch);

        // -------------------------------------------------------
        // 4a. Compute foot target for each leg  (paper §3.2 / §5)
        // -------------------------------------------------------
        std::array<Vector3, 6> footTarget;

        for (size_t i = 0; i < 6; ++i) {
            int group = legs[i].gaitGroup;
            bool isSwing;
            double legPhase; // 0→1 within the current sub-phase

            // Group 0: swing in [0, 0.5), stance in [0.5, 1.0)
            // Group 1: stance in [0, 0.5), swing in [0.5, 1.0)
            if (group == 0) {
                if (t < 0.5) {
                    isSwing = true;
                    legPhase = t / 0.5;
                } else {
                    isSwing = false;
                    legPhase = (t - 0.5) / 0.5;
                }
            } else {
                if (t < 0.5) {
                    isSwing = false;
                    legPhase = t / 0.5;
                } else {
                    isSwing = true;
                    legPhase = (t - 0.5) / 0.5;
                }
            }

            // Stride endpoints relative to the foot home position
            Vector3 footFront = footHome[i] + forward * stepLength;
            Vector3 footBack = footHome[i] - forward * stepLength;

            if (isSwing) {
                // Swing phase (paper §3.2): parabolic 3D arc from back → front
                Vector3 groundPos = footBack + (footFront - footBack) * legPhase;
                double lift = 4.0 * stepHeight * legPhase * (1.0 - legPhase);
                footTarget[i] = groundPos + up * lift;
            } else {
                // Stance phase: foot planted on the ground, sliding front → back
                footTarget[i] = footFront + (footBack - footFront) * legPhase;
            }
        }

        // -------------------------------------------------------
        // 4b. Body bone transforms
        // -------------------------------------------------------
        std::map<std::string, Matrix4x4> boneWorldTransforms;

        auto computeBodyBone = [&](const std::string& name) {
            Vector3 pos = getBonePos(name);
            Vector3 end = getBoneEnd(name);
            Vector3 newPos = bodyTransform.transformPoint(pos);
            Vector3 newEnd = bodyTransform.transformPoint(end);
            boneWorldTransforms[name] = buildBoneWorldTransform(newPos, newEnd);
        };

        computeBodyBone("Head");
        computeBodyBone("Thorax");
        computeBodyBone("Abdomen");

        // Wings follow the thorax
        for (const char* wingName : { "LeftWing", "RightWing" }) {
            if (boneIdx.count(wingName) == 0)
                continue;
            Vector3 pos = getBonePos(wingName);
            Vector3 end = getBoneEnd(wingName);
            Vector3 newPos = bodyTransform.transformPoint(pos);
            Vector3 newEnd = bodyTransform.transformPoint(end);
            boneWorldTransforms[wingName] = buildBoneWorldTransform(newPos, newEnd);
        }

        // -------------------------------------------------------
        // 4c. Leg IK  (paper §3 – CCD IK, refs [34][35])
        //     For each leg: build a 4-joint chain
        //     [coxaPos, coxaEnd, femurEnd, tibiaEnd]
        //     and solve IK toward the foot target.
        // -------------------------------------------------------
        for (size_t i = 0; i < 6; ++i) {
            Vector3 hipPos = bodyTransform.transformPoint(legRest[i].coxaPos);

            std::vector<Vector3> chain = {
                hipPos,
                bodyTransform.transformPoint(legRest[i].coxaEnd),
                bodyTransform.transformPoint(legRest[i].femurEnd),
                bodyTransform.transformPoint(legRest[i].tibiaEnd)
            };

            Vector3 preferPlane = Vector3::crossProduct(chain[1] - chain[0], chain[2] - chain[1]);
            if (parameters.useFabrikIk) {
                Vector3 plane = parameters.planeStabilization ? preferPlane : Vector3();
                solveFabrikIk(chain, footTarget[i], 15, plane);
            } else {
                solveCcdIk(chain, footTarget[i], 15);
            }

            // Build per-bone transforms from IK solution
            boneWorldTransforms[legs[i].coxaName] = buildBoneWorldTransform(chain[0], chain[1]);
            boneWorldTransforms[legs[i].femurName] = buildBoneWorldTransform(chain[1], chain[2]);
            boneWorldTransforms[legs[i].tibiaName] = buildBoneWorldTransform(chain[2], chain[3]);
        }

        // -------------------------------------------------------
        // 4d. Skin matrices = worldTransform × inverseBindMatrix
        // -------------------------------------------------------
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

} // namespace fly

} // namespace dust3d
