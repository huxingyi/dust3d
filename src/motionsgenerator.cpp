#include <QGuiApplication>
#include <QElapsedTimer>
#include <cmath>
#include <QRegularExpression>
#include <QMatrix4x4>
#include "motionsgenerator.h"
#include "vertebratamotion.h"
#include "blockmesh.h"
#include "vertebratamotionparameterswidget.h"
#include "util.h"

MotionsGenerator::MotionsGenerator(RigType rigType,
        const std::vector<RiggerBone> &bones,
        const std::map<int, RiggerVertexWeights> &rigWeights,
        const Object &object) :
    m_rigType(rigType),
    m_bones(bones),
    m_rigWeights(rigWeights),
    m_object(object)
{
}

MotionsGenerator::~MotionsGenerator()
{
    for (auto &it: m_resultSnapshotMeshes)
        delete it.second;
    
    for (auto &item: m_resultPreviewMeshes) {
        for (auto &subItem: item.second) {
            delete subItem.second;
        }
    }
}

void MotionsGenerator::enablePreviewMeshes()
{
    m_previewMeshesEnabled = true;
}

void MotionsGenerator::enableSnapshotMeshes()
{
    m_snapshotMeshesEnabled = true;
}

void MotionsGenerator::addMotion(const QUuid &motionId, const std::map<QString, QString> &parameters)
{
    m_motions[motionId] = parameters;
}

const std::set<QUuid> &MotionsGenerator::generatedMotionIds()
{
    return m_generatedMotionIds;
}

Model *MotionsGenerator::takeResultSnapshotMesh(const QUuid &motionId)
{
    auto findResult = m_resultSnapshotMeshes.find(motionId);
    if (findResult == m_resultSnapshotMeshes.end())
        return nullptr;
    auto result = findResult->second;
    m_resultSnapshotMeshes.erase(findResult);
    return result;
}

std::vector<std::pair<float, SimpleShaderMesh *>> MotionsGenerator::takeResultPreviewMeshes(const QUuid &motionId)
{
    auto findResult = m_resultPreviewMeshes.find(motionId);
    if (findResult == m_resultPreviewMeshes.end())
        return {};
    auto result = findResult->second;
    m_resultPreviewMeshes.erase(findResult);
    return result;
}

std::vector<std::pair<float, JointNodeTree>> MotionsGenerator::takeResultJointNodeTrees(const QUuid &motionId)
{
    auto findResult = m_resultJointNodeTrees.find(motionId);
    if (findResult == m_resultJointNodeTrees.end())
        return {};
    return findResult->second;
}
        
