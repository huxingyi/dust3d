#ifndef AUTO_RIGGER_H
#define AUTO_RIGGER_H
#include <QtGlobal>
#include <QVector3D>
#include <QObject>
#include <QColor>
#include <QDebug>
#include "meshsplitter.h"
#include "skeletonbonemark.h"
#include "rigtype.h"

class AutoRiggerMark
{
public:
    SkeletonBoneMark boneMark;
    SkeletonSide boneSide;
    QVector3D bonePosition;
    std::set<MeshSplitterTriangle> markTriangles;
    const std::set<MeshSplitterTriangle> &bigGroup() const
    {
        return m_firstGroup.size() > m_secondGroup.size() ? 
            m_firstGroup :
            m_secondGroup;
    }
    const std::set<MeshSplitterTriangle> &smallGroup() const
    {
        return m_firstGroup.size() > m_secondGroup.size() ?
            m_secondGroup :
            m_firstGroup;
    }
    bool split(const std::set<MeshSplitterTriangle> &input)
    {
        return MeshSplitter::split(input, markTriangles, m_firstGroup, m_secondGroup);
    }
private:
    std::set<MeshSplitterTriangle> m_firstGroup;
    std::set<MeshSplitterTriangle> m_secondGroup;
};

class AutoRiggerBone
{
public:
    QString name;
    int index;
    QVector3D headPosition;
    QVector3D tailPosition;
    QColor color;
    std::vector<int> children;
};

class AutoRiggerVertexWeights
{
public:
    int boneIndicies[4] = {0, 0, 0, 0};
    float boneWeights[4] = {0, 0, 0, 0};
    void addBone(int boneIndex, float distance)
    {
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
                boneIndicies[i] = item.first;
                boneWeights[i] = item.second / totalDistance;
            }
        } else {
            qDebug() << "totalDistance:" << totalDistance;
        }
    }
private:
    std::vector<std::pair<int, float>> m_boneRawWeights;
};

class AutoRigger : public QObject
{
public:
    AutoRigger(const std::vector<QVector3D> &verticesPositions,
        const std::set<MeshSplitterTriangle> &inputTriangles);
    bool addMarkGroup(SkeletonBoneMark boneMark, SkeletonSide boneSide, QVector3D bonePosition, 
        const std::set<MeshSplitterTriangle> &markTriangles);
    const std::vector<std::pair<QtMsgType, QString>> &messages();
    bool rig();
    const std::vector<AutoRiggerBone> &resultBones();
    const std::map<int, AutoRiggerVertexWeights> &resultWeights();
    const std::vector<QString> &missingMarkNames();
    const std::vector<QString> &errorMarkNames();
private:
    bool validate();
    void addTrianglesToVertices(const std::set<MeshSplitterTriangle> &triangles, std::set<int> &vertices);
    bool calculateBodyTriangles(std::set<MeshSplitterTriangle> &bodyTriangles);
    bool isCutOffSplitter(SkeletonBoneMark boneMark);
    void resolveBoundingBox(const std::set<int> &vertices, QVector3D &xMin, QVector3D &xMax, QVector3D &yMin, QVector3D &yMax, QVector3D &zMin, QVector3D &zMax);
    QVector3D findMinX(const std::set<int> &vertices);
    QVector3D findMaxX(const std::set<int> &vertices);
    QVector3D findMinY(const std::set<int> &vertices);
    QVector3D findMaxY(const std::set<int> &vertices);
    QVector3D findMinZ(const std::set<int> &vertices);
    QVector3D findMaxZ(const std::set<int> &vertices);
    void splitVerticesByY(const std::set<int> &vertices, float y, std::set<int> &greaterEqualThanVertices, std::set<int> &lessThanVertices);
    void splitVerticesByX(const std::set<int> &vertices, float x, std::set<int> &greaterEqualThanVertices, std::set<int> &lessThanVertices);
    void splitVerticesByZ(const std::set<int> &vertices, float z, std::set<int> &greaterEqualThanVertices, std::set<int> &lessThanVertices);
    void addVerticesToWeights(const std::set<int> &vertices, int boneIndex);
    std::vector<std::pair<QtMsgType, QString>> m_messages;
    std::vector<QVector3D> m_verticesPositions;
    std::set<MeshSplitterTriangle> m_inputTriangles;
    std::vector<AutoRiggerMark> m_marks;
    std::map<std::pair<SkeletonBoneMark, SkeletonSide>, std::vector<int>> m_marksMap;
    std::vector<AutoRiggerBone> m_resultBones;
    std::map<int, AutoRiggerVertexWeights> m_resultWeights;
    std::vector<QString> m_missingMarkNames;
    std::vector<QString> m_errorMarkNames;
};

#endif
