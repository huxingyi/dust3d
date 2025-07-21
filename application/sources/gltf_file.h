#ifndef DUST3D_APPLICATION_GLTF_FILE_H_
#define DUST3D_APPLICATION_GLTF_FILE_H_

#include "document.h"
#include "json.hpp"
#include <QByteArray>
#include <QImage>
#include <QMatrix4x4>
#include <QObject>
#include <QQuaternion>
#include <QString>
#include <dust3d/base/object.h>
#include <vector>

class GltfFileWriter : public QObject {
    Q_OBJECT
public:
    GltfFileWriter(dust3d::Object& object,
        const QString& filename,
        QImage* textureImage = nullptr,
        QImage* normalImage = nullptr,
        QImage* ormImage = nullptr);
    bool save();

private:
    QString m_filename;
    QString m_baseName;
    bool m_outputNormal = true;
    bool m_outputUv = true;
    QByteArray m_binByteArray;

private:
    nlohmann::json m_json;
    void saveJsonFile();
    void saveBinFile();

public:
    static bool m_enableComment;
};

#endif
