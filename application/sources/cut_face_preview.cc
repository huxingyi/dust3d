#include "cut_face_preview.h"
#include "theme.h"
#include <QGuiApplication>
#include <QPainter>
#include <QPainterPath>

static std::map<int, QImage*> g_standardCutFaceMap;

QImage* buildCutFaceTemplatePreviewImage(const std::vector<dust3d::Vector2>& cutTemplate)
{
    qreal dpr = qApp->devicePixelRatio();
    int physicalSize = qRound(Theme::partPreviewImageSize * dpr);
    QImage* image = new QImage(physicalSize, physicalSize, QImage::Format_ARGB32);
    image->setDevicePixelRatio(dpr);
    image->fill(Qt::transparent);

    if (cutTemplate.empty())
        return image;

    QPainter painter;
    painter.begin(image);
    painter.setRenderHint(QPainter::Antialiasing);
#if QT_VERSION < 0x060000
    painter.setRenderHint(QPainter::HighQualityAntialiasing);
#endif

    QPen pen(Theme::red, 1.0 * dpr);
    painter.setPen(pen);

    QColor fillColor = Theme::white;
    fillColor.setAlpha(40);
    QBrush brush;
    brush.setColor(fillColor);
    brush.setStyle(Qt::SolidPattern);

    painter.setBrush(brush);

    const float scale = 0.7f;
    QPolygon polygon;
    for (size_t i = 0; i <= cutTemplate.size(); ++i) {
        const auto& it = cutTemplate[i % cutTemplate.size()];
        polygon.append(QPoint((it.x() * scale + 1.0) * 0.5 * Theme::partPreviewImageSize,
            (-it.y() * scale + 1.0) * 0.5 * Theme::partPreviewImageSize));
    }

    QPainterPath path;
    path.addPolygon(polygon);
    painter.fillPath(path, brush);
    painter.drawPath(path);

    painter.setPen(Qt::NoPen);
    QColor dotColor = Theme::white;
    dotColor.setAlpha(160);
    painter.setBrush(QBrush(dotColor));
    double dotRadius = 0.5 * dpr;
    for (size_t i = 0; i < cutTemplate.size(); ++i) {
        const auto& it = cutTemplate[i];
        double cx = (it.x() * scale + 1.0) * 0.5 * Theme::partPreviewImageSize;
        double cy = (-it.y() * scale + 1.0) * 0.5 * Theme::partPreviewImageSize;
        painter.drawEllipse(QPointF(cx, cy), dotRadius, dotRadius);
    }

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
