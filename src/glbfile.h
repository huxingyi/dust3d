#ifndef DUST3D_GLB_FILE_H
#define DUST3D_GLB_FILE_H
#include <QObject>
#include <QString>
#include <QByteArray>
#include <QMatrix4x4>
#include <vector>
#include <QQuaternion>
#include <QImage>
#include "object.h"
#include "json.hpp"
#include "document.h"

class GlbFileWriter : public QObject
{
    Q_OBJECT
public:
    GlbFileWriter(Object &object,
        const std::vector<RiggerBone> *resultRigBones,
        const std::map<int, RiggerVertexWeights> *resultRigWeights,
        const QString &filename,
        bool textureHasTransparencySettings,
        QImage *textureImage=nullptr,
        QImage *normalImage=nullptr,
        QImage *ormImage=nullptr,
        const std::vector<std::pair<QString, std::vector<std::pair<float, JointNodeTree>>>> *motions=nullptr);
    bool save();
private:
    QString m_filename;
    bool m_outputNormal;
    bool m_outputAnimation;
    bool m_outputUv;
    QByteArray m_binByteArray;
    QByteArray m_jsonByteArray;
private:
    nlohmann::json m_json;
public:
    static bool m_enableComment;
};

#endif
