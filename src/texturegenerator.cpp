#include <QPainter>
#include <QGuiApplication>
#include <QRegion>
#include <QPolygon>
#include <QElapsedTimer>
#include <QRadialGradient>
#include "texturegenerator.h"
#include "theme.h"
#include "util.h"
#include "texturetype.h"
#include "material.h"

int TextureGenerator::m_textureSize = 1024;
QColor TextureGenerator::m_defaultTextureColor = Qt::white; //Qt::darkGray;

TextureGenerator::TextureGenerator(const Outcome &outcome, Snapshot *snapshot) :
    m_resultTextureGuideImage(nullptr),
    m_resultTextureImage(nullptr),
    m_resultTextureBorderImage(nullptr),
    m_resultTextureColorImage(nullptr),
    m_resultTextureNormalImage(nullptr),
    m_resultTextureMetalnessRoughnessAmbientOcclusionImage(nullptr),
    m_resultTextureRoughnessImage(nullptr),
    m_resultTextureMetalnessImage(nullptr),
    m_resultTextureAmbientOcclusionImage(nullptr),
    m_resultMesh(nullptr),
    m_snapshot(snapshot)
{
    m_outcome = new Outcome();
    *m_outcome = outcome;
}

TextureGenerator::~TextureGenerator()
{
    delete m_outcome;
    delete m_resultTextureGuideImage;
    delete m_resultTextureImage;
    delete m_resultTextureBorderImage;
    delete m_resultTextureColorImage;
    delete m_resultTextureNormalImage;
    delete m_resultTextureMetalnessRoughnessAmbientOcclusionImage;
    delete m_resultTextureRoughnessImage;
    delete m_resultTextureMetalnessImage;
    delete m_resultTextureAmbientOcclusionImage;
    delete m_resultMesh;
    delete m_snapshot;
}

QImage *TextureGenerator::takeResultTextureGuideImage()
{
    QImage *resultTextureGuideImage = m_resultTextureGuideImage;
    m_resultTextureGuideImage = nullptr;
    return resultTextureGuideImage;
}

QImage *TextureGenerator::takeResultTextureImage()
{
    QImage *resultTextureImage = m_resultTextureImage;
    m_resultTextureImage = nullptr;
    return resultTextureImage;
}

QImage *TextureGenerator::takeResultTextureBorderImage()
{
    QImage *resultTextureBorderImage = m_resultTextureBorderImage;
    m_resultTextureBorderImage = nullptr;
    return resultTextureBorderImage;
}

QImage *TextureGenerator::takeResultTextureColorImage()
{
    QImage *resultTextureColorImage = m_resultTextureColorImage;
    m_resultTextureColorImage = nullptr;
    return resultTextureColorImage;
}

QImage *TextureGenerator::takeResultTextureNormalImage()
{
    QImage *resultTextureNormalImage = m_resultTextureNormalImage;
    m_resultTextureNormalImage = nullptr;
    return resultTextureNormalImage;
}

QImage *TextureGenerator::takeResultTextureMetalnessRoughnessAmbientOcclusionImage()
{
    QImage *resultTextureMetalnessRoughnessAmbientOcclusionImage = m_resultTextureMetalnessRoughnessAmbientOcclusionImage;
    m_resultTextureMetalnessRoughnessAmbientOcclusionImage = nullptr;
    return resultTextureMetalnessRoughnessAmbientOcclusionImage;
}

QImage *TextureGenerator::takeResultTextureRoughnessImage()
{
    QImage *resultTextureRoughnessImage = m_resultTextureRoughnessImage;
    m_resultTextureRoughnessImage = nullptr;
    return resultTextureRoughnessImage;
}

QImage *TextureGenerator::takeResultTextureMetalnessImage()
{
    QImage *resultTextureMetalnessImage = m_resultTextureMetalnessImage;
    m_resultTextureMetalnessImage = nullptr;
    return resultTextureMetalnessImage;
}