void MotionsGenerator::generateMotion(const QUuid &motionId)
{
    if (m_bones.empty())
        return;
    
    std::map<QString, std::vector<int>> chains;
    
    QRegularExpression reJoints("^([a-zA-Z]+\\d*)_Joint\\d+$");
    QRegularExpression reSpine("^([a-zA-Z]+)\\d*$");
    for (int index = 0; index < (int)m_bones.size(); ++index) {
        const auto &bone = m_bones[index];
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
    
    double radiusScale = 0.5;
    
    std::vector<VertebrataMotion::Node> spineNodes;
    if (!spineBones.empty()) {
        const auto &it = spineBones[0];
        if (it.second) {
            spineNodes.push_back({m_bones[it.first].tailPosition,
                m_bones[it.first].tailRadius * radiusScale,
                it.first, 
                true});
        } else {
            spineNodes.push_back({m_bones[it.first].headPosition,
                m_bones[it.first].headRadius * radiusScale,
                it.first,
                false});
        }
    }
    std::map<int, size_t> spineBoneToNodeMap;
    for (size_t i = 0; i < spineBones.size(); ++i) {
        const auto &it = spineBones[i];
        if (it.second) {
            spineBoneToNodeMap[it.first] = i;
            if (m_bones[it.first].name == "Spine1")
                spineBoneToNodeMap[0] = spineNodes.size();
            spineNodes.push_back({m_bones[it.first].headPosition,
                m_bones[it.first].headRadius * radiusScale,
                it.first,
                false});
        } else {
            spineNodes.push_back({m_bones[it.first].tailPosition,
                m_bones[it.first].tailRadius * radiusScale,
                it.first,
                true});
        }
    }
    
    VertebrataMotion *vertebrataMotion = new VertebrataMotion;
    
    VertebrataMotion::Parameters parameters = 
        VertebrataMotionParametersWidget::toVertebrataMotionParameters(m_motions[motionId]);
    if ("Vertical" == valueOfKeyInMapOrEmpty(m_bones[0].attributes, "spineDirection"))
        parameters.biped = true;
    vertebrataMotion->setParameters(parameters);
    vertebrataMotion->setSpineNodes(spineNodes);
    
    double groundY = std::numeric_limits<double>::max();
    for (const auto &it: spineNodes) {
        if (it.position.y() - it.radius < groundY)
            groundY = it.position.y() - it.radius;
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
        int virtualBoneIndex = m_bones[chain.second[0]].parent;
        if (-1 == virtualBoneIndex)
            continue;
        int spineBoneIndex = m_bones[virtualBoneIndex].parent;
        auto findSpine = spineBoneToNodeMap.find(spineBoneIndex);
        if (findSpine == spineBoneToNodeMap.end())
            continue;
        legNodes.push_back({m_bones[virtualBoneIndex].headPosition,
            m_bones[virtualBoneIndex].headRadius * radiusScale,
            virtualBoneIndex, 
            false});
        legNodes.push_back({m_bones[virtualBoneIndex].tailPosition,
            m_bones[virtualBoneIndex].tailRadius * radiusScale,
            virtualBoneIndex,
            true});
        for (const auto &it: chain.second) {
            legNodes.push_back({m_bones[it].tailPosition,
                m_bones[it].tailRadius * radiusScale,
                it,
                true});
        }
        vertebrataMotion->setLegNodes(findSpine->second, side, legNodes);
        for (const auto &it: legNodes) {
            if (it.position.y() - it.radius < groundY)
                groundY = it.position.y() - it.radius;
        }
    }
    vertebrataMotion->setGroundY(groundY);
    vertebrataMotion->generate();
    
    std::vector<std::pair<float, JointNodeTree>> jointNodeTrees;
    std::vector<std::pair<float, SimpleShaderMesh *>> previewMeshes;
    Model *snapshotMesh = nullptr;
    
    std::vector<QMatrix4x4> bindTransforms(m_bones.size());
    for (size_t i = 0; i < m_bones.size(); ++i) {
        const auto &bone = m_bones[i];
        QMatrix4x4 parentMatrix;
        QMatrix4x4 translationMatrix;
        if (-1 != bone.parent) {
            const auto &parentBone = m_bones[bone.parent];
            parentMatrix = bindTransforms[bone.parent];
            translationMatrix.translate(bone.headPosition - parentBone.headPosition);
        } else {
            translationMatrix.translate(bone.headPosition);
        }
        bindTransforms[i] = parentMatrix * translationMatrix;
    }
    
    const auto &vertebrataMotionFrames = vertebrataMotion->frames();
    for (size_t frameIndex = 0; frameIndex < vertebrataMotionFrames.size(); ++frameIndex) {
        const auto &frame = vertebrataMotionFrames[frameIndex];
        std::vector<RiggerBone> transformedBones = m_bones;
        for (const auto &node: frame) {
            if (-1 == node.boneIndex)
                continue;
            if (node.isTail) {
                transformedBones[node.boneIndex].tailPosition = node.position;
                for (const auto &childIndex: m_bones[node.boneIndex].children)
                    transformedBones[childIndex].headPosition = node.position;
            } else {
                transformedBones[node.boneIndex].headPosition = node.position;
                auto parentIndex = m_bones[node.boneIndex].parent;
                if (-1 != parentIndex) {
                    transformedBones[parentIndex].tailPosition = node.position;
                    for (const auto &childIndex: m_bones[parentIndex].children)
                        transformedBones[childIndex].headPosition = node.position;
                }
            }
        }
        
        std::vector<QMatrix4x4> poseTransforms(transformedBones.size());
        std::vector<QMatrix4x4> poseRotations(transformedBones.size());
        for (size_t i = 0; i < transformedBones.size(); ++i) {
            const auto &oldBone = m_bones[i];
            const auto &bone = transformedBones[i];
            QMatrix4x4 parentMatrix;
            QMatrix4x4 translationMatrix;
            QMatrix4x4 rotationMatrix;
            QMatrix4x4 parentRotation;
            if (-1 != bone.parent) {
                const auto &oldParentBone = m_bones[oldBone.parent];
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
        
        JointNodeTree jointNodeTree(&m_bones);
        for (size_t i = 0; i < m_bones.size(); ++i) {
            const auto &bone = transformedBones[i];
            if (-1 != bone.parent) {
                jointNodeTree.updateMatrix(i, poseTransforms[bone.parent].inverted() * poseTransforms[i]);
            } else {
                jointNodeTree.updateMatrix(i, poseTransforms[i]);
            }
        }
        
        jointNodeTrees.push_back({0.017f, jointNodeTree});
        
        const std::vector<JointNode> &jointNodes = jointNodeTree.nodes();
        std::vector<QMatrix4x4> jointNodeMatrices(m_bones.size());
        for (size_t i = 0; i < m_bones.size(); ++i) {
            const auto &bone = transformedBones[i];
            QMatrix4x4 translationMatrix;
            translationMatrix.translate(jointNodes[i].translation);
            QMatrix4x4 rotationMatrix;
            rotationMatrix.rotate(jointNodes[i].rotation);
            if (-1 != bone.parent) {
                jointNodeMatrices[i] *= jointNodeMatrices[bone.parent];
            }
            jointNodeMatrices[i] *= translationMatrix * rotationMatrix;
        }
        for (size_t i = 0; i < m_bones.size(); ++i)
            jointNodeMatrices[i] = jointNodeMatrices[i] * bindTransforms[i].inverted();
        
        std::vector<QVector3D> transformedVertices(m_object.vertices.size());
        for (size_t i = 0; i < m_object.vertices.size(); ++i) {
            const auto &weight = m_rigWeights[i];
            for (int x = 0; x < 4; x++) {
                float factor = weight.boneWeights[x];
                if (factor > 0) {
                    transformedVertices[i] += jointNodeMatrices[weight.boneIndices[x]] * m_object.vertices[i] * factor;
                }
            }
        }
        
        std::vector<QVector3D> frameVertices = transformedVertices;
        std::vector<std::vector<size_t>> frameFaces = m_object.triangles;
        std::vector<std::vector<QVector3D>> frameCornerNormals;
        const std::vector<std::vector<QVector3D>> *triangleVertexNormals = m_object.triangleVertexNormals();
        if (nullptr == triangleVertexNormals) {
            frameCornerNormals.resize(frameFaces.size());
            for (size_t i = 0; i < m_object.triangles.size(); ++i) {
                const auto &triangle = m_object.triangles[i];
                QVector3D triangleNormal = QVector3D::normal(
                    transformedVertices[triangle[0]],
                    transformedVertices[triangle[1]],
                    transformedVertices[triangle[2]]
                );
                frameCornerNormals[i] = {
                    triangleNormal, triangleNormal, triangleNormal
                };
            }
        } else {
            frameCornerNormals = *triangleVertexNormals;
        }
        
        if (m_snapshotMeshesEnabled) {
            if (frameIndex == vertebrataMotionFrames.size() / 2) {
                delete snapshotMesh;
                snapshotMesh = new Model(frameVertices, frameFaces, frameCornerNormals);
            }
        }
        
        if (m_previewMeshesEnabled) {
            BlockMesh blockMesh;
            blockMesh.addBlock(
                QVector3D(0.0, groundY + parameters.groundOffset, 0.0), 100.0,
                QVector3D(0.0, groundY + parameters.groundOffset - 0.02, 0.0), 100.0);
            for (const auto &bone: transformedBones) {
                if (0 == bone.index)
                    continue;
                blockMesh.addBlock(bone.headPosition, bone.headRadius * 0.5,
                    bone.tailPosition, bone.tailRadius * 0.5);
            }
            blockMesh.build();
            std::vector<QVector3D> *resultVertices = blockMesh.takeResultVertices();
            std::vector<std::vector<size_t>> *resultFaces = blockMesh.takeResultFaces();
            size_t oldVertexCount = frameVertices.size();
            for (const auto &v: *resultVertices)
                frameVertices.push_back(QVector3D(v.x() - 0.5, v.y(), v.z()));
            for (const auto &f: *resultFaces) {
                std::vector<size_t> newF = f;
                for (auto &v: newF)
                    v += oldVertexCount;
                frameFaces.push_back(newF);

                QVector3D triangleNormal = QVector3D::normal(
                    (*resultVertices)[f[0]],
                    (*resultVertices)[f[1]],
                    (*resultVertices)[f[2]]
                );
                frameCornerNormals.push_back({
                    triangleNormal, triangleNormal, triangleNormal
                });
            }
            delete resultFaces;
            delete resultVertices;
            
            previewMeshes.push_back({0.017f, new SimpleShaderMesh(
                new std::vector<QVector3D>(frameVertices), 
                new std::vector<std::vector<size_t>>(frameFaces),
                new std::vector<std::vector<QVector3D>>(frameCornerNormals))});
        }
    }
    
    if (m_previewMeshesEnabled)
        m_resultPreviewMeshes[motionId] = previewMeshes;
    m_resultJointNodeTrees[motionId] = jointNodeTrees;
    m_resultSnapshotMeshes[motionId] = snapshotMesh;
}

void MotionsGenerator::generate()
{
    for (const auto &it: m_motions) {
        generateMotion(it.first);
        m_generatedMotionIds.insert(it.first);
    }
}

void MotionsGenerator::process()
{
    QElapsedTimer countTimeConsumed;
    countTimeConsumed.start();
    
    generate();
    
    qDebug() << "The motions generation took" << countTimeConsumed.elapsed() << "milliseconds";
    
    emit finished();
}
