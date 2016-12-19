#include "vector3d.h"

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

void vec3Midpoint(vec3 *a, vec3 *b, vec3 *mid) {
  mid->x = (a->x + b->x) * 0.5;
  mid->y = (a->y + b->y) * 0.5;
  mid->z = (a->z + b->z) * 0.5;
}

void vec3Sub(vec3 *a, vec3 *b, vec3 *result) {
  result->x = a->x - b->x;
  result->y = a->y - b->y;
  result->z = a->z - b->z;
}

void vec3CrossProduct(vec3 *a, vec3 *b, vec3 *result) {
  result->x = a->y * b->z - a->z * b->y;
  result->y = a->z * b->x - a->x * b->z;
  result->z = a->x * b->y - a->y * b->x;
}

float vec3DotProduct(vec3 *a, vec3 *b) {
  return a->x * b->x + a->y * b->y + a->z * b->z;
}
