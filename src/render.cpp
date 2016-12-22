#include <QtGui>
#include <QtOpenGL>
#include <assert.h>
#include <math.h>
#include "render.h"
#include "draw.h"
#include "bmesh.h"
#include "matrix.h"

static const float bmeshNodeColors[][3] {
  {0, 0.78, 1},
  {1, 0, 0},
  {1, 1, 1}
};

static const float bmeshEdgeColor[3] = {1, 1, 0};

static QGLWidget *_this = 0;

static int drawBmeshNode(bmesh *bm, bmeshNode *node) {
  glColor3fv(bmeshNodeColors[node->type]);
  drawSphere(&node->position, node->radius, 36, 24);
  return 0;
}

static void drawBmeshNodeRecursively(bmesh *bm, bmeshNode *node) {
  int childIndex = node->firstChildIndex;
  int childNodeIndex;
  drawBmeshNode(bm, node);
  while (-1 != childIndex) {
    childNodeIndex = bmeshGetNodeNextChild(bm, node, &childIndex);
    if (-1 == childNodeIndex) {
      break;
    }
    drawBmeshNodeRecursively(bm, bmeshGetNode(bm, childNodeIndex));
  }
}

static void drawBmeshNodeQuardRecursively(bmesh *bm, bmeshNode *node) {
  int childIndex = node->firstChildIndex;
  int childNodeIndex;
  
  /*
  matrix matTmp;
  matrix matCalc;
  float quad[4][3] = {
    {-node->radius, +node->radius, 0},
    {-node->radius, -node->radius, 0},
    {+node->radius, -node->radius, 0},
    {+node->radius, +node->radius, 0},
  };
  matrixLoadIdentity(&matCalc);
  matrixAppend(&matCalc,
    matrixTranslate(&matTmp, node->position.x, node->position.y,
      node->position.z));
  matrixAppend(&matCalc,
    matrixRotate(&matTmp,
      node->rotateAngle, node->rotateAround.x, node->rotateAround.y,
      node->rotateAround.z));
  matrixTransformVector(&matCalc, quad[0]);
  matrixTransformVector(&matCalc, quad[1]);
  matrixTransformVector(&matCalc, quad[2]);
  matrixTransformVector(&matCalc, quad[3]);
  
  glVertex3fv(quad[0]);
  glVertex3fv(quad[1]);
  glVertex3fv(quad[2]);
  glVertex3fv(quad[3]);
  */

  /*
  glPushMatrix();
  glTranslatef(node->position.x, node->position.y,
    node->position.z);
  glRotatef(node->rotateAngle, node->rotateAround.x, node->rotateAround.y,
    node->rotateAround.z);
  glBegin(GL_QUADS);
    glVertex3f(-node->radius, +node->radius, 0);
    glVertex3f(-node->radius, -node->radius, 0);
    glVertex3f(+node->radius, -node->radius, 0);
    glVertex3f(+node->radius, +node->radius, 0);
  glEnd();
  glPopMatrix();
  */
  
  while (-1 != childIndex) {
    childNodeIndex = bmeshGetNodeNextChild(bm, node, &childIndex);
    if (-1 == childNodeIndex) {
      break;
    }
    drawBmeshNodeQuardRecursively(bm, bmeshGetNode(bm, childNodeIndex));
  }
}

static int drawBmeshEdge(bmesh *bm, bmeshEdge *edge) {
  glColor3fv(bmeshEdgeColor);
  bmeshNode *firstNode = bmeshGetNode(bm, edge->firstNodeIndex);
  bmeshNode *secondNode = bmeshGetNode(bm, edge->secondNodeIndex);
  drawCylinder(&firstNode->position, &secondNode->position, 0.1, 36, 24);
  return 0;
}

int drawText(int x, int y, char *text) {
  QFont font = QFont("Arial");
  font.setPointSize(9);
  font.setBold(false);
  _this->renderText(x, y, QString(text), font);
  return 0;
}

Render::Render(QWidget *parent)
    : QGLWidget(QGLFormat(QGL::SampleBuffers), parent) {
  QTimer *timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(update()));
  timer->start(100);

  mouseX = 0;
  mouseY = 0;
  cameraAngleX = 20;
  cameraAngleY = -225;
  cameraDistance = 7;
}

