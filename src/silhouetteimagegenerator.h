#ifndef DUST3D_SILHOUETTE_IMAGE_GENERATOR_H
#define DUST3D_SILHOUETTE_IMAGE_GENERATOR_H
#include <QObject>
#include <QImage>
#include "snapshot.h"

class SilhouetteImageGenerator : public QObject
{
    Q_OBJECT
public:
    SilhouetteImageGenerator(int width, int height, Snapshot *snapshot);
    ~SilhouetteImageGenerator();
    QImage *takeResultImage();
    void generate();
signals:
    void finished();
public slots:
    void process();
private:
    int m_width = 0;
    int m_height = 0;
    QImage *m_resultImage = nullptr;
    Snapshot *m_snapshot = nullptr;
};

#endif
