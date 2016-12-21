#ifndef DRAW_SPHERE_H
#define DRAW_SPHERE_H
#include "vector3d.h"
#ifdef __APPLE__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  vec3 pt[3];
} triangle;

typedef struct {
  vec3 pt[4];
} quard;

typedef struct {
  int npoly;
  triangle *poly;
} object;

int drawInit(void);
int drawSphere(vec3 *origin, float radius, int slices, int stacks);
void drawTriangle(triangle *poly);
int drawCylinder(vec3 *topOrigin, vec3 *bottomOrigin, float radius, int slices,
    int stacks);
int drawGrid(float size, float step);
int drawText(int x, int y, char *text);
int drawPrintf(int x, int y, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