Render::~Render(void) {
}

void Render::initializeGL() {
  glShadeModel(GL_SMOOTH);
  glEnable(GL_CULL_FACE);
  glEnable(GL_BLEND);
  glDepthFunc(GL_LEQUAL);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glEnable(GL_CULL_FACE);

  qglClearColor(QWidget::palette().color(QWidget::backgroundRole()));
  glClearStencil(0);
  glClearDepth(1.0f);
  
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specularColor);

  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  glEnable(GL_COLOR_MATERIAL);

  drawInit();
}

#include "../data/bmesh_test_1.h"

void Render::paintGL() {
  static bmesh *bm = 0;

  _this = this;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glPushMatrix();

  glTranslatef(0, 0, -cameraDistance);
  glRotatef(cameraAngleX, 1, 0, 0);
  glRotatef(cameraAngleY, 0, 1, 0);

  glColor3f(0, 0, 0);
  drawPrintf(0, 10, "cameraAngleX:%f cameraAngleY:%f cameraDistance:%f",
    cameraAngleX, cameraAngleY, cameraDistance);

  drawGrid(10, 1);

  glEnable(GL_LIGHTING);

  if (0 == bm) {
    bmeshNode node;
    bmeshEdge edge;
    int i;
    bm = bmeshCreate();

    for (i = 0; i < sizeof(bmeshTest1Nodes) / sizeof(bmeshTest1Nodes[0]); ++i) {
      memset(&node, 0, sizeof(node));
      node.position.x = bmeshTest1Nodes[i][1];
      node.position.y = bmeshTest1Nodes[i][2];
      node.position.z = bmeshTest1Nodes[i][3];
      node.radius = bmeshTest1Nodes[i][4];
      node.type = bmeshTest1Nodes[i][5];
      bmeshAddNode(bm, &node);
    }

    for (i = 0; i < sizeof(bmeshTest1Edges) / sizeof(bmeshTest1Edges[0]); ++i) {
      memset(&edge, 0, sizeof(edge));
      edge.firstNodeIndex = bmeshTest1Edges[i][0];
      edge.secondNodeIndex = bmeshTest1Edges[i][1];
      bmeshAddEdge(bm, &edge);
    }

    bmeshGenerateInbetweenNodes(bm);
  }

  drawBmeshNodeRecursively(bm, bmeshGetRootNode(bm));
  
  //glBegin(GL_QUADS);
  //drawBmeshNodeQuardRecursively(bm, bmeshGetRootNode(bm));
  //glEnd();

  {
    int index;
    /*
    for (index = 0; index < bmeshGetNodeNum(bm); ++index) {
      bmeshNode *node = bmeshGetNode(bm, index);
      drawBmeshNode(bm, node);
    }*/
    for (index = 0; index < bmeshGetEdgeNum(bm); ++index) {
      bmeshEdge *edge = bmeshGetEdge(bm, index);
      drawBmeshEdge(bm, edge);
    }
    glColor4f(1.0f, 1.0f, 1.0f, 0.5);
    glBegin(GL_QUADS);
    for (index = 0; index < bmeshGetQuadNum(bm); ++index) {
      quad *q = bmeshGetQuad(bm, index);
      vec3 normal;
      int j;
      vec3Normal(&q->pt[0], &q->pt[1], &q->pt[2], &normal);
      for (j = 0; j < 4; ++j) {
        glNormal3f(normal.x, normal.y, normal.z);
        glVertex3f(q->pt[j].x, q->pt[j].y, q->pt[j].z);
      }
    }
    glEnd();
    glColor3f(0.0f, 0.0f, 0.0f);
    for (index = 0; index < bmeshGetQuadNum(bm); ++index) {
      quad *q = bmeshGetQuad(bm, index);
      int j;
      glBegin(GL_LINE_STRIP);
      for (j = 0; j < 4; ++j) {
        glVertex3f(q->pt[j].x, q->pt[j].y, q->pt[j].z);
      }
      glVertex3f(q->pt[0].x, q->pt[0].y, q->pt[0].z);
      glEnd();
    }
  }

  glPopMatrix();
}

void Render::resizeGL(int w, int h) {
  glViewport(0, 0, (GLsizei)w, (GLsizei)h);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60.0f, w/h, 1, 1000);

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
