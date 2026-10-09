#ifndef DUST3D_BIPED_POSE_H_
#define DUST3D_BIPED_POSE_H_

#include <dust3d/animation/common.h>

namespace dust3d {
namespace biped {

    inline Matrix4x4 composePose(Matrix4x4 a, const Matrix4x4& b)
    {
        a *= b;
        return a;
    }

    inline double easePose(double x)
    {
        x = std::clamp(x, 0.0, 1.0);
        return x * x * x * (x * (x * 6.0 - 15.0) + 10.0);
    }

    inline Vector3 facing(const RigStructure& rig, const std::map<std::string, size_t>& idx)
    {
        Vector3 v = (animation::getBoneEnd(rig, idx, "LeftFoot") + animation::getBoneEnd(rig, idx, "RightFoot")) * 0.5
            - animation::getBonePos(rig, idx, "Hips");
        v.setY(0.0);
        return v.lengthSquared() < 1e-10 ? Vector3(0, 0, 1) : v.normalized();
    }

    // Animation-space posture only: the bind skeleton and inverse binds never change.
    // armPosture: 0 = auto (sideways arms), 1 = relaxed humanoid, 2 = original bind.
    inline double relaxedArmWeight(const RigStructure& rig, const std::map<std::string, size_t>& idx,
        const std::string& upper, const AnimationParams& params)
    {
        using namespace animation;
        Vector3 dir = getBoneEnd(rig, idx, upper) - getBonePos(rig, idx, upper);
        Vector3 forward = facing(rig, idx);
        Vector3 lateral = Vector3::crossProduct(Vector3(0, 1, 0), forward);
        double posture = params.getValue("armPosture", 0.0);
        if (dir.lengthSquared() < 1e-10 || posture >= 1.5)
            return 0.0;
        Vector3 unit = dir.normalized();
        std::string lower = upper.substr(0, upper.size() - 8) + "LowerArm";
        Vector3 lowerDir = getBoneEnd(rig, idx, lower) - getBonePos(rig, idx, lower);
        double lowerSpread = lowerDir.lengthSquared() < 1e-10 ? 0.0
                                                              : std::abs(Vector3::dotProduct(lowerDir.normalized(), lateral));
        if (posture < 0.5 && ((std::abs(Vector3::dotProduct(unit, lateral)) < 0.20 && lowerSpread < 0.30) || std::abs(Vector3::dotProduct(unit, forward)) > 0.65))
            return 0.0;
        return std::clamp(params.getValue("relaxedArmsFactor", 1.0), 0.0, 1.0);
    }

    inline Matrix4x4 relaxedArm(const RigStructure& rig, const std::map<std::string, size_t>& idx,
        const std::string& upper, const AnimationParams& params)
    {
        using namespace animation;
        Vector3 dir = getBoneEnd(rig, idx, upper) - getBonePos(rig, idx, upper);
        Vector3 forward = facing(rig, idx);
        Vector3 lateral = Vector3::crossProduct(Vector3(0, 1, 0), forward);
        Vector3 outward = lateral * (Vector3::dotProduct(dir, lateral) >= 0 ? 1.0 : -1.0);
        Vector3 target = (Vector3(0, -1, 0) + outward * 0.16 + forward * 0.06).normalized();
        return turnAbout(getBonePos(rig, idx, upper), dir, target, relaxedArmWeight(rig, idx, upper, params));
    }

    inline Matrix4x4 relaxedForearm(const RigStructure& rig, const std::map<std::string, size_t>& idx,
        const std::string& prefix, const AnimationParams& params)
    {
        using namespace animation;
        std::string upper = prefix + "UpperArm", lower = prefix + "LowerArm";
        Matrix4x4 u = relaxedArm(rig, idx, upper, params);
        Vector3 restDir = getBoneEnd(rig, idx, upper) - getBonePos(rig, idx, upper);
        double weight = relaxedArmWeight(rig, idx, upper, params);
        if (weight <= 0.0)
            return u;
        Vector3 forward = facing(rig, idx);
        Vector3 lateral = Vector3::crossProduct(Vector3(0, 1, 0), forward);
        Vector3 outward = lateral * (Vector3::dotProduct(restDir, lateral) >= 0 ? 1.0 : -1.0);
        Vector3 target = (Vector3(0, -1, 0) + outward * 0.10 + forward * 0.18).normalized();
        Vector3 dir = getBoneEnd(rig, idx, lower) - getBonePos(rig, idx, lower);
        return composePose(turnAbout(u.transformPoint(getBonePos(rig, idx, lower)), u.transformVector(dir), target, weight), u);
    }

