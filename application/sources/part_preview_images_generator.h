#ifndef DUST3D_APPLICATION_PART_PREVIEW_IMAGES_GENERATOR_H_
#define DUST3D_APPLICATION_PART_PREVIEW_IMAGES_GENERATOR_H_

#include <QObject>
#include <QImage>
#include <map>
#include <dust3d/base/uuid.h>
#include "model_offscreen_render.h"

class PartPreviewImagesGenerator : public QObject
{
    Q_OBJECT
public:
    PartPreviewImagesGenerator(ModelOffscreenRender *modelOffscreenRender) :
        m_offscreenRender(modelOffscreenRender)
    {
    }
    
    struct PreviewInput
    {
        ModelMesh *mesh = nullptr;
        bool isCutFace = false;
    };
    
    ~PartPreviewImagesGenerator()
    {
        for (const auto &it: m_partPreviews)
            delete it.second.mesh;
        
        delete m_partImages;
        
        delete m_offscreenRender;
    }

    void addPart(const dust3d::Uuid &partId, ModelMesh *previewMesh, bool isCutFace);
    void generate();
    std::map<dust3d::Uuid, QImage> *takePartImages();
signals:
    void finished();
public slots:
    void process();
private:
    std::map<dust3d::Uuid, PreviewInput> m_partPreviews;
    ModelOffscreenRender *m_offscreenRender = nullptr;
    std::map<dust3d::Uuid, QImage> *m_partImages = nullptr;
};

#endif
