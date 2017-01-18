#include <QtGui>
#include <QtOpenGL>
#include <assert.h>
#include <math.h>
#include "render.h"
#include "draw.h"
#include "bmesh.h"
#include "matrix.h"
#include "vector3d.h"
#include "subdivide.h"

static const float bmeshBallColors[][4] {
  {0.00, 0.78, 1.00, 0.5},
  {1.00, 0.00, 0.00, 0.5},
  {1.00, 1.00, 1.00, 0.5}
};

static const float bmeshBoneColor[3] = {1, 1, 0};

static Render *_this = 0;
static int debugOutputTop = 0;

int drawDebugPrintf(const char *fmt, ...) {
  int x = 0;
  int y = debugOutputTop + 10;
  va_list args;
  char text[1024];
  va_start(args, fmt);
  vsnprintf(text, sizeof(text), fmt, args);
  debugOutputTop += 9;
  if (debugOutputTop > 400) {
    debugOutputTop = 0;
  }
  glColor3f(0, 0, 0);
  return drawText(x, y, text);
}

static int drawBmeshBall(bmesh *bm, bmeshBall *ball) {
  float color[4];
  memcpy(color, bmeshBallColors[ball->type], sizeof(color));
  if (BMESH_BALL_TYPE_ROOT == ball->type || BMESH_BALL_TYPE_KEY == ball->type) {
    vec3 a = {_this->mouseWorldNearX, _this->mouseWorldNearY,
      _this->mouseWorldNearZ};
    vec3 b = {_this->mouseWorldFarX, _this->mouseWorldFarY,
      _this->mouseWorldFarZ};
    vec3 ab, ap, q;
    float abDotAb;
    float apDotAb;
    float t;
    vec3Sub(&b, &a, &ab);
    vec3Sub(&ball->position, &a, &ap);
    abDotAb = vec3DotProduct(&ab, &ab);
    apDotAb = vec3DotProduct(&ap, &ab);
    t = apDotAb / abDotAb;
    if (t < 0.0f) {
      t = 0.0f;
    } else if (t > 1.0f) {
      t = 1.0f;
    }
    vec3Scale(&ab, t, &q);
    vec3Add(&a, &q, &q);
    //drawDebugPrintf("q: %f,%f,%f t:%f", q.x, q.y, q.z, t);
    if (vec3Distance(&ball->position, &q) <= ball->radius) {
      color[3] = 1;
    }
  }
  glColor4fv(color);
  drawSphere(&ball->position, ball->radius, 36, 24);
  return 0;
}

static void drawBmeshBallRecursively(bmesh *bm, bmeshBall *ball) {
  bmeshBallIterator iterator;
  bmeshBall *child;
  drawBmeshBall(bm, ball);
  for (child = bmeshGetBallFirstChild(bm, ball, &iterator);
      child;
      child = bmeshGetBallNextChild(bm, ball, &iterator)) {
    drawBmeshBallRecursively(bm, child);
  }
}

static void drawBmeshBallQuad(bmeshBall *ball) {
  vec3 normal;
  int j;
  vec3 z, y;
  quad q;
  
  vec3Scale(&ball->localYaxis, ball->radius, &y);
  vec3Scale(&ball->localZaxis, ball->radius, &z);
  vec3Sub(&ball->position, &y, &q.pt[0]);
  vec3Add(&q.pt[0], &z, &q.pt[0]);
  vec3Sub(&ball->position, &y, &q.pt[1]);
  vec3Sub(&q.pt[1], &z, &q.pt[1]);
  vec3Add(&ball->position, &y, &q.pt[2]);
  vec3Sub(&q.pt[2], &z, &q.pt[2]);
  vec3Add(&ball->position, &y, &q.pt[3]);
  vec3Add(&q.pt[3], &z, &q.pt[3]);
  
  glColor4f(1.0f, 1.0f, 1.0f, 0.5);
  glBegin(GL_QUADS);
  vec3Normal(&q.pt[0], &q.pt[1], &q.pt[2], &normal);
  for (j = 0; j < 4; ++j) {
    glNormal3f(normal.x, normal.y, normal.z);
    glVertex3f(q.pt[j].x, q.pt[j].y, q.pt[j].z);
  }
  glEnd();
  
  glColor3f(0.0f, 0.0f, 0.0f);
  glBegin(GL_LINE_STRIP);
  for (j = 0; j < 4; ++j) {
    glVertex3f(q.pt[j].x, q.pt[j].y, q.pt[j].z);
  }
  glVertex3f(q.pt[0].x, q.pt[0].y, q.pt[0].z);
  glEnd();
}

