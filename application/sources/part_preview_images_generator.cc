#include <QDebug>
#include "part_preview_images_generator.h"
#include "theme.h"

void PartPreviewImagesGenerator::addPart(const dust3d::Uuid &partId, Model *previewMesh, bool isCutFace)
{
    m_partPreviews.insert({partId, {previewMesh, isCutFace}});
}

void PartPreviewImagesGenerator::process()
{
    generate();
    
    emit finished();
}

std::map<dust3d::Uuid, QImage> *PartPreviewImagesGenerator::takePartImages()
{
    std::map<dust3d::Uuid, QImage> *partImages = m_partImages;
    m_partImages = nullptr;
    return partImages;
}

void PartPreviewImagesGenerator::generate()
{
    delete m_partImages;
    m_partImages = new std::map<dust3d::Uuid, QImage>;
    
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
