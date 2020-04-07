#ifndef DUST3D_NORMAL_AND_DEPTH_MAPS_GENERATOR_H
#define DUST3D_NORMAL_AND_DEPTH_MAPS_GENERATOR_H
#include <QObject>
#include <QImage>
#include "modelwidget.h"
#include "modeloffscreenrender.h"
#include "model.h"

class NormalAndDepthMapsGenerator : public QObject
{
    Q_OBJECT
public:
    NormalAndDepthMapsGenerator(ModelWidget *modelWidget);
    void updateMesh(Model *mesh);
    void setRenderThread(QThread *thread);
    ~NormalAndDepthMapsGenerator();
    QImage *takeNormalMap();
    QImage *takeDepthMap();
signals:
    void finished();
public slots:
    void process();
private:
    ModelOffscreenRender *m_normalMapRender = nullptr;
    ModelOffscreenRender *m_depthMapRender = nullptr;
    QSize m_viewPortSize;
    QImage *m_normalMap = nullptr;
    QImage *m_depthMap = nullptr;
    
    ModelOffscreenRender *createOfflineRender(ModelWidget *modelWidget, int purpose);
    void generate();
};

#endif
