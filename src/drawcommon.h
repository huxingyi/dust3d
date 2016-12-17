#ifndef DRAW_COMMON_H
#define DRAW_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  float x;
  float y;
  float z;
} point;

typedef struct {
  point pt[3];
} triangle;

typedef struct {
  int npoly;
  triangle *poly;
} object;

void normalize(point *p);
void midpoint(point *a, point *b, point *mid);
void drawTriangle(triangle *poly);
int drawCylinder(int slices, float radius, float height);

#ifdef __cplusplus
}
#endif

#endif
