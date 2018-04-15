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
    std::vector<QString> partIdList;
public:
    void resolveBoundingBox(QRectF *mainProfile, QRectF *sideProfile, const QString &partId=QString());
};

#endif
