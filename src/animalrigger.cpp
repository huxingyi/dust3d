#include <QDebug>
#include <cmath>
#include <queue>
#include <unordered_set>
#include "animalrigger.h"

AnimalRigger::AnimalRigger(const std::vector<QVector3D> &verticesPositions,
        const std::set<MeshSplitterTriangle> &inputTriangles) :
    Rigger(verticesPositions, inputTriangles)
{
}

bool AnimalRigger::validate()
{
    if (m_marksMap.empty()) {
        m_messages.push_back(std::make_pair(QtCriticalMsg,
            tr("Please mark the neck, limbs and joints from the context menu")));
        return false;
    }
    
    return true;
}

bool AnimalRigger::isCutOffSplitter(BoneMark boneMark)
{
    return BoneMark::Joint != boneMark;
}

BoneMark AnimalRigger::translateBoneMark(BoneMark boneMark)
{
    return boneMark;
}

bool AnimalRigger::collectJontsForChain(int markIndex, std::vector<int> &jointMarkIndices)
{
    const auto &mark = m_marks[markIndex];
    
    jointMarkIndices.push_back(markIndex);
    
    // First insert joints, then the limb node,
    // because the limb node may contains the triangles of other joint becuase of expanding in split
    std::map<MeshSplitterTriangle, int> triangleToMarkMap;
    for (size_t i = 0; i < m_marks.size(); ++i) {
        const auto &item = m_marks[i];
        if (item.boneMark == BoneMark::Joint) {
            for (const auto &triangle: item.markTriangles)
                triangleToMarkMap.insert({triangle, i});
        }
    }
    for (const auto &triangle: mark.markTriangles)
        triangleToMarkMap.insert({triangle, markIndex});
    
    if (triangleToMarkMap.size() <= 1) {
        qDebug() << "Collect joints for limb failed because of lack marks";
        return true;
    }
    
    const auto &group = mark.smallGroup();
    if (group.empty()) {
        qDebug() << "Collect joints for limb failed because of lack verticies";
        return false;
    }
    
    // Build the edge to triangle map;
    std::map<std::pair<int, int>, const MeshSplitterTriangle *> edgeToTriangleMap;
    for (const auto &triangle: group) {
        for (int i = 0; i < 3; ++i) {
            int j = (i + 1) % 3;
            edgeToTriangleMap.insert({{triangle.indices[i], triangle.indices[j]}, &triangle});
        }
    }
    
    // Find startup triangles
    std::queue<const MeshSplitterTriangle *> waitTriangles;
    for (const auto &triangle: mark.markTriangles) {
        for (int i = 0; i < 3; i++) {
            int j = (i + 1) % 3;
            auto findOppositeTriangleResult = edgeToTriangleMap.find({triangle.indices[j], triangle.indices[i]});
            if (findOppositeTriangleResult == edgeToTriangleMap.end())
                continue;
            const MeshSplitterTriangle *startupTriangle = findOppositeTriangleResult->second;
            triangleToMarkMap.insert({*startupTriangle, markIndex});
            waitTriangles.push(startupTriangle);
        }
    }
    
    if (waitTriangles.empty()) {
        qDebug() << "Couldn't find a triangle to start";
        return false;
    }
    
    // Traverse all the triangles and fill the triangle to mark map
    std::unordered_set<const MeshSplitterTriangle *> processedTriangles;
    std::unordered_set<int> processedSourceMarks;
    while (!waitTriangles.empty()) {
        const MeshSplitterTriangle *triangle = waitTriangles.front();
        waitTriangles.pop();
        if (processedTriangles.find(triangle) != processedTriangles.end())
            continue;
        processedTriangles.insert(triangle);
        int sourceMark = -1;
        auto findTriangleSourceMarkResult = triangleToMarkMap.find(*triangle);
        if (findTriangleSourceMarkResult != triangleToMarkMap.end()) {
            sourceMark = findTriangleSourceMarkResult->second;
            processedSourceMarks.insert(sourceMark);
        }
        for (int i = 0; i < 3; i++) {
            int j = (i + 1) % 3;
            auto findOppositeTriangleResult = edgeToTriangleMap.find({triangle->indices[j], triangle->indices[i]});
            if (findOppositeTriangleResult == edgeToTriangleMap.end())
                continue;
            const MeshSplitterTriangle *neighborTriangle = findOppositeTriangleResult->second;
            auto findTriangleSourceMarkResult = triangleToMarkMap.find(*neighborTriangle);
            if (findTriangleSourceMarkResult == triangleToMarkMap.end()) {
                if (-1 != sourceMark)
                    triangleToMarkMap.insert({*neighborTriangle, sourceMark});
            } else {
                processedSourceMarks.insert(findTriangleSourceMarkResult->second);
            }
            waitTriangles.push(neighborTriangle);
        }
    }
    //qDebug() << "processedTriangles:" << processedTriangles.size() << "processedSourceMarks:" << processedSourceMarks.size();
    
    std::map<int, std::set<int>> pairs;
    std::set<std::pair<int, int>> processedEdges;
    for (const auto &edge: edgeToTriangleMap) {
        if (processedEdges.find(edge.first) != processedEdges.end())
            continue;
        std::pair<int, int> oppositeEdge = {edge.first.second, edge.first.first};
        processedEdges.insert(edge.first);
        processedEdges.insert(oppositeEdge);
        auto findOppositeTriangleResult = edgeToTriangleMap.find(oppositeEdge);
        if (findOppositeTriangleResult == edgeToTriangleMap.end())
            continue;
        const MeshSplitterTriangle *oppositeTriangle = findOppositeTriangleResult->second;
        auto findFirstTriangleMarkResult = triangleToMarkMap.find(*edge.second);
        if (findFirstTriangleMarkResult == triangleToMarkMap.end())
            continue;
        auto findSecondTriangleMarkResult = triangleToMarkMap.find(*oppositeTriangle);
        if (findSecondTriangleMarkResult == triangleToMarkMap.end())
            continue;
        if (findFirstTriangleMarkResult->second != findSecondTriangleMarkResult->second) {
            //qDebug() << "New pair added:" << findFirstTriangleMarkResult->second << findSecondTriangleMarkResult->second;
            pairs[findFirstTriangleMarkResult->second].insert(findSecondTriangleMarkResult->second);
            pairs[findSecondTriangleMarkResult->second].insert(findFirstTriangleMarkResult->second);
        }
    }
    
    std::set<int> visited;
    auto findPairResult = pairs.find(markIndex);
    visited.insert(markIndex);
    if (findPairResult == pairs.end() && processedSourceMarks.size() > 1) {
        // Couldn't find the limb node, we pick the nearest joint
        float minLength2 = std::numeric_limits<float>::max();
        int nearestMarkIndex = -1;
        for (const auto &item: pairs) {
            const auto &joint = m_marks[item.first];
            float length2 = (joint.bonePosition - mark.bonePosition).lengthSquared();
            if (length2 < minLength2) {
                nearestMarkIndex = item.first;
                minLength2 = length2;
            }
        }
        if (-1 == nearestMarkIndex) {
            qDebug() << "Find nearest joint failed";
            return false;
        }
        jointMarkIndices.push_back(nearestMarkIndex);
        visited.insert(nearestMarkIndex);
        findPairResult = pairs.find(nearestMarkIndex);
    }
    while (findPairResult != pairs.end()) {
        int linkTo = -1;
        for (const auto &item: findPairResult->second) {
            if (visited.find(item) != visited.end()) {
                continue;
            }
            linkTo = item;
            //qDebug() << "Link" << findPairResult->first << "to" << linkTo;
            visited.insert(linkTo);
            jointMarkIndices.push_back(linkTo);
            break;
        }
        if (-1 == linkTo)
            break;
        findPairResult = pairs.find(linkTo);
    }
    
    return true;
}

