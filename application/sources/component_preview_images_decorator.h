#ifndef DUST3D_APPLICATION_COMPONENT_PREVIEW_IMAGES_DECORATOR_H_
#define DUST3D_APPLICATION_COMPONENT_PREVIEW_IMAGES_DECORATOR_H_

#include <QImage>
#include <dust3d/base/uuid.h>
#include <memory>
#include <unordered_map>

class ComponentPreviewImagesDecorator : public QObject {
    Q_OBJECT
public:
    struct PreviewInput {
        dust3d::Uuid id;
        std::unique_ptr<QImage> image;
        bool isDirectory = false;
    };

    ComponentPreviewImagesDecorator(std::unique_ptr<std::vector<PreviewInput>> previewInputs);
    std::unique_ptr<std::unordered_map<dust3d::Uuid, std::unique_ptr<QImage>>> takeResultImages();
signals:
    void finished();
public slots:
    void process();

private:
    std::unique_ptr<std::vector<PreviewInput>> m_previewInputs;
    std::unique_ptr<std::unordered_map<dust3d::Uuid, std::unique_ptr<QImage>>> m_resultImages;
    void decorate();
};

#endif
