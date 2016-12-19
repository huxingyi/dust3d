#include <QtGui>
#include <QtOpenGL>
#include <assert.h>
#include <math.h>
#include "render.h"
#include "draw.h"
#include "bmesh.h"
#include "matrix.h"

static int drawBmeshNode(bmesh *bm, bmeshNode *node) {
  float color1[3] = {1, 0, 0};
  glColor3fv(color1);
  drawSphere(&node->position, node->radius, 3);
  return 0;
}

static int drawBmeshEdge(bmesh *bm, bmeshEdge *edge) {
  float color2[3] = {0, 0, 1};
  glColor3fv(color2);
  bmeshNode *firstNode = bmeshGetNode(bm, edge->firstNode);
  bmeshNode *secondNode = bmeshGetNode(bm, edge->secondNode);
  drawCylinder(&firstNode->position, &secondNode->position, 0.04, 40);
  return 0;
}

Render::Render(QWidget *parent)
    : QGLWidget(QGLFormat(QGL::SampleBuffers), parent) {
  QTimer *timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(update()));
  timer->start(100);

  mouseX = 0;
  mouseY = 0;
  cameraAngleX = 50;
  cameraAngleY = 70;
  cameraDistance = 3;
}

Render::~Render(void) {
}

void Render::initializeGL() {
  glShadeModel(GL_SMOOTH);
  glEnable(GL_CULL_FACE);
  glEnable(GL_BLEND);

  qglClearColor(QWidget::palette().color(QWidget::backgroundRole()));
  glClearDepth(1.0f);

  GLfloat ambientLight[] = {0.0f, 0.0f, 0.0f, 1.0f};
  GLfloat diffuseLight[] = {0.9f, 0.9f, 0.9f, 1.0f};
  GLfloat specularLight[] = {1, 1, 1, 1};
  glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
  glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);

  float lightDirection[4] = {0, 0, 1, 0};
  glLightfv(GL_LIGHT0, GL_POSITION, lightDirection);

  glEnable(GL_LIGHT0);

  float shininess = 64.0f;
  float specularColor[] = {1.0, 1.0f, 1.0f, 1.0f};
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess); // range 0 ~ 128
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specularColor);

  glClearStencil(0);
  glClearDepth(1.0f);
  glDepthFunc(GL_LEQUAL);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glEnable(GL_CULL_FACE);

  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  glEnable(GL_COLOR_MATERIAL);
}

void Render::paintGL() {
  static bmesh *bm = 0;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glPushMatrix();

  glTranslatef(0, 0, -cameraDistance);
  glRotatef(cameraAngleX, 1, 0, 0);
  glRotatef(cameraAngleY, 0, 1, 0);

  drawGrid(10, 1);

  glEnable(GL_LIGHTING);

  if (0 == bm) {
    bmeshNode node;
    bmeshEdge edge;
    bm = bmeshCreate();

    memset(&node, 0, sizeof(node));
    node.position.x = -1.0;
    node.position.y = 0.3;
    node.position.z = 1.0;
    node.radius = 0.45;
    bmeshAddNode(bm, &node);

    memset(&node, 0, sizeof(node));
    node.position.x = -0.5;
    node.position.y = 0.2;
    node.position.z = 0.5;
    node.radius = 0.1;
    bmeshAddNode(bm, &node);

    memset(&edge, 0, sizeof(edge));
    edge.firstNode = 1;
    edge.secondNode = 0;
    bmeshAddEdge(bm, &edge);
  }

  {
    int index;
    for (index = 0; index < bmeshGetNodeNum(bm); ++index) {
      bmeshNode *node = bmeshGetNode(bm, index);
      drawBmeshNode(bm, node);
    }
    for (index = 0; index < bmeshGetEdgeNum(bm); ++index) {
      bmeshEdge *edge = bmeshGetEdge(bm, index);
      drawBmeshEdge(bm, edge);
    }
  }

  glPopMatrix();
}

void Render::resizeGL(int w, int h) {
  glViewport(0, 0, (GLsizei)w, (GLsizei)h);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-2, 2, -1.5, 1.5, 1, 1000);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void Render::mousePressEvent(QMouseEvent *event) {
  mouseX = event->x();
  mouseY = event->y();
}

void Render::mouseMoveEvent(QMouseEvent *event) {
  cameraAngleY += (event->x() - mouseX);
  cameraAngleX += (event->y() - mouseY);
  update();
  mouseX = event->x();
  mouseY = event->y();
}

void Render::wheelEvent(QWheelEvent * event) {
  cameraDistance -= event->delta() * 0.01f;
}
