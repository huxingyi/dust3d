#include "drawcommon.h"
#include <math.h>

void normalize(point *p) {
  double mag;
  mag = p->x * p->x + p->y * p->y + p->z * p->z;
  if (mag != 0.0) {
    mag = 1.0 / sqrt(mag);
    p->x *= mag;
    p->y *= mag;
    p->z *= mag;
  }
}

void midpoint(point *a, point *b, point *mid) {
  mid->x = (a->x + b->x) * 0.5;
  mid->y = (a->y + b->y) * 0.5;
  mid->z = (a->z + b->z) * 0.5;
}

