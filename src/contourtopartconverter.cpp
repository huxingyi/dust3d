#include <QImage>
#include <QPainter>
#include <cmath>
#include <QUuid>
#include <cmath>
#include <QTransform>
#include "contourtopartconverter.h"
#include "imageskeletonextractor.h"
#include "util.h"

const float ContourToPartConverter::m_targetImageHeight = 64.0f;
const float ContourToPartConverter::m_minEdgeLength = 0.025;
const float ContourToPartConverter::m_radiusEpsilon = 0.0025;

ContourToPartConverter::ContourToPartConverter(const QPolygonF &mainProfile,
        const QPolygonF &sideProfile, const QSizeF &canvasSize) :
    m_mainProfile(mainProfile),
    m_sideProfile(sideProfile),
    m_canvasSize(canvasSize)
{
}

void ContourToPartConverter::process()
{
    convert();
    emit finished();
}

const Snapshot &ContourToPartConverter::getSnapshot()
{
    return m_snapshot;
}

void ContourToPartConverter::extractSkeleton(const QPolygonF &polygon,
        std::vector<std::pair<QVector2D, float>> *skeleton)
{
    auto originalBoundingBox = polygon.boundingRect();
    QPointF polygonTopLeft = originalBoundingBox.topLeft();
    
    float scaleFactor = m_targetImageHeight / originalBoundingBox.height();
    
    QTransform transform;
    transform = transform.scale(scaleFactor, scaleFactor);
    QPolygonF scaledPolygon = transform.map(polygon);
    
    QRectF boundingBox = scaledPolygon.boundingRect();
    QImage *image = new QImage(boundingBox.width() + 4, boundingBox.height() + 4, QImage::Format_Grayscale8);
    image->fill(QColor(255, 255, 255));
    
    qreal offsetX = 1 - boundingBox.left();
    qreal offsetY = 1 - boundingBox.top();
    
    QPainterPath path;
    path.addPolygon(scaledPolygon.translated(offsetX, offsetY));
    
    QPainter painter;
    painter.begin(image);
    painter.setPen(Qt::PenStyle::NoPen);
    painter.fillPath(path, Qt::black);
    painter.end();
    
    ImageSkeletonExtractor imageSkeletonExtractor;
    imageSkeletonExtractor.setImage(image);
    imageSkeletonExtractor.extract();
    
    std::vector<std::pair<int, int>> imageSkeleton;
    int imageArea = imageSkeletonExtractor.getArea();
    imageSkeletonExtractor.getSkeleton(&imageSkeleton);
    const std::set<std::pair<int, int>> &blackPixels = imageSkeletonExtractor.getBlackPixels();
    
    std::vector<std::pair<int, int>> selectedNodes;
    if (imageSkeleton.size() >= 2) {
        int targetLength = std::ceil(0.5 * (float)imageArea / imageSkeleton.size());
        int minLength = m_minEdgeLength * imageSkeleton.size();
        if (targetLength < minLength)
            targetLength = minLength;
        size_t newInsertNum = imageSkeleton.size() / targetLength;
        if (newInsertNum < 1)
            newInsertNum = 1;
        if (newInsertNum > 100)
            newInsertNum = 100;
        float stepFactor = 1.0 / (newInsertNum + 1);
        float factor = stepFactor;
        selectedNodes.push_back(imageSkeleton[0]);
        for (size_t i = 0; i < newInsertNum && factor < 1.0; factor += stepFactor, ++i) {
            size_t index = factor * imageSkeleton.size();
            if (index <= 0 || index >= imageSkeleton.size())
                continue;
            selectedNodes.push_back(imageSkeleton[index]);
        }
        selectedNodes.push_back(imageSkeleton[imageSkeleton.size() - 1]);
    }
    
    std::vector<QVector3D> selectedPositions;
    selectedPositions.reserve(selectedNodes.size());
    for (const auto &it: selectedNodes)
        selectedPositions.push_back(QVector3D(it.first, it.second, 0.0f));
    
    std::vector<QVector3D> selectedDirections;
    selectedDirections.reserve(selectedNodes.size());
    for (size_t i = 0; i < selectedPositions.size(); ++i) {
        QVector3D sumOfDirections;
        if (i > 0) {
            sumOfDirections += selectedPositions[i] - selectedPositions[i - 1];
        }
        if (i + 1 < selectedPositions.size()) {
            sumOfDirections += selectedPositions[i + 1] - selectedPositions[i];
        }
        selectedDirections.push_back(sumOfDirections.normalized());
    }
    
    std::vector<int> selectedRadius;
    selectedRadius.reserve(selectedNodes.size());
    for (size_t i = 0; i < selectedDirections.size(); ++i) {
        selectedRadius.push_back(calculateNodeRadius(selectedPositions[i], selectedDirections[i],
            blackPixels));
    }
    
    skeleton->resize(selectedRadius.size());
    auto canvasHeight = m_canvasSize.height();
    for (size_t i = 0; i < skeleton->size(); ++i) {
        const auto &node = selectedNodes[i];
        (*skeleton)[i] = std::make_pair(QVector2D(node.first / scaleFactor + polygonTopLeft.x(),
                node.second / scaleFactor + polygonTopLeft.y()) / canvasHeight,
            selectedRadius[i] / scaleFactor / canvasHeight);
    }
}

