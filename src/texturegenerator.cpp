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
#include "preferences.h"

QColor TextureGenerator::m_defaultTextureColor = Qt::transparent;

TextureGenerator::TextureGenerator(const Object &object, Snapshot *snapshot) :
    m_resultTextureColorImage(nullptr),
    m_resultTextureNormalImage(nullptr),
    m_resultTextureRoughnessImage(nullptr),
    m_resultTextureMetalnessImage(nullptr),
    m_resultTextureAmbientOcclusionImage(nullptr),
    m_resultMesh(nullptr),
    m_snapshot(snapshot),
    m_hasTransparencySettings(false),
    m_textureSize(Preferences::instance().textureSize())
{
    m_object = new Object();
    *m_object = object;
    if (m_textureSize <= 0)
        m_textureSize = 1024;
}

TextureGenerator::~TextureGenerator()
{
    delete m_object;
    delete m_resultTextureColorImage;
    delete m_resultTextureNormalImage;
    delete m_resultTextureRoughnessImage;
    delete m_resultTextureMetalnessImage;
    delete m_resultTextureAmbientOcclusionImage;
    delete m_resultMesh;
    delete m_snapshot;
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

Object *TextureGenerator::takeObject()
{
    Object *object = m_object;
    m_object = nullptr;
    return object;
}

Model *TextureGenerator::takeResultMesh()
{
    Model *resultMesh = m_resultMesh;
    m_resultMesh = nullptr;
    return resultMesh;
}

void TextureGenerator::addPartColorMap(QUuid partId, const QImage *image, float tileScale)
{
    if (nullptr == image)
        return;
    m_partColorTextureMap[partId] = std::make_pair(*image, tileScale);
}

void TextureGenerator::addPartNormalMap(QUuid partId, const QImage *image, float tileScale)
{
    if (nullptr == image)
        return;
    m_partNormalTextureMap[partId] = std::make_pair(*image, tileScale);
}

void TextureGenerator::addPartMetalnessMap(QUuid partId, const QImage *image, float tileScale)
{
    if (nullptr == image)
        return;
    m_partMetalnessTextureMap[partId] = std::make_pair(*image, tileScale);
}

void TextureGenerator::addPartRoughnessMap(QUuid partId, const QImage *image, float tileScale)
{
    if (nullptr == image)
        return;
    m_partRoughnessTextureMap[partId] = std::make_pair(*image, tileScale);
}

void TextureGenerator::addPartAmbientOcclusionMap(QUuid partId, const QImage *image, float tileScale)
{
    if (nullptr == image)
        return;
    m_partAmbientOcclusionTextureMap[partId] = std::make_pair(*image, tileScale);
}

void TextureGenerator::prepare()
{
    if (nullptr == m_snapshot)
        return;
    
    std::map<QUuid, QUuid> updatedMaterialIdMap;
    std::map<QUuid, bool> updatedCountershadedMap;
    for (const auto &partIt: m_snapshot->parts) {
        QUuid materialId;
        auto materialIdIt = partIt.second.find("materialId");
        if (materialIdIt != partIt.second.end())
            materialId = QUuid(materialIdIt->second);
        QUuid partId = QUuid(partIt.first);
        updatedMaterialIdMap.insert({partId, materialId});
        updatedCountershadedMap.insert({partId,
            isTrueValueString(valueOfKeyInMapOrEmpty(partIt.second, "countershaded"))});
    }
    for (const auto &bmeshNode: m_object->nodes) {
    
        bool countershaded = bmeshNode.countershaded;
        auto findUpdatedCountershadedMap = updatedCountershadedMap.find(bmeshNode.mirrorFromPartId.isNull() ? bmeshNode.partId : bmeshNode.mirrorFromPartId);
        if (findUpdatedCountershadedMap != updatedCountershadedMap.end())
            countershaded = findUpdatedCountershadedMap->second;
        if (countershaded)
            m_countershadedPartIds.insert(bmeshNode.partId);
        
        for (size_t i = 0; i < (int)TextureType::Count - 1; ++i) {
            TextureType forWhat = (TextureType)(i + 1);
            MaterialTextures materialTextures;
            QUuid materialId = bmeshNode.materialId;
            auto findUpdatedMaterialIdResult = updatedMaterialIdMap.find(bmeshNode.mirrorFromPartId.isNull() ? bmeshNode.partId : bmeshNode.mirrorFromPartId);
            if (findUpdatedMaterialIdResult != updatedMaterialIdMap.end())
                materialId = findUpdatedMaterialIdResult->second;
            float tileScale = 1.0;
            initializeMaterialTexturesFromSnapshot(*m_snapshot, materialId, materialTextures, tileScale);
            const QImage *image = materialTextures.textureImages[i];
            if (nullptr != image) {
                if (TextureType::BaseColor == forWhat)
                    addPartColorMap(bmeshNode.partId, image, tileScale);
                else if (TextureType::Normal == forWhat)
                    addPartNormalMap(bmeshNode.partId, image, tileScale);
                else if (TextureType::Metallic == forWhat)
                    addPartMetalnessMap(bmeshNode.partId, image, tileScale);
                else if (TextureType::Roughness == forWhat)
                    addPartRoughnessMap(bmeshNode.partId, image, tileScale);
                else if (TextureType::AmbientOcclusion == forWhat)
                    addPartAmbientOcclusionMap(bmeshNode.partId, image, tileScale);
            }
        }
    }
}

bool TextureGenerator::hasTransparencySettings()
{
    return m_hasTransparencySettings;
}

void TextureGenerator::generate()
{
    m_resultMesh = new Model(*m_object);
    
    if (nullptr == m_object->triangleVertexUvs())
        return;
    if (nullptr == m_object->triangleSourceNodes())
        return;
    if (nullptr == m_object->partUvRects())
        return;
    
    QElapsedTimer countTimeConsumed;
    countTimeConsumed.start();
    
    prepare();
    
    bool hasNormalMap = false;
    bool hasMetalnessMap = false;
    bool hasRoughnessMap = false;
    bool hasAmbientOcclusionMap = false;
    
    const auto &triangleVertexUvs = *m_object->triangleVertexUvs();
    const auto &triangleSourceNodes = *m_object->triangleSourceNodes();
    const auto &partUvRects = *m_object->partUvRects();
    const auto &triangleNormals = m_object->triangleNormals;
    
    std::map<QUuid, QColor> partColorMap;
    std::map<std::pair<QUuid, QUuid>, const ObjectNode *> nodeMap;
    std::map<QUuid, float> partColorSolubilityMap;
    std::map<QUuid, float> partMetalnessMap;
    std::map<QUuid, float> partRoughnessMap;
    for (const auto &item: m_object->nodes) {
        if (!m_hasTransparencySettings) {
            if (!qFuzzyCompare(1.0, item.color.alphaF()))
                m_hasTransparencySettings = true;
        }
        nodeMap.insert({{item.partId, item.nodeId}, &item});
        partColorMap.insert({item.partId, item.color});
        partColorSolubilityMap.insert({item.partId, item.colorSolubility});
        partMetalnessMap.insert({item.partId, item.metalness});
        partRoughnessMap.insert({item.partId, item.roughness});
    }
    
    auto createImageBeginTime = countTimeConsumed.elapsed();
    
    m_resultTextureColorImage = new QImage(TextureGenerator::m_textureSize, TextureGenerator::m_textureSize, QImage::Format_ARGB32);
    m_resultTextureColorImage->fill(m_hasTransparencySettings ? m_defaultTextureColor : Qt::white);
    
    m_resultTextureNormalImage = new QImage(TextureGenerator::m_textureSize, TextureGenerator::m_textureSize, QImage::Format_ARGB32);
    m_resultTextureNormalImage->fill(QColor(128, 128, 255));
    
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
            float fillExpandSize = 2;
            for (const auto &rect: rects) {
                QRectF translatedRect = {
                    rect.left() * TextureGenerator::m_textureSize - fillExpandSize,
                    rect.top() * TextureGenerator::m_textureSize - fillExpandSize,
                    rect.width() * TextureGenerator::m_textureSize + fillExpandSize * 2,
                    rect.height() * TextureGenerator::m_textureSize + fillExpandSize * 2
                };
                texturePainter.fillRect(translatedRect, brush);
            }
        }
    }
    
    for (const auto &it: partUvRects) {
        const auto &partId = it.first;
        const auto &rects = it.second;
        auto findMetalnessResult = partMetalnessMap.find(partId);
        if (findMetalnessResult != partMetalnessMap.end()) {
            if (qFuzzyCompare(findMetalnessResult->second, (float)0.0))
                continue;
            const auto &color = QColor(findMetalnessResult->second * 255,
                findMetalnessResult->second * 255,
                findMetalnessResult->second * 255);
            QBrush brush(color);
            float fillExpandSize = 2;
            for (const auto &rect: rects) {
                QRectF translatedRect = {
                    rect.left() * TextureGenerator::m_textureSize - fillExpandSize,
                    rect.top() * TextureGenerator::m_textureSize - fillExpandSize,
                    rect.width() * TextureGenerator::m_textureSize + fillExpandSize * 2,
                    rect.height() * TextureGenerator::m_textureSize + fillExpandSize * 2
                };
                textureMetalnessPainter.fillRect(translatedRect, brush);
                hasMetalnessMap = true;
            }
        }
    }
    
    for (const auto &it: partUvRects) {
        const auto &partId = it.first;
        const auto &rects = it.second;
        auto findRoughnessResult = partRoughnessMap.find(partId);
        if (findRoughnessResult != partRoughnessMap.end()) {
            if (qFuzzyCompare(findRoughnessResult->second, (float)1.0))
                continue;
            const auto &color = QColor(findRoughnessResult->second * 255,
                findRoughnessResult->second * 255,
                findRoughnessResult->second * 255);
            QBrush brush(color);
            float fillExpandSize = 2;
            for (const auto &rect: rects) {
                QRectF translatedRect = {
                    rect.left() * TextureGenerator::m_textureSize - fillExpandSize,
                    rect.top() * TextureGenerator::m_textureSize - fillExpandSize,
                    rect.width() * TextureGenerator::m_textureSize + fillExpandSize * 2,
                    rect.height() * TextureGenerator::m_textureSize + fillExpandSize * 2
                };
                textureRoughnessPainter.fillRect(translatedRect, brush);
                hasRoughnessMap = true;
            }
        }
    }
    
    auto drawTexture = [&](const std::map<QUuid, std::pair<QPixmap, QPixmap>> &map, QPainter &painter, bool useAlpha) {
        for (const auto &it: partUvRects) {
            const auto &partId = it.first;
            const auto &rects = it.second;
            float alpha = 1.0;
            if (useAlpha) {
                auto findSourceColorResult = partColorMap.find(partId);
                if (findSourceColorResult != partColorMap.end()) {
                    const auto &color = findSourceColorResult->second;
                    alpha = color.alphaF();
                }
            }
            auto findTextureResult = map.find(partId);
            if (findTextureResult != map.end()) {
                const auto &pixmap = findTextureResult->second.first;
                const auto &rotatedPixmap = findTextureResult->second.second;
                painter.setOpacity(alpha);
                for (const auto &rect: rects) {
                    QRectF translatedRect = {
                        rect.left() * TextureGenerator::m_textureSize,
                        rect.top() * TextureGenerator::m_textureSize,
                        rect.width() * TextureGenerator::m_textureSize,
                        rect.height() * TextureGenerator::m_textureSize
                    };
                    if (translatedRect.width() < translatedRect.height()) {
                        painter.drawTiledPixmap(translatedRect, rotatedPixmap, QPointF(rect.top(), rect.left()));
                    } else {
                        painter.drawTiledPixmap(translatedRect, pixmap, rect.topLeft());
                    }
                }
                painter.setOpacity(1.0);
            }
        }
    };
    
    auto convertTextureImageToPixmap = [&](const std::map<QUuid, std::pair<QImage, float>> &sourceMap,
            std::map<QUuid, std::pair<QPixmap, QPixmap>> &targetMap) {
        for (const auto &it: sourceMap) {
            float tileScale = it.second.second;
            const auto &image = it.second.first;
            auto newSize = image.size() * tileScale;
            QImage scaledImage = image.scaled(newSize);
            QPoint center = scaledImage.rect().center();
            QMatrix matrix;
            matrix.translate(center.x(), center.y());
            matrix.rotate(90);
            auto rotatedImage = scaledImage.transformed(matrix).mirrored(true, false);
            targetMap[it.first] = std::make_pair(QPixmap::fromImage(scaledImage),
                QPixmap::fromImage(rotatedImage));
        }
    };
    
    std::map<QUuid, std::pair<QPixmap, QPixmap>> partColorTexturePixmaps;
    std::map<QUuid, std::pair<QPixmap, QPixmap>> partNormalTexturePixmaps;
    std::map<QUuid, std::pair<QPixmap, QPixmap>> partMetalnessTexturePixmaps;
    std::map<QUuid, std::pair<QPixmap, QPixmap>> partRoughnessTexturePixmaps;
    std::map<QUuid, std::pair<QPixmap, QPixmap>> partAmbientOcclusionTexturePixmaps;
    
    convertTextureImageToPixmap(m_partColorTextureMap, partColorTexturePixmaps);
    convertTextureImageToPixmap(m_partNormalTextureMap, partNormalTexturePixmaps);
    convertTextureImageToPixmap(m_partMetalnessTextureMap, partMetalnessTexturePixmaps);
    convertTextureImageToPixmap(m_partRoughnessTextureMap, partRoughnessTexturePixmaps);
    convertTextureImageToPixmap(m_partAmbientOcclusionTextureMap, partAmbientOcclusionTexturePixmaps);
    
    drawTexture(partColorTexturePixmaps, texturePainter, true);
    drawTexture(partNormalTexturePixmaps, textureNormalPainter, false);
    drawTexture(partMetalnessTexturePixmaps, textureMetalnessPainter, false);
    drawTexture(partRoughnessTexturePixmaps, textureRoughnessPainter, false);
    drawTexture(partAmbientOcclusionTexturePixmaps, textureAmbientOcclusionPainter, false);
    
    auto drawBySolubility = [&](const QUuid &partId, size_t triangleIndex, size_t firstVertexIndex, size_t secondVertexIndex,
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
        float alpha = 1.0;
        const auto &findColor = partColorMap.find(partId);
        if (findColor == partColorMap.end())
            return;
        alpha = findColor->second.alphaF();
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
                QRectF fillTarget((middlePoint.x() - finalRadius),
                    (middlePoint.y() - finalRadius),
                    (finalRadius + finalRadius),
                    (finalRadius + finalRadius));
                auto clippedRect = it.intersected(fillTarget);
                QRectF translatedRect = {
                    clippedRect.left() * TextureGenerator::m_textureSize,
                    clippedRect.top() * TextureGenerator::m_textureSize,
                    clippedRect.width() * TextureGenerator::m_textureSize,
                    clippedRect.height() * TextureGenerator::m_textureSize
                };
                texturePainter.setOpacity(alpha);
                auto findTextureResult = partColorTexturePixmaps.find(neighborPartId);
                if (findTextureResult != partColorTexturePixmaps.end()) {
                    const auto &pixmap = findTextureResult->second.first;
                    const auto &rotatedPixmap = findTextureResult->second.second;
                    
                    QImage tmpImage(translatedRect.width(), translatedRect.height(), QImage::Format_ARGB32);
                    QPixmap tmpPixmap = QPixmap::fromImage(tmpImage);
                    QPainter tmpPainter;
                    QRectF tmpImageFrame = QRectF(0, 0, translatedRect.width(), translatedRect.height());
                    
                    // Fill tiled texture
                    tmpPainter.begin(&tmpPixmap);
                    tmpPainter.setOpacity(alpha);
                    if (it.width() < it.height()) {
                        tmpPainter.drawTiledPixmap(tmpImageFrame, rotatedPixmap, QPointF(translatedRect.top(), translatedRect.left()));
                    } else {
                        tmpPainter.drawTiledPixmap(tmpImageFrame, pixmap, translatedRect.topLeft());
                    }
                    tmpPainter.setOpacity(1.0);
                    tmpPainter.end();
                    
                    // Apply gradient
                    QRadialGradient gradient(QPointF(middlePoint.x() * TextureGenerator::m_textureSize - translatedRect.left(),
                        middlePoint.y() * TextureGenerator::m_textureSize - translatedRect.top()),
                        finalRadius * TextureGenerator::m_textureSize);
                    gradient.setColorAt(0.0, findNeighborColor->second);
                    gradient.setColorAt(1.0, Qt::transparent);
                    
                    tmpPainter.begin(&tmpPixmap);
                    tmpPainter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
                    tmpPainter.fillRect(tmpImageFrame, gradient);
                    tmpPainter.end();
                    
                    texturePainter.drawPixmap(translatedRect, tmpPixmap, tmpImageFrame);
                } else {
                    QRadialGradient gradient(QPointF(middlePoint.x() * TextureGenerator::m_textureSize,
                        middlePoint.y() * TextureGenerator::m_textureSize),
                        finalRadius * TextureGenerator::m_textureSize);
                    gradient.setColorAt(0.0, findNeighborColor->second);
                    gradient.setColorAt(1.0, Qt::transparent);
                    texturePainter.fillRect(translatedRect, gradient);
                }
                texturePainter.setOpacity(1.0);
                break;
            }
        }
    };
    
    std::map<std::pair<size_t, size_t>, std::tuple<size_t, size_t, size_t>> halfEdgeToTriangleMap;
    for (size_t i = 0; i < m_object->triangles.size(); ++i) {
        const auto &triangleIndices = m_object->triangles[i];
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
        drawBySolubility(source.first, std::get<0>(it.second), std::get<1>(it.second), std::get<2>(it.second), oppositeSource.first);
        drawBySolubility(oppositeSource.first, std::get<0>(opposite->second), std::get<1>(opposite->second), std::get<2>(opposite->second), source.first);
    }
    
    // Draw belly white
    texturePainter.setCompositionMode(QPainter::CompositionMode_SoftLight);
    for (size_t triangleIndex = 0; triangleIndex < m_object->triangles.size(); ++triangleIndex) {
        const auto &normal = triangleNormals[triangleIndex];
        const std::pair<QUuid, QUuid> &source = triangleSourceNodes[triangleIndex];
        const auto &partId = source.first;
        if (m_countershadedPartIds.find(partId) == m_countershadedPartIds.end())
            continue;
        
        const auto &allRects = partUvRects.find(partId);
        if (allRects == partUvRects.end()) {
            qDebug() << "Found part uv rects failed";
            continue;
        }
        
        const auto &findObjectNode = nodeMap.find(source);
        if (findObjectNode == nodeMap.end())
            continue;
        const ObjectNode *objectNode = findObjectNode->second;
        if (qAbs(QVector3D::dotProduct(objectNode->direction, QVector3D(0, 1, 0))) >= 0.707) {
            if (QVector3D::dotProduct(normal, QVector3D(0, 0, 1)) <= 0.0)
                continue;
        } else {
            if (QVector3D::dotProduct(normal, QVector3D(0, -1, 0)) <= 0.0)
                continue;
        }
        
        const auto &triangleIndices = m_object->triangles[triangleIndex];
        if (triangleIndices.size() != 3) {
            qDebug() << "Found invalid triangle indices";
            continue;
        }
        
        const std::vector<QVector2D> &uv = triangleVertexUvs[triangleIndex];
        QVector2D middlePoint = (uv[0] + uv[1] + uv[2]) / 3.0;
        float finalRadius = (uv[0].distanceToPoint(uv[1]) +
            uv[1].distanceToPoint(uv[2]) +
            uv[2].distanceToPoint(uv[0])) / 3.0;
        QRadialGradient gradient(QPointF(middlePoint.x() * TextureGenerator::m_textureSize,
            middlePoint.y() * TextureGenerator::m_textureSize),
            finalRadius * TextureGenerator::m_textureSize);
        gradient.setColorAt(0.0, Qt::white);
        gradient.setColorAt(1.0, Qt::transparent);
        for (const auto &it: allRects->second) {
            if (it.contains(middlePoint.x(), middlePoint.y())) {
                QRectF fillTarget((middlePoint.x() - finalRadius),
                    (middlePoint.y() - finalRadius),
                    (finalRadius + finalRadius),
                    (finalRadius + finalRadius));
                auto clippedRect = it.intersected(fillTarget);
                QRectF translatedRect = {
                    clippedRect.left() * TextureGenerator::m_textureSize,
                    clippedRect.top() * TextureGenerator::m_textureSize,
                    clippedRect.width() * TextureGenerator::m_textureSize,
                    clippedRect.height() * TextureGenerator::m_textureSize
                };
                texturePainter.fillRect(translatedRect, gradient);
            }
        }
        
        // Fill the neighbor halfedges
        for (int i = 0; i < 3; ++i) {
            int j = (i + 1) % 3;
            auto oppositeHalfEdge = std::make_pair(triangleIndices[j], triangleIndices[i]);
            const auto &opposite = halfEdgeToTriangleMap.find(oppositeHalfEdge);
            if (opposite == halfEdgeToTriangleMap.end())
                continue;
            auto oppositeTriangleIndex = std::get<0>(opposite->second);
            const std::pair<QUuid, QUuid> &oppositeSource = triangleSourceNodes[oppositeTriangleIndex];
            if (partId == oppositeSource.first)
                continue;
            const auto &oppositeAllRects = partUvRects.find(oppositeSource.first);
            if (oppositeAllRects == partUvRects.end()) {
                qDebug() << "Found part uv rects failed";
                continue;
            }
            const std::vector<QVector2D> &oppositeUv = triangleVertexUvs[oppositeTriangleIndex];
            QVector2D oppositeMiddlePoint = (oppositeUv[std::get<1>(opposite->second)] + oppositeUv[std::get<2>(opposite->second)]) * 0.5;
            QRadialGradient oppositeGradient(QPointF(oppositeMiddlePoint.x() * TextureGenerator::m_textureSize,
                oppositeMiddlePoint.y() * TextureGenerator::m_textureSize),
                finalRadius * TextureGenerator::m_textureSize);
            oppositeGradient.setColorAt(0.0, Qt::white);
            oppositeGradient.setColorAt(1.0, Qt::transparent);
            for (const auto &it: oppositeAllRects->second) {
                if (it.contains(oppositeMiddlePoint.x(), oppositeMiddlePoint.y())) {
                    QRectF fillTarget((oppositeMiddlePoint.x() - finalRadius),
                        (oppositeMiddlePoint.y() - finalRadius),
                        (finalRadius + finalRadius),
                        (finalRadius + finalRadius));
                    auto clippedRect = it.intersected(fillTarget);
                    QRectF translatedRect = {
                        clippedRect.left() * TextureGenerator::m_textureSize,
                        clippedRect.top() * TextureGenerator::m_textureSize,
                        clippedRect.width() * TextureGenerator::m_textureSize,
                        clippedRect.height() * TextureGenerator::m_textureSize
                    };
                    texturePainter.fillRect(translatedRect, oppositeGradient);
                }
            }
        }
    }
    
    hasNormalMap = !m_partNormalTextureMap.empty();
    if (!m_partMetalnessTextureMap.empty())
        hasMetalnessMap = true;
    if (!m_partRoughnessTextureMap.empty())
        hasRoughnessMap = true;
    hasAmbientOcclusionMap = !m_partAmbientOcclusionTextureMap.empty();
    
    auto paintTextureEndTime = countTimeConsumed.elapsed();

    texturePainter.end();
    textureNormalPainter.end();
    textureMetalnessPainter.end();
    textureRoughnessPainter.end();
    textureAmbientOcclusionPainter.end();
    
    if (!hasNormalMap) {
        delete m_resultTextureNormalImage;
        m_resultTextureNormalImage = nullptr;
    }
    
    if (!hasMetalnessMap && !hasRoughnessMap && !hasAmbientOcclusionMap) {
        delete m_resultTextureMetalnessImage;
        m_resultTextureMetalnessImage = nullptr;

        delete m_resultTextureRoughnessImage;
        m_resultTextureRoughnessImage = nullptr;

        delete m_resultTextureAmbientOcclusionImage;
        m_resultTextureAmbientOcclusionImage = nullptr;
    }
    
    auto createResultBeginTime = countTimeConsumed.elapsed();
    m_resultMesh->setTextureImage(new QImage(*m_resultTextureColorImage));
    if (nullptr != m_resultTextureNormalImage)
        m_resultMesh->setNormalMapImage(new QImage(*m_resultTextureNormalImage));
    if (hasMetalnessMap || hasRoughnessMap || hasAmbientOcclusionMap) {
        m_resultMesh->setMetalnessRoughnessAmbientOcclusionImage(combineMetalnessRoughnessAmbientOcclusionImages(
            m_resultTextureMetalnessImage,
            m_resultTextureRoughnessImage,
            m_resultTextureAmbientOcclusionImage));
        m_resultMesh->setHasMetalnessInImage(hasMetalnessMap);
        m_resultMesh->setHasRoughnessInImage(hasRoughnessMap);
        m_resultMesh->setHasAmbientOcclusionInImage(hasAmbientOcclusionMap);
    }
    auto createResultEndTime = countTimeConsumed.elapsed();
    
    qDebug() << "The texture[" << TextureGenerator::m_textureSize << "x" << TextureGenerator::m_textureSize << "] generation took" << countTimeConsumed.elapsed() << "milliseconds";
}

