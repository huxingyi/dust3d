#include <math.h>
#include "vector3d.h"
#include "matrix.h"

float vec3Length(vec3 *p) {
  double mag;
  mag = p->x * p->x + p->y * p->y + p->z * p->z;
  return sqrt(mag);
}

void vec3Normalize(vec3 *p) {
  double mag;
  mag = p->x * p->x + p->y * p->y + p->z * p->z;
  if (mag != 0.0) {
    mag = 1.0 / sqrt(mag);
    p->x *= mag;
    p->y *= mag;
    p->z *= mag;
  }
}

void vec3Scale(vec3 *a, float scale, vec3 *result) {
  result->x = a->x * scale;
  result->y = a->y * scale;
  result->z = a->z * scale;
}

void vec3Midpoint(vec3 *a, vec3 *b, vec3 *mid) {
  vec3Lerp(a, b, 0.5, mid);
}

void vec3Lerp(vec3 *a, vec3 *b, float frac, vec3 *result) {
  result->x = a->x + (b->x - a->x) * frac;
  result->y = a->y + (b->y - a->y) * frac;
  result->z = a->z + (b->z - a->z) * frac;
}

void vec3Sub(vec3 *a, vec3 *b, vec3 *result) {
  result->x = a->x - b->x;
  result->y = a->y - b->y;
  result->z = a->z - b->z;
}

void vec3Add(vec3 *a, vec3 *b, vec3 *result) {
  result->x = a->x + b->x;
  result->y = a->y + b->y;
  result->z = a->z + b->z;
}

void vec3CrossProduct(vec3 *a, vec3 *b, vec3 *result) {
  result->x = a->y * b->z - a->z * b->y;
  result->y = a->z * b->x - a->x * b->z;
  result->z = a->x * b->y - a->y * b->x;
}

float vec3DotProduct(vec3 *a, vec3 *b) {
  return a->x * b->x + a->y * b->y + a->z * b->z;
}

float vec3Distance(vec3 *a, vec3 *b) {
  vec3 p;
  vec3Sub(a, b, &p);
  return vec3Length(&p);
}

void vec3Normal(vec3 *a, vec3 *b, vec3 *c, vec3 *normal) {
  float v1[3], v2[3], vr[3], val;
  
  v1[0] = a->x - b->x;
  v1[1] = a->y - b->y;
  v1[2] = a->z - b->z;

  v2[0] = a->x - c->x;
  v2[1] = a->y - c->y;
  v2[2] = a->z - c->z;

  vr[0] = v1[1] * v2[2] - v2[1] * v1[2];
  vr[1] = v2[0] * v1[2] - v1[0] * v2[2];
  vr[2] = v1[0] * v2[1] - v2[0] * v1[1];

  val = sqrt(vr[0]*vr[0] + vr[1]*vr[1] + vr[2]*vr[2]);

  normal->x = vr[0]/val;
  normal->y = vr[1]/val;
  normal->z = vr[2]/val;
}

void vec3RotateAlong(vec3 *a, float angle, vec3 *axis, vec3 *result) {
  float vec[3];
  matrix matRotate;
  matrix matFinal;
  matrixLoadIdentity(&matFinal);
  matrixAppend(&matFinal, matrixRotate(&matRotate, angle,
    axis->x, axis->y, axis->z));
  vec[0] = a->x;
  vec[1] = a->y;
  vec[2] = a->z;
  matrixTransformVector(&matFinal, vec);
  result->x = vec[0];
  result->y = vec[1];
  result->z = vec[2];
}

float vec3Angle(vec3 *a, vec3 *b) {
  float angle;
  vec3 p;
  float distance;
  float acosParam;
  vec3Sub(a, b, &p);
  distance = vec3Length(&p);
  if (0 == distance) {
    return 0;
  }
  acosParam = vec3DotProduct(a, b) / distance;
  if (acosParam < -1) {
    acosParam = -1;
  }
  if (acosParam > 1) {
    acosParam = 1;
  }
  angle = 180 / M_PI * acos(acosParam);
  return angle;
}

void vec3ProjectOver(vec3 *a, vec3 *over, vec3 *result) {
  float length = vec3DotProduct(a, over);
  result->x = length * over->x;
  result->y = length * over->y;
  result->z = length * over->z;
}