int ContourToPartConverter::calculateNodeRadius(const QVector3D &node,
        const QVector3D &direction,
        const std::set<std::pair<int, int>> &black)
{
    const QVector3D pointer = {0.0f, 0.0f, 1.0f};
    QVector3D offsetDirection = QVector3D::crossProduct(direction, pointer);
    int radius = 1;
    while (true) {
        QVector3D offset = radius * offsetDirection;
        QVector3D sidePosition = node + offset;
        QVector3D otherSidePosition = node - offset;
        if (black.find(std::make_pair((int)sidePosition.x(), (int)sidePosition.y())) == black.end())
            break;
        if (black.find(std::make_pair((int)otherSidePosition.x(), (int)otherSidePosition.y())) == black.end())
            break;
        ++radius;
    }
    return radius;
}

void ContourToPartConverter::smoothRadius(std::vector<std::pair<QVector2D, float>> *skeleton)
{
    if (skeleton->empty())
        return;
    
    std::vector<float> newRadius;
    newRadius.reserve(skeleton->size() + 2);
    newRadius.push_back(skeleton->front().second);
    for (const auto &it: (*skeleton)) {
        newRadius.push_back(it.second);
    }
    newRadius.push_back(skeleton->back().second);
    for (size_t h = 0; h < skeleton->size(); ++h) {
        size_t i = h + 1;
        size_t j = h + 2;
        (*skeleton)[h].second = (newRadius[h] +
            newRadius[i] + newRadius[j]) / 3;
    }
}

void ContourToPartConverter::optimizeNodes()
{
    auto oldNodes = m_nodes;
    m_nodes.clear();
    m_nodes.reserve(oldNodes.size());
    for (size_t i = 0; i < oldNodes.size(); ++i) {
        if (i > 0 && i + 1 < oldNodes.size()) {
            size_t h = i - 1;
            size_t j = i + 1;
            if (std::abs(oldNodes[i].second - oldNodes[h].second) < m_radiusEpsilon &&
                    std::abs(oldNodes[i].second - oldNodes[j].second) < m_radiusEpsilon) {
                auto degrees = degreesBetweenVectors((oldNodes[i].first - oldNodes[h].first).normalized(),
                    (oldNodes[j].first - oldNodes[i].first).normalized());
                if (degrees < 15)
                    continue;
            }
        }
        m_nodes.push_back(oldNodes[i]);
    }
}

void ContourToPartConverter::alignSkeleton(const std::vector<std::pair<QVector2D, float>> &referenceSkeleton,
        std::vector<std::pair<QVector2D, float>> &adjustSkeleton)
{
    if (referenceSkeleton.empty() || adjustSkeleton.empty())
        return;
    float sumOfDistance2 = 0.0;
    float reversedSumOfDistance2 = 0.0;
    for (size_t i = 0; i < adjustSkeleton.size(); ++i) {
        size_t j = ((float)i / adjustSkeleton.size()) * referenceSkeleton.size();
        if (j >= referenceSkeleton.size())
            continue;
        size_t k = referenceSkeleton.size() - 1 - j;
        sumOfDistance2 += std::pow(adjustSkeleton[i].first.y() - referenceSkeleton[j].first.y(), 2.0f);
        reversedSumOfDistance2 += std::pow(adjustSkeleton[i].first.y() - referenceSkeleton[k].first.y(), 2.0f);
    }
    if (sumOfDistance2 <= reversedSumOfDistance2)
        return;
    std::reverse(adjustSkeleton.begin(), adjustSkeleton.end());
}

