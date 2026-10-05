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
#include <dust3d/animation/biped/pose.h>
#include <dust3d/animation/biped/secondary_motion.h>
namespace dust3d {
namespace biped {
    void simulateSecondaryMotion(const RigStructure& rig, RigAnimationClip& clip)
    {
        using namespace animation;
        if (clip.frames.empty())
            return;
        auto idx = buildBoneIndexMap(rig);
        auto rest = restBoneWorldTransforms(rig);
        HairChainSimulator hair;
        CapeGridSimulator cape;
        if (idx.count("HairBack1"))
            hair.initialize(rig, idx, { "HairBack1", "HairBack2", "HairBack3" }, rest.at("Head"), 0.22, 0.97, 0.4);
        if (idx.count("CenterCape1"))
            cape.initialize(rig, idx, rest.at("Chest"), 0.08, 0.85, 1.2, 0.15);
        if (!hair.active && !cape.active)
            return;
        auto step = [&](const Matrix4x4& head, const Matrix4x4& chest, double dt,
                        std::map<std::string, Matrix4x4>& out) {
            if (hair.active)
                hair.step(head, dt, out);
            if (cape.active)
                cape.step(chest, dt, out);
        };
        auto interpolate = [](const Matrix4x4& from, const Matrix4x4& to, double t) {
            Matrix4x4 result;
            Vector3 a = from.transformPoint(Vector3()), b = to.transformPoint(Vector3());
            result.translate(a + (b - a) * t);
            result.rotate(Quaternion::slerp(poseOrientation(from), poseOrientation(to), t));
            return result;
        };
        std::map<std::string, Matrix4x4> simulated;
        auto& initial = clip.frames.front().boneWorldTransforms;
        // Avoid replaying an action during warm-up: its initial pose is stationary.
        for (int i = 0; i < 120; ++i)
            step(initial.at("Head"), initial.at("Chest"), 1.0 / 60.0, simulated);
        // Like gait.cc, run complete loop cycles before recording secondary motion.
        int passes = clip.loop ? 3 : 1;
        for (int pass = 0; pass < passes; ++pass) {
            for (size_t i = 0; i < clip.frames.size(); ++i) {
                auto& current = clip.frames[i];
                if (i > 0 || (clip.loop && pass > 0)) {
                    const auto& previous = clip.frames[i ? i - 1 : clip.frames.size() - 1];
                    double dt = i ? current.time - previous.time : clip.durationSeconds - previous.time;
                    // Sparse clips must not feed a whole action into one Verlet step.
                    int substeps = std::max(1, static_cast<int>(std::min(240.0, std::ceil(dt * 60.0))));
                    for (int j = 1; j <= substeps; ++j) {
                        double t = double(j) / substeps;
                        step(interpolate(previous.boneWorldTransforms.at("Head"), current.boneWorldTransforms.at("Head"), t),
                            interpolate(previous.boneWorldTransforms.at("Chest"), current.boneWorldTransforms.at("Chest"), t),
                            dt / substeps, simulated);
                    }
                }
                if (pass == passes - 1)
                    for (const auto& bone : simulated)
                        current.boneWorldTransforms[bone.first] = bone.second;
            }
        }
        // Roll correction, boundary recovery and skin matrices are finalized by
        // AnimationGenerator after all primary and secondary poses are present.
    }
} // namespace biped
} // namespace dust3d
