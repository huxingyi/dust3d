#ifndef DUST3D_DOCUMENT_SAVER_H
#define DUST3D_DOCUMENT_SAVER_H
#include <QString>
#include <QObject>
#include <QByteArray>
#include <map>
#include "snapshot.h"

class DocumentSaver : public QObject
{
    Q_OBJECT
public:
    DocumentSaver(const QString *filename, 
        Snapshot *snapshot, 
        QByteArray *turnaroundPngByteArray,
        QString *script,
        std::map<QString, std::map<QString, QString>> *variables);
    ~DocumentSaver();
    static bool save(const QString *filename, 
        Snapshot *snapshot, 
        const QByteArray *turnaroundPngByteArray,
        const QString *script,
        const std::map<QString, std::map<QString, QString>> *variables);
signals:
    void finished();
public slots:
    void process();
private:
    const QString *m_filename = nullptr;
    Snapshot *m_snapshot = nullptr;
    QByteArray *m_turnaroundPngByteArray = nullptr;
    QString *m_script = nullptr;
    std::map<QString, std::map<QString, QString>> *m_variables = nullptr;
};

#endif