QImage *TextureGenerator::combineMetalnessRoughnessAmbientOcclusionImages(QImage *metalnessImage,
        QImage *roughnessImage,
        QImage *ambientOcclusionImage)
{
    QImage *textureMetalnessRoughnessAmbientOcclusionImage = nullptr;
    if (nullptr != metalnessImage ||
            nullptr != roughnessImage ||
            nullptr != ambientOcclusionImage) {
        int textureSize = 0;
        if (nullptr != metalnessImage)
            textureSize = metalnessImage->height();
        if (nullptr != roughnessImage)
            textureSize = roughnessImage->height();
        if (nullptr != ambientOcclusionImage)
            textureSize = ambientOcclusionImage->height();
        if (textureSize > 0) {
            textureMetalnessRoughnessAmbientOcclusionImage = new QImage(textureSize, textureSize, QImage::Format_ARGB32);
            textureMetalnessRoughnessAmbientOcclusionImage->fill(QColor(255, 255, 0));
            for (int row = 0; row < textureMetalnessRoughnessAmbientOcclusionImage->height(); ++row) {
                for (int col = 0; col < textureMetalnessRoughnessAmbientOcclusionImage->width(); ++col) {
                    QColor color(255, 255, 0);
                    if (nullptr != metalnessImage)
                        color.setBlue(qGray(metalnessImage->pixel(col, row)));
                    if (nullptr != roughnessImage)
                        color.setGreen(qGray(roughnessImage->pixel(col, row)));
                    if (nullptr != ambientOcclusionImage)
                        color.setRed(qGray(ambientOcclusionImage->pixel(col, row)));
                    textureMetalnessRoughnessAmbientOcclusionImage->setPixelColor(col, row, color);
                }
            }
        }
    }
    return textureMetalnessRoughnessAmbientOcclusionImage;
}

void TextureGenerator::process()
{
    generate();

    emit finished();
}
