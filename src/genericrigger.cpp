#include <QDebug>
#include <cmath>
#include <queue>
#include <unordered_set>
#include "genericrigger.h"

GenericRigger::GenericRigger(const std::vector<QVector3D> &verticesPositions,
        const std::set<MeshSplitterTriangle> &inputTriangles) :
    Rigger(verticesPositions, inputTriangles)
{
}

bool GenericRigger::validate()
{
    return true;
}

bool GenericRigger::isCutOffSplitter(BoneMark boneMark)
{
    return BoneMark::Limb == boneMark;
}

BoneMark GenericRigger::translateBoneMark(BoneMark boneMark)
{
    if (boneMark == BoneMark::Neck ||
            boneMark == BoneMark::Shoulder ||
            boneMark == BoneMark::Hip ||
            boneMark == BoneMark::Limb)
        return BoneMark::Limb;
    return BoneMark::Joint;
}

void GenericRigger::collectJointsForLimb(int markIndex, std::vector<int> &jointMarkIndicies)
{
    const auto &mark = m_marks[markIndex];
    
    jointMarkIndicies.push_back(markIndex);
    
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
        return;
    }
    
    const auto &group = mark.smallGroup();
    if (group.empty()) {
        qDebug() << "Collect joints for limb failed because of lack verticies";
        return;
    }
    
    // Build the edge to triangle map;
    std::map<std::pair<int, int>, const MeshSplitterTriangle *> edgeToTriangleMap;
    for (const auto &triangle: group) {
        for (int i = 0; i < 3; ++i) {
            int j = (i + 1) % 3;
            edgeToTriangleMap.insert({{triangle.indicies[i], triangle.indicies[j]}, &triangle});
        }
    }
    
    // Find startup triangles
    std::queue<const MeshSplitterTriangle *> waitTriangles;
    for (const auto &triangle: mark.markTriangles) {
        for (int i = 0; i < 3; i++) {
            int j = (i + 1) % 3;
            auto findOppositeTriangleResult = edgeToTriangleMap.find({triangle.indicies[j], triangle.indicies[i]});
            if (findOppositeTriangleResult == edgeToTriangleMap.end())
                continue;
            const MeshSplitterTriangle *startupTriangle = findOppositeTriangleResult->second;
            triangleToMarkMap.insert({*startupTriangle, markIndex});
            waitTriangles.push(startupTriangle);
        }
    }
    
    if (waitTriangles.empty()) {
        qDebug() << "Couldn't find a triangle to start";
        return;
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
            auto findOppositeTriangleResult = edgeToTriangleMap.find({triangle->indicies[j], triangle->indicies[i]});
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
            return;
        }
        jointMarkIndicies.push_back(nearestMarkIndex);
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
            jointMarkIndicies.push_back(linkTo);
            break;
        }
        if (-1 == linkTo)
            break;
        findPairResult = pairs.find(linkTo);
    }
}

