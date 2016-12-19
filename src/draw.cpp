#include <QtGui>
#include <QtOpenGL>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "draw.h"

/*
 * This drawSphere function modified from [Jon Leech's implementation of sphere](ftp://ftp.ee.lbl.gov/sphere.c)
 * Jeremy HU (huxingyi@msn.com) 2016/12/16
*/

#define XPLUS {  1,  0,  0 }  /*  X */
#define XMIN  { -1,  0,  0 }  /* -X */
#define YPLUS {  0,  1,  0 }  /*  Y */
#define YMIN  {  0, -1,  0 }  /* -Y */
#define ZPLUS {  0,  0,  1 }  /*  Z */
#define ZMIN  {  0,  0, -1 }  /* -Z */

static const triangle octahedron[] = {
  {{XPLUS, ZPLUS, YPLUS}},
  {{YPLUS, ZPLUS, XMIN}},
  {{XMIN,  ZPLUS, YMIN}},
  {{YMIN,  ZPLUS, XPLUS}},
  {{XPLUS, YPLUS, ZMIN}},
  {{YPLUS, XMIN,  ZMIN}},
  {{XMIN,  YMIN,  ZMIN}},
  {{YMIN,  XPLUS, ZMIN}},
};

static void subdivide(object *old, object *subdivided) {
  int i;
  for (i = 0; i < old->npoly; ++i) {
    triangle *oldt = &old->poly[i];
    triangle *newt = &subdivided->poly[i * 4];
    vec3 a, b, c;

    vec3Midpoint(&oldt->pt[0], &oldt->pt[2], &a);
    vec3Midpoint(&oldt->pt[0], &oldt->pt[1], &b);
    vec3Midpoint(&oldt->pt[1], &oldt->pt[2], &c);

    vec3Normalize(&a);
    vec3Normalize(&b);
    vec3Normalize(&c);

    newt->pt[0] = oldt->pt[0];
    newt->pt[1] = b;
    newt->pt[2] = a;
    newt++;

    newt->pt[0] = b;
    newt->pt[1] = oldt->pt[1];
    newt->pt[2] = c;
    newt++;

    newt->pt[0] = a;
    newt->pt[1] = b;
    newt->pt[2] = c;
    newt++;

    newt->pt[0] = a;
    newt->pt[1] = c;
    newt->pt[2] = oldt->pt[2];
  }
}

int drawSphere(vec3 *origin, float radius, int level) {
  int lv, i;
  object oldObj, newObj;

  if (level < 1) {
    fprintf(stderr, "%s:level max greater than 0.\n", __FUNCTION__);
    return -1;
  }

  oldObj.npoly = sizeof(octahedron) / sizeof(octahedron[0]);
  oldObj.poly = (triangle *)malloc(oldObj.npoly * sizeof(triangle));
  if (!oldObj.poly) {
    fprintf(stderr, "%s:insufficient memory.\n", __FUNCTION__);
    return -1;
  }
  memcpy(oldObj.poly, octahedron, oldObj.npoly * sizeof(triangle));

  for (lv = 0; lv < level; lv++) {
    newObj.npoly = oldObj.npoly * 4;
    newObj.poly = (triangle *)malloc(newObj.npoly * sizeof(triangle));
    if (!newObj.poly) {
      fprintf(stderr, "%s:insufficient memory(levelLoop:%d).\n",
        __FUNCTION__, lv);
      free(oldObj.poly);
      return -1;
    }

    subdivide(&oldObj, &newObj);

    free(oldObj.poly);
    oldObj = newObj;
  }

  glPushMatrix();
  glTranslatef(origin->x, origin->y, origin->z);
  glScalef(radius, radius, radius);
  for (i = 0; i < newObj.npoly; ++i) {
    drawTriangle(&newObj.poly[i]);
  }
  glPopMatrix();

  free(newObj.poly);

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

int drawCylinder(vec3 *topOrigin, vec3 *bottomOrigin, float radius, int slices) {
  float theta = (2.0 * M_PI) / (float)slices;
  float a = 0.0f;
  int lv;
  float x, y, z;
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

  // strips
  glBegin(GL_TRIANGLE_STRIP);
  for (a = 0, lv = 0; lv <= slices; ++lv) {
    float cosa = cos(a);
    float sina = sin(a);
    x = cosa * radius;
    y = sina * radius;
    z = 0;
    glNormal3f(cosa, sina, 0);
    glVertex3f(x, y, z);
    z = height;
    glNormal3f(cosa, sina, 0);
    glVertex3f(x, y, z);
    a += theta;
  }
  glEnd();

  // bottom cap
  z = 0;
  glBegin(GL_TRIANGLE_FAN);
  glNormal3f(0, 0, -1);
  glVertex3f(0, 0, z);
  for (a = 0, lv = 0; lv <= slices; ++lv) {
    x = cos(a) * radius;
    y = sin(a) * radius;
    glNormal3f(0, 0, -1);
    glVertex3f(x, y, z);
    a += theta;
  }
  glEnd();

  // top cap
  z = height;
  glBegin(GL_TRIANGLE_FAN);
  glNormal3f(0, 0, 1);
  glVertex3f(0, 0, z);
  for (a = 0, lv = 0; lv <= slices; ++lv) {
    x = cos(a) * radius;
    y = sin(a) * radius;
    glNormal3f(0, 0, 1);
    glVertex3f(x, y, z);
    a += theta;
  }
  glEnd();

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