void ContourToPartConverter::convert()
{
    std::vector<std::pair<QVector2D, float>> sideSkeleton;
    std::vector<std::pair<QVector2D, float>> mainSkeleton;
    auto mainBoundingBox = m_mainProfile.boundingRect();
    extractSkeleton(m_sideProfile, &sideSkeleton);
    extractSkeleton(m_mainProfile, &mainSkeleton);
    smoothRadius(&sideSkeleton);
    smoothRadius(&mainSkeleton);
    if (!sideSkeleton.empty()) {
        alignSkeleton(sideSkeleton, mainSkeleton);
        float defaultX = mainBoundingBox.center().x() / m_canvasSize.height();
        float area = mainBoundingBox.width() * mainBoundingBox.height();
        float mainBoundingBoxWidthHeightOffset = std::abs(mainBoundingBox.width() - mainBoundingBox.height());
        float rectRadius = std::sqrt(area) * 0.5;
        bool useCalculatedX = mainBoundingBoxWidthHeightOffset >= rectRadius;
        m_nodes.reserve(sideSkeleton.size());
        for (size_t i = 0; i < sideSkeleton.size(); ++i) {
            const auto &it = sideSkeleton[i];
            float x = defaultX;
            if (useCalculatedX) {
                size_t j = ((float)i / sideSkeleton.size()) * mainSkeleton.size();
                x = j < mainSkeleton.size() ? mainSkeleton[j].first.x() : defaultX;
            }
            m_nodes.push_back(std::make_pair(QVector3D(x, it.first.y(), it.first.x()),
                it.second));
        }
    }
    optimizeNodes();
    nodesToSnapshot();
}

void ContourToPartConverter::nodesToSnapshot()
{
    if (m_nodes.empty())
        return;
    
    auto partId = QUuid::createUuid();
    auto partIdString = partId.toString();
    std::map<QString, QString> snapshotPart;
    snapshotPart["id"] = partIdString;
    snapshotPart["subdived"] = "true";
    snapshotPart["rounded"] = "true";
    snapshotPart["base"] = "YZ";
    m_snapshot.parts[partIdString] = snapshotPart;
    
    auto componentId = QUuid::createUuid();
    auto componentIdString = componentId.toString();
    std::map<QString, QString> snapshotComponent;
    snapshotComponent["id"] = componentIdString;
    snapshotComponent["combineMode"] = "Normal";
    snapshotComponent["linkDataType"] = "partId";
    snapshotComponent["linkData"] = partIdString;
    m_snapshot.components[componentIdString] = snapshotComponent;
    
    m_snapshot.rootComponent["children"] = componentIdString;
    
    std::vector<QString> nodeIdStrings;
    nodeIdStrings.reserve(m_nodes.size());
    for (const auto &it: m_nodes) {
        auto nodeId = QUuid::createUuid();
        auto nodeIdString = nodeId.toString();
        std::map<QString, QString> snapshotNode;
        snapshotNode["id"] = nodeIdString;
        snapshotNode["x"] = QString::number(it.first.x());
        snapshotNode["y"] = QString::number(it.first.y());
        snapshotNode["z"] = QString::number(it.first.z());
        snapshotNode["radius"] = QString::number(it.second);
        snapshotNode["partId"] = partIdString;
        m_snapshot.nodes[nodeIdString] = snapshotNode;
        nodeIdStrings.push_back(nodeIdString);
    }
    
    for (size_t i = 1; i < nodeIdStrings.size(); ++i) {
        size_t h = i - 1;
        auto edgeId = QUuid::createUuid();
        auto edgeIdString = edgeId.toString();
        std::map<QString, QString> snapshotEdge;
        snapshotEdge["id"] = edgeIdString;
        snapshotEdge["from"] = nodeIdStrings[h];
        snapshotEdge["to"] = nodeIdStrings[i];
        snapshotEdge["partId"] = partIdString;
        m_snapshot.edges[edgeIdString] = snapshotEdge;
    }
}

