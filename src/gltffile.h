#ifndef GLTF_FILE_H
#define GLTF_FILE_H
#include <QObject>
#include <QString>
#include <QByteArray>
#include <QMatrix4x4>
#include <vector>
#include "meshresultcontext.h"
#include "json.hpp"
#include "skeletondocument.h"

class GLTFFileWriter : public QObject
{
    Q_OBJECT
public:
    GLTFFileWriter(MeshResultContext &resultContext, const QString &filename);
    bool save();
    const QString &textureFilenameInGltf();
private:
    QByteArray m_data;
    QString getMatrixStringInColumnOrder(const QMatrix4x4 &mat);
    QString m_filename;
    QString m_textureFilename;
    bool m_outputNormal;
    bool m_outputAnimation;
    bool m_outputUv;
    bool m_testOutputAsWhole;
private:
    nlohmann::json m_json;
public:
    static bool m_enableComment;
};

#endif