bool AnimalRigger::rig()
{
    if (!validate())
        return false;
    
    std::set<MeshSplitterTriangle> bodyTriangles;
    if (!calculateBodyTriangles(bodyTriangles))
        return false;
    
    std::set<int> bodyVerticies;
    bool isMainBodyVerticalAligned = false;
    float bodyLength = 0;
    addTrianglesToVertices(bodyTriangles, bodyVerticies);
    {
        QVector3D xMin, xMax, yMin, yMax, zMin, zMax;
        resolveBoundingBox(bodyVerticies, xMin, xMax, yMin, yMax, zMin, zMax);
        float yLength = fabs(yMax.y() - yMin.y());
        float zLength = fabs(zMax.z() - zMin.z());
        isMainBodyVerticalAligned = yLength > zLength;
        bodyLength = isMainBodyVerticalAligned ? yLength : zLength;
    }
    qDebug() << "isMainBodyVerticalAligned:" << isMainBodyVerticalAligned << "bodyLength:" << bodyLength;
    
    // Collect all branchs
    auto neckIndices = m_marksMap.find(std::make_pair(BoneMark::Neck, SkeletonSide::None));
    auto tailIndices = m_marksMap.find(std::make_pair(BoneMark::Tail, SkeletonSide::None));
    auto leftLimbIndices = m_marksMap.find(std::make_pair(BoneMark::Limb, SkeletonSide::Left));
    auto rightLimbIndices = m_marksMap.find(std::make_pair(BoneMark::Limb, SkeletonSide::Right));
    bool hasTail = false;
    std::vector<int> nosideMarkIndices;
    std::vector<int> leftMarkIndices;
    std::vector<int> righMarkIndices;
    if (neckIndices != m_marksMap.end()) {
        for (const auto &index: neckIndices->second)
            nosideMarkIndices.push_back(index);
    }
    if (tailIndices != m_marksMap.end()) {
        for (const auto &index: tailIndices->second)
            nosideMarkIndices.push_back(index);
        hasTail = true;
    }
    if (leftLimbIndices != m_marksMap.end()) {
        for (const auto &index: leftLimbIndices->second)
            leftMarkIndices.push_back(index);
    }
    if (rightLimbIndices != m_marksMap.end()) {
        for (const auto &index: rightLimbIndices->second)
            righMarkIndices.push_back(index);
    }
    
    // Generate spine for main body
    auto sortMarkIndicesInSpineOrder = [=](const int &first, const int &second) {
        return isMainBodyVerticalAligned ?
            (m_marks[first].bonePosition.y() < m_marks[second].bonePosition.y()) :
            (m_marks[first].bonePosition.z() < m_marks[second].bonePosition.z());
    };
    std::sort(nosideMarkIndices.begin(), nosideMarkIndices.end(), sortMarkIndicesInSpineOrder);
    std::sort(leftMarkIndices.begin(), leftMarkIndices.end(), sortMarkIndicesInSpineOrder);
    std::sort(righMarkIndices.begin(), righMarkIndices.end(), sortMarkIndicesInSpineOrder);
    const static std::vector<int> s_empty;
    const std::vector<int> *chainColumns[3] = {&leftMarkIndices,
        &nosideMarkIndices,
        &righMarkIndices
    };
    enum Column
    {
        Left = 0,
        Noside,
        Right
    };
    struct SpineNode
    {
        float coord;
        float radius;
        QVector3D position;
        std::set<int> chainMarkIndices;
    };
    std::vector<SpineNode> rawSpineNodes;
    for (size_t sideIndices[3] = {0, 0, 0};
            sideIndices[Column::Noside] < chainColumns[Column::Noside]->size() ||
            sideIndices[Column::Left] < chainColumns[Column::Left]->size() ||
            sideIndices[Column::Right] < chainColumns[Column::Right]->size();) {
        float choosenCoord = std::numeric_limits<float>::max();
        int choosenColumn = -1;
        for (size_t side = Column::Left; side <= Column::Right; ++side) {
            if (sideIndices[side] < chainColumns[side]->size()) {
                const auto &mark = m_marks[chainColumns[side]->at(sideIndices[side])];
                const auto &coord = isMainBodyVerticalAligned ? mark.bonePosition.y() :
                        mark.bonePosition.z();
                if (coord < choosenCoord) {
                    choosenCoord = coord;
                    choosenColumn = side;
                }
            }
        }
        if (-1 == choosenColumn) {
            qDebug() << "Should not come here, coord corrupted";
            break;
        }
        // Find all the chains before or near this choosenCoord
        QVector3D sumOfChainPositions;
        int countOfChains = 0;
        std::set<int> chainMarkIndices;
        std::vector<float> leftXs;
        std::vector<float> rightXs;
        std::vector<float> middleRadiusCollection;
        for (size_t side = Column::Left; side <= Column::Right; ++side) {
            if (sideIndices[side] < chainColumns[side]->size()) {
                const auto &mark = m_marks[chainColumns[side]->at(sideIndices[side])];
                const auto &coord = isMainBodyVerticalAligned ? mark.bonePosition.y() :
                        mark.bonePosition.z();
                if (coord <= choosenCoord + bodyLength / 5) {
                    chainMarkIndices.insert(chainColumns[side]->at(sideIndices[side]));
                    sumOfChainPositions += mark.bonePosition;
                    ++countOfChains;
                    ++sideIndices[side];
                    if (Column::Left == side)
                        leftXs.push_back(mark.bonePosition.x());
                    else if (Column::Right == side)
                        rightXs.push_back(mark.bonePosition.x());
                    else
                        middleRadiusCollection.push_back(mark.nodeRadius);
                }
            }
        }
        if (countOfChains <= 0) {
            qDebug() << "Should not come here, there must be at least one chain";
            break;
        }
        
        rawSpineNodes.push_back(SpineNode());
        SpineNode &spineNode = rawSpineNodes.back();
        spineNode.coord = choosenCoord;
        spineNode.radius = calculateSpineRadius(leftXs, rightXs, middleRadiusCollection);
        spineNode.chainMarkIndices = chainMarkIndices;
        spineNode.position = sumOfChainPositions / countOfChains;
    }
    
    if (rawSpineNodes.empty()) {
        qDebug() << "Couldn't find chain to create a spine";
        return false;
    }
    
    // Reassemble spine nodes, each spine will be cut off as two
    std::vector<SpineNode> spineNodes;
    for (size_t i = 0; i < rawSpineNodes.size(); ++i) {
        const auto &raw = rawSpineNodes[i];
        spineNodes.push_back(raw);
        if (i + 1 < rawSpineNodes.size()) {
            SpineNode intermediate;
            const auto &nextRaw = rawSpineNodes[i + 1];
            intermediate.coord = (raw.coord + nextRaw.coord) / 2;
            intermediate.radius = (raw.radius + nextRaw.radius) / 2;
            intermediate.position = (raw.position + nextRaw.position) / 2;
            spineNodes.push_back(intermediate);
        }
    }
    // Move the chain mark indices to the new generated intermediate spine
    for (size_t i = 2; i < spineNodes.size(); i += 2) {
        auto &spineNode = spineNodes[i];
        std::vector<int> needMoveIndices;
        for (const auto &markIndex: spineNode.chainMarkIndices) {
            const auto &chain = m_marks[markIndex];
            if (chain.boneSide != SkeletonSide::None)
                needMoveIndices.push_back(markIndex);
        }
        auto &previousSpineNode = spineNodes[i - 1];
        for (const auto &markIndex: needMoveIndices) {
            previousSpineNode.chainMarkIndices.insert(markIndex);
            spineNode.chainMarkIndices.erase(markIndex);
        }
    }
    
    std::map<QString, int> boneIndexMap;
    
    m_resultBones.push_back(RiggerBone());
    RiggerBone &bodyBone = m_resultBones.back();
    bodyBone.index = m_resultBones.size() - 1;
    bodyBone.name = Rigger::rootBoneName;
    bodyBone.headPosition = QVector3D(0, 0, 0);
    boneIndexMap[bodyBone.name] = bodyBone.index;
    
    auto remainingSpineVerticies = bodyVerticies;
    const std::vector<QColor> twoColorsForSpine = {BoneMarkToColor(BoneMark::Neck), BoneMarkToColor(BoneMark::Tail)};
    const std::vector<QColor> twoColorsForChain = {BoneMarkToColor(BoneMark::Joint), BoneMarkToColor(BoneMark::Limb)};
    int spineGenerateOrder = 1;
    std::map<std::pair<QString, SkeletonSide>, int> chainOrderMapBySide;
    for (int spineNodeIndex = 0; spineNodeIndex < (int)spineNodes.size(); ++spineNodeIndex) {
        const auto &spineNode = spineNodes[spineNodeIndex];
        std::set<int> spineBoneVertices;
        QVector3D tailPosition;
        float tailRadius = 0;

        if (spineNodeIndex + 1 < (int)spineNodes.size()) {
            float distance = (spineNodes[spineNodeIndex + 1].position - spineNode.position).length();
            QVector3D currentSpineDirection = (spineNodes[spineNodeIndex + 1].position - spineNode.position).normalized();
            QVector3D previousSpineDirection = currentSpineDirection;
            if (spineNodeIndex - 1 >= 0) {
                previousSpineDirection = (spineNodes[spineNodeIndex].position - spineNodes[spineNodeIndex - 1].position).normalized();
            }
            auto planeNormal = (currentSpineDirection + previousSpineDirection).normalized();
            auto pointOnPlane = spineNode.position + planeNormal * distance * 1.25;
            auto perpVector = isMainBodyVerticalAligned ? QVector3D(1, 0, 0) : QVector3D(0, 1, 0);
            auto vectorOnPlane = QVector3D::crossProduct(planeNormal, perpVector);
            // Move this point to far away, so the checking vector will not collapse with the plane normal
            pointOnPlane += vectorOnPlane.normalized() * 1000;
            {
                std::set<int> frontOrCoincidentVertices;
                std::set<int> backVertices;
                splitVerticesByPlane(remainingSpineVerticies,
                    pointOnPlane,
                    planeNormal,
                    frontOrCoincidentVertices,
                    backVertices);
                spineBoneVertices = backVertices;
            }
            // Split again, this time, we step back a little bit
            pointOnPlane = spineNode.position + planeNormal * distance * 0.85;
            pointOnPlane += vectorOnPlane.normalized() * 1000;
            {
                std::set<int> frontOrCoincidentVertices;
                std::set<int> backVertices;
                splitVerticesByPlane(remainingSpineVerticies,
                    pointOnPlane,
                    planeNormal,
                    frontOrCoincidentVertices,
                    backVertices);
                remainingSpineVerticies = frontOrCoincidentVertices;
            }
            tailPosition = spineNodes[spineNodeIndex + 1].position;
            tailRadius = spineNodes[spineNodeIndex + 1].radius;
        } else {
            spineBoneVertices = remainingSpineVerticies;
            tailPosition = findExtremPointFrom(spineBoneVertices, spineNode.position);
            tailRadius = spineNode.radius;
        }
        
        QVector3D spineBoneHeadPosition = spineNode.position;
        float spineBoneHeadRadius = spineNode.radius;
        QVector3D averagePoint = averagePosition(spineBoneVertices);
        if (isMainBodyVerticalAligned) {
            //qDebug() << "Update spine position's z from:" << spineBoneHeadPosition.z() << "to:" << averagePoint.z();
            spineBoneHeadPosition.setZ(averagePoint.z());
        } else {
            //qDebug() << "Update spine position's y from:" << spineBoneHeadPosition.y() << "to:" << averagePoint.y();
            spineBoneHeadPosition.setY(averagePoint.y());
        }
        
        QString spineName = namingSpine(spineGenerateOrder, hasTail);
        
        m_resultBones.push_back(RiggerBone());
        RiggerBone &spineBone = m_resultBones.back();
        spineBone.index = m_resultBones.size() - 1;
        spineBone.name = spineName;
        spineBone.headPosition = spineBoneHeadPosition;
        spineBone.headRadius = spineBoneHeadRadius;
        spineBone.tailPosition = tailPosition;
        spineBone.tailRadius = tailRadius;
        spineBone.color = twoColorsForSpine[spineGenerateOrder % 2];
        addVerticesToWeights(spineBoneVertices, spineBone.index);
        boneIndexMap[spineBone.name] = spineBone.index;
        
        //qDebug() << "Added spine:" << spineBone.name << "head:" << spineBone.headPosition << "tail:" << spineBone.tailPosition;
        
        if (1 == spineGenerateOrder) {
            m_resultBones[boneIndexMap[Rigger::rootBoneName]].tailPosition = spineBone.headPosition;
            m_resultBones[boneIndexMap[Rigger::rootBoneName]].children.push_back(spineBone.index);
        } else {
            m_resultBones[boneIndexMap[namingSpine(spineGenerateOrder - 1, hasTail)]].tailPosition = spineBone.headPosition;
            m_resultBones[boneIndexMap[namingSpine(spineGenerateOrder - 1, hasTail)]].children.push_back(spineBone.index);
        }
        
        for (const auto &chainMarkIndex: spineNode.chainMarkIndices) {
            const auto &chainMark = m_marks[chainMarkIndex];
            
            QString chainBaseName = BoneMarkToString(chainMark.boneMark);
            int chainGenerateOrder = ++chainOrderMapBySide[{chainBaseName, chainMark.boneSide}];
            QString chainName = namingChainPrefix(chainBaseName, chainMark.boneSide, chainGenerateOrder, spineNode.chainMarkIndices.size());
            
            m_resultBones.push_back(RiggerBone());
            RiggerBone &ribBone = m_resultBones.back();
            ribBone.index = m_resultBones.size() - 1;
            ribBone.name = namingConnector(spineName, chainName);
            ribBone.headPosition = spineBoneHeadPosition;
            ribBone.headRadius = spineBoneHeadRadius;
            //qDebug() << "Added connector:" << ribBone.name;
            boneIndexMap[ribBone.name] = ribBone.index;
            if (1 == spineGenerateOrder) {
                m_resultBones[boneIndexMap[Rigger::rootBoneName]].children.push_back(ribBone.index);
            } else {
                m_resultBones[boneIndexMap[namingSpine(spineGenerateOrder, hasTail)]].children.push_back(ribBone.index);
            }
            
            std::vector<int> jointMarkIndices;
            if (!collectJontsForChain(chainMarkIndex, jointMarkIndices)) {
                m_jointErrorItems.push_back(chainName);
            }
            
            //qDebug() << "Adding chain:" << chainName << " joints:" << jointMarkIndices.size();
            
            int jointGenerateOrder = 1;
            
            auto boneColor = [&]() {
                return twoColorsForChain[jointGenerateOrder % 2];
            };
            auto addToParentBone = [&](QVector3D headPosition, float headRadius, SkeletonSide side, int boneIndex) {
                if (1 == jointGenerateOrder) {
                    m_resultBones[boneIndexMap[namingConnector(spineName, chainName)]].tailPosition = headPosition;
                    m_resultBones[boneIndexMap[namingConnector(spineName, chainName)]].tailRadius = headRadius;
                    m_resultBones[boneIndexMap[namingConnector(spineName, chainName)]].children.push_back(boneIndex);
                } else {
                    QString parentLimbBoneName = namingChain(chainBaseName, side, chainGenerateOrder, spineNode.chainMarkIndices.size(), jointGenerateOrder - 1);
                    m_resultBones[boneIndexMap[parentLimbBoneName]].tailPosition = headPosition;
                    m_resultBones[boneIndexMap[parentLimbBoneName]].tailRadius = headRadius;
                    m_resultBones[boneIndexMap[parentLimbBoneName]].children.push_back(boneIndex);
                }
            };
            
            std::set<int> remainingLimbVertices;
            addTrianglesToVertices(chainMark.smallGroup(), remainingLimbVertices);
            addTrianglesToVertices(chainMark.markTriangles, remainingLimbVertices);
            
            std::vector<QVector3D> jointPositions;
            for (jointGenerateOrder = 1; jointGenerateOrder <= (int)jointMarkIndices.size(); ++jointGenerateOrder) {
                int jointMarkIndex = jointMarkIndices[jointGenerateOrder - 1];
                const auto jointMark = m_marks[jointMarkIndex];
                jointPositions.push_back(jointMark.bonePosition);
            }
            std::set<int> lastJointBoneVerticies;
            if (jointPositions.size() >= 2)
            {
                QVector3D cutoffPlaneNormal = (jointPositions[jointPositions.size() - 1] - jointPositions[jointPositions.size() - 2]).normalized();
                QVector3D pointOnPlane = jointPositions[jointPositions.size() - 1];
                std::set<int> frontOrCoincidentVertices;
                std::set<int> backVertices;
                splitVerticesByPlane(remainingLimbVertices,
                    pointOnPlane,
                    cutoffPlaneNormal,
                    frontOrCoincidentVertices,
                    backVertices);
                lastJointBoneVerticies = frontOrCoincidentVertices;
            } else {
                lastJointBoneVerticies = remainingLimbVertices;
            }
            // Calculate the tail position from remaining verticies
            jointPositions.push_back(findExtremPointFrom(lastJointBoneVerticies, jointPositions.back()));
            
            for (jointGenerateOrder = 1; jointGenerateOrder <= (int)jointMarkIndices.size(); ++jointGenerateOrder) {
                int jointMarkIndex = jointMarkIndices[jointGenerateOrder - 1];
                const auto &jointMark = m_marks[jointMarkIndex];
                m_resultBones.push_back(RiggerBone());
                RiggerBone &jointBone = m_resultBones.back();
                jointBone.index = m_resultBones.size() - 1;
                jointBone.name = namingChain(chainBaseName, chainMark.boneSide, chainGenerateOrder, spineNode.chainMarkIndices.size(), jointGenerateOrder);
                jointBone.headPosition = jointPositions[jointGenerateOrder - 1];
                jointBone.headRadius = jointMark.nodeRadius;
                jointBone.tailPosition = jointPositions[jointGenerateOrder];
                jointBone.color = boneColor();
                if (jointGenerateOrder == (int)jointMarkIndices.size()) {
                    addVerticesToWeights(remainingLimbVertices, jointBone.index);
                } else {
                    int nextJointMarkIndex = jointMarkIndices[jointGenerateOrder];
                    const auto &nextJointMark = m_marks[nextJointMarkIndex];
                    auto nextBoneDirection = (jointPositions[jointGenerateOrder + 1] - jointPositions[jointGenerateOrder]).normalized();
                    auto currentBoneDirection = (jointBone.tailPosition - jointBone.headPosition).normalized();
                    auto planeNormal = (currentBoneDirection + nextBoneDirection).normalized();
                    auto pointOnPlane = jointBone.tailPosition + planeNormal * nextJointMark.nodeRadius;
                    {
                        std::set<int> frontOrCoincidentVertices;
                        std::set<int> backVertices;
                        splitVerticesByPlane(remainingLimbVertices,
                            pointOnPlane,
                            planeNormal,
                            frontOrCoincidentVertices,
                            backVertices);
                        addVerticesToWeights(backVertices, jointBone.index);
                    }
                    pointOnPlane = jointBone.tailPosition - planeNormal * nextJointMark.nodeRadius;
                    {
                        std::set<int> frontOrCoincidentVertices;
                        std::set<int> backVertices;
                        splitVerticesByPlane(remainingLimbVertices,
                            pointOnPlane,
                            planeNormal,
                            frontOrCoincidentVertices,
                            backVertices);
                        remainingLimbVertices = frontOrCoincidentVertices;
                    }
                }
                
                boneIndexMap[jointBone.name] = jointBone.index;
                addToParentBone(jointBone.headPosition, jointBone.headRadius, chainMark.boneSide, jointBone.index);
            }
            
            ++chainGenerateOrder;
        }
        
        ++spineGenerateOrder;
    }

    // Finalize weights
    for (auto &weights: m_resultWeights) {
        weights.second.finalizeWeights();
    }
    
    return true;
}

