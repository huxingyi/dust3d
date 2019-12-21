#ifndef DUST3D_IMAGE_SKELETON_EXTRACTOR_H
#define DUST3D_IMAGE_SKELETON_EXTRACTOR_H
#include <QImage>
#include <QObject>
#include <vector>

class ImageSkeletonExtractor : QObject
{
    Q_OBJECT
public:
    const std::vector<std::pair<int, int>> neighborOffsets = {
        { 0, -1},
        { 1, -1},
        { 1,  0},
        { 1,  1},
        { 0,  1},
        {-1,  1},
        {-1,  0},
        {-1, -1},
    };
    enum {
        P2 = 0,
        P3,
        P4,
        P5,
        P6,
        P7,
        P8,
        P9
    };

    ~ImageSkeletonExtractor();
    void setImage(QImage *image);
    void extract();
    QImage *takeResultGrayscaleImage();
private:
    QImage *m_image = nullptr;
    QImage *m_grayscaleImage = nullptr;
    static const int m_targetHeight;
    
    bool isBlack(int i, int j)
    {
        return QColor(m_grayscaleImage->pixel(i, j)).black() > 0;
    }
    
    bool isWhite(int i, int j)
    {
        return !isBlack(i, j);
    }
    
    void setWhite(int i, int j)
    {
        m_grayscaleImage->setPixel(i, j, qRgb(255, 255, 255));
    }
    
    int countNeighborTransitions(int i, int j)
    {
        int num = 0;
        for (size_t m = 0; m < neighborOffsets.size(); ++m) {
            size_t n = (m + 1) % neighborOffsets.size();
            if (isWhite(i + neighborOffsets[m].first, j + neighborOffsets[m].second) &&
                    isBlack(i + neighborOffsets[n].first, j + neighborOffsets[n].second)) {
                ++num;
            }
        }
        return num;
    }
    
    int countBlackNeighbors(int i, int j)
    {
        int num = 0;
        for (const auto &it: neighborOffsets) {
            if (isBlack(i + it.first, j + it.second))
                ++num;
        }
        return num;
    }
    
    bool firstSubiterationSatisfied(int i, int j);
    bool secondSubiterationSatisfied(int i, int j);
};

#endif
