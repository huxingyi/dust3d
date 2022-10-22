#ifndef DUST3D_APPLICATION_TUBE_PREVIEW_IMAGE_GENERATOR_H_
#define DUST3D_APPLICATION_TUBE_PREVIEW_IMAGE_GENERATOR_H_

#include <QImage>
#include <QObject>
#include <dust3d/base/vector2.h>
#include <memory>
#include <vector>

class UvPreviewImageGenerator : public QObject {
    Q_OBJECT
public:
    UvPreviewImageGenerator(std::vector<std::vector<dust3d::Vector2>>&& faceUvs);
    void generate();
signals:
    void finished();
public slots:
    void process();

private:
    std::vector<std::vector<dust3d::Vector2>> m_faceUvs;
    std::unique_ptr<QImage> m_previewImage;
    static const size_t m_uvImageSize;
};

#endif
