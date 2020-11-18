#include <QTimer>
#include <QDebug>
#include <QDateTime>
#include <QUuid>
#include <QThread>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include "autosaver.h"
#include "documentsaver.h"
#include "snapshotxml.h"

AutoSaver::AutoSaver(Document *document) :
    m_document(document)
{
    QString dir = autoSavedDir();
    if (!dir.isEmpty()) {
        QDir dirTest(dir);
        dirTest.mkpath(dirTest.absolutePath());
    }
    
    m_filename = autoSavedDir() + QDir::separator() + QString("%1-%2.d3b").arg(
        QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss")).arg(
        QUuid::createUuid().toString());
    
    QTimer *timer = new QTimer(this);
    timer->setInterval(1000 * 10);
    connect(timer, &QTimer::timeout, this, &AutoSaver::check);
    timer->start();
}

QString AutoSaver::autoSavedDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

void AutoSaver::removeFile()
{
    QFile::remove(m_filename);
}

void AutoSaver::stop()
{
    m_needStop = true;
    if (nullptr != m_documentSaver)
        return;
    
    removeFile();
    deleteLater();
}

void AutoSaver::documentChanged()
{
    m_autoSaved = false;
}

void AutoSaver::autoSaveDone()
{
    delete m_documentSaver;
    m_documentSaver = nullptr;
    
    qDebug() << "Auto saving done";
    
    if (m_needStop) {
        removeFile();
        deleteLater();
        return;
    }
}

void AutoSaver::check()
{
    if (m_autoSaved)
        return;
    
    if (nullptr != m_documentSaver) {
        m_autoSaved = false;
        return;
    }
    
    m_autoSaved = true;
    
    qDebug() << "Start auto saving...";
    
    Snapshot *snapshot = new Snapshot;
    m_document->toSnapshot(snapshot);

    QByteArray *turnaroundPngByteArray = nullptr;
    if (!m_document->turnaround.isNull() && m_document->turnaroundPngByteArray.size() > 0) {
        turnaroundPngByteArray = new QByteArray(m_document->turnaroundPngByteArray);
    }
    
    QString *script = nullptr;
    if (!m_document->script().isEmpty()) {
        script = new QString(m_document->script());
    }
    
    std::map<QString, std::map<QString, QString>> *scriptVariables = nullptr;
    const auto &variables = m_document->variables();
    if (!variables.empty()) {
        scriptVariables = new std::map<QString, std::map<QString, QString>>(variables);
    }
    
    Object *object = nullptr;
    DocumentSaver::Textures *textures = nullptr;
    
    if (m_document->objectLocked) {
        object = new Object(m_document->currentPostProcessedObject());
        textures = new DocumentSaver::Textures;
        if (nullptr != m_document->textureImage) {
            textures->textureImage = new QImage(*m_document->textureImage);
        }
        if (nullptr != m_document->textureNormalImage) {
            textures->textureNormalImage = new QImage(*m_document->textureNormalImage);
        }
        if (nullptr != m_document->textureMetalnessImage) {
            textures->textureMetalnessImage = new QImage(*m_document->textureMetalnessImage);
        }
        if (nullptr != m_document->textureRoughnessImage) {
            textures->textureRoughnessImage = new QImage(*m_document->textureRoughnessImage);
        }
        if (nullptr != m_document->textureAmbientOcclusionImage) {
            textures->textureAmbientOcclusionImage = new QImage(*m_document->textureAmbientOcclusionImage);
        }
    }
    QThread *thread = new QThread;
    m_documentSaver = new DocumentSaver(&m_filename,
        snapshot,
        object,
        textures,
        turnaroundPngByteArray,
        script,
        scriptVariables);
    m_documentSaver->moveToThread(thread);
    connect(thread, &QThread::started, m_documentSaver, &DocumentSaver::process);
    connect(m_documentSaver, &DocumentSaver::finished, this, &AutoSaver::autoSaveDone);
    connect(m_documentSaver, &DocumentSaver::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}