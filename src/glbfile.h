#ifndef DUST3D_GLB_FILE_H
#define DUST3D_GLB_FILE_H
#include <QObject>
#include <QString>
#include <QByteArray>
#include <QMatrix4x4>
#include <vector>
#include <QQuaternion>
#include <QImage>
#include "outcome.h"
#include "json.hpp"
#include "document.h"

class GlbFileWriter : public QObject
{
    Q_OBJECT
public:
    GlbFileWriter(Outcome &outcome,
        const std::vector<AutoRiggerBone> *resultRigBones,
        const std::map<int, AutoRiggerVertexWeights> *resultRigWeights,
        const QString &filename,
        QImage *textureImage=nullptr);
    bool save();
private:
    QString m_filename;
    bool m_outputNormal;
    bool m_outputAnimation;
    bool m_outputUv;
    bool m_testOutputAsWhole;
    QByteArray m_binByteArray;
    QByteArray m_jsonByteArray;
private:
    nlohmann::json m_json;
public:
    static bool m_enableComment;
};

#endif