    // Pose a complete arm by directions, preserving joint offsets, lengths and bind roll.
    inline void aimArm(const RigStructure& rig, const std::map<std::string, size_t>& idx,
        const std::map<std::string, Matrix4x4>& rest, std::map<std::string, Matrix4x4>& world,
        const std::string& prefix, const Matrix4x4& shoulderLayer,
        const Vector3& upperTarget, const Vector3& lowerTarget, double weight,
        const AnimationParams& params, std::map<std::string, Quaternion>* swingHistory = nullptr)
    {
        using namespace animation;
        const std::string shoulder = prefix + "Shoulder", upper = prefix + "UpperArm", lower = prefix + "LowerArm", hand = prefix + "Hand";
        if (!rest.count(shoulder) || !rest.count(upper) || !rest.count(lower) || !rest.count(hand))
            return;
        world[shoulder] = composePose(shoulderLayer, rest.at(shoulder));
        Matrix4x4 base = composePose(shoulderLayer, relaxedArm(rig, idx, upper, params));
        Vector3 dir = getBoneEnd(rig, idx, upper) - getBonePos(rig, idx, upper);
        Matrix4x4 u = composePose(turnAbout(base.transformPoint(getBonePos(rig, idx, upper)), base.transformVector(dir), upperTarget, weight, swingHistory ? &(*swingHistory)[upper] : nullptr), base);
        Vector3 lowerDir = getBoneEnd(rig, idx, lower) - getBonePos(rig, idx, lower);
        Matrix4x4 lowerBase = u;
        lowerBase *= relaxedArm(rig, idx, upper, params).inverted();
        lowerBase *= relaxedForearm(rig, idx, prefix, params);
        Matrix4x4 l = composePose(turnAbout(lowerBase.transformPoint(getBonePos(rig, idx, lower)), lowerBase.transformVector(lowerDir), lowerTarget, weight, swingHistory ? &(*swingHistory)[lower] : nullptr), lowerBase);
        world[upper] = composePose(u, rest.at(upper));
        world[lower] = composePose(l, rest.at(lower));
        world[hand] = composePose(l, rest.at(hand));
    }

    inline Quaternion poseOrientation(const Matrix4x4& matrix)
    {
        Vector3 c0 = matrix.transformVector(Vector3(1, 0, 0));
        Vector3 c1 = matrix.transformVector(Vector3(0, 1, 0));
        Vector3 c2 = matrix.transformVector(Vector3(0, 0, 1));
        double r00 = c0.x(), r10 = c0.y(), r20 = c0.z();
        double r01 = c1.x(), r11 = c1.y(), r21 = c1.z();
        double r02 = c2.x(), r12 = c2.y(), r22 = c2.z();
        double trace = r00 + r11 + r22;
        if (trace > 0.0) {
            double s = std::sqrt(trace + 1.0) * 2.0;
            return Quaternion(0.25 * s, (r21 - r12) / s, (r02 - r20) / s, (r10 - r01) / s).normalized();
        }
        if (r00 > r11 && r00 > r22) {
            double s = std::sqrt(1.0 + r00 - r11 - r22) * 2.0;
            return Quaternion((r21 - r12) / s, 0.25 * s, (r01 + r10) / s, (r02 + r20) / s).normalized();
        }
        if (r11 > r22) {
            double s = std::sqrt(1.0 + r11 - r00 - r22) * 2.0;
            return Quaternion((r02 - r20) / s, (r01 + r10) / s, 0.25 * s, (r12 + r21) / s).normalized();
        }
        double s = std::sqrt(1.0 + r22 - r00 - r11) * 2.0;
        return Quaternion((r10 - r01) / s, (r02 + r20) / s, (r12 + r21) / s, 0.25 * s).normalized();
    }

