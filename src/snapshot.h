#ifndef DUST3D_SNAPSHOT_H
#define DUST3D_SNAPSHOT_H
#include <vector>
#include <map>
#include <QString>
#include <QRectF>
#include <QSizeF>
extern "C" {
#include <crc64.h>
}

class Snapshot
{
public:
    std::map<QString, QString> canvas;
    std::map<QString, std::map<QString, QString>> nodes;
    std::map<QString, std::map<QString, QString>> edges;
    std::map<QString, std::map<QString, QString>> parts;
    std::map<QString, std::map<QString, QString>> components;
    std::map<QString, QString> rootComponent;
    std::map<QString, std::map<QString, QString>> motions;
    std::vector<std::pair<std::map<QString, QString>, std::vector<std::pair<std::map<QString, QString>, std::vector<std::map<QString, QString>>>>>> materials; // std::pair<Material attributes, layers>  layer: std::pair<Layer attributes, maps>

    void resolveBoundingBox(QRectF *mainProfile, QRectF *sideProfile, const QString &partId=QString()) const;
};

#endif
