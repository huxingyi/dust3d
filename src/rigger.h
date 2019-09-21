#ifndef DUST3D_RIGGER_H
#define DUST3D_RIGGER_H
#include <QtGlobal>
#include <QVector3D>
#include <QObject>
#include <QColor>
#include <QDebug>
#include <QVector2D>
#include "meshsplitter.h"
#include "bonemark.h"
#include "rigtype.h"
#include "skeletonside.h"

class RiggerMark
{
public:
    BoneMark boneMark;
    SkeletonSide boneSide;
    QVector3D bonePosition;
    float nodeRadius = 0;
    std::set<MeshSplitterTriangle> markTriangles;
    const std::set<MeshSplitterTriangle> &bigGroup() const
    {
        return m_isFirstBiggerThenSecond ? m_firstGroup : m_secondGroup;
    }
    const std::set<MeshSplitterTriangle> &smallGroup() const
    {
        return m_isFirstBiggerThenSecond ? m_secondGroup : m_firstGroup;
    }
    bool split(const std::vector<QVector3D> &verticesPositions, const std::set<MeshSplitterTriangle> &input, int expandRound=0)
    {
        int totalRound = 1 + expandRound;
        for (int round = 0; round < totalRound; ++round) {
            m_firstGroup.clear();
            m_secondGroup.clear();
            bool splitResult = MeshSplitter::split(input, markTriangles, m_firstGroup, m_secondGroup, round > 0);
            if (splitResult) {
                sortByDistanceFromOrigin(verticesPositions);
                return true;
            }
        }
        return false;
    }
private:
    std::set<MeshSplitterTriangle> m_firstGroup;
    std::set<MeshSplitterTriangle> m_secondGroup;
    bool m_isFirstBiggerThenSecond = false;
    
    float minDistance2FromOrigin(const std::vector<QVector3D> &verticesPositions, const std::set<MeshSplitterTriangle> &triangles)
    {
        float minDistance2 = std::numeric_limits<float>::max();
        for (const auto &triangle: triangles) {
            for (size_t i = 0; i < 3; ++i) {
                float distance2 = verticesPositions[triangle.indices[i]].lengthSquared();
                if (distance2 < minDistance2)
                    minDistance2 = distance2;
            }
        }
        return minDistance2;
    }
    
    void sortByDistanceFromOrigin(const std::vector<QVector3D> &verticesPositions)
    {
        m_isFirstBiggerThenSecond = minDistance2FromOrigin(verticesPositions, m_firstGroup) <
            minDistance2FromOrigin(verticesPositions, m_secondGroup);
    }
};

class RiggerBone
{
public:
    QString name;
    int index = -1;
    QVector3D headPosition;
    QVector3D tailPosition;
    float headRadius = 0.0;
    float tailRadius = 0.0;
    QColor color;
    QVector3D baseNormal;
    std::vector<int> children;
};

class RiggerVertexWeights
{
public:
    int boneIndices[4] = {0, 0, 0, 0};
    float boneWeights[4] = {0, 0, 0, 0};
    void addBone(int boneIndex, float distance)
    {
        if (m_boneRawIndices.find(boneIndex) != m_boneRawIndices.end())
            return;
        m_boneRawIndices.insert(boneIndex);
        if (qFuzzyIsNull(distance))
            distance = 0.0001;
        m_boneRawWeights.push_back(std::make_pair(boneIndex, 1.0 / distance));
    }
    void finalizeWeights()
    {
        std::sort(m_boneRawWeights.begin(), m_boneRawWeights.end(),
                [](const std::pair<int, float> &a, const std::pair<int, float> &b) {
            return a.second > b.second;
        });
        float totalDistance = 0;
        for (size_t i = 0; i < m_boneRawWeights.size() && i < 4; i++) {
            const auto &item = m_boneRawWeights[i];
            totalDistance += item.second;
        }
        if (totalDistance > 0) {
            for (size_t i = 0; i < m_boneRawWeights.size() && i < 4; i++) {
                const auto &item = m_boneRawWeights[i];
                boneIndices[i] = item.first;
                boneWeights[i] = item.second / totalDistance;
            }
        } else {
            qDebug() << "totalDistance:" << totalDistance;
        }
    }
private:
    std::set<int> m_boneRawIndices;
    std::vector<std::pair<int, float>> m_boneRawWeights;
};

class Rigger : public QObject
{
    Q_OBJECT
public:
    Rigger(const std::vector<QVector3D> &verticesPositions,
        const std::set<MeshSplitterTriangle> &inputTriangles);
    bool addMarkGroup(BoneMark boneMark, SkeletonSide boneSide, QVector3D bonePosition, float nodeRadius, const std::set<MeshSplitterTriangle> &markTriangles);
    const std::vector<std::pair<QtMsgType, QString>> &messages();
    const std::vector<RiggerBone> &resultBones();
    const std::map<int, RiggerVertexWeights> &resultWeights();
    virtual bool rig() = 0;
    static QString rootBoneName;
    //static QString firstSpineBoneName;
protected:
    virtual bool validate() = 0;
    virtual bool isCutOffSplitter(BoneMark boneMark) = 0;
    virtual BoneMark translateBoneMark(BoneMark boneMark);
    void addTrianglesToVertices(const std::set<MeshSplitterTriangle> &triangles, std::set<int> &vertices);
    bool calculateBodyTriangles(std::set<MeshSplitterTriangle> &bodyTriangles);
    void resolveBoundingBox(const std::set<int> &vertices, QVector3D &xMin, QVector3D &xMax, QVector3D &yMin, QVector3D &yMax, QVector3D &zMin, QVector3D &zMax);
    QVector3D findMinX(const std::set<int> &vertices);
    QVector3D findMaxX(const std::set<int> &vertices);
    QVector3D findMinY(const std::set<int> &vertices);
    QVector3D findMaxY(const std::set<int> &vertices);
    QVector3D findMinZ(const std::set<int> &vertices);
    QVector3D findMaxZ(const std::set<int> &vertices);
    QVector3D averagePosition(const std::set<int> &vertices);
    void splitVerticesByY(const std::set<int> &vertices, float y, std::set<int> &greaterEqualThanVertices, std::set<int> &lessThanVertices);
    void splitVerticesByX(const std::set<int> &vertices, float x, std::set<int> &greaterEqualThanVertices, std::set<int> &lessThanVertices);
    void splitVerticesByZ(const std::set<int> &vertices, float z, std::set<int> &greaterEqualThanVertices, std::set<int> &lessThanVertices);
    void splitVerticesByPlane(const std::set<int> &vertices, const QVector3D &pointOnPlane, const QVector3D &planeNormal, std::set<int> &frontOrCoincidentVertices, std::set<int> &backVertices);
    void addVerticesToWeights(const std::set<int> &vertices, int boneIndex);
    std::vector<std::pair<QtMsgType, QString>> m_messages;
    std::vector<QVector3D> m_verticesPositions;
    std::set<MeshSplitterTriangle> m_inputTriangles;
    std::vector<RiggerMark> m_marks;
    std::map<std::pair<BoneMark, SkeletonSide>, std::vector<int>> m_marksMap;
    std::vector<RiggerBone> m_resultBones;
    std::map<int, RiggerVertexWeights> m_resultWeights;
    std::vector<QString> m_cutoffErrorItems;
    std::vector<QString> m_jointErrorItems;
    static size_t m_maxCutOffSplitterExpandRound;
    bool m_extraMessagedAdded;
};

#endif
