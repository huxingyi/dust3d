#include <QtGui>
#include <QtOpenGL>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef __APPLE__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif
#include "draw.h"

static GLUquadricObj *quadricId = 0;

int drawInit(void) {
  if (0 == quadricId) {
    quadricId = gluNewQuadric();
    gluQuadricDrawStyle(quadricId, GLU_FILL);
  }
  return 0;
}

int drawSphere(vec3 *origin, float radius, int slices, int stacks) {
  glPushMatrix();
  glTranslatef(origin->x, origin->y, origin->z);
  gluSphere(quadricId, radius, slices, stacks);
  glPopMatrix();
  return 0;
}

void drawTriangle(triangle *poly) {
  int i;
  glBegin(GL_TRIANGLES);
  for (i = 0; i < 3; ++i) {
    vec3 *pt = &poly->pt[i];
    glNormal3f(pt->x, pt->y, pt->z);
    glVertex3f(pt->x, pt->y, pt->z);
  }
  glEnd();
}

int drawCylinder(vec3 *topOrigin, vec3 *bottomOrigin, float radius, int slices,
    int stacks) {
  vec3 zAxis = {0, 0, 1};
  vec3 p, t;
  float height = 0;
  float angle = 0;

  if (slices <= 0) {
    fprintf(stderr, "%s:Invalid parameter(slices:%d).\n", __FUNCTION__, slices);
    return -1;
  }

  vec3Sub(topOrigin, bottomOrigin, &p);
  vec3CrossProduct(&zAxis, &p, &t);
  height = vec3Length(&p);
  if (height > 0) {
    angle = 180 / M_PI * acos(vec3DotProduct(&zAxis, &p) / height);
  }

  glPushMatrix();

  glTranslatef(bottomOrigin->x, bottomOrigin->y,
    bottomOrigin->z);
  glRotatef(angle, t.x, t.y, t.z);
  gluCylinder(quadricId, radius, radius, height, slices, stacks);

  glPopMatrix();

  return 0;
}

int drawGrid(float size, float step) {
  glDisable(GL_LIGHTING);

  // x z plane
  glBegin(GL_LINES);
  for (GLfloat i = -size; i <= size; i += step) {
    if (0 == i) {
      glColor3f(0.0, 0.0, 1.0);
      glVertex3f(i, 0, size); glVertex3f(i, 0, -size);
      glColor3f(1.0, 0.0, 0.0);
      glVertex3f(size, 0, i); glVertex3f(-size, 0, i);
    } else {
      glColor3f(0.39, 0.39, 0.39);
      glVertex3f(i, 0, size); glVertex3f(i, 0, -size);
      glVertex3f(size, 0, i); glVertex3f(-size, 0, i);
    }
  }
  glEnd();

  glEnable(GL_LIGHTING);
  return 0;
}
