#ifndef MODEL_WIDGET_H
#define MODEL_WIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QMatrix4x4>
#include <QMutex>
#include <QRubberBand>
#include "mesh.h"

QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)

class SkeletonEditGraphicsView;

class ModelWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    ModelWidget(QWidget *parent = 0);
    ~ModelWidget();

    static bool isTransparent() { return m_transparent; }
    static void setTransparent(bool t) { m_transparent = t; }
    
    void updateMesh(Mesh *mesh);
    void exportMeshAsObj(const QString &filename);
    void setGraphicsView(SkeletonEditGraphicsView *view);

public slots:
    void setXRotation(int angle);
    void setYRotation(int angle);
    void setZRotation(int angle);
    void cleanup();

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
private:
    bool m_core;
    int m_xRot;
    int m_yRot;
    int m_zRot;
    QPoint m_lastPos;
    QOpenGLVertexArrayObject m_vaoTriangle;
    QOpenGLBuffer m_vboTriangle;
    QOpenGLVertexArrayObject m_vaoEdge;
    QOpenGLBuffer m_vboEdge;
    QOpenGLShaderProgram *m_program;
    int m_renderTriangleVertexCount;
    int m_renderEdgeVertexCount;
    int m_projMatrixLoc;
    int m_mvMatrixLoc;
    int m_normalMatrixLoc;
    int m_lightPosLoc;
    QMatrix4x4 m_proj;
    QMatrix4x4 m_camera;
    QMatrix4x4 m_world;
    static bool m_transparent;
    
    Mesh *m_mesh;
    QMutex m_meshMutex;
    bool m_meshUpdated;
    bool m_moveStarted;
    QPoint m_moveStartPos;
    QRect m_moveStartGeometry;
    
    SkeletonEditGraphicsView *m_graphicsView;
};

#endif
