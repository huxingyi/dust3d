/*
 *  Copyright (c) 2026 Jeremy HU <jeremy-at-dust3d dot org>. All rights reserved. 
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:

 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.

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
#include <dust3d/base/debug.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/part_target.h>
#include <dust3d/base/position_key.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/string.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>
#include <limits>

namespace dust3d {

static bool targetPartIsModel(const Snapshot* snapshot, const std::string& partIdString)
{
    if (!snapshot || partIdString.empty())
        return false;

    auto partIt = snapshot->parts.find(partIdString);
    if (partIt == snapshot->parts.end())
        return false;

    auto target = PartTargetFromString(String::valueOrEmpty(partIt->second, "target").c_str());

    return PartTarget::Model == target || PartTarget::StitchingLine == target || PartTarget::StitchingLoop == target || PartTarget::ImportedModel == target;
}

static bool nodeBelongsToModelPart(const Snapshot* snapshot, const Uuid& nodeId)
{
    if (!snapshot)
        return false;

    auto it = snapshot->nodes.find(nodeId.toString());
    if (it == snapshot->nodes.end())
        return false;

    return targetPartIsModel(snapshot, String::valueOrEmpty(it->second, "partId"));
}

static bool edgeBelongsToModelPart(const Snapshot* snapshot,
    const std::map<std::string, std::string>& edgeAttributes)
{
    if (!snapshot)
        return false;

    std::string fromNode = String::valueOrEmpty(edgeAttributes, "from");
    std::string toNode = String::valueOrEmpty(edgeAttributes, "to");
    if (fromNode.empty() || toNode.empty())
        return false;

    Uuid fromId(fromNode);
    Uuid toId(toNode);
    return nodeBelongsToModelPart(snapshot, fromId)
        && nodeBelongsToModelPart(snapshot, toId);
}

static bool boneUsesParentEndAsReference(const std::string& boneName)
{
    return boneName.find("Left") != std::string::npos
        || boneName.find("Right") != std::string::npos;
}

RigGenerator::RigGenerator()
{
}

RigGenerator::~RigGenerator()
{
}

bool RigGenerator::generateRig(const Snapshot* snapshot, const RigStructure& templateRig, RigStructure& actualRig)
{
    if (!snapshot) {
        m_errorMessage = "Snapshot not initialized";
        return false;
    }

    actualRig = templateRig; // Copy template structure

    // Extract coordinate transformation offsets from snapshot's canvas
    m_mainProfileMiddleX = String::toFloat(String::valueOrEmpty(snapshot->canvas, "originX"));
    m_mainProfileMiddleY = String::toFloat(String::valueOrEmpty(snapshot->canvas, "originY"));
    m_sideProfileMiddleX = String::toFloat(String::valueOrEmpty(snapshot->canvas, "originZ"));

    // Clear template positions - they are only for template visualization
    for (auto& bone : actualRig.bones) {
        bone.posX = 0.0f;
        bone.posY = 0.0f;
        bone.posZ = 0.0f;
        bone.endX = 0.0f;
        bone.endY = 0.0f;
        bone.endZ = 0.0f;
    }

    // Build bone name -> index map for parent lookups
    std::map<std::string, size_t> boneNameToIndex;
    for (size_t i = 0; i < actualRig.bones.size(); ++i) {
        boneNameToIndex[actualRig.bones[i].name] = i;
    }

    // Pre-collect edge connectivity info in a single pass over edges:
    // - allEdgeNodes: nodes that appear in any bone-assigned edge
    // - boneEdgeNodesMap: bone name -> set of nodes in edges assigned to that bone
    // - nodesWithAnyEdge: nodes that appear in ANY model edge (even without bone assignment)
    std::set<Uuid> allEdgeNodes;
    std::map<std::string, std::set<Uuid>> boneEdgeNodesMap;
    std::set<Uuid> nodesWithAnyEdge;
    for (const auto& edgePair : snapshot->edges) {
        const auto& edgeAttributes = edgePair.second;
        if (!edgeBelongsToModelPart(snapshot, edgeAttributes))
            continue;
        std::string fromNode = String::valueOrEmpty(edgeAttributes, "from");
        std::string toNode = String::valueOrEmpty(edgeAttributes, "to");
        if (!fromNode.empty())
            nodesWithAnyEdge.insert(Uuid(fromNode));
        if (!toNode.empty())
            nodesWithAnyEdge.insert(Uuid(toNode));
        std::string edgeBoneName = String::valueOrEmpty(edgeAttributes, "boneName");
        if (edgeBoneName.empty())
            continue;
        if (!fromNode.empty()) {
            Uuid id(fromNode);
            allEdgeNodes.insert(id);
            boneEdgeNodesMap[edgeBoneName].insert(id);
        }
        if (!toNode.empty()) {
            Uuid id(toNode);
            allEdgeNodes.insert(id);
            boneEdgeNodesMap[edgeBoneName].insert(id);
        }
    }

    // Clear single node bone map before processing bones
    m_singleNodeBoneMap.clear();

    // Process bones in topological order (parents before children)
    std::vector<size_t> processingOrder;
    std::set<std::string> processed;
    while (processingOrder.size() < actualRig.bones.size()) {
        bool progress = false;
        for (size_t i = 0; i < actualRig.bones.size(); ++i) {
            const auto& bone = actualRig.bones[i];
            if (processed.count(bone.name))
                continue;
            if (bone.parent.empty() || processed.count(bone.parent)) {
                processingOrder.push_back(i);
                processed.insert(bone.name);
                progress = true;
            }
        }
        if (!progress) {
            // Circular or missing parent - add remaining in order
            for (size_t i = 0; i < actualRig.bones.size(); ++i) {
                if (!processed.count(actualRig.bones[i].name)) {
                    processingOrder.push_back(i);
                    processed.insert(actualRig.bones[i].name);
                }
            }
            break;
        }
    }

    // For each bone, compute actual position from edge assignments
    for (size_t boneIdx : processingOrder) {
        auto& bone = actualRig.bones[boneIdx];

        std::vector<std::vector<Uuid>> nodeChains;
        if (!extractNodeChainsForBone(snapshot, bone.name, nodeChains)) {
            dust3dDebug << "No edges assigned to bone:" << bone.name;
            if (bone.parent.empty()) {
                // Root bone with no bindings: give it a tiny upward tail so it has
                // a valid, non-degenerate orientation for exporters and game engines.
                bone.endY = -0.25f;
            }
            continue;
        }

        // Attach truly isolated nodes (no edges at all) to this bone
        // if they are nearest to this bone's edge-connected nodes.
        if (!boneEdgeNodesMap[bone.name].empty()) {
            attachSingleNodesToBone(snapshot, bone.name,
                boneEdgeNodesMap[bone.name], allEdgeNodes, nodesWithAnyEdge, nodeChains);
        }

        // Determine reference point from parent bone position.
        // Leg, limb and arm bones should align against the parent's end
        // position so the chain keeps its anatomical direction.
        // Root bones (no parent) use the origin.
        float refX = 0, refY = 0, refZ = 0;
        if (!bone.parent.empty()) {
            auto parentIt = boneNameToIndex.find(bone.parent);
            if (parentIt != boneNameToIndex.end()) {
                const auto& parentBone = actualRig.bones[parentIt->second];
                if (boneUsesParentEndAsReference(bone.name)) {
                    refX = parentBone.endX;
                    refY = parentBone.endY;
                    refZ = parentBone.endZ;
                } else {
                    refX = parentBone.posX;
                    refY = parentBone.posY;
                    refZ = parentBone.posZ;
                }
            }
        }

        // Orient each chain so its end closest to the reference comes first
        for (auto& chain : nodeChains) {
            orientChainTowardPoint(snapshot, chain, refX, refY, refZ);
        }

        // Sort chains by distance of their front node to the reference
        std::sort(nodeChains.begin(), nodeChains.end(),
            [&](const std::vector<Uuid>& a, const std::vector<Uuid>& b) {
                float ax = 0, ay = 0, az = 0, bx = 0, by = 0, bz = 0;
                getNodePosition(snapshot, a.front(), ax, ay, az);
                getNodePosition(snapshot, b.front(), bx, by, bz);
                float da = (ax - refX) * (ax - refX) + (ay - refY) * (ay - refY) + (az - refZ) * (az - refZ);
                float db = (bx - refX) * (bx - refX) + (by - refY) * (by - refY) + (bz - refZ) * (bz - refZ);
                return da < db;
            });

        // Compute the central bone axis across all chains:
        //   1. Average all chain-front positions -> approximate bone root
        //   2. Average all chain-back  positions -> approximate bone tip
        //   3. Build a unit direction from root to tip
        //   4. Project every chain endpoint onto that axis to find the true extents
        // This handles multiple parallel chains (e.g. cross-section rings of a limb)
        // by producing a single representative centre line rather than picking one
        // arbitrary chain.
        float sumBeginX = 0, sumBeginY = 0, sumBeginZ = 0;
        float sumEndX = 0, sumEndY = 0, sumEndZ = 0;
        size_t validChains = 0;

        for (const auto& chain : nodeChains) {
            float fx, fy, fz, bx, by, bz;
            if (getNodePosition(snapshot, chain.front(), fx, fy, fz) && getNodePosition(snapshot, chain.back(), bx, by, bz)) {
                sumBeginX += fx;
                sumBeginY += fy;
                sumBeginZ += fz;
                sumEndX += bx;
                sumEndY += by;
                sumEndZ += bz;
                ++validChains;
            }
        }

        if (validChains > 0) {
            float avgBeginX = sumBeginX / validChains;
            float avgBeginY = sumBeginY / validChains;
            float avgBeginZ = sumBeginZ / validChains;
            float avgEndX = sumEndX / validChains;
            float avgEndY = sumEndY / validChains;
            float avgEndZ = sumEndZ / validChains;

            float dx = avgEndX - avgBeginX;
            float dy = avgEndY - avgBeginY;
            float dz = avgEndZ - avgBeginZ;
            float axisLen = std::sqrt(dx * dx + dy * dy + dz * dz);

            if (axisLen > 1e-6f) {
                dx /= axisLen;
                dy /= axisLen;
                dz /= axisLen;

                // Project all chain endpoints onto the central axis and track extents
                float minT = 0.0f, maxT = 0.0f;
                bool firstProjection = true;
                for (const auto& chain : nodeChains) {
                    float px, py, pz;
                    for (const Uuid& endNode : { chain.front(), chain.back() }) {
                        if (!getNodePosition(snapshot, endNode, px, py, pz))
                            continue;
                        float t = (px - avgBeginX) * dx + (py - avgBeginY) * dy + (pz - avgBeginZ) * dz;
                        if (firstProjection) {
                            minT = maxT = t;
                            firstProjection = false;
                        } else {
                            minT = std::min(minT, t);
                            maxT = std::max(maxT, t);
                        }
                    }
                }

                bone.posX = avgBeginX + minT * dx;
                bone.posY = avgBeginY + minT * dy;
                bone.posZ = avgBeginZ + minT * dz;
                bone.endX = avgBeginX + maxT * dx;
                bone.endY = avgBeginY + maxT * dy;
                bone.endZ = avgBeginZ + maxT * dz;
            } else {
                bone.posX = avgBeginX;
                bone.posY = avgBeginY;
                bone.posZ = avgBeginZ;
                bone.endX = avgEndX;
                bone.endY = avgEndY;
                bone.endZ = avgEndZ;
            }
        }

        size_t totalNodes = 0;
        for (const auto& chain : nodeChains)
            totalNodes += chain.size();

        dust3dDebug << "Computed bone" << bone.name.c_str()
                    << "from" << totalNodes << "nodes in" << nodeChains.size() << "chains"
                    << "position:" << bone.posX << bone.posY << bone.posZ
                    << "endPosition:" << bone.endX << bone.endY << bone.endZ;
    }

    // Fish rig patch: ensure "Head" bone direction aligns with "BodyFront"
    if (actualRig.type == "Fish") {
        RigNode* headBone = nullptr;
        RigNode* bodyFrontBone = nullptr;
        for (auto& bone : actualRig.bones) {
            if (bone.name == "Head")
                headBone = &bone;
            else if (bone.name == "BodyFront")
                bodyFrontBone = &bone;
        }
        if (headBone && bodyFrontBone) {
            Vector3 headDir(headBone->endX - headBone->posX,
                headBone->endY - headBone->posY,
                headBone->endZ - headBone->posZ);
            Vector3 bodyFrontDir(bodyFrontBone->endX - bodyFrontBone->posX,
                bodyFrontBone->endY - bodyFrontBone->posY,
                bodyFrontBone->endZ - bodyFrontBone->posZ);
            float dot = Vector3::dotProduct(headDir, bodyFrontDir);
            if (dot < 0.0f) {
                // Head is pointing away from BodyFront — reverse it
                std::swap(headBone->posX, headBone->endX);
                std::swap(headBone->posY, headBone->endY);
                std::swap(headBone->posZ, headBone->endZ);
                dust3dDebug << "Fish rig patch: reversed Head bone direction to align with BodyFront";
            }
        }
    }

    if (actualRig.type == "Biped") {
        RigNode* hipsBone = nullptr;
        RigNode* spineBone = nullptr;
        for (auto& bone : actualRig.bones) {
            if (bone.name == "Hips")
                hipsBone = &bone;
            else if (bone.name == "Spine")
                spineBone = &bone;
        }
        if (hipsBone && spineBone) {
            Vector3 hipsDir(hipsBone->endX - hipsBone->posX,
                hipsBone->endY - hipsBone->posY,
                hipsBone->endZ - hipsBone->posZ);
            Vector3 spineDir(spineBone->endX - spineBone->posX,
                spineBone->endY - spineBone->posY,
                spineBone->endZ - spineBone->posZ);
            if (!hipsDir.isZero() && !spineDir.isZero()) {
                float dot = Vector3::dotProduct(hipsDir, spineDir);
                if (dot < 0.0f) {
                    std::swap(hipsBone->posX, hipsBone->endX);
                    std::swap(hipsBone->posY, hipsBone->endY);
                    std::swap(hipsBone->posZ, hipsBone->endZ);
                    dust3dDebug << "Biped rig patch: reversed Hips bone direction to align with Spine";
                }
            }
        }
    }

    // Biped rig patch: ensure Chest and Head are oriented correctly along the
    // spine chain (Hips → Spine → Chest → Neck → Head).  After the generic
    // bone-fitting step the computed pos/end order for Chest or Head may be
    // reversed relative to the upward chain direction.  We fix that by
    // checking the dot product of each bone's direction against the preceding
    // chain link's direction and flipping when negative.
    if (actualRig.type == "Biped") {
        RigNode* spineBone = nullptr;
        RigNode* chestBone = nullptr;
        RigNode* neckBone = nullptr;
        RigNode* headBone = nullptr;
        for (auto& bone : actualRig.bones) {
            if (bone.name == "Spine")
                spineBone = &bone;
            else if (bone.name == "Chest")
                chestBone = &bone;
            else if (bone.name == "Neck")
                neckBone = &bone;
            else if (bone.name == "Head")
                headBone = &bone;
        }
        // Align Chest in the spine chain: begin = Spine's end, end = Neck's begin
        if (chestBone) {
            if (spineBone) {
                chestBone->posX = spineBone->endX;
                chestBone->posY = spineBone->endY;
                chestBone->posZ = spineBone->endZ;
                dust3dDebug << "Biped rig patch: Chest bone begin set to Spine end";
            }
            if (neckBone) {
                chestBone->endX = neckBone->posX;
                chestBone->endY = neckBone->posY;
                chestBone->endZ = neckBone->posZ;
                dust3dDebug << "Biped rig patch: Chest bone end set to Neck begin";
            }
        }
        // Align Head in the spine chain: begin = Neck's end (if Neck exists),
        // then flip direction if it points away from the chain.
        if (headBone) {
            if (neckBone) {
                headBone->posX = neckBone->endX;
                headBone->posY = neckBone->endY;
                headBone->posZ = neckBone->endZ;
                dust3dDebug << "Biped rig patch: Head bone begin set to Neck end";
            }
            // Ensure Head end is not behind its begin relative to the neck/chest direction
            RigNode* refBone = neckBone ? neckBone : chestBone;
            if (refBone) {
                Vector3 refDir(refBone->endX - refBone->posX,
                    refBone->endY - refBone->posY,
                    refBone->endZ - refBone->posZ);
                Vector3 headDir(headBone->endX - headBone->posX,
                    headBone->endY - headBone->posY,
                    headBone->endZ - headBone->posZ);
                if (!refDir.isZero() && !headDir.isZero()) {
                    if (Vector3::dotProduct(refDir, headDir) < 0.0f) {
                        std::swap(headBone->posX, headBone->endX);
                        std::swap(headBone->posY, headBone->endY);
                        std::swap(headBone->posZ, headBone->endZ);
                        dust3dDebug << "Biped rig patch: reversed Head bone direction to align with spine chain";
                    }
                }
            }
        }
    }

    // Biped rig patch: ensure LeftUpperLeg and RightUpperLeg point in the same
    // direction as their corresponding LowerLeg bones. If the dot product of
    // UpperLeg and LowerLeg directions is negative, reverse the UpperLeg bone.
    if (actualRig.type == "Biped") {
        static const std::pair<const char*, const char*> legPairs[] = {
            { "LeftUpperLeg", "LeftLowerLeg" },
            { "RightUpperLeg", "RightLowerLeg" },
        };
        for (const auto& pair : legPairs) {
            RigNode* upperLegBone = nullptr;
            RigNode* lowerLegBone = nullptr;
            for (auto& bone : actualRig.bones) {
                if (bone.name == pair.first)
                    upperLegBone = &bone;
                else if (bone.name == pair.second)
                    lowerLegBone = &bone;
            }
            if (upperLegBone && lowerLegBone) {
                Vector3 upperDir(upperLegBone->endX - upperLegBone->posX,
                    upperLegBone->endY - upperLegBone->posY,
                    upperLegBone->endZ - upperLegBone->posZ);
                Vector3 lowerDir(lowerLegBone->endX - lowerLegBone->posX,
                    lowerLegBone->endY - lowerLegBone->posY,
                    lowerLegBone->endZ - lowerLegBone->posZ);
                if (!upperDir.isZero() && !lowerDir.isZero()) {
                    if (Vector3::dotProduct(upperDir, lowerDir) < 0.0f) {
                        std::swap(upperLegBone->posX, upperLegBone->endX);
                        std::swap(upperLegBone->posY, upperLegBone->endY);
                        std::swap(upperLegBone->posZ, upperLegBone->endZ);
                        dust3dDebug << "Biped rig patch: reversed" << pair.first << "direction to align with" << pair.second;
                    }
                }
            }
        }
    }

    // Quadruped rig patch: ensure "Head" bone starts at Neck's end and
    // points in the forward direction derived from the spine chain
    // (Spine → Chest → Neck), not relying on any fixed axis assumption.
    if (actualRig.type == "Quadruped") {
        RigNode* headBone = nullptr;
        RigNode* neckBone = nullptr;
        RigNode* chestBone = nullptr;
        RigNode* spineBone = nullptr;
        for (auto& bone : actualRig.bones) {
            if (bone.name == "Head")
                headBone = &bone;
            else if (bone.name == "Neck")
                neckBone = &bone;
            else if (bone.name == "Chest")
                chestBone = &bone;
            else if (bone.name == "Spine")
                spineBone = &bone;
        }
        if (headBone && neckBone && chestBone && spineBone) {
            // Determine forward direction from the spine chain:
            // average of Spine→Chest and Chest→Neck directions
            Vector3 spineDir(chestBone->posX - spineBone->posX,
                chestBone->posY - spineBone->posY,
                chestBone->posZ - spineBone->posZ);
            Vector3 chestDir(neckBone->posX - chestBone->posX,
                neckBone->posY - chestBone->posY,
                neckBone->posZ - chestBone->posZ);
            Vector3 forwardDir = spineDir + chestDir;
            if (forwardDir.lengthSquared() < 1e-8f)
                forwardDir = spineDir;
            if (forwardDir.lengthSquared() > 1e-8f) {
                forwardDir.normalize();

                // Head bone must start at Neck's end position
                Vector3 neckEnd(neckBone->endX, neckBone->endY, neckBone->endZ);

                // Head bone's end should be the original endpoint that is
                // farthest in the forward direction from the neck end
                Vector3 origPos(headBone->posX, headBone->posY, headBone->posZ);
                Vector3 origEnd(headBone->endX, headBone->endY, headBone->endZ);
                float dotPos = Vector3::dotProduct(origPos - neckEnd, forwardDir);
                float dotEnd = Vector3::dotProduct(origEnd - neckEnd, forwardDir);

                // Pick the endpoint that is most forward as the head tip
                Vector3 headTip = (dotEnd >= dotPos) ? origEnd : origPos;

                // If both endpoints are behind the neck end, project the
                // head length along the forward direction instead
                float headLength = (origEnd - origPos).length();
                if (headLength < 1e-6f)
                    headLength = (neckEnd - Vector3(neckBone->posX, neckBone->posY, neckBone->posZ)).length() * 0.5f;
                float tipForwardDist = Vector3::dotProduct(headTip - neckEnd, forwardDir);
                if (tipForwardDist < headLength * 0.1f) {
                    headTip = neckEnd + forwardDir * headLength;
                }

                headBone->posX = neckEnd.x();
                headBone->posY = neckEnd.y();
                headBone->posZ = neckEnd.z();
                headBone->endX = headTip.x();
                headBone->endY = headTip.y();
                headBone->endZ = headTip.z();

                dust3dDebug << "Quadruped rig patch: Head bone repositioned to start at Neck end,"
                            << "pointing forward along spine direction";
            }
        }
    }

    // Quadruped rig patch: ensure each Foot bone's start position matches
    // the corresponding LowerLeg bone's end position so the limb chain is
    // continuous (FrontLeft/FrontRight/BackLeft/BackRight).
    if (actualRig.type == "Quadruped") {
        static const std::pair<const char*, const char*> footChains[] = {
            { "FrontLeftLowerLeg", "FrontLeftFoot" },
            { "FrontRightLowerLeg", "FrontRightFoot" },
            { "BackLeftLowerLeg", "BackLeftFoot" },
            { "BackRightLowerLeg", "BackRightFoot" },
        };
        for (const auto& pair : footChains) {
            RigNode* lowerLegBone = nullptr;
            RigNode* footBone = nullptr;
            for (auto& bone : actualRig.bones) {
                if (bone.name == pair.first)
                    lowerLegBone = &bone;
                else if (bone.name == pair.second)
                    footBone = &bone;
            }
            if (lowerLegBone && footBone) {
                footBone->posX = lowerLegBone->endX;
                footBone->posY = lowerLegBone->endY;
                footBone->posZ = lowerLegBone->endZ;
                dust3dDebug << "Quadruped rig patch:" << pair.second << "start set to" << pair.first << "end";
            }
        }
    }

    // Bird rig patch: align Pelvis→Spine direction (analogous to Biped Hips→Spine).
    if (actualRig.type == "Bird") {
        RigNode* pelvisBone = nullptr;
        RigNode* spineBone = nullptr;
        for (auto& bone : actualRig.bones) {
            if (bone.name == "Pelvis")
                pelvisBone = &bone;
            else if (bone.name == "Spine")
                spineBone = &bone;
        }
        if (pelvisBone && spineBone) {
            Vector3 pelvisDir(pelvisBone->endX - pelvisBone->posX,
                pelvisBone->endY - pelvisBone->posY,
                pelvisBone->endZ - pelvisBone->posZ);
            Vector3 spineDir(spineBone->endX - spineBone->posX,
                spineBone->endY - spineBone->posY,
                spineBone->endZ - spineBone->posZ);
            if (!pelvisDir.isZero() && !spineDir.isZero()) {
                if (Vector3::dotProduct(pelvisDir, spineDir) < 0.0f) {
                    std::swap(pelvisBone->posX, pelvisBone->endX);
                    std::swap(pelvisBone->posY, pelvisBone->endY);
                    std::swap(pelvisBone->posZ, pelvisBone->endZ);
                    dust3dDebug << "Bird rig patch: reversed Pelvis bone direction to align with Spine";
                }
            }
        }
    }

    // Bird rig patch: snap the spine chain (Spine→Chest→Neck→Head) so each
    // bone begins at the previous bone's end, and flip Head if it points away
    // from the chain — mirrors the Biped spine chain alignment.
    if (actualRig.type == "Bird") {
        RigNode* spineBone = nullptr;
        RigNode* chestBone = nullptr;
        RigNode* neckBone = nullptr;
        RigNode* headBone = nullptr;
        for (auto& bone : actualRig.bones) {
            if (bone.name == "Spine")
                spineBone = &bone;
            else if (bone.name == "Chest")
                chestBone = &bone;
            else if (bone.name == "Neck")
                neckBone = &bone;
            else if (bone.name == "Head")
                headBone = &bone;
        }
        if (chestBone) {
            if (spineBone) {
                chestBone->posX = spineBone->endX;
                chestBone->posY = spineBone->endY;
                chestBone->posZ = spineBone->endZ;
                dust3dDebug << "Bird rig patch: Chest bone begin set to Spine end";
            }
            if (neckBone) {
                chestBone->endX = neckBone->posX;
                chestBone->endY = neckBone->posY;
                chestBone->endZ = neckBone->posZ;
                dust3dDebug << "Bird rig patch: Chest bone end set to Neck begin";
            }
        }
        if (headBone) {
            if (neckBone) {
                headBone->posX = neckBone->endX;
                headBone->posY = neckBone->endY;
                headBone->posZ = neckBone->endZ;
                dust3dDebug << "Bird rig patch: Head bone begin set to Neck end";
            }
            RigNode* refBone = neckBone ? neckBone : chestBone;
            if (refBone) {
                Vector3 refDir(refBone->endX - refBone->posX,
                    refBone->endY - refBone->posY,
                    refBone->endZ - refBone->posZ);
                Vector3 headDir(headBone->endX - headBone->posX,
                    headBone->endY - headBone->posY,
                    headBone->endZ - headBone->posZ);
                if (!refDir.isZero() && !headDir.isZero()) {
                    if (Vector3::dotProduct(refDir, headDir) < 0.0f) {
                        std::swap(headBone->posX, headBone->endX);
                        std::swap(headBone->posY, headBone->endY);
                        std::swap(headBone->posZ, headBone->endZ);
                        dust3dDebug << "Bird rig patch: reversed Head bone direction to align with spine chain";
                    }
                }
            }
        }
    }

    // Bird rig patch: ensure UpperLeg and LowerLeg point in the same direction
    // (same bone names as Biped, so the same fix applies).
    if (actualRig.type == "Bird") {
        static const std::pair<const char*, const char*> legPairs[] = {
            { "LeftUpperLeg", "LeftLowerLeg" },
            { "RightUpperLeg", "RightLowerLeg" },
        };
        for (const auto& pair : legPairs) {
            RigNode* upperLegBone = nullptr;
            RigNode* lowerLegBone = nullptr;
            for (auto& bone : actualRig.bones) {
                if (bone.name == pair.first)
                    upperLegBone = &bone;
                else if (bone.name == pair.second)
                    lowerLegBone = &bone;
            }
            if (upperLegBone && lowerLegBone) {
                Vector3 upperDir(upperLegBone->endX - upperLegBone->posX,
                    upperLegBone->endY - upperLegBone->posY,
                    upperLegBone->endZ - upperLegBone->posZ);
                Vector3 lowerDir(lowerLegBone->endX - lowerLegBone->posX,
                    lowerLegBone->endY - lowerLegBone->posY,
                    lowerLegBone->endZ - lowerLegBone->posZ);
                if (!upperDir.isZero() && !lowerDir.isZero()) {
                    if (Vector3::dotProduct(upperDir, lowerDir) < 0.0f) {
                        std::swap(upperLegBone->posX, upperLegBone->endX);
                        std::swap(upperLegBone->posY, upperLegBone->endY);
                        std::swap(upperLegBone->posZ, upperLegBone->endZ);
                        dust3dDebug << "Bird rig patch: reversed" << pair.first << "direction to align with" << pair.second;
                    }
                }
            }
        }
    }

    // Bird rig patch: snap Foot bone starts to their LowerLeg bone ends, and
    // align wing limb chains (WingElbow, WingHand) in the same direction as
    // the preceding wing segment.
    if (actualRig.type == "Bird") {
        // Foot chaining
        static const std::pair<const char*, const char*> footChains[] = {
            { "LeftLowerLeg", "LeftFoot" },
            { "RightLowerLeg", "RightFoot" },
        };
        for (const auto& pair : footChains) {
            RigNode* lowerLegBone = nullptr;
            RigNode* footBone = nullptr;
            for (auto& bone : actualRig.bones) {
                if (bone.name == pair.first)
                    lowerLegBone = &bone;
                else if (bone.name == pair.second)
                    footBone = &bone;
            }
            if (lowerLegBone && footBone) {
                footBone->posX = lowerLegBone->endX;
                footBone->posY = lowerLegBone->endY;
                footBone->posZ = lowerLegBone->endZ;
                dust3dDebug << "Bird rig patch:" << pair.second << "start set to" << pair.first << "end";
            }
        }
    }

    // Jaw rig patch: ensure Jaw starts from the Head bone start and its end
    // is the farthest assigned node from that start point.
    {
        RigNode* headBone = nullptr;
        RigNode* jawBone = nullptr;
        for (auto& bone : actualRig.bones) {
            if (bone.name == "Head")
                headBone = &bone;
            else if (bone.name == "Jaw")
                jawBone = &bone;
        }
        if (headBone && jawBone) {
            std::vector<std::vector<Uuid>> jawChains;
            if (extractNodeChainsForBone(snapshot, jawBone->name, jawChains)) {
                float startX = headBone->posX;
                float startY = headBone->posY;
                float startZ = headBone->posZ;
                float maxDist2 = -1.0f;
                float farX = jawBone->endX;
                float farY = jawBone->endY;
                float farZ = jawBone->endZ;

                for (const auto& chain : jawChains) {
                    for (const auto& nodeId : chain) {
                        float nx = 0, ny = 0, nz = 0;
                        if (!getNodePosition(snapshot, nodeId, nx, ny, nz))
                            continue;
                        float dx = nx - startX;
                        float dy = ny - startY;
                        float dz = nz - startZ;
                        float dist2 = dx * dx + dy * dy + dz * dz;
                        if (dist2 > maxDist2) {
                            maxDist2 = dist2;
                            farX = nx;
                            farY = ny;
                            farZ = nz;
                        }
                    }
                }

                if (maxDist2 >= 0.0f) {
                    jawBone->posX = startX;
                    jawBone->posY = startY;
                    jawBone->posZ = startZ;
                    jawBone->endX = farX;
                    jawBone->endY = farY;
                    jawBone->endZ = farZ;
                    dust3dDebug << "Jaw rig patch: Jaw bone aligned to Head start with farthest jaw node end";
                }
            }
        }
    }

    // Generate collision capsule radius for each bone in rig data
    for (auto& bone : actualRig.bones) {
        Vector3 start(bone.posX, bone.posY, bone.posZ);
        Vector3 end(bone.endX, bone.endY, bone.endZ);
        float length = (end - start).length();
        if (length < 1e-6f) {
            bone.capsuleRadius = 0.01f;
            continue;
        }

        // Determine capsule radius from node radius average on the bone's node chains.
        std::vector<std::vector<Uuid>> nodeChains;
        float radiusSum = 0.0f;
        size_t radiusCount = 0;
        if (extractNodeChainsForBone(snapshot, bone.name, nodeChains)) {
            for (const auto& chain : nodeChains) {
                for (const auto& nodeId : chain) {
                    float nx = 0, ny = 0, nz = 0;
                    if (getNodePosition(snapshot, nodeId, nx, ny, nz)) {
                        auto it = snapshot->nodes.find(nodeId.toString());
                        if (it != snapshot->nodes.end()) {
                            float nodeRadius = String::toFloat(String::valueOrEmpty(it->second, "radius"));
                            if (nodeRadius > 1e-6f) {
                                radiusSum += nodeRadius;
                                ++radiusCount;
                            }
                        }
                    }
                }
            }
        }

        float radius = 0.0f;
        if (radiusCount > 0)
            radius = radiusSum / static_cast<float>(radiusCount);
        else
            radius = std::max(0.01f, length * 0.12f);

        bone.capsuleRadius = std::max(0.01f, radius);
    }

    m_errorMessage = "";
    return true;
}

bool RigGenerator::computeBoneWorldTransforms(const RigStructure& rigStructure,
    std::map<std::string, Matrix4x4>& boneWorldTransforms)
{
    boneWorldTransforms.clear();

    // Build bone name map for parent lookup and ensure order
    std::map<std::string, size_t> boneNameToIndex;
    for (size_t i = 0; i < rigStructure.bones.size(); ++i) {
        boneNameToIndex[rigStructure.bones[i].name] = i;
    }

    std::vector<size_t> processingOrder;
    std::set<std::string> processed;
    while (processingOrder.size() < rigStructure.bones.size()) {
        bool progress = false;
        for (size_t i = 0; i < rigStructure.bones.size(); ++i) {
            const auto& bone = rigStructure.bones[i];
            if (processed.count(bone.name))
                continue;
            if (bone.parent.empty() || processed.count(bone.parent)) {
                processingOrder.push_back(i);
                processed.insert(bone.name);
                progress = true;
            }
        }
        if (!progress) {
            for (size_t i = 0; i < rigStructure.bones.size(); ++i) {
                if (!processed.count(rigStructure.bones[i].name)) {
                    processingOrder.push_back(i);
                    processed.insert(rigStructure.bones[i].name);
                }
            }
            break;
        }
    }

    for (size_t boneIdx : processingOrder) {
        const auto& node = rigStructure.bones[boneIdx];

        Vector3 boneHead(node.posX, node.posY, node.posZ);
        Vector3 boneTail(node.endX, node.endY, node.endZ);
        Vector3 boneDirection = boneTail - boneHead;

        Matrix4x4 restTransform;
        restTransform.translate(boneHead);

        if (!boneDirection.isZero()) {
            Quaternion orient = Quaternion::rotationTo(Vector3(0.0, 0.0, 1.0), boneDirection.normalized());
            restTransform.rotate(orient);
        }

        if (!node.parent.empty()) {
            auto parentIt = boneNameToIndex.find(node.parent);
            if (parentIt != boneNameToIndex.end()) {
                auto worldIt = boneWorldTransforms.find(node.parent);
                if (worldIt != boneWorldTransforms.end()) {
                    const Matrix4x4& parentWorld = worldIt->second;
                    Matrix4x4 parentInverse = parentWorld.inverted();
                    Matrix4x4 localTransform = parentInverse;
                    localTransform *= restTransform;

                    Matrix4x4 finalWorld = parentWorld;
                    finalWorld *= localTransform;
                    boneWorldTransforms.emplace(node.name, finalWorld);
                    continue;
                }
            }
        }

        boneWorldTransforms.emplace(node.name, restTransform);
    }

    return true;
}

bool RigGenerator::computeBoneInverseBindMatrices(const RigStructure& rigStructure,
    std::map<std::string, Matrix4x4>& inverseBindMatrices)
{
    inverseBindMatrices.clear();

    std::map<std::string, Matrix4x4> boneWorldTransforms;
    if (!computeBoneWorldTransforms(rigStructure, boneWorldTransforms)) {
        m_errorMessage = "Failed to compute bone world transforms";
        return false;
    }

    for (const auto& pair : boneWorldTransforms) {
        inverseBindMatrices.emplace(pair.first, pair.second.inverted());
    }

    m_errorMessage = "";
    return true;
}

float RigGenerator::computeTwoBoneLerp(const RigStructure& rigStructure,
    const std::string& bone1, const std::string& bone2,
    const Snapshot* snapshot, const Uuid& nodeId)
{
    // Build parent lookup
    std::map<std::string, std::string> parentOf;
    for (const auto& bone : rigStructure.bones)
        parentOf[bone.name] = bone.parent;

    // Determine parent-child relationship between the two bones
    bool bone1IsParentOfBone2 = (parentOf.count(bone2) && parentOf[bone2] == bone1);
    bool bone2IsParentOfBone1 = (parentOf.count(bone1) && parentOf[bone1] == bone2);

    if (!bone1IsParentOfBone2 && !bone2IsParentOfBone1) {
        // Siblings or unrelated bones: equal weight
        return 0.5f;
    }

    // Parent-child: we want to favor the child bone at joints for better deformation.
    // The node at a joint (e.g. elbow, knee) should be weighted more toward the child
    // bone so the joint bends cleanly.
    //
    // Convention: lerp=0 means fully bone1, lerp=1 means fully bone2.
    // We compute based on node proximity to each bone's axis.

    const std::string& parentBone = bone1IsParentOfBone2 ? bone1 : bone2;
    const std::string& childBone = bone1IsParentOfBone2 ? bone2 : bone1;
    // childLerp: the lerp value that means "fully child bone"
    float childLerp = bone1IsParentOfBone2 ? 1.0f : 0.0f;

    // Determine joint bias based on bone semantics and rig type
    // Joints like elbows, knees, and articulation points benefit from
    // stronger child influence (0.6-0.75 toward child)
    float jointBias = 0.6f; // default: slight child preference

    const std::string& rigType = rigStructure.type;

    // Recognize limb joints that need stronger child bias for clean bending
    auto isLimbJoint = [](const std::string& child) -> bool {
        return child.find("LowerArm") != std::string::npos
            || child.find("LowerLeg") != std::string::npos
            || child.find("Hand") != std::string::npos
            || child.find("Foot") != std::string::npos
            || child.find("Femur") != std::string::npos
            || child.find("Tibia") != std::string::npos
            || child.find("Elbow") != std::string::npos;
    };

    auto isSpineJoint = [](const std::string& parent, const std::string& child) -> bool {
        return (parent.find("Spine") != std::string::npos || parent.find("Chest") != std::string::npos
                   || parent.find("Hips") != std::string::npos || parent.find("Pelvis") != std::string::npos)
            && (child.find("Spine") != std::string::npos || child.find("Chest") != std::string::npos
                || child.find("Neck") != std::string::npos);
    };

    auto isTailJoint = [](const std::string& child) -> bool {
        return child.find("Tail") != std::string::npos;
    };

    auto isHeadNeckJoint = [](const std::string& parent, const std::string& child) -> bool {
        return (parent.find("Neck") != std::string::npos && child.find("Head") != std::string::npos)
            || (parent.find("Head") != std::string::npos && (child.find("Jaw") != std::string::npos || child.find("Beak") != std::string::npos));
    };

    if (isLimbJoint(childBone)) {
        // Limb joints (elbows, knees): strong child bias for clean bending
        jointBias = 0.7f;
    } else if (isHeadNeckJoint(parentBone, childBone)) {
        // Head/neck: moderate child bias
        jointBias = 0.65f;
    } else if (isSpineJoint(parentBone, childBone)) {
        // Spine chain: gentler blend for organic deformation
        jointBias = 0.55f;
    } else if (isTailJoint(childBone)) {
        // Tail segments: gentle blend
        jointBias = 0.55f;
    }

    // Fish rigs: body segments should blend smoothly
    if (rigType == "Fish") {
        if (childBone.find("Body") != std::string::npos || childBone.find("Tail") != std::string::npos) {
            jointBias = 0.5f; // equal blend for smooth fish undulation
        } else if (childBone.find("Fin") != std::string::npos) {
            jointBias = 0.7f; // fins should follow their own bone
        }
    }

    // Snake: smooth sequential blending along spine
    if (rigType == "Snake") {
        jointBias = 0.5f;
    }

    // Return lerp: jointBias toward child bone
    // childLerp=1 means bone2 is child, so lerp = jointBias
    // childLerp=0 means bone1 is child, so lerp = 1 - jointBias
    return (childLerp > 0.5f) ? jointBias : (1.0f - jointBias);
}

bool RigGenerator::computeNodeBoneInfluences(const Snapshot* snapshot,
    const RigStructure& rigStructure,
    std::map<Uuid, NodeBoneInfluence>& nodeBoneInfluences)
{
    if (!snapshot) {
        m_errorMessage = "Snapshot not initialized";
        return false;
    }

    nodeBoneInfluences.clear();

    // Pre-build a map from node -> set of bone names from connected edges (single pass over edges)
    std::map<std::string, std::set<std::string>> nodeToBoneNames;
    for (const auto& edgePair : snapshot->edges) {
        const auto& edgeAttributes = edgePair.second;
        if (!edgeBelongsToModelPart(snapshot, edgeAttributes))
            continue;
        std::string boneName = String::valueOrEmpty(edgeAttributes, "boneName");
        if (boneName.empty())
            continue;
        std::string fromNode = String::valueOrEmpty(edgeAttributes, "from");
        std::string toNode = String::valueOrEmpty(edgeAttributes, "to");
        if (!fromNode.empty())
            nodeToBoneNames[fromNode].insert(boneName);
        if (!toNode.empty())
            nodeToBoneNames[toNode].insert(boneName);
    }

    // For each node in the snapshot, determine which bones influence it
    for (const auto& nodePair : snapshot->nodes) {
        const std::string& nodeIdString = nodePair.first;
        Uuid nodeId(nodeIdString);

        if (!nodeBelongsToModelPart(snapshot, nodeId))
            continue;

        const auto& boneNamesIt = nodeToBoneNames.find(nodeIdString);
        if (boneNamesIt == nodeToBoneNames.end() || boneNamesIt->second.empty()) {
            // No bone-assigned edges for this node.
            // Use m_singleNodeBoneMap (populated by attachSingleNodesToBone) for truly isolated nodes.
            auto singleIt = m_singleNodeBoneMap.find(nodeId);
            if (singleIt != m_singleNodeBoneMap.end()) {
                NodeBoneInfluence influence(singleIt->second);
                nodeBoneInfluences[nodeId] = influence;
                dust3dDebug << "Single node" << nodeIdString.c_str()
                            << "inherited bone from attachSingleNodesToBone:" << singleIt->second.c_str();
            }
            continue;
        }

        const std::set<std::string>& boneNames = boneNamesIt->second;

        if (boneNames.size() == 1) {
            // Single bone influence
            std::string boneName = *boneNames.begin();
            NodeBoneInfluence influence(boneName);
            nodeBoneInfluences[nodeId] = influence;
            dust3dDebug << "Node" << nodeIdString.c_str() << "bound to single bone:" << boneName.c_str();
        } else if (boneNames.size() == 2) {
            // Two bone influences - interpolate
            auto it = boneNames.begin();
            std::string bone1 = *it;
            std::string bone2 = *(++it);

            float lerp = computeTwoBoneLerp(rigStructure, bone1, bone2, snapshot, nodeId);
            NodeBoneInfluence influence(bone1, bone2, lerp);
            nodeBoneInfluences[nodeId] = influence;
            dust3dDebug << "Node" << nodeIdString.c_str()
                        << "bound to two bones:" << bone1.c_str() << "+" << bone2.c_str()
                        << "lerp:" << lerp;
        } else {
            // More than 2 bones: pick the two most prominent
            // For now, pick first two alphabetically
            auto it = boneNames.begin();
            std::string bone1 = *it;
            std::string bone2 = *(++it);
            float lerp = computeTwoBoneLerp(rigStructure, bone1, bone2, snapshot, nodeId);
            NodeBoneInfluence influence(bone1, bone2, lerp);
            nodeBoneInfluences[nodeId] = influence;
            dust3dDebug << "Node" << nodeIdString.c_str()
                        << "has" << boneNames.size() << "different bones, using:" << bone1.c_str() << bone2.c_str();
        }
    }

    m_errorMessage = "";
    return true;
}

bool RigGenerator::computeVertexBoneBindings(Object* object,
    const std::map<Uuid, NodeBoneInfluence>& nodeBoneInfluences)
{
    if (!object) {
        m_errorMessage = "Object not initialized";
        return false;
    }

    if (object->vertices.empty()) {
        m_errorMessage = "Object has no vertices";
        return false;
    }

    // Initialize vertex bone arrays parallel to vertices
    object->vertexBone1.resize(object->vertices.size());
    object->vertexBone2.resize(object->vertices.size());

    // For each vertex, trace back to its source node and apply bone influence
    for (size_t i = 0; i < object->vertices.size(); ++i) {
        const Vector3& vertexPos = object->vertices[i];
        PositionKey posKey(vertexPos);

        // Find source node for this vertex
        auto it = object->positionToNodeIdMap.find(posKey);
        if (it == object->positionToNodeIdMap.end()) {
            // Vertex has no source node mapping
            continue;
        }

        Uuid nodeId = it->second;

        // Look up bone influence for this node
        auto boneIt = nodeBoneInfluences.find(nodeId);
        if (boneIt == nodeBoneInfluences.end()) {
            // Node has no bone influence
            continue;
        }

        const NodeBoneInfluence& influence = boneIt->second;
        VertexBoneBinding binding = influence.toVertexBinding();

        // Store binding in parallel arrays
        object->vertexBone1[i] = { binding.bone1, binding.weight1 };
        object->vertexBone2[i] = { binding.bone2, binding.weight2 };
    }

    m_errorMessage = "";
    return true;
}

void RigGenerator::attachSingleNodesToBone(const Snapshot* snapshot,
    const std::string& boneName,
    const std::set<Uuid>& boneEdgeNodes,
    const std::set<Uuid>& allEdgeNodes,
    const std::set<Uuid>& nodesWithAnyEdge,
    std::vector<std::vector<Uuid>>& nodeChains)
{
    for (const auto& nodePair : snapshot->nodes) {
        Uuid nodeId(nodePair.first);

        if (!nodeBelongsToModelPart(snapshot, nodeId))
            continue;

        // Only attach nodes that have NO edges at all (truly isolated).
        // If a node has edges (even without bone assignment), skip it.
        if (nodesWithAnyEdge.count(nodeId))
            continue;

        // Already claimed by another bone
        if (m_singleNodeBoneMap.count(nodeId))
            continue;

        float nx = 0, ny = 0, nz = 0;
        if (!getNodePosition(snapshot, nodeId, nx, ny, nz))
            continue;

        // Find nearest node among this bone's edge-connected nodes
        float bestDist = std::numeric_limits<float>::max();
        bool foundNearest = false;
        for (const auto& candidateId : boneEdgeNodes) {
            float cx = 0, cy = 0, cz = 0;
            if (!getNodePosition(snapshot, candidateId, cx, cy, cz))
                continue;
            float dx = nx - cx, dy = ny - cy, dz = nz - cz;
            float dist = dx * dx + dy * dy + dz * dz;
            if (dist < bestDist) {
                bestDist = dist;
                foundNearest = true;
            }
        }

        if (foundNearest) {
            // Verify this bone is truly the nearest bone overall
            bool nearerBoneExists = false;
            for (const auto& candidateId : allEdgeNodes) {
                if (boneEdgeNodes.count(candidateId))
                    continue;
                float cx = 0, cy = 0, cz = 0;
                if (!getNodePosition(snapshot, candidateId, cx, cy, cz))
                    continue;
                float dx = nx - cx, dy = ny - cy, dz = nz - cz;
                float dist = dx * dx + dy * dy + dz * dz;
                if (dist < bestDist) {
                    nearerBoneExists = true;
                    break;
                }
            }
            if (!nearerBoneExists) {
                dust3dDebug << "Single node" << nodeId.toString().c_str()
                            << "attached to bone" << boneName.c_str();
                nodeChains.push_back({ nodeId });
                m_singleNodeBoneMap[nodeId] = boneName;
            }
        }
    }
}

bool RigGenerator::extractNodeChainsForBone(const Snapshot* snapshot,
    const std::string& boneName,
    std::vector<std::vector<Uuid>>& nodeChains)
{
    nodeChains.clear();

    std::map<Uuid, std::vector<Uuid>> adjacency;
    std::set<Uuid> allNodes;
    buildNodeAdjacency(snapshot, boneName, adjacency, allNodes);

    if (allNodes.empty()) {
        return false;
    }

    // Find all connected components and traverse each as a chain
    std::set<Uuid> globalVisited;

    for (const auto& seedNode : allNodes) {
        if (globalVisited.count(seedNode))
            continue;

        // Discover connected component via DFS
        std::set<Uuid> componentNodes;
        std::vector<Uuid> stack;
        stack.push_back(seedNode);
        while (!stack.empty()) {
            Uuid n = stack.back();
            stack.pop_back();
            if (componentNodes.count(n))
                continue;
            componentNodes.insert(n);
            for (const auto& neighbor : adjacency[n]) {
                if (!componentNodes.count(neighbor))
                    stack.push_back(neighbor);
            }
        }

        // Find a degree-1 node in this component to start the linear walk
        Uuid startNode;
        for (const auto& n : componentNodes) {
            if (adjacency[n].size() == 1) {
                startNode = n;
                break;
            }
        }
        if (startNode.isNull())
            startNode = *componentNodes.begin();

        // Traverse chain linearly
        std::vector<Uuid> chain;
        Uuid current = startNode;
        Uuid prev;

        while (!current.isNull() && !globalVisited.count(current)) {
            chain.push_back(current);
            globalVisited.insert(current);

            Uuid next;
            const auto& neighbors = adjacency[current];
            for (const auto& neighbor : neighbors) {
                if (neighbor != prev) {
                    next = neighbor;
                    break;
                }
            }
            prev = current;
            current = next;
        }

        if (!chain.empty())
            nodeChains.push_back(chain);
    }

    return !nodeChains.empty();
}

void RigGenerator::orientChainTowardPoint(const Snapshot* snapshot,
    std::vector<Uuid>& chain,
    float refX, float refY, float refZ)
{
    if (chain.size() < 2)
        return;

    float frontX, frontY, frontZ;
    float backX, backY, backZ;

    if (!getNodePosition(snapshot, chain.front(), frontX, frontY, frontZ))
        return;
    if (!getNodePosition(snapshot, chain.back(), backX, backY, backZ))
        return;

    float distFront = (frontX - refX) * (frontX - refX)
        + (frontY - refY) * (frontY - refY)
        + (frontZ - refZ) * (frontZ - refZ);
    float distBack = (backX - refX) * (backX - refX)
        + (backY - refY) * (backY - refY)
        + (backZ - refZ) * (backZ - refZ);

    if (distBack < distFront) {
        std::reverse(chain.begin(), chain.end());
    }
}

bool RigGenerator::getNodePositionInternal(const Snapshot* snapshot, const Uuid& nodeId,
    float& x, float& y, float& z, std::set<std::string>& visited)
{
    if (!snapshot)
        return false;

    std::string nodeIdString = nodeId.toString();
    if (visited.count(nodeIdString))
        return false; // cycle detected

    visited.insert(nodeIdString);

    auto it = snapshot->nodes.find(nodeIdString);
    if (it == snapshot->nodes.end())
        return false;

    std::string mirrorFromNodeId = String::valueOrEmpty(it->second, "__mirrorFromNodeId");
    if (!mirrorFromNodeId.empty()) {
        Uuid mirrorNode(mirrorFromNodeId);
        float mx = 0, my = 0, mz = 0;
        if (getNodePositionInternal(snapshot, mirrorNode, mx, my, mz, visited)) {
            x = -mx;
            y = my;
            z = mz;
            return true;
        }

        // Fall back to this node when mirror source fails
    }

    // Apply same coordinate transformation as MeshGenerator uses
    x = (String::toFloat(String::valueOrEmpty(it->second, "x")) - m_mainProfileMiddleX);
    y = (m_mainProfileMiddleY - String::toFloat(String::valueOrEmpty(it->second, "y")));
    z = (m_sideProfileMiddleX - String::toFloat(String::valueOrEmpty(it->second, "z")));
    return true;
}

bool RigGenerator::getNodePosition(const Snapshot* snapshot, const Uuid& nodeId,
    float& x, float& y, float& z)
{
    std::set<std::string> visited;
    return getNodePositionInternal(snapshot, nodeId, x, y, z, visited);
}

void RigGenerator::buildNodeAdjacency(const Snapshot* snapshot,
    const std::string& boneName,
    std::map<Uuid, std::vector<Uuid>>& adjacency,
    std::set<Uuid>& allNodes)
{
    adjacency.clear();
    allNodes.clear();

    auto edgesForBone = getEdgesWithBoneName(snapshot, boneName);

    for (const auto* edge : edgesForBone) {
        if (!edge)
            continue;

        std::string fromNodeIdString = String::valueOrEmpty(*edge, "from");
        std::string toNodeIdString = String::valueOrEmpty(*edge, "to");

        if (fromNodeIdString.empty() || toNodeIdString.empty())
            continue;

        Uuid n1(fromNodeIdString);
        Uuid n2(toNodeIdString);

        adjacency[n1].push_back(n2);
        adjacency[n2].push_back(n1);

        allNodes.insert(n1);
        allNodes.insert(n2);
    }
}

std::vector<const std::map<std::string, std::string>*> RigGenerator::getEdgesWithBoneName(
    const Snapshot* snapshot,
    const std::string& boneName)
{
    std::vector<const std::map<std::string, std::string>*> result;

    for (const auto& edgePair : snapshot->edges) {
        const auto& edgeAttributes = edgePair.second;
        std::string edgeBoneName = String::valueOrEmpty(edgeAttributes, "boneName");
        if (edgeBoneName == boneName && edgeBelongsToModelPart(snapshot, edgeAttributes)) {
            result.push_back(&edgeAttributes);
        }
    }

    return result;
}

bool RigGenerator::nodeHasEdgeWithBoneName(const Snapshot* snapshot,
    const Uuid& nodeId,
    const std::string& boneName)
{
    std::string nodeIdString = nodeId.toString();

    for (const auto& edgePair : snapshot->edges) {
        const auto& edgeAttributes = edgePair.second;
        std::string fromNode = String::valueOrEmpty(edgeAttributes, "from");
        std::string toNode = String::valueOrEmpty(edgeAttributes, "to");
        std::string edgeBoneName = String::valueOrEmpty(edgeAttributes, "boneName");

        if ((fromNode == nodeIdString || toNode == nodeIdString) && edgeBoneName == boneName) {
            return true;
        }
    }

    return false;
}

std::string RigGenerator::getEdgeBoneName(const std::map<std::string, std::string>* edge)
{
    if (!edge)
        return "";
    return String::valueOrEmpty(*edge, "boneName");
}

bool RigGenerator::generateEyelidBones(Object* object, const Snapshot* snapshot, RigStructure& actualRig)
{
    if (!object || !snapshot)
        return false;

    // Find Head bone
    RigNode* headBone = nullptr;
    for (auto& bone : actualRig.bones) {
        if (bone.name == "Head") {
            headBone = &bone;
            break;
        }
    }
    if (!headBone)
        return false;

    // Collect vertices bound to Head bone
    std::set<size_t> headVertices;
    for (size_t i = 0; i < object->vertexBone1.size(); ++i) {
        if (object->vertexBone1[i].first == "Head")
            headVertices.insert(i);
    }
    if (headVertices.empty()) {
        return false;
    }

    // The mesh uses separate (non-shared) triangles: neighboring triangles have
    // duplicate vertices at the same position but with different indices.
    // We use PositionKey (quantized xyz) as vertex identity to merge duplicates.

    // Map position -> position key ID
    std::map<PositionKey, size_t> posKeyToId;
    std::vector<Vector3> posKeyPositions; // id -> representative position
    auto getOrCreatePosId = [&](size_t vertexIdx) -> size_t {
        PositionKey pk(object->vertices[vertexIdx]);
        auto it = posKeyToId.find(pk);
        if (it != posKeyToId.end())
            return it->second;
        size_t id = posKeyPositions.size();
        posKeyToId[pk] = id;
        posKeyPositions.push_back(object->vertices[vertexIdx]);
        return id;
    };

    // Build half-edge face counts for all head-bone triangles using position IDs.
    // An open edge (boundary) is one where one side has no neighboring triangle,
    // i.e. the edge appears in exactly one face.
    std::map<std::pair<size_t, size_t>, int> edgeFaceCount;
    // Store the face normal of the single triangle adjacent to each boundary edge.
    // Used later to orient the loop normal without relying on any external reference.
    std::map<std::pair<size_t, size_t>, Vector3> edgeFaceNormal;
    size_t headTriCount = 0;
    auto makeEdge = [](size_t a, size_t b) -> std::pair<size_t, size_t> {
        return a < b ? std::make_pair(a, b) : std::make_pair(b, a);
    };

    for (size_t ti = 0; ti < object->triangles.size(); ++ti) {
        const auto& face = object->triangles[ti];
        bool anyHead = false;
        for (size_t vi : face) {
            if (headVertices.count(vi)) {
                anyHead = true;
                break;
            }
        }
        if (!anyHead)
            continue;
        ++headTriCount;

        std::vector<size_t> faceIds;
        faceIds.reserve(face.size());
        for (size_t vi : face) {
            faceIds.push_back(getOrCreatePosId(vi));
        }
        // Compute triangle face normal (outward by mesh winding convention)
        Vector3 faceNormal;
        if (faceIds.size() >= 3) {
            const Vector3& v0 = posKeyPositions[faceIds[0]];
            const Vector3& v1 = posKeyPositions[faceIds[1]];
            const Vector3& v2 = posKeyPositions[faceIds[2]];
            faceNormal = Vector3::crossProduct(v1 - v0, v2 - v0);
        }
        for (size_t i = 0; i < faceIds.size(); ++i) {
            size_t a = faceIds[i];
            size_t b = faceIds[(i + 1) % faceIds.size()];
            if (a != b) {
                edgeFaceCount[makeEdge(a, b)]++;
                // Overwrite is fine: for boundary edges (count==1) this will be the only face
                edgeFaceNormal[makeEdge(a, b)] = faceNormal;
            }
        }
    }

    // Boundary edges: appear in exactly one face (open half-edge)
    std::map<size_t, std::vector<size_t>> boundaryAdj;
    for (const auto& ef : edgeFaceCount) {
        if (ef.second == 1) {
            boundaryAdj[ef.first.first].push_back(ef.first.second);
            boundaryAdj[ef.first.second].push_back(ef.first.first);
        }
    }

    if (boundaryAdj.empty())
        return false;

    // Trace boundary loops using position IDs
    std::set<size_t> visited;
    std::vector<std::vector<size_t>> allLoops;
    for (const auto& ba : boundaryAdj) {
        if (visited.count(ba.first))
            continue;
        std::vector<size_t> loop;
        size_t current = ba.first;
        size_t prev = SIZE_MAX;
        while (!visited.count(current)) {
            visited.insert(current);
            loop.push_back(current);
            auto adjIt = boundaryAdj.find(current);
            if (adjIt == boundaryAdj.end())
                break;
            const auto& neighbors = adjIt->second;
            size_t next = SIZE_MAX;
            for (size_t n : neighbors) {
                if (n != prev) {
                    next = n;
                    break;
                }
            }
            if (next == SIZE_MAX)
                break;
            prev = current;
            current = next;
        }
        if (loop.size() >= 3)
            allLoops.push_back(loop);
    }

    if (allLoops.empty())
        return false;

    // Compute center and size of each loop, then pick the two smallest
    // (the eye holes), filtering out the large outer boundary of the head region
    struct LoopInfo {
        size_t index;
        Vector3 center;
        float avgRadius;
    };
    std::vector<LoopInfo> loopInfos;
    for (size_t li = 0; li < allLoops.size(); ++li) {
        const auto& loop = allLoops[li];
        Vector3 center;
        for (size_t pid : loop)
            center = center + posKeyPositions[pid];
        center = center * (1.0f / loop.size());
        float avgR = 0;
        for (size_t pid : loop)
            avgR += (posKeyPositions[pid] - center).length();
        avgR /= loop.size();
        loopInfos.push_back({ li, center, avgR });
    }

    // Compute head bone length as a scale reference
    float headLength = Vector3(headBone->endX - headBone->posX,
        headBone->endY - headBone->posY,
        headBone->endZ - headBone->posZ)
                           .length();
    if (headLength < 1e-6f)
        headLength = 0.1f;

    // Filter out degenerate micro-loops (radius < 1% of head length)
    // and huge outer-boundary loops (radius > 80% of head length)
    float minRadius = headLength * 0.01f;
    float maxRadius = headLength * 0.8f;
    std::vector<LoopInfo> candidates;
    for (const auto& li : loopInfos) {
        if (li.avgRadius >= minRadius && li.avgRadius <= maxRadius)
            candidates.push_back(li);
    }

    dust3dDebug << "Eyelid detection: after filtering (min=" << minRadius << " max=" << maxRadius
                << "):" << candidates.size() << "candidate loops from" << loopInfos.size() << "total";

    // Find the best symmetric pair: similar radius, symmetric x positions,
    // both in the upper-front region of the head
    Vector3 headCenter(
        (headBone->posX + headBone->endX) * 0.5f,
        (headBone->posY + headBone->endY) * 0.5f,
        (headBone->posZ + headBone->endZ) * 0.5f);

    int bestI = -1, bestJ = -1;
    float bestScore = std::numeric_limits<float>::max();
    for (size_t i = 0; i < candidates.size(); ++i) {
        for (size_t j = i + 1; j < candidates.size(); ++j) {
            const auto& a = candidates[i];
            const auto& b = candidates[j];
            // Must be on opposite sides of x=0 (or center x)
            if (a.center.x() * b.center.x() > 0)
                continue; // same side, skip
            // Radius similarity (0 = perfect match)
            float radiusDiff = std::abs(a.avgRadius - b.avgRadius)
                / std::max(a.avgRadius, b.avgRadius);
            // Y similarity (eyes should be at similar height)
            float yDiff = std::abs(a.center.y() - b.center.y()) / headLength;
            // Z similarity
            float zDiff = std::abs(a.center.z() - b.center.z()) / headLength;
            // Vertex count similarity
            float vertDiff = std::abs((float)allLoops[a.index].size() - (float)allLoops[b.index].size())
                / std::max((float)allLoops[a.index].size(), (float)allLoops[b.index].size());

            float score = radiusDiff + yDiff + zDiff + vertDiff;

            dust3dDebug << "  pair candidate (" << i << "," << j << ")"
                        << "radii=" << a.avgRadius << "," << b.avgRadius
                        << "verts=" << allLoops[a.index].size() << "," << allLoops[b.index].size()
                        << "score=" << score;

            if (score < bestScore) {
                bestScore = score;
                bestI = (int)i;
                bestJ = (int)j;
            }
        }
    }

    if (bestI < 0 || bestJ < 0) {
        dust3dDebug << "Eyelid detection: no valid symmetric pair found";
        return false;
    }

    std::vector<std::vector<size_t>> loops = {
        allLoops[candidates[bestI].index],
        allLoops[candidates[bestJ].index]
    };
    std::vector<LoopInfo> selectedInfos = { candidates[bestI], candidates[bestJ] };

    dust3dDebug << "Eyelid detection: selected pair with radii"
                << selectedInfos[0].avgRadius << "and" << selectedInfos[1].avgRadius
                << "verts=" << loops[0].size() << "and" << loops[1].size()
                << "score=" << bestScore;

    // Compute normal of each selected loop
    struct HoleInfo {
        Vector3 center;
        Vector3 normal;
    };
    std::vector<HoleInfo> holes;
    for (size_t si = 0; si < 2; ++si) {
        const auto& loop = loops[si];
        Vector3 center = selectedInfos[si].center;

        // Compute normal via Newell's method
        Vector3 normal;
        for (size_t i = 0; i < loop.size(); ++i) {
            const Vector3& cur = posKeyPositions[loop[i]];
            const Vector3& nxt = posKeyPositions[loop[(i + 1) % loop.size()]];
            normal.setX(normal.x() + (cur.y() - nxt.y()) * (cur.z() + nxt.z()));
            normal.setY(normal.y() + (cur.z() - nxt.z()) * (cur.x() + nxt.x()));
            normal.setZ(normal.z() + (cur.x() - nxt.x()) * (cur.y() + nxt.y()));
        }
        if (!normal.isZero())
            normal.normalize();

        // Orient the loop normal using half-edge topology:
        // Each boundary edge belongs to exactly one triangle whose face normal
        // points outward from the mesh surface. The loop normal should point
        // *opposite* to that face normal (i.e., through the hole opening).
        // Traverse the loop until we find a boundary edge with a stored face normal.
        for (size_t i = 0; i < loop.size() && !normal.isZero(); ++i) {
            auto key = makeEdge(loop[i], loop[(i + 1) % loop.size()]);
            auto fnIt = edgeFaceNormal.find(key);
            if (fnIt != edgeFaceNormal.end() && !fnIt->second.isZero()) {
                // Face normal points outward from mesh; loop normal should agree
                if (Vector3::dotProduct(normal, fnIt->second) < 0)
                    normal = Vector3(-normal.x(), -normal.y(), -normal.z());
                break;
            }
        }

        holes.push_back({ center, normal });
    }

    // Classify left/right by x coordinate
    size_t leftIdx = holes[0].center.x() > holes[1].center.x() ? 0 : 1;
    size_t rightIdx = 1 - leftIdx;

    float eyelidLength = 0.03f;
    // Use loop radius as a rough guide for bone length
    for (size_t hi = 0; hi < 2; ++hi) {
        float maxDist = 0;
        for (size_t pid : loops[hi]) {
            float d = (posKeyPositions[pid] - holes[hi].center).length();
            if (d > maxDist)
                maxDist = d;
        }
        if (maxDist > eyelidLength)
            eyelidLength = maxDist * 0.5f;
    }

    // Create 4 eyelid bones: upper + lower for each eye.
    // Split each loop into upper and lower halves by Y relative to the hole center.
    // Upper lid vertices (Y >= center.Y) bind to UpperEyelid bone,
    // lower lid vertices (Y < center.Y) bind to LowerEyelid bone.
    //
    // Bone direction = rotation axis for blinking (horizontal, perpendicular to
    // the outward eye normal). This lets the animation simply rotate around the
    // bone direction without needing to derive the axis from cross products.
    auto addEyelidBone = [&](const std::string& name, size_t holeIdx, float yOffset) {
        Vector3 up(0.0f, 1.0f, 0.0f);
        Vector3 rotAxis = Vector3::crossProduct(holes[holeIdx].normal, up);
        if (rotAxis.lengthSquared() < 1e-8f) {
            Vector3 forward(0.0f, 0.0f, 1.0f);
            rotAxis = Vector3::crossProduct(holes[holeIdx].normal, forward);
        }
        rotAxis.normalize();

        float halfLen = eyelidLength * 0.5f;
        RigNode eyelid;
        eyelid.name = name;
        eyelid.parent = "Head";
        eyelid.posX = holes[holeIdx].center.x() - rotAxis.x() * halfLen;
        eyelid.posY = holes[holeIdx].center.y() + yOffset - rotAxis.y() * halfLen;
        eyelid.posZ = holes[holeIdx].center.z() - rotAxis.z() * halfLen;
        eyelid.endX = holes[holeIdx].center.x() + rotAxis.x() * halfLen;
        eyelid.endY = holes[holeIdx].center.y() + yOffset + rotAxis.y() * halfLen;
        eyelid.endZ = holes[holeIdx].center.z() + rotAxis.z() * halfLen;
        eyelid.capsuleRadius = eyelidLength * 0.5f;
        actualRig.bones.push_back(eyelid);
    };

    addEyelidBone("LeftUpperEyelid", leftIdx, 0);
    addEyelidBone("LeftLowerEyelid", leftIdx, 0);
    addEyelidBone("RightUpperEyelid", rightIdx, 0);
    addEyelidBone("RightLowerEyelid", rightIdx, 0);

    // Rebind loop vertices to upper/lower eyelid bones.
    // Split the loop at the two eye corners (min/max projection along the hinge
    // axis). The two paths between the corners form the upper and lower lids.
    for (size_t hi = 0; hi < 2; ++hi) {
        const std::string side = (hi == leftIdx) ? "Left" : "Right";
        const std::string upperName = side + "UpperEyelid";
        const std::string lowerName = side + "LowerEyelid";
        const auto& loop = loops[hi];

        // Compute hinge axis for this eye
        Vector3 hingeAxis = Vector3::crossProduct(holes[hi].normal, Vector3(0, 1, 0));
        if (hingeAxis.lengthSquared() < 1e-8f)
            hingeAxis = Vector3::crossProduct(holes[hi].normal, Vector3(0, 0, 1));
        hingeAxis.normalize();

        // Find the two corner vertices: farthest from the hole center
        size_t corner1 = 0;
        float maxDistSq1 = 0;
        for (size_t li = 0; li < loop.size(); ++li) {
            float dSq = (posKeyPositions[loop[li]] - holes[hi].center).lengthSquared();
            if (dSq > maxDistSq1) {
                maxDistSq1 = dSq;
                corner1 = li;
            }
        }
        // Second corner: farthest from the first corner
        size_t corner2 = 0;
        float maxDistSq2 = 0;
        for (size_t li = 0; li < loop.size(); ++li) {
            float dSq = (posKeyPositions[loop[li]] - posKeyPositions[loop[corner1]]).lengthSquared();
            if (dSq > maxDistSq2) {
                maxDistSq2 = dSq;
                corner2 = li;
            }
        }
        // Split the loop into two ordered paths at the two corners.
        // Each path goes from corner1 -> ... -> corner2, excluding corners.
        // The position along the path determines the eyelid bone weight:
        // 0 at corners, 1 at the midpoint of the path.
        std::vector<size_t> pathA, pathB;
        {
            size_t i = corner1;
            i = (i + 1) % loop.size();
            while (i != corner2) {
                pathA.push_back(loop[i]);
                i = (i + 1) % loop.size();
            }
            i = corner2;
            i = (i + 1) % loop.size();
            while (i != corner1) {
                pathB.push_back(loop[i]);
                i = (i + 1) % loop.size();
            }
        }

        float sumYA = 0, sumYB = 0;
        for (size_t pid : pathA)
            sumYA += posKeyPositions[pid].y();
        for (size_t pid : pathB)
            sumYB += posKeyPositions[pid].y();

        std::set<size_t> upperSet, lowerSet;
        for (size_t pid : ((sumYA >= sumYB) ? pathA : pathB))
            upperSet.insert(pid);
        for (size_t pid : ((sumYA >= sumYB) ? pathB : pathA))
            lowerSet.insert(pid);

        Vector3 boneMid = holes[hi].center;

        size_t upperCount = 0, lowerCount = 0;
        for (size_t vi = 0; vi < object->vertices.size(); ++vi) {
            PositionKey pk(object->vertices[vi]);
            auto it = posKeyToId.find(pk);
            if (it == posKeyToId.end())
                continue;
            size_t pid = it->second;
            if (upperSet.count(pid)) {
                object->vertexBone1[vi] = { upperName, 1.0f };
                object->vertexBone2[vi] = { "", 0.0f };
                ++upperCount;
            } else if (lowerSet.count(pid)) {
                object->vertexBone1[vi] = { lowerName, 1.0f };
                object->vertexBone2[vi] = { "", 0.0f };
                ++lowerCount;
            }
        }

        const std::vector<size_t>& upperPath = (sumYA >= sumYB) ? pathA : pathB;
        const std::vector<size_t>& lowerPath = (sumYA >= sumYB) ? pathB : pathA;

        auto computeClosingAngle = [&](const std::vector<size_t>& path, const Vector3& pivot) -> float {
            if (path.empty())
                return 0.0f;
            Vector3 midVertex = posKeyPositions[path[path.size() / 2]];
            Vector3 a = (midVertex - pivot).normalized();
            Vector3 b = holes[hi].normal.normalized();
            return std::acos(std::max(-1.0, std::min(1.0, (double)Vector3::dotProduct(a, b))));
        };

        float upperClosing = -computeClosingAngle(upperPath, boneMid);
        float lowerClosing = computeClosingAngle(lowerPath, boneMid);

        for (auto& bone : actualRig.bones) {
            if (bone.name == upperName)
                bone.closingAngle = upperClosing;
            else if (bone.name == lowerName)
                bone.closingAngle = lowerClosing;
        }
    }

    return true;
}

bool RigGenerator::applyRigBindings(Object* object, const Snapshot* snapshot, RigStructure* actualRig)
{
    if (!object || !snapshot) {
        m_errorMessage = "Object or snapshot not initialized";
        return false;
    }

    // Use RigGenerator to compute bone influences and bind vertices
    std::map<Uuid, NodeBoneInfluence> nodeBoneInfluences;

    // Build a default empty rig structure if no actual rig is provided
    RigStructure emptyRig;
    const RigStructure& rigForInfluences = actualRig ? *actualRig : emptyRig;

    if (!computeNodeBoneInfluences(snapshot, rigForInfluences, nodeBoneInfluences)) {
        dust3dDebug << "Failed to compute node bone influences:" << getErrorMessage().c_str();
        return false;
    }

    if (!computeVertexBoneBindings(object, nodeBoneInfluences)) {
        dust3dDebug << "Failed to compute vertex bone bindings:" << getErrorMessage().c_str();
        return false;
    }

    dust3dDebug << "Applied rig bindings to" << object->vertices.size() << "vertices";

    // Ground the model if the rig has ground contact bones:
    // Find the lowest foot/tibia bone Y and translate so feet touch Y=0 (up = (0,1,0))
    if (actualRig) {
        auto isGroundContactBone = [](const std::string& name) -> bool {
            return name.find("Foot") != std::string::npos
                || name.find("Tibia") != std::string::npos;
        };

        float lowestFootY = std::numeric_limits<float>::max();
        bool hasContactBone = false;
        for (const auto& bone : actualRig->bones) {
            if (isGroundContactBone(bone.name)) {
                float tipY = std::min(bone.posY, bone.endY);
                if (tipY < lowestFootY) {
                    lowestFootY = tipY;
                    hasContactBone = true;
                }
            }
        }

        if (!hasContactBone) {
            // No explicit Foot bone available; use the lowest bone endpoint instead.
            for (const auto& bone : actualRig->bones) {
                if (bone.name.find("Root") != std::string::npos)
                    continue;
                float tipY = std::min(bone.posY, bone.endY);
                if (tipY < lowestFootY) {
                    lowestFootY = tipY;
                    hasContactBone = true;
                }
            }
        }

        if (hasContactBone) {
            float offsetY = -lowestFootY;
            dust3dDebug << "Grounding model: offsetY=" << offsetY;

            for (auto& bone : actualRig->bones) {
                bone.posY += offsetY;
                bone.endY += offsetY;
            }

            for (auto& vertex : object->vertices) {
                vertex.setY(vertex.y() + offsetY);
            }
        }
    }

    return true;
}

}
