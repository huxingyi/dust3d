#ifndef DUST3D_PART_PREVIEW_IMAGES_GENERATOR_H
#define DUST3D_PART_PREVIEW_IMAGES_GENERATOR_H
#include <QObject>
#include <QUuid>
#include <QImage>
#include <map>
#include "modeloffscreenrender.h"

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
        Model *mesh = nullptr;
        bool isCutFace = false;
    };
    
    ~PartPreviewImagesGenerator()
    {
        for (const auto &it: m_partPreviews)
            delete it.second.mesh;
        
        delete m_partImages;
        
        delete m_offscreenRender;
    }

    void addPart(const QUuid &partId, Model *previewMesh, bool isCutFace);
    void generate();
    std::map<QUuid, QImage> *takePartImages();
signals:
    void finished();
public slots:
    void process();
private:
    std::map<QUuid, PreviewInput> m_partPreviews;
    ModelOffscreenRender *m_offscreenRender = nullptr;
    std::map<QUuid, QImage> *m_partImages = nullptr;
};

#endif
