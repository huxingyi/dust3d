#include <QGuiApplication>
#include <QDebug>
#include <QElapsedTimer>
#include "riggenerator.h"
#include "rigger.h"

RigGenerator::RigGenerator(const Outcome &outcome) :
    m_outcome(new Outcome(outcome))
{
}

RigGenerator::~RigGenerator()
{
    delete m_outcome;
    delete m_resultMesh;
    delete m_autoRigger;
    delete m_resultBones;
    delete m_resultWeights;
}

Outcome *RigGenerator::takeOutcome()
{
    Outcome *outcome = m_outcome;
    m_outcome = nullptr;
    return outcome;
}

std::vector<RiggerBone> *RigGenerator::takeResultBones()
{
    std::vector<RiggerBone> *resultBones = m_resultBones;
    m_resultBones = nullptr;
    return resultBones;
}

std::map<int, RiggerVertexWeights> *RigGenerator::takeResultWeights()
{
    std::map<int, RiggerVertexWeights> *resultWeights = m_resultWeights;
    m_resultWeights = nullptr;
    return resultWeights;
}

MeshLoader *RigGenerator::takeResultMesh()
{
    MeshLoader *resultMesh = m_resultMesh;
    m_resultMesh = nullptr;
    return resultMesh;
}

bool RigGenerator::isSucceed()
{
    return m_isSucceed;
}

const std::vector<QString> &RigGenerator::missingMarkNames()
{
    return m_missingMarkNames;
}

const std::vector<QString> &RigGenerator::errorMarkNames()
{
    return m_errorMarkNames;
}

void RigGenerator::generate()
{
    if (nullptr == m_outcome->triangleSourceNodes())
        return;
    
    std::vector<QVector3D> inputVerticesPositions;
    std::set<MeshSplitterTriangle> inputTriangles;
    
    const auto &triangleSourceNodes = *m_outcome->triangleSourceNodes();
    const std::vector<std::vector<QVector3D>> *triangleVertexNormals = m_outcome->triangleVertexNormals();
    
    std::map<std::pair<QUuid, QUuid>, const OutcomeNode *> nodeMap;
    for (const auto &item: m_outcome->nodes) {
        nodeMap.insert({{item.partId, item.nodeId}, &item});
    }
    
    for (const auto &vertex: m_outcome->vertices) {
        inputVerticesPositions.push_back(vertex);
    }
    std::map<std::pair<BoneMark, SkeletonSide>, std::tuple<QVector3D, int, std::set<MeshSplitterTriangle>>> marksMap;
    for (size_t triangleIndex = 0; triangleIndex < m_outcome->triangles.size(); triangleIndex++) {
        const auto &sourceTriangle = m_outcome->triangles[triangleIndex];
        MeshSplitterTriangle newTriangle;
        for (int i = 0; i < 3; i++)
            newTriangle.indicies[i] = sourceTriangle[i];
        auto findBmeshNodeResult = nodeMap.find(triangleSourceNodes[triangleIndex]);
        if (findBmeshNodeResult != nodeMap.end()) {
            const auto &bmeshNode = *findBmeshNodeResult->second;
            if (bmeshNode.boneMark != BoneMark::None) {
                SkeletonSide boneSide = SkeletonSide::None;
                if (BoneMarkHasSide(bmeshNode.boneMark)) {
                    boneSide = bmeshNode.origin.x() > 0 ? SkeletonSide::Left : SkeletonSide::Right;
                }
                auto &marks = marksMap[std::make_pair(bmeshNode.boneMark, boneSide)];
                std::get<0>(marks) += bmeshNode.origin;
                std::get<1>(marks) += 1;
                std::get<2>(marks).insert(newTriangle);
            }
        }
        inputTriangles.insert(newTriangle);
    }
    m_autoRigger = new Rigger(inputVerticesPositions, inputTriangles);
    for (const auto &marks: marksMap) {
        m_autoRigger->addMarkGroup(marks.first.first, marks.first.second,
            std::get<0>(marks.second) / std::get<1>(marks.second),
            std::get<2>(marks.second));
    }
    m_isSucceed = m_autoRigger->rig();
    
    if (m_isSucceed) {
        qDebug() << "Rig succeed";
    } else {
        qDebug() << "Rig failed";
        m_missingMarkNames = m_autoRigger->missingMarkNames();
        m_errorMarkNames = m_autoRigger->errorMarkNames();
        for (const auto &message: m_autoRigger->messages()) {
            qDebug() << "errorType:" << message.first << "Message:" << message.second;
        }
    }
    
    // Blend vertices colors according to bone weights
    
    std::vector<QColor> inputVerticesColors(m_outcome->vertices.size());
    if (m_isSucceed) {
        const auto &resultWeights = m_autoRigger->resultWeights();
        const auto &resultBones = m_autoRigger->resultBones();
        
        m_resultWeights = new std::map<int, RiggerVertexWeights>;
        *m_resultWeights = resultWeights;
        
        m_resultBones = new std::vector<RiggerBone>;
        *m_resultBones = resultBones;
        
        for (const auto &weightItem: resultWeights) {
            size_t vertexIndex = weightItem.first;
            const auto &weight = weightItem.second;
            int blendR = 0, blendG = 0, blendB = 0;
            for (int i = 0; i < 4; i++) {
                int boneIndex = weight.boneIndicies[i];
                if (boneIndex > 0) {
                    const auto &bone = resultBones[boneIndex];
                    blendR += bone.color.red() * weight.boneWeights[i];
                    blendG += bone.color.green() * weight.boneWeights[i];
                    blendB += bone.color.blue() * weight.boneWeights[i];
                }
            }
            QColor blendColor = QColor(blendR, blendG, blendB, 255);
            inputVerticesColors[vertexIndex] = blendColor;
        }
    }
    
    // Create mesh for demo
    
    Vertex *triangleVertices = new Vertex[m_outcome->triangles.size() * 3];
    int triangleVerticesNum = 0;
    const QVector3D defaultUv = QVector3D(0, 0, 0);
    for (size_t triangleIndex = 0; triangleIndex < m_outcome->triangles.size(); triangleIndex++) {
        const auto &sourceTriangle = m_outcome->triangles[triangleIndex];
        for (int i = 0; i < 3; i++) {
            Vertex &currentVertex = triangleVertices[triangleVerticesNum++];
            const auto &sourcePosition = inputVerticesPositions[sourceTriangle[i]];
            const auto &sourceColor = inputVerticesColors[sourceTriangle[i]];
            const auto *sourceNormal = &defaultUv;
            if (nullptr != triangleVertexNormals)
                sourceNormal = &(*triangleVertexNormals)[triangleIndex][i];
            currentVertex.posX = sourcePosition.x();
            currentVertex.posY = sourcePosition.y();
            currentVertex.posZ = sourcePosition.z();
            currentVertex.texU = 0;
            currentVertex.texV = 0;
            currentVertex.colorR = sourceColor.redF();
            currentVertex.colorG = sourceColor.greenF();
            currentVertex.colorB = sourceColor.blueF();
            currentVertex.normX = sourceNormal->x();
            currentVertex.normY = sourceNormal->y();
            currentVertex.normZ = sourceNormal->z();
        }
    }
    m_resultMesh = new MeshLoader(triangleVertices, triangleVerticesNum);
}

void RigGenerator::process()
{
    QElapsedTimer countTimeConsumed;
    countTimeConsumed.start();
    
    generate();
    
    qDebug() << "The rig generation took" << countTimeConsumed.elapsed() << "milliseconds";
    
    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}
