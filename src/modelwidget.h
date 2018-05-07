#ifndef MODEL_WIDGET_H
#define MODEL_WIDGET_H
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QMatrix4x4>
#include <QMutex>
#include <QRubberBand>
#include "meshloader.h"
#include "modelshaderprogram.h"
#include "modelmeshbinder.h"

class SkeletonGraphicsFunctions;

class ModelWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    ModelWidget(QWidget *parent = 0);
    ~ModelWidget();

    static bool isTransparent() { return m_transparent; }
    static void setTransparent(bool t) { m_transparent = t; }
    
    void updateMesh(MeshLoader *mesh);
    void exportMeshAsObj(const QString &filename);
    void exportMeshAsObjPlusMaterials(const QString &filename);
    void setGraphicsFunctions(SkeletonGraphicsFunctions *graphicsFunctions);
    void toggleWireframe();
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
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
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
    SkeletonGraphicsFunctions *m_graphicsFunctions;
private:
    QPoint m_lastPos;
    ModelMeshBinder m_meshBinder;
    QMatrix4x4 m_proj;
    QMatrix4x4 m_camera;
    QMatrix4x4 m_world;
    static bool m_transparent;
    QPoint m_moveStartPos;
    QRect m_moveStartGeometry;
};

#endif
