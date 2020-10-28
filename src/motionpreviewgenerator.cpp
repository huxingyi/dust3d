#include <QDebug>
#include <QRegularExpression>
#include "motionpreviewgenerator.h"
#include "jointnodetree.h"
#include "blockmesh.h"

void MotionPreviewGenerator::process()
{
    generate();
    
    emit finished();
}

void MotionPreviewGenerator::generate()
{
    for (size_t i = 0; i < (*m_bones).size(); ++i) {
        const auto &bone = (*m_bones)[i];
        qDebug() << "Bone[" << i << "]: " << bone.name << "parent:" << bone.parent;
    }
    
    std::map<QString, std::vector<int>> chains;
    
    QRegularExpression reJoints("^([a-zA-Z]+\\d*)_Joint\\d+$");
    QRegularExpression reSpine("^([a-zA-Z]+)\\d*$");
    for (int index = 0; index < (int)(*m_bones).size(); ++index) {
        const auto &bone = (*m_bones)[index];
        const auto &item = bone.name;
        QRegularExpressionMatch match = reJoints.match(item);
        if (match.hasMatch()) {
            QString name = match.captured(1);
            chains[name].push_back(index);
        } else {
            match = reSpine.match(item);
            if (match.hasMatch()) {
                QString name = match.captured(1);
                if (item.startsWith(name + "0"))
                    chains[name + "0"].push_back(index);
                else
                    chains[name].push_back(index);
            } else if (item.startsWith("Virtual_")) {
                //qDebug() << "Ignore connector:" << item;
            } else {
                qDebug() << "Unrecognized bone name:" << item;
            }
        }
    }
    
    std::vector<std::pair<int, bool>> spineBones;
    auto findSpine = chains.find("Spine");
    if (findSpine != chains.end()) {
        for (const auto &it: findSpine->second) {
            spineBones.push_back({it, true});
        }
    }
    auto findNeck = chains.find("Neck");
    if (findNeck != chains.end()) {
        for (const auto &it: findNeck->second) {
            spineBones.push_back({it, true});
        }
    }
    std::reverse(spineBones.begin(), spineBones.end());
    auto findSpine0 = chains.find("Spine0");
    if (findSpine0 != chains.end()) {
        for (const auto &it: findSpine0->second) {
            spineBones.push_back({it, false});
        }
    }
    auto findTail = chains.find("Tail");
    if (findTail != chains.end()) {
        for (const auto &it: findTail->second) {
            spineBones.push_back({it, false});
        }
    }
    
    //for (size_t i = 0; i < spineBones.size(); ++i) {
    //    qDebug() << "spineBones[" << i << "]:" << (*m_bones)[spineBones[i].first].name;
    //}
    
    double radiusScale = 0.5;
    
    std::vector<VertebrataMotion::Node> spineNodes;
    if (!spineBones.empty()) {
        const auto &it = spineBones[0];
        if (it.second) {
            spineNodes.push_back({(*m_bones)[it.first].tailPosition,
                (*m_bones)[it.first].tailRadius * radiusScale,
                it.first, 
                true});
        } else {
            spineNodes.push_back({(*m_bones)[it.first].headPosition,
                (*m_bones)[it.first].headRadius * radiusScale,
                it.first,
                false});
        }
    }
    std::map<int, size_t> spineBoneToNodeMap;
    for (size_t i = 0; i < spineBones.size(); ++i) {
        const auto &it = spineBones[i];
        if (it.second) {
            spineBoneToNodeMap[it.first] = i;
            if ((*m_bones)[it.first].name == "Spine1")
                spineBoneToNodeMap[0] = spineNodes.size();
            spineNodes.push_back({(*m_bones)[it.first].headPosition,
                (*m_bones)[it.first].headRadius * radiusScale,
                it.first,
                false});
        } else {
            spineNodes.push_back({(*m_bones)[it.first].tailPosition,
                (*m_bones)[it.first].tailRadius * radiusScale,
                it.first,
                true});
        }
    }
    m_vertebrataMotion = new VertebrataMotion;
    
    m_vertebrataMotion->setParameters(m_parameters);
    m_vertebrataMotion->setSpineNodes(spineNodes);
    
    m_groundY = std::numeric_limits<double>::max();
    for (const auto &it: spineNodes) {
        if (it.position.y() - it.radius < m_groundY)
            m_groundY = it.position.y() - it.radius;
    }
    
    for (const auto &chain: chains) {
        std::vector<VertebrataMotion::Node> legNodes;
        VertebrataMotion::Side side;
        if (chain.first.startsWith("LeftLimb")) {
            side = VertebrataMotion::Side::Left;
        } else if (chain.first.startsWith("RightLimb")) {
            side = VertebrataMotion::Side::Right;
        } else {
            continue;
        }
        int virtualBoneIndex = (*m_bones)[chain.second[0]].parent;
        if (-1 == virtualBoneIndex)
            continue;
        int spineBoneIndex = (*m_bones)[virtualBoneIndex].parent;
        auto findSpine = spineBoneToNodeMap.find(spineBoneIndex);
        if (findSpine == spineBoneToNodeMap.end())
            continue;
        legNodes.push_back({(*m_bones)[virtualBoneIndex].headPosition,
            (*m_bones)[virtualBoneIndex].headRadius * radiusScale,
            virtualBoneIndex, 
            false});
        legNodes.push_back({(*m_bones)[virtualBoneIndex].tailPosition,
            (*m_bones)[virtualBoneIndex].tailRadius * radiusScale,
            virtualBoneIndex,
            true});
        for (const auto &it: chain.second) {
            legNodes.push_back({(*m_bones)[it].tailPosition,
                (*m_bones)[it].tailRadius * radiusScale,
                it,
                true});
        }
        m_vertebrataMotion->setLegNodes(findSpine->second, side, legNodes);
        for (const auto &it: legNodes) {
            if (it.position.y() - it.radius < m_groundY)
                m_groundY = it.position.y() - it.radius;
        }
    }
    m_vertebrataMotion->setGroundY(m_groundY);
    m_vertebrataMotion->generate();
    
    if (m_previewSkinnedMesh)
        makeSkinnedMesh();
}