    // Relaxed action boundaries share idle's complete arm bind frame, including
    // forearm roll. Blend parent-space rotations so joint lengths stay unchanged.
    inline void settleRelaxedArms(const RigStructure& rig, const AnimationParams& params,
        BoneAnimationFrame& frame, double weight)
    {
        if (weight <= 0.0)
            return;
        using namespace animation;
        auto idx = buildBoneIndexMap(rig);
        auto rest = restBoneWorldTransforms(rig);
        const auto original = frame.boneWorldTransforms;
        auto target = original;
        if (!original.count("Chest"))
            return;
        Matrix4x4 chest = composePose(original.at("Chest"), rest.at("Chest").inverted());
        for (const std::string prefix : { "Left", "Right" }) {
            aimArm(rig, idx, rest, target, prefix, chest, Vector3(), Vector3(), 0.0, params);
            for (const std::string suffix : { "Shoulder", "UpperArm", "LowerArm", "Hand" }) {
                std::string name = prefix + suffix;
                if (!idx.count(name) || !original.count(name) || !target.count(name))
                    continue;
                const auto& parent = rig.bones[idx.at(name)].parent;
                if (!original.count(parent) || !target.count(parent) || !frame.boneWorldTransforms.count(parent))
                    continue;
                Matrix4x4 from = composePose(original.at(parent).inverted(), original.at(name));
                Matrix4x4 to = composePose(target.at(parent).inverted(), target.at(name));
                Matrix4x4 local;
                Vector3 start = from.transformPoint(Vector3()), end = to.transformPoint(Vector3());
                local.translate(start + (end - start) * weight);
                local.rotate(Quaternion::slerp(poseOrientation(from), poseOrientation(to), weight));
                frame.boneWorldTransforms[name] = composePose(frame.boneWorldTransforms.at(parent), weight >= 1.0 ? to : local);
            }
        }
    }

    // A common stationary drape lets actions blend with idle after changing the
    // neutral arms. Undriven secondary bones use it throughout; simulated one-shots
    // blend to it only at their entry and recovery boundaries.
    inline std::map<std::string, Matrix4x4> neutralSecondary(const RigStructure& rig,
        const std::map<std::string, size_t>& idx, const std::map<std::string, Matrix4x4>& rest)
    {
        using namespace animation;
        auto drape = rest;
        HairChainSimulator hair;
        CapeGridSimulator cape;
        if (idx.count("HairBack1") && rest.count("Head"))
            hair.initialize(rig, idx, { "HairBack1", "HairBack2", "HairBack3" }, rest.at("Head"), 0.22, 0.97, 0.4);
        if (idx.count("CenterCape1") && rest.count("Chest"))
            cape.initialize(rig, idx, rest.at("Chest"), 0.08, 0.85, 1.2, 0.15);
        for (int i = 0; i < 120; ++i) {
            if (hair.active)
                hair.step(rest.at("Head"), 1.0 / 60.0, drape);
            if (cape.active)
                cape.step(rest.at("Chest"), 1.0 / 60.0, drape);
        }
        for (const auto& bone : rig.bones) {
            if (bone.name.find("HairBack") != 0 && bone.name.find("Cape") == std::string::npos)
                continue;
            Vector3 from = rest.at(bone.name).transformPoint(Vector3());
            Vector3 fromDir = rest.at(bone.name).transformVector(Vector3(0, 0, 1));
            Vector3 to = drape.at(bone.name).transformPoint(Vector3());
            Vector3 toDir = drape.at(bone.name).transformVector(Vector3(0, 0, 1));
            drape[bone.name] = boneFromRest(rest.at(bone.name), from, from + fromDir, to, to + toDir);
        }
        return drape;
    }

