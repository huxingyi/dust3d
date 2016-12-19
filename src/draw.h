#ifndef DRAW_SPHERE_H
#define DRAW_SPHERE_H
#include "vector3d.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  vec3 pt[3];
} triangle;

typedef struct {
  int npoly;
  triangle *poly;
} object;

int drawSphere(vec3 *origin, float radius, int level);
void drawTriangle(triangle *poly);
int drawCylinder(vec3 *topOrigin, vec3 *bottomOrigin, float radius, int slices);
int drawGrid(float size, float step);

#ifdef __cplusplus
}
#endif

#endif
