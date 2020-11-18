#ifndef DUST3D_DOCUMENT_SAVER_H
#define DUST3D_DOCUMENT_SAVER_H
#include <QString>
#include <QObject>
#include <QByteArray>
#include <map>
#include <set>
#include <QUuid>
#include "snapshot.h"
#include "object.h"

class DocumentSaver : public QObject
{
    Q_OBJECT
public:
    class Textures
    {
    public:
        QImage *textureImage = nullptr;
        QByteArray *textureImageByteArray = nullptr;
        QImage *textureNormalImage = nullptr;
        QByteArray *textureNormalImageByteArray = nullptr;
        QImage *textureMetalnessImage = nullptr;
        QByteArray *textureMetalnessImageByteArray = nullptr;
        QImage *textureRoughnessImage = nullptr;
        QByteArray *textureRoughnessImageByteArray = nullptr;
        QImage *textureAmbientOcclusionImage = nullptr;
        QByteArray *textureAmbientOcclusionImageByteArray = nullptr;
        
        ~Textures()
        {
            delete textureImage;
            delete textureImageByteArray;
            delete textureNormalImage;
            delete textureNormalImageByteArray;
            delete textureMetalnessImage;
            delete textureMetalnessImageByteArray;
            delete textureRoughnessImage;
            delete textureRoughnessImageByteArray;
            delete textureAmbientOcclusionImage;
            delete textureAmbientOcclusionImageByteArray;
        }
    };

    DocumentSaver(const QString *filename, 
        Snapshot *snapshot,
        Object *object,
        Textures *textures,
        QByteArray *turnaroundPngByteArray,
        QString *script,
        std::map<QString, std::map<QString, QString>> *variables);
    ~DocumentSaver();
    static bool save(const QString *filename, 
        Snapshot *snapshot,
        const Object *object,
        Textures *textures,
        const QByteArray *turnaroundPngByteArray,
        const QString *script,
        const std::map<QString, std::map<QString, QString>> *variables);
    static void collectUsedResourceIds(const Snapshot *snapshot,
        std::set<QUuid> &imageIds,
        std::set<QUuid> &fileIds);
signals:
    void finished();
public slots:
    void process();
private:
    const QString *m_filename = nullptr;
    Snapshot *m_snapshot = nullptr;
    Object *m_object = nullptr;
    Textures *m_textures = nullptr;
    QByteArray *m_turnaroundPngByteArray = nullptr;
    QString *m_script = nullptr;
    std::map<QString, std::map<QString, QString>> *m_variables = nullptr;
};

#endif
