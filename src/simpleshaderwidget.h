// This file follow the Stackoverflow content license: CC BY-SA 4.0,
// since it's based on Prashanth N Udupa's work: https://stackoverflow.com/questions/35134270/how-to-use-qopenglframebufferobject-for-shadow-mapping
#ifndef DUST3D_SIMPLE_SHADER_WIDGET_H
#define DUST3D_SIMPLE_SHADER_WIDGET_H
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QResizeEvent>
#include <QMatrix4x4>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QVector3D>
#include <QPoint>
#include "simpleshadermesh.h"
#include "simpleshadermeshbinder.h"

class SimpleShaderWidget: public QOpenGLWidget, public QOpenGLFunctions
{
public:
    SimpleShaderWidget(QWidget *parent=nullptr);
    ~SimpleShaderWidget();
    
    void updateMesh(SimpleShaderMesh *mesh);
    void zoom(float delta);
    
protected:
    void resizeEvent(QResizeEvent *event) override;
    void resizeGL(int w, int h) override;
    void initializeGL() override;
    void paintGL() override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    
private:
    uint m_shadowMapFrameBufferId = 0;
    uint m_shadowMapTextureId = 0;
    bool m_shadowMapInitialized = false;
    QMatrix4x4 m_sceneMatrix;
    QMatrix4x4 m_projectionMatrix;
    QMatrix4x4 m_viewMatrix;
    QMatrix4x4 m_cameraPositionMatrix;
    QMatrix4x4 m_lightPositionMatrix;
    QMatrix4x4 m_lightViewMatrix;
    SimpleShaderMeshBinder m_meshBinder;
    QPoint m_lastPos;
    bool m_moveStarted = false;
    int m_rotationX = 0;
    int m_rotationY = 0;
    float m_zoom = 10.5f;
    QVector3D m_eyePosition = QVector3D(0.0, 0.0, 1.0);
    
    void setRotationX(int angle);
    void setRotationY(int angle);
    void renderToShadowMap();
    void renderToScreen();
    void normalizeAngle(int &angle);
};

#endif
