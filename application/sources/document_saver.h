#ifndef DUST3D_APPLICATION_DOCUMENT_SAVER_H_
#define DUST3D_APPLICATION_DOCUMENT_SAVER_H_

#include <QByteArray>
#include <QObject>
#include <QString>
#include <dust3d/base/ds3_file.h>
#include <dust3d/base/snapshot.h>
#include <dust3d/base/uuid.h>
#include <map>
#include <set>

class DocumentSaver : public QObject {
    Q_OBJECT
public:
    DocumentSaver(const QString* filename,
        dust3d::Snapshot* snapshot,
        QByteArray* turnaroundPngByteArray);
    ~DocumentSaver();
    static bool save(const QString* filename,
        dust3d::Snapshot* snapshot,
        const QByteArray* turnaroundPngByteArray);
    static bool save(QByteArray& byteArray,
        dust3d::Snapshot* snapshot,
        const QByteArray* turnaroundPngByteArray);
    static bool save(dust3d::Ds3FileWriter& ds3Writer,
        dust3d::Snapshot* snapshot,
        const QByteArray* turnaroundPngByteArray);
    static void collectUsedResourceIds(const dust3d::Snapshot* snapshot,
        std::set<dust3d::Uuid>& imageIds);
signals:
    void finished();
public slots:
    void process();

private:
    const QString* m_filename = nullptr;
    dust3d::Snapshot* m_snapshot = nullptr;
    QByteArray* m_turnaroundPngByteArray = nullptr;
};

#endif