static void drawBmeshBallQuadRecursively(bmesh *bm, bmeshBall *ball) {
  bmeshBallIterator iterator;
  bmeshBall *child;

  /*
  matrix matTmp;
  matrix matCalc;
  float quad[4][3] = {
    {-ball->radius, +ball->radius, 0},
    {-ball->radius, -ball->radius, 0},
    {+ball->radius, -ball->radius, 0},
    {+ball->radius, +ball->radius, 0},
  };
  matrixLoadIdentity(&matCalc);
  matrixAppend(&matCalc,
    matrixTranslate(&matTmp, ball->position.x, ball->position.y,
      ball->position.z));
  matrixAppend(&matCalc,
    matrixRotate(&matTmp,
      ball->rotateAngle, ball->rotateAround.x, ball->rotateAround.y,
      ball->rotateAround.z));
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
  glTranslatef(ball->position.x, ball->position.y,
    ball->position.z);
  glRotatef(ball->rotateAngle, ball->rotateAround.x, ball->rotateAround.y,
    ball->rotateAround.z);
  glBegin(GL_QUADS);
    glVertex3f(-ball->radius, +ball->radius, 0);
    glVertex3f(-ball->radius, -ball->radius, 0);
    glVertex3f(+ball->radius, -ball->radius, 0);
    glVertex3f(+ball->radius, +ball->radius, 0);
  glEnd();
  glPopMatrix();
  */
  
  //drawBmeshBallQuad(ball);

  for (child = bmeshGetBallFirstChild(bm, ball, &iterator);
      child;
      child = bmeshGetBallNextChild(bm, ball, &iterator)) {
    drawBmeshBallQuadRecursively(bm, child);
  }
}

