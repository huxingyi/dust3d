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
    std::map<QString, std::map<QString, QString>> components;
    std::map<QString, QString> rootComponent;
    std::vector<std::pair<std::map<QString, QString>, std::map<QString, std::map<QString, QString>>>> poses; // std::pair<Pose attributes, Bone attributes>
    std::vector<std::pair<std::map<QString, QString>, std::vector<std::map<QString, QString>>>> motions; // std::pair<Motion attributes, clips>
    std::vector<std::pair<std::map<QString, QString>, std::vector<std::pair<std::map<QString, QString>, std::vector<std::map<QString, QString>>>>>> materials; // std::pair<Material attributes, layers>  layer: std::pair<Layer attributes, maps>
public:
    void resolveBoundingBox(QRectF *mainProfile, QRectF *sideProfile, const QString &partId=QString());
};

#endif