float AnimalRigger::calculateSpineRadius(const std::vector<float> &leftXs,
        const std::vector<float> &rightXs,
        const std::vector<float> &middleRadiusCollection)
{
    float leftX = 0;
    if (!leftXs.empty())
        leftX = std::accumulate(leftXs.begin(), leftXs.end(), 0.0) / leftXs.size();
    
    float rightX = 0;
    if (!rightXs.empty())
        rightX = std::accumulate(rightXs.begin(), rightXs.end(), 0.0) / rightXs.size();
    
    float limbSpanRadius = qAbs(leftX - rightX) * 0.5;
    
    float middleRadius = 0;
    if (!middleRadiusCollection.empty()) {
        middleRadius = std::accumulate(middleRadiusCollection.begin(),
            middleRadiusCollection.end(), 0.0) / middleRadiusCollection.size();
    }
    
    std::vector<float> radiusCollection;
    if (!qFuzzyIsNull(limbSpanRadius))
        radiusCollection.push_back(limbSpanRadius);
    if (!qFuzzyIsNull(middleRadius))
        radiusCollection.push_back(middleRadius);
    
    if (radiusCollection.empty())
        return 0.0;
    
    return std::accumulate(radiusCollection.begin(), radiusCollection.end(), 0.0) / radiusCollection.size();
}

