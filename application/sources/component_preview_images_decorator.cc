#include <dust3d/base/debug.h>
#include <QPainter>
#include "theme.h"
#include "component_preview_images_decorator.h"

ComponentPreviewImagesDecorator::ComponentPreviewImagesDecorator(std::unique_ptr<std::vector<PreviewInput>> previewInputs)
{
    m_previewInputs = std::move(previewInputs);
}

std::unique_ptr<std::unordered_map<dust3d::Uuid, std::unique_ptr<QImage>>> ComponentPreviewImagesDecorator::takeResultImages()
{
    return std::move(m_resultImages);
}

void ComponentPreviewImagesDecorator::decorate()
{
    if (nullptr == m_previewInputs)
        return;

    m_resultImages = std::make_unique<std::unordered_map<dust3d::Uuid, std::unique_ptr<QImage>>>();

    QPointF iconOffset(Theme::previewIconMargin, 0);
    for (auto &it: *m_previewInputs) {
        if (it.isDirectory) {
            QPainter painter(it.image.get());
            painter.setRenderHints(QPainter::Antialiasing);
            QPolygonF polygon;
            polygon << iconOffset + QPointF(it.image->width() / 4, 0) << iconOffset + QPointF(it.image->width() / 2, 0);
            polygon << iconOffset + QPointF(0.0, it.image->height() / 2) << iconOffset + QPointF(0.0, it.image->height() / 4);
            QPainterPath painterPath;
            painterPath.addPolygon(polygon);
            painter.setBrush(Theme::green);
            painter.setPen(Qt::NoPen);
            painter.drawPath(painterPath);
        }
        m_resultImages->emplace(it.id, std::move(it.image));
    }
}

void ComponentPreviewImagesDecorator::process()
{
    decorate();
    emit finished();
}
