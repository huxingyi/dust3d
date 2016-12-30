#ifndef _3DSTRUCT_H
#define _3DSTRUCT_H
#include "vector3d.h"

typedef struct {
  vec3 pt[3];
} triangle;

typedef struct {
  vec3 pt[4];
} quad;

typedef struct face3 {
  int indices[3];
} face3;

typedef struct face4 {
  int indices[4];
} face4;

#endif
