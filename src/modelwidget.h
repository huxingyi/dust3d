#ifndef DUST3D_MODEL_WIDGET_H
#define DUST3D_MODEL_WIDGET_H
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QMatrix4x4>
#include <QMutex>
#include <QRubberBand>
#include <QVector2D>
#include "meshloader.h"
#include "modelshaderprogram.h"
#include "modelmeshbinder.h"

class SkeletonGraphicsFunctions;

class ModelWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
signals:
    void mouseRayChanged(const QVector3D &near, const QVector3D &far);
public:
    ModelWidget(QWidget *parent = 0);
    ~ModelWidget();
    static bool isTransparent()
    {
        return m_transparent;
    }
    static void setTransparent(bool t)
    {
        m_transparent = t;
    }
    void updateMesh(MeshLoader *mesh);
    void setGraphicsFunctions(SkeletonGraphicsFunctions *graphicsFunctions);
    void toggleWireframe();
    void toggleUvCheck();
    void enableMove(bool enabled);
    void enableZoom(bool enabled);
    void enableMousePicking(bool enabled);
    bool inputMousePressEventFromOtherWidget(QMouseEvent *event);
    bool inputMouseMoveEventFromOtherWidget(QMouseEvent *event);
    bool inputWheelEventFromOtherWidget(QWheelEvent *event);
    bool inputMouseReleaseEventFromOtherWidget(QMouseEvent *event);
    QPoint convertInputPosFromOtherWidget(QMouseEvent *event);
public slots:
    void setXRotation(int angle);
    void setYRotation(int angle);
    void setZRotation(int angle);
    void cleanup();
    void zoom(float delta);
    void setMousePickTargetPositionInModelSpace(QVector3D position);
signals:
    void xRotationChanged(int angle);
    void yRotationChanged(int angle);
    void zRotationChanged(int angle);
protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
public:
    int xRot();
    int yRot();
    int zRot();
private:
    int m_xRot;
    int m_yRot;
    int m_zRot;
    ModelShaderProgram *m_program;
    bool m_moveStarted;
    bool m_moveEnabled;
    bool m_zoomEnabled;
    bool m_mousePickingEnabled;
    QVector3D m_mousePickTargetPositionInModelSpace;
private:
    QPoint m_lastPos;
    ModelMeshBinder m_meshBinder;
    QMatrix4x4 m_projection;
    QMatrix4x4 m_camera;
    QMatrix4x4 m_world;
    static bool m_transparent;
    static const QVector3D m_cameraPosition;
    QPoint m_moveStartPos;
    QRect m_moveStartGeometry;
    std::pair<QVector3D, QVector3D> screenPositionToMouseRay(const QPoint &screenPosition);
};

#endif
