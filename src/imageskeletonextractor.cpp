#include <QDebug>
#include <queue>
#include <set>
#include "imageskeletonextractor.h"

// This is an implementation of the following paper:
// <A Fast Parallel Algorithm for Thinning Digital Patterns>
// T. Y. ZHANG and C. Y. SUEN

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

void ImageSkeletonExtractor::calculateAreaAndBlackPixels()
{
    m_area = 0;
    m_blackPixels.clear();
    for (int i = 1; i < (int)m_grayscaleImage->width() - 1; ++i) {
        for (int j = 1; j < (int)m_grayscaleImage->height() - 1; ++j) {
            if (isBlack(i, j)) {
                ++m_area;
                m_blackPixels.insert({i, j});
            }
        }
    }
}

const std::set<std::pair<int, int>> &ImageSkeletonExtractor::getBlackPixels()
{
    return m_blackPixels;
}

int ImageSkeletonExtractor::getArea()
{
    return m_area;
}

void ImageSkeletonExtractor::extract()
{
    m_grayscaleImage = new QImage(m_image->convertToFormat(QImage::Format_Grayscale8));
    calculateAreaAndBlackPixels();
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
    }
}

void ImageSkeletonExtractor::getSkeleton(std::vector<std::pair<int, int>> *skeleton)
{
    if (nullptr == m_grayscaleImage)
        return;
    
    std::map<std::pair<int, int>, std::vector<std::pair<int, int>>> links;
    for (int i = 1; i < (int)m_grayscaleImage->width() - 1; ++i) {
        for (int j = 1; j < (int)m_grayscaleImage->height() - 1; ++j) {
            if (!isBlack(i, j))
                continue;
            auto ij = std::make_pair(i, j);
            auto p2 = std::make_pair(i + neighborOffsets[P2].first, j + neighborOffsets[P2].second);
            bool hasP3 = true;
            bool hasP5 = true;
            bool hasP7 = true;
            bool hasP9 = true;
            if (isBlack(p2.first, p2.second)) {
                links[ij].push_back(p2);
                hasP3 = false;
                hasP9 = false;
            }
            auto p4 = std::make_pair(i + neighborOffsets[P4].first, j + neighborOffsets[P4].second);
            if (isBlack(p4.first, p4.second)) {
                links[ij].push_back(p4);
                hasP3 = false;
                hasP5 = false;
            }
            auto p6 = std::make_pair(i + neighborOffsets[P6].first, j + neighborOffsets[P6].second);
            if (isBlack(p6.first, p6.second)) {
                links[ij].push_back(p6);
                hasP5 = false;
                hasP7 = false;
            }
            auto p8 = std::make_pair(i + neighborOffsets[P8].first, j + neighborOffsets[P8].second);
            if (isBlack(p8.first, p8.second)) {
                links[ij].push_back(p8);
                hasP7 = false;
                hasP9 = false;
            }
            if (hasP3) {
                auto p3 = std::make_pair(i + neighborOffsets[P3].first, j + neighborOffsets[P3].second);
                if (isBlack(p3.first, p3.second)) {
                    links[ij].push_back(p3);
                }
            }
            if (hasP5) {
                auto p5 = std::make_pair(i + neighborOffsets[P5].first, j + neighborOffsets[P5].second);
                if (isBlack(p5.first, p5.second)) {
                    links[ij].push_back(p5);
                }
            }
            if (hasP7) {
                auto p7 = std::make_pair(i + neighborOffsets[P7].first, j + neighborOffsets[P7].second);
                if (isBlack(p7.first, p7.second)) {
                    links[ij].push_back(p7);
                }
            }
            if (hasP9) {
                auto p9 = std::make_pair(i + neighborOffsets[P9].first, j + neighborOffsets[P9].second);
                if (isBlack(p9.first, p9.second)) {
                    links[ij].push_back(p9);
                }
            }
        }
    }

    auto calculateRouteLength = [&](const std::pair<int, int> &branch, const std::pair<int, int> &start) {
        std::set<std::pair<int, int>> visited;
        visited.insert(branch);
        std::queue<std::pair<int, int>> waitPoints;
        waitPoints.push(start);
        size_t addLength = 0;
        while (!waitPoints.empty()) {
            auto point = waitPoints.front();
            waitPoints.pop();
            if (visited.find(point) != visited.end())
                continue;
            visited.insert(point);
            auto findLink = links.find(point);
            if (findLink == links.end())
                break;
            if (findLink->second.size() > 2) {
                addLength = links.size(); // This will make sure the branch node is not been removed
                break;
            }
            for (const auto &it: findLink->second) {
                if (visited.find(it) != visited.end())
                    continue;
                waitPoints.push(it);
            }
        }
        return visited.size() + addLength;
    };
    for (auto &it: links) {
        if (it.second.size() > 2) {
            std::vector<std::pair<std::pair<int, int>, size_t>> routes;
            routes.reserve(it.second.size());
            for (size_t i = 0; i < it.second.size(); ++i) {
                routes.push_back(std::make_pair(it.second[i], calculateRouteLength(it.first, it.second[i])));
            }
            std::sort(routes.begin(), routes.end(), [](const std::pair<std::pair<int, int>, size_t> &first,
                    const std::pair<std::pair<int, int>, size_t> &second) {
                return first.second < second.second;
            });
            it.second = std::vector<std::pair<int, int>> {routes[routes.size() - 2].first,
                routes[routes.size() - 1].first};
        }
    }
    
    std::queue<std::pair<int, int>> waitPoints;
    for (const auto &it: links) {
        if (1 == it.second.size()) {
            waitPoints.push(it.first);
            break;
        }
    }
    std::set<std::pair<int, int>> visited;
    while (!waitPoints.empty()) {
        auto point = waitPoints.front();
        waitPoints.pop();
        if (visited.find(point) != visited.end())
            continue;
        visited.insert(point);
        skeleton->push_back(point);
        auto findLink = links.find(point);
        if (findLink == links.end())
            break;
        for (const auto &it: findLink->second) {
            if (visited.find(it) != visited.end())
                continue;
            waitPoints.push(it);
        }
    }
}
