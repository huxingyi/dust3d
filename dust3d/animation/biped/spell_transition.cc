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

#include <dust3d/animation/biped/cast.h>
#include <dust3d/animation/biped/channel.h>
#include <dust3d/animation/biped/idle.h>
#include <dust3d/animation/biped/pose_sequence.h>
#include <dust3d/animation/biped/secondary_motion.h>
#include <dust3d/animation/biped/spell_transition.h>
namespace dust3d {
namespace biped {
    bool spellMotion(const RigStructure& rig, const std::map<std::string, Matrix4x4>& inverse,
        RigAnimationClip& clip, const AnimationParams& params, SpellKind kind)
    {
        bool casting = kind == SpellKind::CastStart || kind == SpellKind::CastRelease || kind == SpellKind::CastRecover;
        AnimationParams sourceParams = params;
        sourceParams.setValue("durationSeconds", 1);
        sourceParams.setValue("frameCount", 121);
        RigAnimationClip source, neutral;
        source.loop = !casting;
        if (!(casting ? cast(rig, inverse, source, sourceParams) : channel(rig, inverse, source, sourceParams)))
            return false;
        for (auto& frame : source.frames)
            if (casting)
                relaxUndirectedArms(rig, params, frame);
        if (!casting && !idle(rig, inverse, neutral, sourceParams))
            return false;
        double begin = kind == SpellKind::CastRelease ? 0.20 : kind == SpellKind::CastRecover ? 0.60
                                                                                              : 0;
        double finish = kind == SpellKind::CastStart ? 0.20 : kind == SpellKind::CastRelease ? 0.60
                                                                                             : 1;
        clip.durationSeconds = params.getValue("durationSeconds", 0.5);
        int count = std::clamp(static_cast<int>(params.getValue("frameCount", 30)), 2, 4999);
        clip.frames.resize(count);
        for (int i = 0; i < count; ++i) {
            double t = double(i) / (count - 1);
            auto& frame = clip.frames[i];
            frame.time = t * clip.durationSeconds;
            double sourceTime = begin + (finish - begin) * t;
            double key = sourceTime * (source.frames.size() - 1);
            size_t a = std::min(static_cast<size_t>(key), source.frames.size() - 1), z = std::min(a + 1, source.frames.size() - 1);
            double weight = key - a;
            const auto* from = &source.frames[a].boneWorldTransforms;
            const auto* to = &source.frames[z].boneWorldTransforms;
            if (!casting) {
                from = &neutral.frames.front().boneWorldTransforms;
                to = &source.frames.front().boneWorldTransforms;
                weight = kind == SpellKind::ChannelEnter ? easePose(t) : 1 - easePose(t);
            }
            // Blend in parent space to preserve joint offsets throughout enter/exit.
            // Resolve recursively because the rig need not be in hierarchy order.
            auto idx = animation::buildBoneIndexMap(rig);
            auto rest = animation::restBoneWorldTransforms(rig);
            std::map<std::string, bool> visiting;
            std::map<std::string, bool> primary;
            for (const char* name : { "Root", "Hips", "Spine", "Chest", "Neck", "Head",
                     "LeftShoulder", "LeftUpperArm", "LeftLowerArm", "LeftHand", "RightShoulder", "RightUpperArm", "RightLowerArm", "RightHand",
                     "LeftUpperLeg", "LeftLowerLeg", "LeftFoot", "RightUpperLeg", "RightLowerLeg", "RightFoot", "TailBase", "TailMid", "TailTip" })
                primary[name] = true;
            std::function<void(const std::string&)> blend = [&](const std::string& name) {
                if (frame.boneWorldTransforms.count(name) || !idx.count(name) || visiting[name])
                    return;
                visiting[name] = true;
                const auto& bone = rig.bones[idx.at(name)];
                if (name.find("HairBack") == 0 || name.find("Cape") != std::string::npos)
                    return;
                if (idx.count(bone.parent))
                    blend(bone.parent);
                auto posed = [&](const std::map<std::string, Matrix4x4>& poses, const std::string& key) {
                    auto it = poses.find(key);
                    return it == poses.end() ? rest.at(key) : it->second;
                };
                Matrix4x4 a = posed(*from, name), z = posed(*to, name);
                if (idx.count(bone.parent)) {
                    a = composePose(posed(*from, bone.parent).inverted(), a);
                    z = composePose(posed(*to, bone.parent).inverted(), z);
                }
                Matrix4x4 local;
                Vector3 start = a.transformPoint(Vector3()), end = z.transformPoint(Vector3());
                local.translate(start + (end - start) * weight);
                local.rotate(Quaternion::slerp(poseOrientation(a), poseOrientation(z), weight));
                if (frame.boneWorldTransforms.count(bone.parent))
                    local = composePose(frame.boneWorldTransforms.at(bone.parent), local);
                frame.boneWorldTransforms[name] = local;
            };
            for (const auto& bone : rig.bones)
                if (primary.count(bone.name))
                    blend(bone.name);
            // Leave accessories and secondary descendants to final inheritance,
            // after the hair/cape simulation has posed their actual parents.
            for (auto it = frame.boneWorldTransforms.begin(); it != frame.boneWorldTransforms.end();) {
                if (!primary.count(it->first))
                    it = frame.boneWorldTransforms.erase(it);
                else
                    ++it;
            }
            if (kind == SpellKind::ChannelInterrupt) {
                auto idx = animation::buildBoneIndexMap(rig);
                Vector3 up(0, 1, 0), forward = facing(rig, idx), left = Vector3::crossProduct(up, forward);
                Vector3 spine = frame.boneWorldTransforms.at("Spine").transformPoint(Vector3());
                Matrix4x4 recoil = animation::rotationAbout(spine, left, -0.15 * actionEnvelope(t));
                for (const std::string name : { "Chest", "Neck", "Head", "LeftShoulder", "LeftUpperArm", "LeftLowerArm", "LeftHand", "RightShoulder", "RightUpperArm", "RightLowerArm", "RightHand" })
                    if (frame.boneWorldTransforms.count(name))
                        frame.boneWorldTransforms[name] = composePose(recoil, frame.boneWorldTransforms.at(name));
            }
        }
        simulateSecondaryMotion(rig, clip);
        for (auto& frame : clip.frames)
            animation::finishFrame(frame, inverse);
        if (kind == SpellKind::CastRelease)
            addClipEvent(clip, "release", 0.35, "RightHand");
        if (kind == SpellKind::CastRecover || kind == SpellKind::ChannelExit || kind == SpellKind::ChannelInterrupt)
            addClipEvent(clip, "recovery", 0.8);
        return true;
    }
} // namespace biped
} // namespace dust3d