QImage *TextureGenerator::takeResultTextureAmbientOcclusionImage()
{
    QImage *resultTextureAmbientOcclusionImage = m_resultTextureAmbientOcclusionImage;
    m_resultTextureAmbientOcclusionImage = nullptr;
    return resultTextureAmbientOcclusionImage;
}

Outcome *TextureGenerator::takeOutcome()
{
    Outcome *outcome = m_outcome;
    m_resultTextureImage = nullptr;
    return outcome;
}

MeshLoader *TextureGenerator::takeResultMesh()
{
    MeshLoader *resultMesh = m_resultMesh;
    m_resultMesh = nullptr;
    return resultMesh;
}

void TextureGenerator::addPartColorMap(QUuid partId, const QImage *image)
{
    if (nullptr == image)
        return;
    m_partColorTextureMap[partId] = *image;
}

void TextureGenerator::addPartNormalMap(QUuid partId, const QImage *image)
{
    if (nullptr == image)
        return;
    m_partNormalTextureMap[partId] = *image;
}

void TextureGenerator::addPartMetalnessMap(QUuid partId, const QImage *image)
{
    if (nullptr == image)
        return;
    m_partMetalnessTextureMap[partId] = *image;
}

void TextureGenerator::addPartRoughnessMap(QUuid partId, const QImage *image)
{
    if (nullptr == image)
        return;
    m_partRoughnessTextureMap[partId] = *image;
}

void TextureGenerator::addPartAmbientOcclusionMap(QUuid partId, const QImage *image)
{
    if (nullptr == image)
        return;
    m_partAmbientOcclusionTextureMap[partId] = *image;
}

QPainterPath TextureGenerator::expandedPainterPath(const QPainterPath &painterPath, int expandSize)
{
    QPainterPathStroker stroker;
    stroker.setWidth(expandSize);
    stroker.setJoinStyle(Qt::RoundJoin);
    return (stroker.createStroke(painterPath) + painterPath).simplified();
}

void TextureGenerator::prepare()
{
    if (nullptr == m_snapshot)
        return;
    
    std::map<QUuid, QUuid> updatedMaterialIdMap;
    for (const auto &partIt: m_snapshot->parts) {
        QUuid materialId;
        auto materialIdIt = partIt.second.find("materialId");
        if (materialIdIt != partIt.second.end())
            materialId = QUuid(materialIdIt->second);
        QUuid partId = QUuid(partIt.first);
        updatedMaterialIdMap.insert({partId, materialId});
    }
    for (const auto &bmeshNode: m_outcome->nodes) {
        for (size_t i = 0; i < (int)TextureType::Count - 1; ++i) {
            TextureType forWhat = (TextureType)(i + 1);
            MaterialTextures materialTextures;
            QUuid materialId = bmeshNode.materialId;
            auto findUpdatedMaterialIdResult = updatedMaterialIdMap.find(bmeshNode.mirrorFromPartId.isNull() ? bmeshNode.partId : bmeshNode.mirrorFromPartId);
            if (findUpdatedMaterialIdResult != updatedMaterialIdMap.end())
                materialId = findUpdatedMaterialIdResult->second;
            initializeMaterialTexturesFromSnapshot(*m_snapshot, materialId, materialTextures);
            const QImage *image = materialTextures.textureImages[i];
            if (nullptr != image) {
                if (TextureType::BaseColor == forWhat)
                    addPartColorMap(bmeshNode.partId, image);
                else if (TextureType::Normal == forWhat)
                    addPartNormalMap(bmeshNode.partId, image);
                else if (TextureType::Metalness == forWhat)
                    addPartMetalnessMap(bmeshNode.partId, image);
                else if (TextureType::Roughness == forWhat)
                    addPartRoughnessMap(bmeshNode.partId, image);
                else if (TextureType::AmbientOcclusion == forWhat)
                    addPartAmbientOcclusionMap(bmeshNode.partId, image);
            }
        }
    }
}