QVector3D AnimalRigger::findExtremPointFrom(const std::set<int> &verticies, const QVector3D &from)
{
    std::vector<QVector3D> extremCoords(6, from);
    resolveBoundingBox(verticies, extremCoords[0], extremCoords[1], extremCoords[2], extremCoords[3], extremCoords[4], extremCoords[5]);
    float maxDistance2 = std::numeric_limits<float>::min();
    QVector3D choosenExtreamCoord = from;
    for (size_t i = 0; i < 6; ++i) {
        const auto &position = extremCoords[i];
        auto length2 = (position - from).lengthSquared();
        if (length2 >= maxDistance2) {
            maxDistance2 = length2;
            choosenExtreamCoord = position;
        }
    }
    return choosenExtreamCoord;
}

QString AnimalRigger::namingChain(const QString &baseName, SkeletonSide side, int orderInSide, int totalInSide, int jointOrder)
{
    return namingChainPrefix(baseName, side, orderInSide, totalInSide) + "_Joint" + QString::number(jointOrder);
}

QString AnimalRigger::namingSpine(int spineOrder, bool hasTail)
{
    if (hasTail) {
        if (spineOrder <= 2) {
            return "Spine0" + QString::number(spineOrder);
        }
        spineOrder -= 2;
    }
    return "Spine" + QString::number(spineOrder);
}

QString AnimalRigger::namingConnector(const QString &spineName, const QString &chainName)
{
    return "Virtual_" + spineName + "_" + chainName;
}

QString AnimalRigger::namingChainPrefix(const QString &baseName, SkeletonSide side, int orderInSide, int totalInSide)
{
    return SkeletonSideToString(side) + baseName + (totalInSide == 1 ? QString() : QString::number(orderInSide));
}
