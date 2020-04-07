#include <QGuiApplication>
#include "turnaroundloader.h"

TurnaroundLoader::TurnaroundLoader(const QString &filename, QSize viewSize) :
    m_resultImage(nullptr)
{
    m_filename = filename;
    m_viewSize = viewSize;
}

TurnaroundLoader::TurnaroundLoader(const QImage &image, QSize viewSize)
{
    m_inputImage = image;
    m_viewSize = viewSize;
}

TurnaroundLoader::~TurnaroundLoader()
{
    delete m_resultImage;
}

QImage *TurnaroundLoader::takeResultImage()
{
    QImage *returnImage = m_resultImage;
    m_resultImage = nullptr;
    return returnImage;
}

void TurnaroundLoader::process()
{
    if (m_inputImage.isNull()) {
        QImage image(m_filename);
        m_resultImage = new QImage(image.scaled(m_viewSize, Qt::KeepAspectRatio));
    } else {
        m_resultImage = new QImage(m_inputImage.scaled(m_viewSize, Qt::KeepAspectRatio));
    }
    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}
