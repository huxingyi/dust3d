#ifndef DUST3D_APPLICATION_GLB_FILE_H_
#define DUST3D_APPLICATION_GLB_FILE_H_

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QMatrix4x4>
#include <vector>
#include <QQuaternion>
#include <QImage>
#include <dust3d/base/object.h>
#include "json.hpp"
#include "document.h"

class GlbFileWriter : public QObject
{
    Q_OBJECT
public:
    GlbFileWriter(dust3d::Object &object,
        const QString &filename,
        QImage *textureImage=nullptr,
        QImage *normalImage=nullptr,
        QImage *ormImage=nullptr);
    bool save();
private:
    QString m_filename;
    bool m_outputNormal = true;
    bool m_outputUv = true;
    QByteArray m_binByteArray;
    QByteArray m_jsonByteArray;
private:
    nlohmann::json m_json;
public:
    static bool m_enableComment;
};

#endif
