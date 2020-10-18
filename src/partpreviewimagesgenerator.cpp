#include <QDebug>
#include "partpreviewimagesgenerator.h"
#include "theme.h"

void PartPreviewImagesGenerator::addPart(const QUuid &partId, Model *previewMesh, bool isCutFace)
{
    m_partPreviews.insert({partId, {previewMesh, isCutFace}});
}

void PartPreviewImagesGenerator::process()
{
    generate();
    
    emit finished();
}

std::map<QUuid, QImage> *PartPreviewImagesGenerator::takePartImages()
{
    std::map<QUuid, QImage> *partImages = m_partImages;
    m_partImages = nullptr;
    return partImages;
}

void PartPreviewImagesGenerator::generate()
{
    delete m_partImages;
    m_partImages = new std::map<QUuid, QImage>;
    
    m_offscreenRender->setZRotation(0);
    m_offscreenRender->setEyePosition(QVector3D(0, 0, -4.0));
    m_offscreenRender->enableEnvironmentLight();
    m_offscreenRender->setRenderPurpose(0);
    for (auto &it: m_partPreviews) {
        if (it.second.isCutFace) {
            m_offscreenRender->setXRotation(0);
            m_offscreenRender->setYRotation(0);
        } else {
            m_offscreenRender->setXRotation(30 * 16);
            m_offscreenRender->setYRotation(-45 * 16);
        }
        m_offscreenRender->updateMesh(it.second.mesh);
        it.second.mesh = nullptr;
        (*m_partImages)[it.first] = m_offscreenRender->toImage(QSize(Theme::partPreviewImageSize, Theme::partPreviewImageSize));
    }
}
