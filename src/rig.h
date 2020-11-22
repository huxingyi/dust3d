#ifndef DUST3D_RIG_H
#define DUST3D_RIG_H
#include <QtGlobal>
#include <QVector3D>
#include <QObject>
#include <QColor>
#include <QDebug>
#include <QVector2D>
#include <set>
#include "bonemark.h"
#include "rigtype.h"
#include "skeletonside.h"

#define MAX_WEIGHT_NUM  4

class RigBone
{
public:
    QString name;
    int index = -1;
    int parent = -1;
    QVector3D headPosition;
    QVector3D tailPosition;
    float headRadius = 0.0;
    float tailRadius = 0.0;
    QColor color;
    std::map<QString, QString> attributes;
    std::vector<int> children;
};

class RigVertexWeights
{
public:
    int boneIndices[MAX_WEIGHT_NUM] = {0};
    float boneWeights[MAX_WEIGHT_NUM] = {0};
    void addBone(int boneIndex, float weight)
    {
        for (auto &it: m_boneRawWeights) {
            if (it.first == boneIndex) {
                it.second += weight;
                return;
            }
        }
        m_boneRawWeights.push_back(std::make_pair(boneIndex, weight));
    }
    void finalizeWeights()
    {
        std::sort(m_boneRawWeights.begin(), m_boneRawWeights.end(),
                [](const std::pair<int, float> &a, const std::pair<int, float> &b) {
            return a.second > b.second;
        });
        float totalWeight = 0;
        for (size_t i = 0; i < m_boneRawWeights.size() && i < MAX_WEIGHT_NUM; i++) {
            const auto &item = m_boneRawWeights[i];
            totalWeight += item.second;
        }
        if (totalWeight > 0) {
            for (size_t i = 0; i < m_boneRawWeights.size() && i < MAX_WEIGHT_NUM; i++) {
                const auto &item = m_boneRawWeights[i];
                boneIndices[i] = item.first;
                boneWeights[i] = item.second / totalWeight;
            }
        }
    }
    const std::vector<std::pair<int, float>> &boneRawWeights() const
    {
        return m_boneRawWeights;
    }
private:
    std::vector<std::pair<int, float>> m_boneRawWeights;
};

#endif
