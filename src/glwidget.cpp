#include <QtGui>
#include <QtOpenGL>
#include <assert.h>
#include "glwidget.h"
#include "drawcommon.h"
#include "drawsphere.h"

void drawTriangle(triangle *poly) {
  int i;
  glBegin(GL_TRIANGLES);
  for (i = 0; i < 3; ++i) {
    point *pt = &poly->pt[i];
    glNormal3f(pt->x, pt->y, pt->z);
    glVertex3f(pt->x, pt->y, pt->z);
  }
  glEnd();
}

GLWidget::GLWidget(QWidget *parent)
  : QGLWidget(QGLFormat(QGL::SampleBuffers), parent) {
}

GLWidget::~GLWidget() {
}

void GLWidget::initializeGL() {
  GLfloat mat_specular[] = {1.0, 1.0, 1.0, 1.0};
  GLfloat mat_shininess[] = {50.0};
  GLfloat light_position[] = {1.0, 1.0, 1.0, 0.0};
  GLfloat light[] = {1.0, 0.2, 0.2};
  GLfloat lmodel_ambient[] = {0.1, 0.1, 0.1, 1.0};

  glClearColor(0.0, 0.0, 0.0, 0.0);
  glShadeModel(GL_SMOOTH);

  glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
  glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

  glLightfv(GL_LIGHT0, GL_POSITION, light_position);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light);
  glLightfv(GL_LIGHT0, GL_SPECULAR, light);
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_DEPTH_TEST);
}

void GLWidget::paintGL() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  drawSphere(3);
}

void GLWidget::resizeGL(int w, int h) {
  glViewport(0, 0, (GLsizei)w, (GLsizei)h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  if (w <= h) {
    glOrtho(-1.5, 1.5, -1.5*(GLfloat)h/(GLfloat)w,
       1.5*(GLfloat)h/(GLfloat)w, -10.0, 10.0);
  } else {
    glOrtho(-1.5*(GLfloat)w/(GLfloat)h,
       1.5*(GLfloat)w/(GLfloat)h, -1.5, 1.5, -10.0, 10.0);
  }
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void GLWidget::mousePressEvent(QMouseEvent *event) {
}

void GLWidget::mouseMoveEvent(QMouseEvent *event) {
}
