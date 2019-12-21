#include "imageskeletonextractor.h"

// This is an implementation of the following paper:
// <A Fast Parallel Algorithm for Thinning Digital Patterns>
// T. Y. ZHANG and C. Y. SUEN

const int ImageSkeletonExtractor::m_targetHeight = 256;

ImageSkeletonExtractor::~ImageSkeletonExtractor()
{
    delete m_image;
    delete m_grayscaleImage;
}

void ImageSkeletonExtractor::setImage(QImage *image)
{
    delete m_image;
    m_image = image;
}

QImage *ImageSkeletonExtractor::takeResultGrayscaleImage()
{
    QImage *resultImage = m_grayscaleImage;
    m_grayscaleImage = nullptr;
    return resultImage;
}

bool ImageSkeletonExtractor::firstSubiterationSatisfied(int i, int j)
{
    if (!isBlack(i, j))
        return false;
    auto blackNeighbors = countBlackNeighbors(i, j);
    if (blackNeighbors < 2 || blackNeighbors > 6)
        return false;
    auto neighborTransitions = countNeighborTransitions(i, j);
    if (1 != neighborTransitions)
        return false;
    if (isBlack(i + neighborOffsets[P2].first, j + neighborOffsets[P2].second) &&
            isBlack(i + neighborOffsets[P4].first, j + neighborOffsets[P4].second) &&
            isBlack(i + neighborOffsets[P6].first, j + neighborOffsets[P6].second)) {
        return false;
    }
    if (isBlack(i + neighborOffsets[P4].first, j + neighborOffsets[P4].second) &&
            isBlack(i + neighborOffsets[P6].first, j + neighborOffsets[P6].second) &&
            isBlack(i + neighborOffsets[P8].first, j + neighborOffsets[P8].second)) {
        return false;
    }
    return true;
}

bool ImageSkeletonExtractor::secondSubiterationSatisfied(int i, int j)
{
    if (!isBlack(i, j))
        return false;
    auto blackNeighbors = countBlackNeighbors(i, j);
    if (blackNeighbors < 2 || blackNeighbors > 6)
        return false;
    auto neighborTransitions = countNeighborTransitions(i, j);
    if (1 != neighborTransitions)
        return false;
    if (isBlack(i + neighborOffsets[P2].first, j + neighborOffsets[P2].second) &&
            isBlack(i + neighborOffsets[P4].first, j + neighborOffsets[P4].second) &&
            isBlack(i + neighborOffsets[P8].first, j + neighborOffsets[P8].second)) {
        return false;
    }
    if (isBlack(i + neighborOffsets[P2].first, j + neighborOffsets[P2].second) &&
            isBlack(i + neighborOffsets[P6].first, j + neighborOffsets[P6].second) &&
            isBlack(i + neighborOffsets[P8].first, j + neighborOffsets[P8].second)) {
        return false;
    }
    return true;
}

void ImageSkeletonExtractor::extract()
{
    m_grayscaleImage = new QImage(m_image->convertToFormat(
        QImage::Format_Grayscale8).scaled(
            QSize(m_targetHeight, m_targetHeight), Qt::KeepAspectRatio));
    
    while (true) {
        std::vector<std::pair<int, int>> firstSatisfied;
        for (int i = 1; i < (int)m_grayscaleImage->width() - 1; ++i) {
            for (int j = 1; j < (int)m_grayscaleImage->height() - 1; ++j) {
                if (firstSubiterationSatisfied(i, j))
                    firstSatisfied.push_back(std::make_pair(i, j));
            }
        }
        for (const auto &it: firstSatisfied)
            setWhite(it.first, it.second);
        std::vector<std::pair<int, int>> secondSatisfied;
        for (int i = 1; i < (int)m_grayscaleImage->width() - 1; ++i) {
            for (int j = 1; j < (int)m_grayscaleImage->height() - 1; ++j) {
                if (secondSubiterationSatisfied(i, j))
                    secondSatisfied.push_back(std::make_pair(i, j));
            }
        }
        for (const auto &it: secondSatisfied)
            setWhite(it.first, it.second);
        if (firstSatisfied.empty() && secondSatisfied.empty())
            break;
        printf("firstSatisfied:%d\r\n", firstSatisfied.size());
        printf("secondSatisfied:%d\r\n", secondSatisfied.size());
    }
}
