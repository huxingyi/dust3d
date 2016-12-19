#ifndef VECTOR_3D_H
#define VECTOR_3D_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  float x;
  float y;
  float z;
} vec3;

void vec3Normalize(vec3 *p);
void vec3Midpoint(vec3 *a, vec3 *b, vec3 *mid);
void vec3CrossProduct(vec3 *a, vec3 *b, vec3 *result);
void vec3Sub(vec3 *a, vec3 *b, vec3 *result);
float vec3DotProduct(vec3 *a, vec3 *b);
float vec3Length(vec3 *p);

#ifdef __cplusplus
}
#endif

#endif