static int drawBmeshBone(bmesh *bm, bmeshBone *bone) {
  glColor3fv(bmeshBoneColor);
  bmeshBall *firstBall = bmeshGetBall(bm, bone->firstBallIndex);
  bmeshBall *secondBall = bmeshGetBall(bm, bone->secondBallIndex);
  drawCylinder(&firstBall->position, &secondBall->position, 0.1, 36, 24);
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
  cameraAngleX = 30;
  cameraAngleY = -312;
  cameraDistance = 14.4;
  //cameraAngleX = 0;
  //cameraAngleY = 0;
  //cameraDistance = 0;
  
  mouseScaleX = 1;
  mouseScaleY = 1;
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

void screenCoordsToWorld(float winX, float winY,
    float *worldNearX, float *worldNearY, float *worldNearZ,
    float *worldFarX, float *worldFarY, float *worldFarZ){
  GLint viewport[4];
  GLdouble modelview[16];
  GLdouble projection[16];
  GLfloat winZ = 0;
  GLdouble x, y, z;

  glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
  glGetDoublev(GL_PROJECTION_MATRIX, projection);
  glGetIntegerv(GL_VIEWPORT, viewport);

  winX = (float)winX;
  winY = (float)viewport[3] - (float)winY;
  
  winZ = 0;
  gluUnProject((GLdouble)winX, (GLdouble)winY, (GLdouble)winZ,
    (const GLdouble *)modelview, (const GLdouble *)projection, viewport,
    &x, &y, &z);
  *worldNearX = (float)x;
  *worldNearY = (float)y;
  *worldNearZ = (float)z;
  {
    vec3 origin = {(float)x, (float)y, (float)z};
    drawDebugPoint(&origin, 0);
  }
  
  winZ = 1;
  gluUnProject((GLdouble)winX, (GLdouble)winY, (GLdouble)winZ,
    (const GLdouble *)modelview, (const GLdouble *)projection, viewport,
    &x, &y, &z);
  *worldFarX = (float)x;
  *worldFarY = (float)y;
  *worldFarZ = (float)z;
  {
    vec3 origin = {(float)x, (float)y, (float)z};
    drawDebugPoint(&origin, 1);
  }
  
  {
    vec3 topOrigin = {*worldNearX, *worldNearY, *worldNearZ};
    vec3 bottomOrigin = {*worldFarX, *worldFarY, *worldFarZ};
    glDisable(GL_LIGHTING);

    // x z plane
    glBegin(GL_LINES);
      glColor3f(0.0, 0.0, 0.0);
      glVertex3f(topOrigin.x, topOrigin.y, topOrigin.z);
      glVertex3f(bottomOrigin.x, bottomOrigin.y, bottomOrigin.z);
    glEnd();

    glEnable(GL_LIGHTING);
    //drawCylinder(&topOrigin, &bottomOrigin, 0.1, 36, 24);
  }
}

#include "../data/bmesh_test_2.h"

void Render::paintGL() {
  static bmesh *bm = 0;

  _this = this;
  debugOutputTop = 0;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  glPushMatrix();

  glTranslatef(0, 0, -cameraDistance);
  glRotatef(cameraAngleX, 1, 0, 0);
  glRotatef(cameraAngleY, 0, 1, 0);

  glColor3f(0, 0, 0);
  drawDebugPrintf("cameraAngleX:%f cameraAngleY:%f cameraDistance:%f",
    cameraAngleX, cameraAngleY, cameraDistance);
  
  screenCoordsToWorld(_this->mouseX, _this->mouseY,
    &_this->mouseWorldNearX, &_this->mouseWorldNearY, &_this->mouseWorldNearZ,
    &_this->mouseWorldFarX, &_this->mouseWorldFarY, &_this->mouseWorldFarZ);
  glColor3f(0.0f, 0.0f, 0.0f);
  drawDebugPrintf("%d,%d -> %f,%f,%f %f,%f,%f", _this->mouseX, _this->mouseY,
    _this->mouseWorldNearX, _this->mouseWorldNearY, _this->mouseWorldNearZ,
    _this->mouseWorldFarX, _this->mouseWorldFarY, _this->mouseWorldFarZ);

  drawGrid(10, 1);

  glEnable(GL_LIGHTING);

  if (0 == bm) {
    bmeshBall ball;
    bmeshBone bone;
    int i;
    bm = bmeshCreate();

    for (i = 0; i < sizeof(bmeshTestBalls) / sizeof(bmeshTestBalls[0]); ++i) {
      memset(&ball, 0, sizeof(ball));
      ball.position.x = bmeshTestBalls[i][1];
      ball.position.y = bmeshTestBalls[i][2];
      ball.position.z = bmeshTestBalls[i][3];
      ball.radius = bmeshTestBalls[i][4];
      ball.type = bmeshTestBalls[i][5];
      bmeshAddBall(bm, &ball);
    }

    for (i = 0; i < sizeof(bmeshTestBones) / sizeof(bmeshTestBones[0]); ++i) {
      memset(&bone, 0, sizeof(bone));
      bone.firstBallIndex = bmeshTestBones[i][0];
      bone.secondBallIndex = bmeshTestBones[i][1];
      bmeshAddBone(bm, &bone);
    }

    bmeshGenerate(bm);
  }
  
  if (bm) {
  
    bmeshDraw(bm);

    drawBmeshBallRecursively(bm, bmeshGetRootBall(bm));

    //glBegin(GL_QUADS);
    drawBmeshBallQuadRecursively(bm, bmeshGetRootBall(bm));
    //glEnd();

    {
      int index;
      /*
      for (index = 0; index < bmeshGetBallNum(bm); ++index) {
        bmeshBall *ball = bmeshGetBall(bm, index);
        drawBmeshBall(bm, ball);
      }*/
      
      for (index = 0; index < bmeshGetBoneNum(bm); ++index) {
        bmeshBone *bone = bmeshGetBone(bm, index);
        //drawBmeshBone(bm, bone);
      }
      /*
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
      }*/
    }
  }
  
  //glColor3f(0.0f, 0.0f, 0.0f);
  //drawTestUnproject();

  glPopMatrix();
  
  //if (bm) {
  //  bmeshDestroy(bm);
  //  bm = 0;
  //}
}

void Render::resizeGL(int w, int h) {
  mouseScaleX = w / width();
  mouseScaleY = h / height();
  
  glViewport(0, 0, (GLsizei)w, (GLsizei)h);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60.0f, w/h, 1, 1000);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

/*
void drawTestUnproject(void) {
  float worldX, worldY, worldZ;
  if (_this) {
    screenCoordsToWorld(_this->mouseX, _this->mouseY, &worldX, &worldY, &worldZ);
    glColor3f(0.0f, 0.0f, 0.0f);
    drawDebugPrintf("%d,%d -> %f,%f,%f", _this->mouseX, _this->mouseY,
      worldX, worldY, worldZ);
  }
}*/

void Render::mousePressEvent(QMouseEvent *event) {
  mouseX = event->x() * mouseScaleX;
  mouseY = event->y() * mouseScaleY;
}

void Render::mouseMoveEvent(QMouseEvent *event) {
  int x = event->x() * mouseScaleX;
  int y = event->y() * mouseScaleY;
  cameraAngleY += (x - mouseX);
  cameraAngleX += (y - mouseY);
  update();
  mouseX = x;
  mouseY = y;
}

void Render::wheelEvent(QWheelEvent * event) {
  cameraDistance -= event->delta() * 0.01f;
}