void MotionPreviewGenerator::makeSkinnedMesh()
{
    if (m_bones->empty())
        return;
    
    std::vector<QMatrix4x4> bindTransforms(m_bones->size());
    for (size_t i = 0; i < m_bones->size(); ++i) {
        const auto &bone = (*m_bones)[i];
        QMatrix4x4 parentMatrix;
        QMatrix4x4 translationMatrix;
        if (-1 != bone.parent) {
            const auto &parentBone = (*m_bones)[bone.parent];
            parentMatrix = bindTransforms[bone.parent];
            translationMatrix.translate(bone.headPosition - parentBone.headPosition);
        } else {
            translationMatrix.translate(bone.headPosition);
        }
        bindTransforms[i] = parentMatrix * translationMatrix;
    }
    
    for (const auto &frame: m_vertebrataMotion->frames()) {
        std::vector<RiggerBone> transformedBones = *m_bones;
        for (const auto &node: frame.nodes) {
            if (-1 == node.boneIndex)
                continue;
            if (node.isTail) {
                transformedBones[node.boneIndex].tailPosition = node.position;
                for (const auto &childIndex: (*m_bones)[node.boneIndex].children)
                    transformedBones[childIndex].headPosition = node.position;
            } else {
                transformedBones[node.boneIndex].headPosition = node.position;
                auto parentIndex = (*m_bones)[node.boneIndex].parent;
                if (-1 != parentIndex) {
                    transformedBones[parentIndex].tailPosition = node.position;
                    for (const auto &childIndex: (*m_bones)[parentIndex].children)
                        transformedBones[childIndex].headPosition = node.position;
                }
            }
        }
        
        std::vector<QMatrix4x4> poseTransforms(transformedBones.size());
        std::vector<QMatrix4x4> poseRotations(transformedBones.size());
        for (size_t i = 0; i < transformedBones.size(); ++i) {
            const auto &oldBone = (*m_bones)[i];
            const auto &bone = transformedBones[i];
            QMatrix4x4 parentMatrix;
            QMatrix4x4 translationMatrix;
            QMatrix4x4 rotationMatrix;
            QMatrix4x4 parentRotation;
            if (-1 != bone.parent) {
                const auto &oldParentBone = (*m_bones)[oldBone.parent];
                const auto &parentBone = transformedBones[bone.parent];
                parentMatrix = poseTransforms[bone.parent];
                parentRotation = poseRotations[bone.parent];
                translationMatrix.translate(oldBone.headPosition - oldParentBone.headPosition);
                QQuaternion rotation = QQuaternion::rotationTo((oldBone.tailPosition - oldBone.headPosition).normalized(),
                    (bone.tailPosition - bone.headPosition).normalized());
                rotationMatrix.rotate(rotation);
            } else {
                translationMatrix.translate(bone.headPosition + (bone.tailPosition - oldBone.tailPosition));
            }
            poseTransforms[i] = parentMatrix * translationMatrix * parentRotation.inverted() * rotationMatrix;
            poseRotations[i] = rotationMatrix;
        }

        std::vector<QMatrix4x4> transforms(m_bones->size());
        for (size_t i = 0; i < m_bones->size(); ++i) {
            transforms[i] = poseTransforms[i] * bindTransforms[i].inverted();
        }
        
        std::vector<QVector3D> transformedVertices(m_outcome->vertices.size());
        for (size_t i = 0; i < m_outcome->vertices.size(); ++i) {
            const auto &weight = (*m_rigWeights)[i];
            for (int x = 0; x < 4; x++) {
                float factor = weight.boneWeights[x];
                if (factor > 0) {
                    transformedVertices[i] += transforms[weight.boneIndices[x]] * m_outcome->vertices[i] * factor;
                }
            }
        }
        
        VertebrataMotion::FrameMesh frameMesh;
        frameMesh.vertices = transformedVertices;
        frameMesh.faces = m_outcome->triangles;
        {
            BlockMesh blockMesh;
            blockMesh.addBlock(
                QVector3D(0.0, m_groundY + m_parameters.groundOffset, 0.0), 100.0,
                QVector3D(0.0, m_groundY + m_parameters.groundOffset - 0.02, 0.0), 100.0);
            for (const auto &bone: transformedBones) {
                if (0 == bone.index)
                    continue;
                blockMesh.addBlock(bone.headPosition, bone.headRadius * 0.5,
                    bone.tailPosition, bone.tailRadius * 0.5);
            }
            blockMesh.build();
            std::vector<QVector3D> *resultVertices = blockMesh.takeResultVertices();
            std::vector<std::vector<size_t>> *resultFaces = blockMesh.takeResultFaces();
            size_t oldVertexCount = frameMesh.vertices.size();
            for (const auto &v: *resultVertices)
                frameMesh.vertices.push_back(QVector3D(v.x() - 0.5, v.y(), v.z()));
            for (const auto &f: *resultFaces) {
                std::vector<size_t> newF = f;
                for (auto &v: newF)
                    v += oldVertexCount;
                frameMesh.faces.push_back(newF);
            }
            delete resultFaces;
            delete resultVertices;
        }
        m_skinnedFrames.push_back(frameMesh);
    }
}
