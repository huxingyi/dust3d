#include <QDebug>
#include <cmath>
#include "theme.h"
#include "bonemark.h"
#include "skeletonside.h"
#include "rigger.h"

size_t Rigger::m_maxCutOffSplitterExpandRound = 3;

Rigger::Rigger(const std::vector<QVector3D> &verticesPositions,
        const std::set<MeshSplitterTriangle> &inputTriangles) :
    m_verticesPositions(verticesPositions),
    m_inputTriangles(inputTriangles)
{
}

BoneMark Rigger::translateBoneMark(BoneMark boneMark)
{
    return boneMark;
}

bool Rigger::calculateBodyTriangles(std::set<MeshSplitterTriangle> &bodyTriangles)
{
    bodyTriangles = m_inputTriangles;
    for (const auto &marksMapIt: m_marksMap) {
        if (isCutOffSplitter(marksMapIt.first.first)) {
            for (const auto index: marksMapIt.second) {
                auto &mark = m_marks[index];
                std::set<MeshSplitterTriangle> intersection;
                std::set_intersection(bodyTriangles.begin(), bodyTriangles.end(),
                    mark.bigGroup().begin(), mark.bigGroup().end(),
                    std::insert_iterator<std::set<MeshSplitterTriangle>>(intersection, intersection.begin()));
                bodyTriangles = intersection;
            }
        }
    }
    if (bodyTriangles.empty()) {
        m_messages.push_back(std::make_pair(QtCriticalMsg,
            tr("Calculate body from marks failed")));
        return false;
    }
    return true;
}

bool Rigger::addMarkGroup(BoneMark boneMark, SkeletonSide boneSide, QVector3D bonePosition, float nodeRadius, QVector3D baseNormal,
        const std::set<MeshSplitterTriangle> &markTriangles)
{
    m_marks.push_back(RiggerMark());

    RiggerMark &mark = m_marks.back();
    mark.boneMark = translateBoneMark(boneMark);
    mark.boneSide = boneSide;
    mark.bonePosition = bonePosition;
    mark.nodeRadius = nodeRadius;
    mark.baseNormal = baseNormal;
    mark.markTriangles = markTriangles;
    
    if (isCutOffSplitter(mark.boneMark)) {
        if (!mark.split(m_inputTriangles, m_maxCutOffSplitterExpandRound)) {
            m_marksMap[std::make_pair(mark.boneMark, mark.boneSide)].push_back(m_marks.size() - 1);
            m_errorMarkNames.push_back(SkeletonSideToDispName(mark.boneSide) + " " + BoneMarkToDispName(mark.boneMark));
            m_messages.push_back(std::make_pair(QtCriticalMsg,
                tr("Mark \"%1 %2\" couldn't cut off the mesh").arg(SkeletonSideToDispName(mark.boneSide)).arg(BoneMarkToString(boneMark))));
            return false;
        }
    }
    
    m_marksMap[std::make_pair(mark.boneMark, mark.boneSide)].push_back(m_marks.size() - 1);

    return true;
}

const std::vector<std::pair<QtMsgType, QString>> &Rigger::messages()
{
    return m_messages;
}

void Rigger::addTrianglesToVertices(const std::set<MeshSplitterTriangle> &triangles, std::set<int> &vertices)
{
    for (const auto &triangle: triangles) {
        for (int i = 0; i < 3; i++) {
            vertices.insert(triangle.indicies[i]);
        }
    }
}

void Rigger::resolveBoundingBox(const std::set<int> &vertices, QVector3D &xMin, QVector3D &xMax, QVector3D &yMin, QVector3D &yMax, QVector3D &zMin, QVector3D &zMax)
{
    bool leftFirstTime = true;
    bool rightFirstTime = true;
    bool topFirstTime = true;
    bool bottomFirstTime = true;
    bool zLeftFirstTime = true;
    bool zRightFirstTime = true;
    for (const auto &index: vertices) {
        const auto &position = m_verticesPositions[index];
        const float &x = position.x();
        const float &y = position.y();
        const float &z = position.z();
        if (leftFirstTime || x < xMin.x()) {
            xMin = position;
            leftFirstTime = false;
        }
        if (topFirstTime || y < yMin.y()) {
            yMin = position;
            topFirstTime = false;
        }
        if (rightFirstTime || x > xMax.x()) {
            xMax = position;
            rightFirstTime = false;
        }
        if (bottomFirstTime || y > yMax.y()) {
            yMax = position;
            bottomFirstTime = false;
        }
        if (zLeftFirstTime || z < zMin.z()) {
            zMin = position;
            zLeftFirstTime = false;
        }
        if (zRightFirstTime || z > zMax.z()) {
            zMax = position;
            zRightFirstTime = false;
        }
    }
}