void TextureGenerator::generate()
{
    if (nullptr == m_outcome->triangleVertexUvs())
        return;
    if (nullptr == m_outcome->triangleSourceNodes())
        return;
    if (nullptr == m_outcome->partUvRects())
        return;
    
    QElapsedTimer countTimeConsumed;
    countTimeConsumed.start();
    
    prepare();
    
    bool hasNormalMap = false;
    bool hasMetalnessMap = false;
    bool hasRoughnessMap = false;
    bool hasAmbientOcclusionMap = false;
    
    const auto &triangleVertexUvs = *m_outcome->triangleVertexUvs();
    const auto &triangleSourceNodes = *m_outcome->triangleSourceNodes();
    const auto &triangleNormals = m_outcome->triangleNormals;
    const auto &partUvRects = *m_outcome->partUvRects();
    
    std::map<QUuid, QColor> partColorMap;
    std::map<std::pair<QUuid, QUuid>, const OutcomeNode *> nodeMap;
    std::map<QUuid, float> partColorSolubilityMap;
    for (const auto &item: m_outcome->nodes) {
        nodeMap.insert({{item.partId, item.nodeId}, &item});
        partColorMap.insert({item.partId, item.color});
        partColorSolubilityMap.insert({item.partId, item.colorSolubility});
    }
    
    auto createImageBeginTime = countTimeConsumed.elapsed();
    
    m_resultTextureColorImage = new QImage(TextureGenerator::m_textureSize, TextureGenerator::m_textureSize, QImage::Format_ARGB32);
    m_resultTextureColorImage->fill(m_defaultTextureColor);
    
    m_resultTextureBorderImage = new QImage(TextureGenerator::m_textureSize, TextureGenerator::m_textureSize, QImage::Format_ARGB32);
    m_resultTextureBorderImage->fill(Qt::transparent);
    
    m_resultTextureNormalImage = new QImage(TextureGenerator::m_textureSize, TextureGenerator::m_textureSize, QImage::Format_ARGB32);
    m_resultTextureNormalImage->fill(QColor(127, 127, 127));
    
    m_resultTextureMetalnessRoughnessAmbientOcclusionImage = new QImage(TextureGenerator::m_textureSize, TextureGenerator::m_textureSize, QImage::Format_ARGB32);
    m_resultTextureMetalnessRoughnessAmbientOcclusionImage->fill(QColor(255, 255, 0));
    
    m_resultTextureMetalnessImage = new QImage(TextureGenerator::m_textureSize, TextureGenerator::m_textureSize, QImage::Format_ARGB32);
    m_resultTextureMetalnessImage->fill(Qt::black);
    
    m_resultTextureRoughnessImage = new QImage(TextureGenerator::m_textureSize, TextureGenerator::m_textureSize, QImage::Format_ARGB32);
    m_resultTextureRoughnessImage->fill(Qt::white);
    
    m_resultTextureAmbientOcclusionImage = new QImage(TextureGenerator::m_textureSize, TextureGenerator::m_textureSize, QImage::Format_ARGB32);
    m_resultTextureAmbientOcclusionImage->fill(Qt::white);
    
    auto createImageEndTime = countTimeConsumed.elapsed();
    
    QColor borderColor = Qt::darkGray;
    QPen pen(borderColor);
    
    QPainter texturePainter;
    texturePainter.begin(m_resultTextureColorImage);
    texturePainter.setRenderHint(QPainter::Antialiasing);
    texturePainter.setRenderHint(QPainter::HighQualityAntialiasing);
    
    QPainter textureBorderPainter;
    textureBorderPainter.begin(m_resultTextureBorderImage);
    textureBorderPainter.setRenderHint(QPainter::Antialiasing);
    textureBorderPainter.setRenderHint(QPainter::HighQualityAntialiasing);
    
    QPainter textureNormalPainter;
    textureNormalPainter.begin(m_resultTextureNormalImage);
    textureNormalPainter.setRenderHint(QPainter::Antialiasing);
    textureNormalPainter.setRenderHint(QPainter::HighQualityAntialiasing);
    
    QPainter textureMetalnessPainter;
    textureMetalnessPainter.begin(m_resultTextureMetalnessImage);
    textureMetalnessPainter.setRenderHint(QPainter::Antialiasing);
    textureMetalnessPainter.setRenderHint(QPainter::HighQualityAntialiasing);
    
    QPainter textureRoughnessPainter;
    textureRoughnessPainter.begin(m_resultTextureRoughnessImage);
    textureRoughnessPainter.setRenderHint(QPainter::Antialiasing);
    textureRoughnessPainter.setRenderHint(QPainter::HighQualityAntialiasing);
    
    QPainter textureAmbientOcclusionPainter;
    textureAmbientOcclusionPainter.begin(m_resultTextureAmbientOcclusionImage);
    textureAmbientOcclusionPainter.setRenderHint(QPainter::Antialiasing);
    textureAmbientOcclusionPainter.setRenderHint(QPainter::HighQualityAntialiasing);
    
    auto paintTextureBeginTime = countTimeConsumed.elapsed();
    texturePainter.setPen(Qt::NoPen);
    
    for (const auto &it: partUvRects) {
        const auto &partId = it.first;
        const auto &rects = it.second;
        auto findSourceColorResult = partColorMap.find(partId);
        if (findSourceColorResult != partColorMap.end()) {
            const auto &color = findSourceColorResult->second;
            QBrush brush(color);
            for (const auto &rect: rects) {
                QRectF translatedRect = {
                    rect.left() * TextureGenerator::m_textureSize,
                    rect.top() * TextureGenerator::m_textureSize,
                    rect.width() * TextureGenerator::m_textureSize,
                    rect.height() * TextureGenerator::m_textureSize
                };
                texturePainter.fillRect(translatedRect, brush);
            }
        }
    }
    
    auto drawGradient = [&](const QUuid &partId, size_t triangleIndex, size_t firstVertexIndex, size_t secondVertexIndex,
            const QUuid &neighborPartId) {
        const std::vector<QVector2D> &uv = triangleVertexUvs[triangleIndex];
        const auto &allRects = partUvRects.find(partId);
        if (allRects == partUvRects.end()) {
            qDebug() << "Found part uv rects failed";
            return;
        }
        const auto &firstPoint = uv[firstVertexIndex];
        const auto &secondPoint = uv[secondVertexIndex];
        auto edgeLength = firstPoint.distanceToPoint(secondPoint);
        auto middlePoint = (firstPoint + secondPoint) / 2.0;
        const auto &findColor = partColorMap.find(partId);
        if (findColor == partColorMap.end())
            return;
        const auto &findNeighborColorSolubility = partColorSolubilityMap.find(neighborPartId);
        if (findNeighborColorSolubility == partColorSolubilityMap.end())
            return;
        if (qFuzzyIsNull(findNeighborColorSolubility->second))
            return;
        const auto &findNeighborColor = partColorMap.find(neighborPartId);
        if (findNeighborColor == partColorMap.end())
            return;
        for (const auto &it: allRects->second) {
            if (it.contains(firstPoint.x(), firstPoint.y()) ||
                    it.contains(secondPoint.x(), secondPoint.y())) {
                float finalRadius = (it.width() + it.height()) * 0.5 * findNeighborColorSolubility->second;
                if (finalRadius < edgeLength)
                    finalRadius = edgeLength;
                QRadialGradient gradient(QPointF(middlePoint.x() * TextureGenerator::m_textureSize,
                    middlePoint.y() * TextureGenerator::m_textureSize),
                    finalRadius * TextureGenerator::m_textureSize);
                gradient.setColorAt(0.0, findNeighborColor->second);
                gradient.setColorAt(1.0, Qt::transparent);
                QRectF fillTarget((middlePoint.x() - finalRadius),
                    (middlePoint.y() - finalRadius),
                    (finalRadius + finalRadius),
                    (finalRadius + finalRadius));
                auto clippedRect = it.intersected(fillTarget);
                texturePainter.fillRect(clippedRect.left() * TextureGenerator::m_textureSize,
                    clippedRect.top() * TextureGenerator::m_textureSize,
                    clippedRect.width() * TextureGenerator::m_textureSize,
                    clippedRect.height() * TextureGenerator::m_textureSize,
                    gradient);
                break;
            }
        }
    };
    
    std::map<std::pair<size_t, size_t>, std::tuple<size_t, size_t, size_t>> halfEdgeToTriangleMap;
    for (size_t i = 0; i < m_outcome->triangles.size(); ++i) {
        const auto &triangleIndices = m_outcome->triangles[i];
        if (triangleIndices.size() != 3) {
            qDebug() << "Found invalid triangle indices";
            continue;
        }
        for (size_t j = 0; j < triangleIndices.size(); ++j) {
            size_t k = (j + 1) % triangleIndices.size();
            halfEdgeToTriangleMap.insert(std::make_pair(std::make_pair(triangleIndices[j], triangleIndices[k]),
                std::make_tuple(i, j, k)));
        }
    }
    for (const auto &it: halfEdgeToTriangleMap) {
        auto oppositeHalfEdge = std::make_pair(it.first.second, it.first.first);
        const auto &opposite = halfEdgeToTriangleMap.find(oppositeHalfEdge);
        if (opposite == halfEdgeToTriangleMap.end())
            continue;
        const std::pair<QUuid, QUuid> &source = triangleSourceNodes[std::get<0>(it.second)];
        const std::pair<QUuid, QUuid> &oppositeSource = triangleSourceNodes[std::get<0>(opposite->second)];
        if (source.first == oppositeSource.first)
            continue;
        drawGradient(source.first, std::get<0>(it.second), std::get<1>(it.second), std::get<2>(it.second), oppositeSource.first);
        drawGradient(oppositeSource.first, std::get<0>(opposite->second), std::get<1>(opposite->second), std::get<2>(opposite->second), source.first);
    }
    
    auto drawTexture = [&](const std::map<QUuid, QImage> &map, QPainter &painter) {
        for (const auto &it: partUvRects) {
            const auto &partId = it.first;
            const auto &rects = it.second;
            auto findTextureResult = map.find(partId);
            if (findTextureResult != map.end()) {
                const auto &image = findTextureResult->second;
                for (const auto &rect: rects) {
                    QRectF translatedRect = {
                        rect.left() * TextureGenerator::m_textureSize,
                        rect.top() * TextureGenerator::m_textureSize,
                        rect.width() * TextureGenerator::m_textureSize,
                        rect.height() * TextureGenerator::m_textureSize
                    };
                    painter.drawImage(translatedRect, image, QRectF(0, 0, image.width(), image.height()));
                }
            }
        }
    };
    
    drawTexture(m_partColorTextureMap, texturePainter);
    drawTexture(m_partNormalTextureMap, textureNormalPainter);
    drawTexture(m_partMetalnessTextureMap, textureMetalnessPainter);
    drawTexture(m_partRoughnessTextureMap, textureRoughnessPainter);
    drawTexture(m_partAmbientOcclusionTextureMap, textureAmbientOcclusionPainter);
    
    for (auto i = 0u; i < triangleVertexUvs.size(); i++) {
        QPainterPath path;
        const std::vector<QVector2D> &uv = triangleVertexUvs[i];
        float points[][2] = {
            {uv[0][0] * TextureGenerator::m_textureSize, uv[0][1] * TextureGenerator::m_textureSize},
            {uv[1][0] * TextureGenerator::m_textureSize, uv[1][1] * TextureGenerator::m_textureSize},
            {uv[2][0] * TextureGenerator::m_textureSize, uv[2][1] * TextureGenerator::m_textureSize}
        };
        path.moveTo(points[0][0], points[0][1]);
        path.lineTo(points[1][0], points[1][1]);
        path.lineTo(points[2][0], points[2][1]);
        path = expandedPainterPath(path);
        const std::pair<QUuid, QUuid> &source = triangleSourceNodes[i];
        // Copy normal texture if there is one
        auto findNormalTextureResult = m_partNormalTextureMap.find(source.first);
        if (findNormalTextureResult != m_partNormalTextureMap.end()) {
            //textureNormalPainter.setClipping(true);
            //textureNormalPainter.setClipPath(path);
            //textureNormalPainter.drawImage(0, 0, findNormalTextureResult->second);
            //textureNormalPainter.setClipping(false);
            hasNormalMap = true;
        } else {
            const auto &triangleNormal = triangleNormals[i];
            QColor brushColor;
            brushColor.setRedF((triangleNormal.x() + 1) / 2);
            brushColor.setGreenF((triangleNormal.y() + 1) / 2);
            brushColor.setBlueF((triangleNormal.z() + 1) / 2);
            textureNormalPainter.fillPath(path, brushColor);
        }
        // Copy metalness texture if there is one
        auto findMetalnessTextureResult = m_partMetalnessTextureMap.find(source.first);
        if (findMetalnessTextureResult != m_partMetalnessTextureMap.end()) {
            //textureMetalnessPainter.setClipping(true);
            //textureMetalnessPainter.setClipPath(path);
            //textureMetalnessPainter.drawImage(0, 0, findMetalnessTextureResult->second);
            //textureMetalnessPainter.setClipping(false);
            hasMetalnessMap = true;
        }
        // Copy roughness texture if there is one
        auto findRoughnessTextureResult = m_partRoughnessTextureMap.find(source.first);
        if (findRoughnessTextureResult != m_partRoughnessTextureMap.end()) {
            //textureRoughnessPainter.setClipping(true);
            //textureRoughnessPainter.setClipPath(path);
            //textureRoughnessPainter.drawImage(0, 0, findRoughnessTextureResult->second);
            //textureRoughnessPainter.setClipping(false);
            hasRoughnessMap = true;
        }
        // Copy ambient occlusion texture if there is one
        auto findAmbientOcclusionTextureResult = m_partAmbientOcclusionTextureMap.find(source.first);
        if (findAmbientOcclusionTextureResult != m_partAmbientOcclusionTextureMap.end()) {
            //textureAmbientOcclusionPainter.setClipping(true);
            //textureAmbientOcclusionPainter.setClipPath(path);
            //textureAmbientOcclusionPainter.drawImage(0, 0, findAmbientOcclusionTextureResult->second);
            //textureAmbientOcclusionPainter.setClipping(false);
            hasAmbientOcclusionMap = true;
        }
    }
    auto paintTextureEndTime = countTimeConsumed.elapsed();
    
    pen.setWidth(0);
    textureBorderPainter.setPen(pen);
    auto paintBorderBeginTime = countTimeConsumed.elapsed();
    for (auto i = 0u; i < triangleVertexUvs.size(); i++) {
        const std::vector<QVector2D> &uv = triangleVertexUvs[i];
        for (auto j = 0; j < 3; j++) {
            int from = j;
            int to = (j + 1) % 3;
            textureBorderPainter.drawLine(uv[from][0] * TextureGenerator::m_textureSize, uv[from][1] * TextureGenerator::m_textureSize,
                uv[to][0] * TextureGenerator::m_textureSize, uv[to][1] * TextureGenerator::m_textureSize);
        }
    }
    auto paintBorderEndTime = countTimeConsumed.elapsed();
    
    texturePainter.end();
    textureBorderPainter.end();
    textureNormalPainter.end();
    textureMetalnessPainter.end();
    textureRoughnessPainter.end();
    textureAmbientOcclusionPainter.end();
    
    if (!hasNormalMap) {
        delete m_resultTextureNormalImage;
        m_resultTextureNormalImage = nullptr;
    }
    
    auto mergeMetalnessRoughnessAmbientOcclusionBeginTime = countTimeConsumed.elapsed();
    if (!hasMetalnessMap && !hasRoughnessMap && !hasAmbientOcclusionMap) {
        delete m_resultTextureMetalnessRoughnessAmbientOcclusionImage;
        m_resultTextureMetalnessRoughnessAmbientOcclusionImage = nullptr;
    } else {
        for (int row = 0; row < m_resultTextureMetalnessRoughnessAmbientOcclusionImage->height(); ++row) {
            for (int col = 0; col < m_resultTextureMetalnessRoughnessAmbientOcclusionImage->width(); ++col) {
                QColor color(255, 255, 0);
                if (hasMetalnessMap)
                    color.setBlue(qGray(m_resultTextureMetalnessImage->pixel(col, row)));
                if (hasRoughnessMap)
                    color.setGreen(qGray(m_resultTextureRoughnessImage->pixel(col, row)));
                if (hasAmbientOcclusionMap)
                    color.setRed(qGray(m_resultTextureAmbientOcclusionImage->pixel(col, row)));
                m_resultTextureMetalnessRoughnessAmbientOcclusionImage->setPixelColor(col, row, color);
            }
        }
        if (!hasMetalnessMap) {
            delete m_resultTextureMetalnessImage;
            m_resultTextureMetalnessImage = nullptr;
        }
        if (!hasRoughnessMap) {
            delete m_resultTextureRoughnessImage;
            m_resultTextureRoughnessImage = nullptr;
        }
        if (!hasAmbientOcclusionMap) {
            delete m_resultTextureAmbientOcclusionImage;
            m_resultTextureAmbientOcclusionImage = nullptr;
        }
    }
    auto mergeMetalnessRoughnessAmbientOcclusionEndTime = countTimeConsumed.elapsed();
    
    m_resultTextureImage = new QImage(*m_resultTextureColorImage);
    
    m_resultTextureGuideImage = new QImage(*m_resultTextureImage);
    QPainter mergeTextureGuidePainter(m_resultTextureGuideImage);
    mergeTextureGuidePainter.setCompositionMode(QPainter::CompositionMode_Multiply);
    mergeTextureGuidePainter.drawImage(0, 0, *m_resultTextureBorderImage);
    mergeTextureGuidePainter.end();
    
    auto createResultBeginTime = countTimeConsumed.elapsed();
    m_resultMesh = new MeshLoader(*m_outcome);
    m_resultMesh->setTextureImage(new QImage(*m_resultTextureImage));
    if (nullptr != m_resultTextureNormalImage)
        m_resultMesh->setNormalMapImage(new QImage(*m_resultTextureNormalImage));
    if (nullptr != m_resultTextureMetalnessRoughnessAmbientOcclusionImage) {
        m_resultMesh->setMetalnessRoughnessAmbientOcclusionImage(new QImage(*m_resultTextureMetalnessRoughnessAmbientOcclusionImage));
        m_resultMesh->setHasMetalnessInImage(hasMetalnessMap);
        m_resultMesh->setHasRoughnessInImage(hasRoughnessMap);
        m_resultMesh->setHasAmbientOcclusionInImage(hasAmbientOcclusionMap);
    }
    auto createResultEndTime = countTimeConsumed.elapsed();
    
    qDebug() << "The texture[" << TextureGenerator::m_textureSize << "x" << TextureGenerator::m_textureSize << "] generation took" << countTimeConsumed.elapsed() << "milliseconds";
    qDebug() << "   :create image took" << (createImageEndTime - createImageBeginTime) << "milliseconds";
    qDebug() << "   :paint texture took" << (paintTextureEndTime - paintTextureBeginTime) << "milliseconds";
    qDebug() << "   :paint border took" << (paintBorderEndTime - paintBorderBeginTime) << "milliseconds";
    qDebug() << "   :merge metalness, roughness, and ambient occlusion texture took" << (mergeMetalnessRoughnessAmbientOcclusionEndTime - mergeMetalnessRoughnessAmbientOcclusionBeginTime) << "milliseconds";
    qDebug() << "   :create result took" << (createResultEndTime - createResultBeginTime) << "milliseconds";
}

void TextureGenerator::process()
{
    generate();

    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}