bool GenericRigger::rig()
{
    if (!validate())
        return false;
    
    std::set<MeshSplitterTriangle> bodyTriangles;
    if (!calculateBodyTriangles(bodyTriangles))
        return false;
    
    std::set<int> bodyVerticies;
    bool isMainBodyVerticalAligned = false;
    addTrianglesToVertices(bodyTriangles, bodyVerticies);
    {
        QVector3D xMin, xMax, yMin, yMax, zMin, zMax;
        resolveBoundingBox(bodyVerticies, xMin, xMax, yMin, yMax, zMin, zMax);
        isMainBodyVerticalAligned = fabs(yMax.y() - yMin.y()) > fabs(zMax.z() - zMin.z());
    }
    
    // Collect all limbs
    auto nosideLimbIndicies = m_marksMap.find(std::make_pair(BoneMark::Limb, SkeletonSide::None));
    auto leftLimbIndicies = m_marksMap.find(std::make_pair(BoneMark::Limb, SkeletonSide::Left));
    auto rightLimbIndicies = m_marksMap.find(std::make_pair(BoneMark::Limb, SkeletonSide::Right));
    
    // Generate spine for main body
    auto sortLimbIndiciesInSpineOrder = [=](const int &first, const int &second) {
        return isMainBodyVerticalAligned ?
            (m_marks[first].bonePosition.y() < m_marks[second].bonePosition.y()) :
            (m_marks[first].bonePosition.z() < m_marks[second].bonePosition.z());
    };
    if (nosideLimbIndicies != m_marksMap.end())
        std::sort(nosideLimbIndicies->second.begin(), nosideLimbIndicies->second.end(), sortLimbIndiciesInSpineOrder);
    if (leftLimbIndicies != m_marksMap.end())
        std::sort(leftLimbIndicies->second.begin(), leftLimbIndicies->second.end(), sortLimbIndiciesInSpineOrder);
    if (rightLimbIndicies != m_marksMap.end())
        std::sort(rightLimbIndicies->second.begin(), rightLimbIndicies->second.end(), sortLimbIndiciesInSpineOrder);
    const static std::vector<int> s_empty;
    const std::vector<int> *limbColumns[3] = {leftLimbIndicies != m_marksMap.end() ? &leftLimbIndicies->second : &s_empty,
        nosideLimbIndicies != m_marksMap.end() ? &nosideLimbIndicies->second : &s_empty,
        rightLimbIndicies != m_marksMap.end() ? &rightLimbIndicies->second : &s_empty
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
        QVector3D position;
        std::set<int> limbMarkIndicies;
    };
    std::vector<SpineNode> spineNodes;
    for (size_t sideIndicies[3] = {0, 0, 0};
            sideIndicies[Column::Noside] < limbColumns[Column::Noside]->size() ||
            sideIndicies[Column::Left] < limbColumns[Column::Left]->size() ||
            sideIndicies[Column::Right] < limbColumns[Column::Right]->size();) {
        float choosenCoord = std::numeric_limits<float>::max();
        int choosenColumn = -1;
        for (size_t side = Column::Left; side <= Column::Right; ++side) {
            if (sideIndicies[side] < limbColumns[side]->size()) {
                const auto &mark = m_marks[limbColumns[side]->at(sideIndicies[side])];
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
        // Find all the limbs before or near this choosenCoord
        QVector3D sumOfLimbPositions;
        int countOfLimbs = 0;
        std::set<int> limbMarkIndicies;
        for (size_t side = Column::Left; side <= Column::Right; ++side) {
            if (sideIndicies[side] < limbColumns[side]->size()) {
                const auto &mark = m_marks[limbColumns[side]->at(sideIndicies[side])];
                const auto &coord = isMainBodyVerticalAligned ? mark.bonePosition.y() :
                        mark.bonePosition.z();
                if (coord <= choosenCoord + 0.001) {
                    limbMarkIndicies.insert(limbColumns[side]->at(sideIndicies[side]));
                    sumOfLimbPositions += mark.bonePosition;
                    ++countOfLimbs;
                    ++sideIndicies[side];
                }
            }
        }
        if (countOfLimbs <= 0) {
            qDebug() << "Should not come here, there must be at least one limb";
            break;
        }
        
        //qDebug() << "Create new spine node from" << countOfLimbs << "limbs current coord:" << choosenCoord;
        
        spineNodes.push_back(SpineNode());
        SpineNode &spineNode = spineNodes.back();
        spineNode.coord = choosenCoord;
        spineNode.limbMarkIndicies = limbMarkIndicies;
        spineNode.position = sumOfLimbPositions / countOfLimbs;
    }
    
    if (spineNodes.empty()) {
        qDebug() << "Couldn't find limbs to create a spine";
        return false;
    }
    
    std::map<QString, int> boneIndexMap;
    
    m_resultBones.push_back(RiggerBone());
    RiggerBone &bodyBone = m_resultBones.back();
    bodyBone.index = m_resultBones.size() - 1;
    bodyBone.name = "Body";
    bodyBone.headPosition = QVector3D(0, 0, 0);
    boneIndexMap[bodyBone.name] = bodyBone.index;
    
    auto remainingSpineVerticies = bodyVerticies;
    const std::vector<QColor> twoColorsForSpine = {QColor(0x57, 0x43, 0x98), Qt::white};
    const std::vector<QColor> twoColorsForLimb = {BoneMarkToColor(BoneMark::Joint), BoneMarkToColor(BoneMark::Hip)};
    int spineGenerateOrder = 1;
    for (int spineNodeIndex = 0; spineNodeIndex < (int)spineNodes.size(); ++spineNodeIndex) {
        const auto &spineNode = spineNodes[spineNodeIndex];
        std::set<int> spineBoneVertices;
        QVector3D tailPosition;
        
        int buttonRow = spineNodes.size() - spineGenerateOrder;
        
        if (spineNodeIndex + 1 < (int)spineNodes.size()) {
            std::set<int> frontOrCoincidentVertices;
            std::set<int> backVertices;
            float distance = (spineNodes[spineNodeIndex + 1].position - spineNode.position).length();
            auto planeNormal = (spineNodes[spineNodeIndex + 1].position - spineNode.position).normalized();
            auto pointOnPlane = spineNode.position + planeNormal * distance * 1.25;
            auto perpVector = isMainBodyVerticalAligned ? QVector3D(1, 0, 0) : QVector3D(0, 1, 0);
            auto vectorOnPlane = QVector3D::crossProduct(planeNormal, perpVector);
            // Move this point to far away, so the checking vector will not collapse with the plane normal
            pointOnPlane += vectorOnPlane.normalized() * 1000;
            splitVerticesByPlane(remainingSpineVerticies,
                pointOnPlane,
                planeNormal,
                frontOrCoincidentVertices,
                backVertices);
            spineBoneVertices = backVertices;
            // Split again, this time, we step back a little bit
            pointOnPlane = spineNode.position + planeNormal * distance * 0.85;
            pointOnPlane += vectorOnPlane.normalized() * 1000;
            splitVerticesByPlane(remainingSpineVerticies,
                pointOnPlane,
                planeNormal,
                frontOrCoincidentVertices,
                backVertices);
            remainingSpineVerticies = frontOrCoincidentVertices;
            tailPosition = spineNodes[spineNodeIndex + 1].position;
        } else {
            spineBoneVertices = remainingSpineVerticies;
            if (isMainBodyVerticalAligned) {
                tailPosition = findMaxY(spineBoneVertices);
            } else {
                tailPosition = findMaxZ(spineBoneVertices);
            }
        }
        
        QVector3D spineBoneHeadPosition = averagePosition(spineBoneVertices);
        if (isMainBodyVerticalAligned) {
            spineBoneHeadPosition.setY(spineNode.coord);
        } else {
            spineBoneHeadPosition.setZ(spineNode.coord);
        }
        
        m_resultBones.push_back(RiggerBone());
        RiggerBone &spineBone = m_resultBones.back();
        spineBone.index = m_resultBones.size() - 1;
        spineBone.name = "Spine" + QString::number(spineGenerateOrder);
        spineBone.headPosition = spineBoneHeadPosition;
        spineBone.tailPosition = tailPosition;
        spineBone.color = twoColorsForSpine[spineGenerateOrder % 2];
        spineBone.hasButton = true;
        spineBone.button = {buttonRow, 0};
        spineBone.buttonParameterType = RiggerButtonParameterType::PitchYawRoll;
        addVerticesToWeights(spineBoneVertices, spineBone.index);
        boneIndexMap[spineBone.name] = spineBone.index;
        
        //qDebug() << spineBone.name << "head:" << spineBone.headPosition << "tail:" << spineBone.tailPosition;
        
        if (1 == spineGenerateOrder) {
            m_resultBones[boneIndexMap["Body"]].tailPosition = spineBone.headPosition;
            m_resultBones[boneIndexMap["Body"]].children.push_back(spineBone.index);
        } else {
            m_resultBones[boneIndexMap["Spine" + QString::number(spineGenerateOrder - 1)]].tailPosition = spineBone.headPosition;
            m_resultBones[boneIndexMap["Spine" + QString::number(spineGenerateOrder - 1)]].children.push_back(spineBone.index);
        }
        
        int limbGenerateOrder = 1;
        for (const auto &limbMarkIndex: spineNode.limbMarkIndicies) {
            const auto &limbMark = m_marks[limbMarkIndex];
            
            m_resultBones.push_back(RiggerBone());
            RiggerBone &ribBone = m_resultBones.back();
            ribBone.index = m_resultBones.size() - 1;
            ribBone.name = "Rib" + QString::number(spineGenerateOrder) + "x" + QString::number(limbGenerateOrder);
            ribBone.headPosition = spineBoneHeadPosition;
            boneIndexMap[ribBone.name] = ribBone.index;
            if (1 == spineGenerateOrder) {
                m_resultBones[boneIndexMap["Body"]].children.push_back(ribBone.index);
            } else {
                m_resultBones[boneIndexMap["Spine" + QString::number(spineGenerateOrder)]].children.push_back(ribBone.index);
            }
            
            std::vector<int> jointMarkIndicies;
            collectJointsForLimb(limbMarkIndex, jointMarkIndicies);
            
            //qDebug() << "Limb markIndex:" << limbMarkIndex << " joints:" << jointMarkIndicies.size();
            
            int jointGenerateOrder = 1;
            
            auto boneColor = [&]() {
                return twoColorsForLimb[(jointGenerateOrder + 1) % 2];
            };
            auto boneColumn = [&]() {
                return limbMark.boneSide == SkeletonSide::Left ? jointGenerateOrder : -jointGenerateOrder;
            };
            auto addToParentBone = [&](QVector3D headPosition, SkeletonSide side, int boneIndex) {
                if (1 == jointGenerateOrder) {
                    m_resultBones[boneIndexMap["Rib" + QString::number(spineGenerateOrder) + "x" + QString::number(limbGenerateOrder)]].tailPosition = headPosition;
                    m_resultBones[boneIndexMap["Rib" + QString::number(spineGenerateOrder) + "x" + QString::number(limbGenerateOrder)]].children.push_back(boneIndex);
                } else {
                    QString parentLimbBoneName = namingLimb(spineGenerateOrder, side, limbGenerateOrder, jointGenerateOrder - 1);
                    m_resultBones[boneIndexMap[parentLimbBoneName]].tailPosition = headPosition;
                    m_resultBones[boneIndexMap[parentLimbBoneName]].children.push_back(boneIndex);
                }
            };
            
            std::set<int> remainingLimbVertices;
            addTrianglesToVertices(limbMark.smallGroup(), remainingLimbVertices);
            addTrianglesToVertices(limbMark.markTriangles, remainingLimbVertices);

            QVector3D lastPosition = spineBoneHeadPosition;
            for (const auto &jointMarkIndex: jointMarkIndicies) {
                const auto jointMark = m_marks[jointMarkIndex];
                int buttonColumn = boneColumn();
                m_resultBones.push_back(RiggerBone());
                RiggerBone &limbBone = m_resultBones.back();
                limbBone.index = m_resultBones.size() - 1;
                limbBone.name = namingLimb(spineGenerateOrder, jointMark.boneSide, limbGenerateOrder, jointGenerateOrder);
                limbBone.headPosition = jointMark.bonePosition;
                limbBone.baseNormal = jointMark.baseNormal;
                limbBone.color = boneColor();
                limbBone.hasButton = true;
                limbBone.button = {buttonRow, buttonColumn};
                limbBone.buttonParameterType = jointGenerateOrder == 1 ? RiggerButtonParameterType::PitchYawRoll : RiggerButtonParameterType::Intersection;
                if (jointGenerateOrder == (int)jointMarkIndicies.size()) {
                    // Calculate the tail position from remaining verticies
                    std::vector<QVector3D> extremCoords(6, jointMark.bonePosition);
                    resolveBoundingBox(remainingLimbVertices, extremCoords[0], extremCoords[1], extremCoords[2], extremCoords[3], extremCoords[4], extremCoords[5]);
                    float maxDistance2 = std::numeric_limits<float>::min();
                    QVector3D choosenExtreamCoord;
                    for (size_t i = 0; i < 6; ++i) {
                        const auto &position = extremCoords[i];
                        auto length2 = (position - jointMark.bonePosition).lengthSquared();
                        if (length2 >= maxDistance2) {
                            maxDistance2 = length2;
                            choosenExtreamCoord = position;
                        }
                    }
                    limbBone.tailPosition = choosenExtreamCoord;
                    addVerticesToWeights(remainingLimbVertices, limbBone.index);
                } else {
                    std::set<int> frontOrCoincidentVertices;
                    std::set<int> backVertices;
                    limbBone.tailPosition = m_marks[jointMarkIndicies[jointGenerateOrder]].bonePosition;
                    auto previousBoneDirection = (limbBone.headPosition - lastPosition).normalized();
                    auto currentBoneDirection = (limbBone.tailPosition - limbBone.headPosition).normalized();
                    auto planeNormal = (previousBoneDirection + currentBoneDirection).normalized();
                    float previousBoneLength = (limbBone.headPosition - lastPosition).length();
                    float currentBoneLength = (limbBone.tailPosition - limbBone.headPosition).length();
                    auto pointOnPlane = limbBone.tailPosition + currentBoneDirection * currentBoneLength * 0.25;
                    splitVerticesByPlane(remainingLimbVertices,
                        pointOnPlane,
                        planeNormal,
                        frontOrCoincidentVertices,
                        backVertices);
                    addVerticesToWeights(backVertices, limbBone.index);
                    pointOnPlane = limbBone.tailPosition - previousBoneDirection * previousBoneLength * 0.1 * (currentBoneLength / std::max(previousBoneLength, (float)0.00001));
                    splitVerticesByPlane(remainingLimbVertices,
                        pointOnPlane,
                        planeNormal,
                        frontOrCoincidentVertices,
                        backVertices);
                    remainingLimbVertices = frontOrCoincidentVertices;
                }
                
                boneIndexMap[limbBone.name] = limbBone.index;
                addToParentBone(limbBone.headPosition, jointMark.boneSide, limbBone.index);
                
                lastPosition = jointMark.bonePosition;
                
                ++jointGenerateOrder;
            }
            
            ++limbGenerateOrder;
        }
        
        ++spineGenerateOrder;
    }
    
    normalizeButtonColumns();
    
    // Finalize weights
    for (auto &weights: m_resultWeights) {
        weights.second.finalizeWeights();
    }
    
    return true;
}

void GenericRigger::normalizeButtonColumns()
{
    double minColumn = std::numeric_limits<int>::max();
    double maxColumn = std::numeric_limits<int>::min();
    for (const auto &bone: m_resultBones) {
        if (!bone.hasButton)
            continue;
        if (bone.button.second < minColumn)
            minColumn = bone.button.second;
        if (bone.button.second > maxColumn)
            maxColumn = bone.button.second;
    }
    int columnNumOfOneSide = (int)std::max(std::abs(minColumn), std::abs(maxColumn));
    for (auto &bone: m_resultBones) {
        if (!bone.hasButton)
            continue;
        bone.button.second += columnNumOfOneSide;
    }
}

QString GenericRigger::namingLimb(int spineOrder, SkeletonSide side, int limbOrder, int jointOrder)
{
    return SkeletonSideToDispName(side) + "Limb" + QString::number(spineOrder) + "x" + QString::number(jointOrder);
}
