#ifndef RENDER_H
#define RENDER_H

#include <QGLWidget>

class Render : public QGLWidget {
  Q_OBJECT

public:
  Render(QWidget *parent = 0);
  ~Render(void);

protected:
  void initializeGL(void);
  void paintGL(void);
  void resizeGL(int width, int height);
  void mousePressEvent(QMouseEvent *event);
  void mouseMoveEvent(QMouseEvent *event);
  void wheelEvent(QWheelEvent * event);

public:
  int mouseX;
  int mouseY;
  float mouseScaleX;
  float mouseScaleY;
  float mouseWorldNearX;
  float mouseWorldNearY;
  float mouseWorldNearZ;
  float mouseWorldFarX;
  float mouseWorldFarY;
  float mouseWorldFarZ;
  float cameraAngleX;
  float cameraAngleY;
  float cameraDistance;
};

#endif