    inline void applySecondaryNeutral(const RigStructure& rig,
        const std::map<std::string, Matrix4x4>& rest, const std::map<std::string, Matrix4x4>& drape,
        BoneAnimationFrame& frame, double boundaryWeight, std::map<std::string, Quaternion>* blendHistory = nullptr)
    {
        using namespace animation;
        auto& world = frame.boneWorldTransforms;
        for (const auto& bone : rig.bones) {
            bool hair = bone.name.find("HairBack") == 0;
            if (!hair && bone.name.find("Cape") == std::string::npos)
                continue;
            const std::string parent = hair ? "Head" : "Chest";
            if (!world.count(parent) || !drape.count(bone.name))
                continue;
            Matrix4x4 target = world.at(parent);
            target *= rest.at(parent).inverted();
            target *= drape.at(bone.name);
            auto posed = world.find(bone.name);
            if (posed == world.end()) {
                world[bone.name] = target;
                continue;
            }
            Matrix4x4 relative = target;
            relative *= posed->second.inverted();
            Quaternion swing = poseOrientation(relative);
            if (blendHistory) {
                auto& previous = (*blendHistory)[bone.name];
                double dot = swing.w() * previous.w() + swing.x() * previous.x()
                    + swing.y() * previous.y() + swing.z() * previous.z();
                if (dot < 0.0 || (dot == 0.0 && swing.w() < 0.0))
                    swing *= -1.0;
                previous = swing;
            }
            if (boundaryWeight <= 0.0)
                continue;
            Vector3 start = posed->second.transformPoint(Vector3());
            Vector3 targetStart = target.transformPoint(Vector3());
            Matrix4x4 blended;
            blended.translate(start + (targetStart - start) * boundaryWeight);
            // Preserve the relative rotation branch as cloth folds past 180 degrees.
            // Independent shortest-path blends can otherwise flip its flat surface.
            Vector3 axis(swing.x(), swing.y(), swing.z());
            if (axis.lengthSquared() > 1e-18)
                blended.rotate(axis.normalized(), 2.0 * std::acos(std::clamp(swing.w(), -1.0, 1.0)) * boundaryWeight);
            blended.rotate(poseOrientation(posed->second));
            posed->second = boundaryWeight >= 1.0 ? target : blended;
        }
    }

    // Legacy actions retain explicit gestures. Only arms near their parent-relative bind
    // direction receive the shared neutral, fading smoothly as the authored gesture grows.
    inline void relaxUndirectedArms(const RigStructure& rig, const AnimationParams& params, BoneAnimationFrame& frame)
    {
        using namespace animation;
        auto idx = buildBoneIndexMap(rig);
        auto rest = restBoneWorldTransforms(rig);
        auto& world = frame.boneWorldTransforms;
        for (const std::string prefix : { "Left", "Right" }) {
            std::string upper = prefix + "UpperArm", shoulder = prefix + "Shoulder";
            if (!world.count(upper) || !world.count(shoulder))
                continue;
            Matrix4x4 parent = composePose(world.at(shoulder), rest.at(shoulder).inverted());
            Vector3 restDir = getBoneEnd(rig, idx, upper) - getBonePos(rig, idx, upper);
            Matrix4x4 localPose = composePose(composePose(parent.inverted(), world.at(upper)), rest.at(upper).inverted());
            double angle = Vector3::angle(restDir, localPose.transformVector(restDir));
            double weight = 1.0 - easePose(angle / 0.9);
            AnimationParams weighted = params;
            weighted.setValue("relaxedArmsFactor", weight * std::clamp(params.getValue("relaxedArmsFactor", 1.0), 0.0, 1.0));
            Matrix4x4 correction = world.at(upper);
            correction *= rest.at(upper).inverted();
            correction *= relaxedArm(rig, idx, upper, weighted);
            correction *= rest.at(upper);
            correction *= world.at(upper).inverted();
            for (const auto& bone : rig.bones) {
                std::string ancestor = bone.name;
                for (size_t depth = 0; depth < rig.bones.size() && idx.count(ancestor); ++depth) {
                    if (ancestor == upper) {
                        if (world.count(bone.name))
                            world[bone.name] = composePose(correction, world.at(bone.name));
                        break;
                    }
                    ancestor = rig.bones[idx.at(ancestor)].parent;
                }
            }
            std::string lower = prefix + "LowerArm";
            if (!world.count(lower))
                continue;
            Matrix4x4 uNeutral = relaxedArm(rig, idx, upper, weighted);
            Matrix4x4 lNeutral = relaxedForearm(rig, idx, prefix, weighted);
            Matrix4x4 bend = world.at(upper);
            bend *= rest.at(upper).inverted();
            bend *= uNeutral.inverted();
            bend *= lNeutral;
            bend *= rest.at(upper);
            bend *= world.at(upper).inverted();
            for (const auto& bone : rig.bones) {
                std::string ancestor = bone.name;
                for (size_t depth = 0; depth < rig.bones.size() && idx.count(ancestor); ++depth) {
                    if (ancestor == lower) {
                        if (world.count(bone.name))
                            world[bone.name] = composePose(bend, world.at(bone.name));
                        break;
                    }
                    ancestor = rig.bones[idx.at(ancestor)].parent;
                }
            }
        }
    }

} // namespace biped
} // namespace dust3d
#endif