QVector3D Rigger::findMinX(const std::set<int> &vertices)
{
    QVector3D minX, minY, minZ, maxX, maxY, maxZ;
    resolveBoundingBox(vertices, minX, maxX, minY, maxY, minZ, maxZ);
    return minX;
}

QVector3D Rigger::findMaxX(const std::set<int> &vertices)
{
    QVector3D minX, minY, minZ, maxX, maxY, maxZ;
    resolveBoundingBox(vertices, minX, maxX, minY, maxY, minZ, maxZ);
    return maxX;
}

QVector3D Rigger::findMinY(const std::set<int> &vertices)
{
    QVector3D minX, minY, minZ, maxX, maxY, maxZ;
    resolveBoundingBox(vertices, minX, maxX, minY, maxY, minZ, maxZ);
    return minY;
}

QVector3D Rigger::findMaxY(const std::set<int> &vertices)
{
    QVector3D minX, minY, minZ, maxX, maxY, maxZ;
    resolveBoundingBox(vertices, minX, maxX, minY, maxY, minZ, maxZ);
    return maxY;
}

QVector3D Rigger::findMinZ(const std::set<int> &vertices)
{
    QVector3D minX, minY, minZ, maxX, maxY, maxZ;
    resolveBoundingBox(vertices, minX, maxX, minY, maxY, minZ, maxZ);
    return minZ;
}

QVector3D Rigger::findMaxZ(const std::set<int> &vertices)
{
    QVector3D minX, minY, minZ, maxX, maxY, maxZ;
    resolveBoundingBox(vertices, minX, maxX, minY, maxY, minZ, maxZ);
    return maxZ;
}

QVector3D Rigger::averagePosition(const std::set<int> &vertices)
{
    if (vertices.empty())
        return QVector3D();
    
    QVector3D sum;
    for (const auto &index: vertices) {
        const auto &position = m_verticesPositions[index];
        sum += position;
    }
    return sum / vertices.size();
}

void Rigger::splitVerticesByY(const std::set<int> &vertices, float y, std::set<int> &greaterEqualThanVertices, std::set<int> &lessThanVertices)
{
    for (const auto &index: vertices) {
        const auto &position = m_verticesPositions[index];
        if (position.y() >= y)
            greaterEqualThanVertices.insert(index);
        else
            lessThanVertices.insert(index);
    }
}

void Rigger::splitVerticesByX(const std::set<int> &vertices, float x, std::set<int> &greaterEqualThanVertices, std::set<int> &lessThanVertices)
{
    for (const auto &index: vertices) {
        const auto &position = m_verticesPositions[index];
        if (position.x() >= x)
            greaterEqualThanVertices.insert(index);
        else
            lessThanVertices.insert(index);
    }
}

void Rigger::splitVerticesByZ(const std::set<int> &vertices, float z, std::set<int> &greaterEqualThanVertices, std::set<int> &lessThanVertices)
{
    for (const auto &index: vertices) {
        const auto &position = m_verticesPositions[index];
        if (position.z() >= z)
            greaterEqualThanVertices.insert(index);
        else
            lessThanVertices.insert(index);
    }
}

void Rigger::splitVerticesByPlane(const std::set<int> &vertices, const QVector3D &pointOnPlane, const QVector3D &planeNormal, std::set<int> &frontOrCoincidentVertices, std::set<int> &backVertices)
{
    for (const auto &index: vertices) {
        const auto &position = m_verticesPositions[index];
        auto line = position - pointOnPlane;
        auto dot = QVector3D::dotProduct(line, planeNormal);
        if (dot >= 0)
            frontOrCoincidentVertices.insert(index);
        else
            backVertices.insert(index);
    }
}

const std::vector<RiggerBone> &Rigger::resultBones()
{
    return m_resultBones;
}

const std::map<int, RiggerVertexWeights> &Rigger::resultWeights()
{
    return m_resultWeights;
}

void Rigger::addVerticesToWeights(const std::set<int> &vertices, int boneIndex)
{
    for (const auto &vertexIndex: vertices) {
        auto &weights = m_resultWeights[vertexIndex];
        auto strongestPoint = (m_resultBones[boneIndex].headPosition * 3 + m_resultBones[boneIndex].tailPosition) / 4;
        float distance = m_verticesPositions[vertexIndex].distanceToPoint(strongestPoint);
        weights.addBone(boneIndex, distance);
    }
}

const std::vector<QString> &Rigger::missingMarkNames()
{
    return m_missingMarkNames;
}

const std::vector<QString> &Rigger::errorMarkNames()
{
    return m_errorMarkNames;
}
