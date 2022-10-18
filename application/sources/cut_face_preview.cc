#include "cut_face_preview.h"
#include "theme.h"
#include <QPainter>
#include <QPainterPath>

static std::map<int, QImage*> g_standardCutFaceMap;

QImage* buildCutFaceTemplatePreviewImage(const std::vector<dust3d::Vector2>& cutTemplate)
{
    QImage* image = new QImage(Theme::partPreviewImageSize, Theme::partPreviewImageSize, QImage::Format_ARGB32);
    image->fill(Qt::transparent);

    if (cutTemplate.empty())
        return image;

    QPainter painter;
    painter.begin(image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::HighQualityAntialiasing);

    QPen pen(Theme::red, 2);
    painter.setPen(pen);

    QBrush brush;
    brush.setColor(Theme::white);
    brush.setStyle(Qt::SolidPattern);

    painter.setBrush(brush);

    const float scale = 0.7f;
    QPolygon polygon;
    for (size_t i = 0; i <= cutTemplate.size(); ++i) {
        const auto& it = cutTemplate[i % cutTemplate.size()];
        polygon.append(QPoint((it.x() * scale + 1.0) * 0.5 * Theme::partPreviewImageSize,
            (it.y() * scale + 1.0) * 0.5 * Theme::partPreviewImageSize));
    }

    QPainterPath path;
    path.addPolygon(polygon);
    painter.fillPath(path, brush);
    painter.drawPath(path);

    painter.end();

    return image;
}

const QImage* cutFacePreviewImage(dust3d::CutFace cutFace)
{
    auto findItem = g_standardCutFaceMap.find((int)cutFace);
    if (findItem != g_standardCutFaceMap.end())
        return findItem->second;
    std::vector<dust3d::Vector2> cutTemplate = dust3d::CutFaceToPoints(cutFace);
    QImage* image = buildCutFaceTemplatePreviewImage(cutTemplate);
    g_standardCutFaceMap.insert({ (int)cutFace, image });
    return image;
}
