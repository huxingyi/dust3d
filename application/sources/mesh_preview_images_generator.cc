#include <QDebug>
#include "mesh_preview_images_generator.h"
#include "theme.h"

void MeshPreviewImagesGenerator::addInput(const dust3d::Uuid &inputId, std::unique_ptr<ModelMesh> previewMesh, bool useFrontView)
{
    m_previewInputMap.insert({inputId, PreviewInput {std::move(previewMesh), useFrontView}});
}

void MeshPreviewImagesGenerator::process()
{
    generate();
    
    emit finished();
}

std::map<dust3d::Uuid, QImage> *MeshPreviewImagesGenerator::takeImages()
{
    return m_partImages.release();
}

void MeshPreviewImagesGenerator::generate()
{
    m_partImages = std::make_unique<std::map<dust3d::Uuid, QImage>>();
    
    m_offscreenRender->setZRotation(0);
    m_offscreenRender->setEyePosition(QVector3D(0, 0, -4.0));
    for (auto &it: m_previewInputMap) {
        if (it.second.useFrontView) {
            m_offscreenRender->setXRotation(0);
            m_offscreenRender->setYRotation(0);
        } else {
            m_offscreenRender->setXRotation(30 * 16);
            m_offscreenRender->setYRotation(-45 * 16);
        }
        m_offscreenRender->updateMesh(it.second.mesh.release());
        (*m_partImages)[it.first] = m_offscreenRender->toImage(QSize(Theme::partPreviewImageSize, Theme::partPreviewImageSize));
    }
}
