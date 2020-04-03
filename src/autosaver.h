#ifndef DUST3D_AUTO_SAVER_H
#define DUST3D_AUTO_SAVER_H
#include <QObject>
#include <QString>
#include "document.h"

class DocumentSaver;

class AutoSaver : public QObject
{
    Q_OBJECT
public:
    AutoSaver(Document *document);
    static QString autoSavedDir();
public slots:
    void documentChanged();
    void check();
    void stop();
private slots:
    void autoSaveDone();
private:
    Document *m_document = nullptr;
    bool m_autoSaved = true;
    DocumentSaver *m_documentSaver = nullptr;
    QString m_filename;
    bool m_needStop = false;

    void removeFile();
};

#endif
