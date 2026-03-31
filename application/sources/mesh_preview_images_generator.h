#ifndef DUST3D_APPLICATION_MESH_PREVIEW_IMAGES_GENERATOR_H_
#define DUST3D_APPLICATION_MESH_PREVIEW_IMAGES_GENERATOR_H_

#include "model_offscreen_render.h"
#include <QImage>
#include <QObject>
#include <dust3d/base/uuid.h>
#include <map>
#include <memory>

class MeshPreviewImagesGenerator : public QObject {
    Q_OBJECT
public:
    MeshPreviewImagesGenerator(ModelOffscreenRender* modelOffscreenRender, qreal devicePixelRatio = 1.0)
        : m_offscreenRender(modelOffscreenRender)
        , m_devicePixelRatio(devicePixelRatio)
    {
    }

    struct PreviewInput {
        std::unique_ptr<ModelMesh> mesh;
        bool useFrontView = false;
    };

    ~MeshPreviewImagesGenerator()
    {
        delete m_offscreenRender;
    }

    void addInput(const dust3d::Uuid& inputId, std::unique_ptr<ModelMesh> previewMesh, bool useFrontView = false);
    void generate();
    std::map<dust3d::Uuid, QImage>* takeImages();
signals:
    void finished();
public slots:
    void process();

private:
    std::map<dust3d::Uuid, PreviewInput> m_previewInputMap;
    ModelOffscreenRender* m_offscreenRender = nullptr;
    std::unique_ptr<std::map<dust3d::Uuid, QImage>> m_partImages;
    qreal m_devicePixelRatio = 1.0;
};

#endif
