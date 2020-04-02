#ifndef DUST3D_NORMAL_AND_DEPTH_MAPS_GENERATOR_H
#define DUST3D_NORMAL_AND_DEPTH_MAPS_GENERATOR_H
#include <QObject>
#include <QImage>
#include "modelwidget.h"
#include "modelofflinerender.h"
#include "meshloader.h"

class NormalAndDepthMapsGenerator : public QObject
{
    Q_OBJECT
public:
    NormalAndDepthMapsGenerator(ModelWidget *modelWidget);
    void updateMesh(MeshLoader *mesh);
    void setRenderThread(QThread *thread);
    ~NormalAndDepthMapsGenerator();
    QImage *takeNormalMap();
    QImage *takeDepthMap();
signals:
    void finished();
public slots:
    void process();
private:
    ModelOfflineRender *m_normalMapRender = nullptr;
    ModelOfflineRender *m_depthMapRender = nullptr;
    QSize m_viewPortSize;
    QImage *m_normalMap = nullptr;
    QImage *m_depthMap = nullptr;
    
    ModelOfflineRender *createOfflineRender(ModelWidget *modelWidget, int purpose);
    void generate();
};

#endif
