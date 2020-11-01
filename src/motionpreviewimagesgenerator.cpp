#include <QDebug>
#include "motionpreviewimagesgenerator.h"
#include "theme.h"

void MotionPreviewImagesGenerator::addMotion(const QUuid &motionId, Model *previewMesh)
{
    m_imagePreviews.insert({motionId, {previewMesh}});
}

void MotionPreviewImagesGenerator::process()
{
    generate();
    
    emit finished();
}

std::map<QUuid, QImage> *MotionPreviewImagesGenerator::takeMotionImages()
{
    std::map<QUuid, QImage> *motionImages = m_motionImages;
    m_motionImages = nullptr;
    return motionImages;
}

void MotionPreviewImagesGenerator::generate()
{
    delete m_motionImages;
    m_motionImages = new std::map<QUuid, QImage>;
    
    for (auto &it: m_imagePreviews) {
        //m_offscreenRender->updateMesh(it.second.mesh);
        //it.second.mesh = nullptr;
        //(*m_motionImages)[it.first] = m_offscreenRender->toImage(QSize(Theme::motionPreviewImageSize, Theme::motionPreviewImageSize));
    }
}
