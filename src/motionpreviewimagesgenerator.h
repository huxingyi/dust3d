#ifndef DUST3D_MOTION_PREVIEW_IMAGES_GENERATOR_H
#define DUST3D_MOTION_PREVIEW_IMAGES_GENERATOR_H
#include <QObject>
#include <QUuid>
#include <QImage>
#include <map>
#include "modeloffscreenrender.h"

class MotionPreviewImagesGenerator : public QObject
{
    Q_OBJECT
public:
    MotionPreviewImagesGenerator(ModelOffscreenRender *offscreenRender) :
        m_offscreenRender(offscreenRender)
    {
    }
    
    struct PreviewInput
    {
        Model *mesh = nullptr;
    };
    
    ~MotionPreviewImagesGenerator()
    {
        for (const auto &it: m_imagePreviews)
            delete it.second.mesh;
        
        delete m_motionImages;
        
        delete m_offscreenRender;
    }

    void addMotion(const QUuid &motionId, Model *previewMesh);
    void generate();
    std::map<QUuid, QImage> *takeMotionImages();
signals:
    void finished();
public slots:
    void process();
private:
    std::map<QUuid, PreviewInput> m_imagePreviews;
    ModelOffscreenRender *m_offscreenRender = nullptr;
    std::map<QUuid, QImage> *m_motionImages = nullptr;
};

#endif
