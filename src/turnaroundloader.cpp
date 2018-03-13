#include "turnaroundloader.h"

TurnaroundLoader::TurnaroundLoader(const QString &filename, QSize viewSize) :
    m_resultImage(NULL)
{
    m_filename = filename;
    m_viewSize = viewSize;
}

TurnaroundLoader::~TurnaroundLoader()
{
    delete m_resultImage;
}

QImage *TurnaroundLoader::takeResultImage()
{
    QImage *returnImage = m_resultImage;
    m_resultImage = NULL;
    return returnImage;
}

void TurnaroundLoader::process()
{
    QImage image(m_filename);
    m_resultImage = new QImage(image.scaled(m_viewSize, Qt::KeepAspectRatio));
    emit finished();
}
