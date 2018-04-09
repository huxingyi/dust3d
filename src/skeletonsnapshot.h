#ifndef SKELETON_SNAPSHOT_H
#define SKELETON_SNAPSHOT_H
#include <vector>
#include <map>
#include <QString>
#include <QRectF>
#include <QSizeF>

class SkeletonSnapshot
{
public:
    std::map<QString, QString> canvas;
    std::map<QString, std::map<QString, QString>> nodes;
    std::map<QString, std::map<QString, QString>> edges;
    std::map<QString, std::map<QString, QString>> parts;
    std::map<QString, std::map<QString, QString>> mirrors;
    std::map<QString, std::map<QString, QString>> trivialMarkers;
    std::vector<QString> partIdList;
public:
    SkeletonSnapshot();
    void splitByConnectivity(std::vector<SkeletonSnapshot> *groups);
    QString rootNode();
    void setRootNode(const QString &nodeName);
    const QRectF &boundingBoxFront();
    const QRectF &boundingBoxSide();
private:
    QString m_rootNode;
    QRectF m_boundingBoxFront;
    QRectF m_boundingBoxSide;
    bool m_boundingBoxResolved;
    bool m_rootNodeResolved;
private:
    void resolveBoundingBox();
    void resolveRootNode();
    QString findMaxNeighborNumberNode();
};

#endif
