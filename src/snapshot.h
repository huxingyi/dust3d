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
    std::vector<std::pair<std::map<QString, QString>, std::map<QString, std::map<QString, QString>>>> poses; // std::pair<Pose attributes, Bone attributes>
    std::vector<std::pair<std::map<QString, QString>, std::vector<std::map<QString, QString>>>> motions; // std::pair<Motion attributes, clips>
    std::vector<std::pair<std::map<QString, QString>, std::vector<std::pair<std::map<QString, QString>, std::vector<std::map<QString, QString>>>>>> materials; // std::pair<Material attributes, layers>  layer: std::pair<Layer attributes, maps>
    
    uint64_t hash() const
    {
        std::vector<unsigned char> buffer;
        auto addQStringToBuffer = [&buffer](const QString &str) {
            auto byteArray = str.toUtf8();
            for (const auto &byte: byteArray)
                buffer.push_back(byte);
        };
        for (const auto &item: canvas) {
            addQStringToBuffer(item.first);
            addQStringToBuffer(item.second);
        }
        for (const auto &item: nodes) {
            addQStringToBuffer(item.first);
            for (const auto &subItem: item.second) {
                addQStringToBuffer(subItem.first);
                addQStringToBuffer(subItem.second);
            }
        }
        for (const auto &item: edges) {
            addQStringToBuffer(item.first);
            for (const auto &subItem: item.second) {
                addQStringToBuffer(subItem.first);
                addQStringToBuffer(subItem.second);
            }
        }
        for (const auto &item: parts) {
            addQStringToBuffer(item.first);
            for (const auto &subItem: item.second) {
                addQStringToBuffer(subItem.first);
                addQStringToBuffer(subItem.second);
            }
        }
        for (const auto &item: components) {
            addQStringToBuffer(item.first);
            for (const auto &subItem: item.second) {
                addQStringToBuffer(subItem.first);
                addQStringToBuffer(subItem.second);
            }
        }
        for (const auto &item: rootComponent) {
            addQStringToBuffer(item.first);
            addQStringToBuffer(item.second);
        }
        for (const auto &item: poses) {
            for (const auto &subItem: item.first) {
                addQStringToBuffer(subItem.first);
                addQStringToBuffer(subItem.second);
            }
            for (const auto &subItem: item.second) {
                addQStringToBuffer(subItem.first);
                for (const auto &subSubItem: subItem.second) {
                    addQStringToBuffer(subSubItem.first);
                    addQStringToBuffer(subSubItem.second);
                }
            }
        }
        for (const auto &item: motions) {
            for (const auto &subItem: item.first) {
                addQStringToBuffer(subItem.first);
                addQStringToBuffer(subItem.second);
            }
            for (const auto &subItem: item.second) {
                for (const auto &subSubItem: subItem) {
                    addQStringToBuffer(subSubItem.first);
                    addQStringToBuffer(subSubItem.second);
                }
            }
        }
        for (const auto &item: materials) {
            for (const auto &subItem: item.first) {
                addQStringToBuffer(subItem.first);
                addQStringToBuffer(subItem.second);
            }
            for (const auto &subItem: item.second) {
                for (const auto &subSubItem: subItem.first) {
                    addQStringToBuffer(subSubItem.first);
                    addQStringToBuffer(subSubItem.second);
                }
                for (const auto &subSubItem: subItem.second) {
                    for (const auto &subSubSubItem: subSubItem) {
                        addQStringToBuffer(subSubSubItem.first);
                        addQStringToBuffer(subSubSubItem.second);
                    }
                }
            }
        }
        return crc64(0, buffer.data(), buffer.size());
    }

    void resolveBoundingBox(QRectF *mainProfile, QRectF *sideProfile, const QString &partId=QString()) const;
};

#endif
