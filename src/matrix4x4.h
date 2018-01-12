#ifndef DUST3D_MATRIX4X4_H
#define DUST3D_MATRIX4X4_H
#include <math.h>
#include "vector3d.h"

#define M00 0
#define M01 1
#define M02 2
#define M03 3
#define M10 4
#define M11 5
#define M12 6
#define M13 7
#define M20 8
#define M21 9
#define M22 10
#define M23 11
#define M30 12
#define M31 13
#define M32 14
#define M33 15

typedef struct matrix4x4 {
  float data[16];
} matrix4x4;

#define MATRIX4X4_IDENTITY {{\
    1, 0, 0, 0,\
    0, 1, 0, 0,\
    0, 0, 1, 0,\
    0, 0, 0, 1,\
}}

#define MATRIX4X4_TRANSLATTION(x, y, z) {{\
    1, 0, 0, 0,\
    0, 1, 0, 0,\
    0, 0, 1, 0,\
    x, y, z, 1,\
}}

#define MATRIX4X4_SCALING(x, y, z) {{\
    x, 0, 0, 0,\
    0, y, 0, 0,\
    0, 0, z, 0,\
    0, 0, 0, 1,\
}}

#define MATRIX4X4_ROTATION_X(a) {{\
    1,       0,       0, 0,\
    0,  cos(a),  sin(a), 0,\
    0, -sin(a),  cos(a), 0,\
    0,       0,       0, 1,\
}}

#define MATRIX4X4_ROTATION_Y(a) {{\
     cos(a), 0, sin(a), 0,\
          0, 1,      0, 0,\
     sin(a), 0, cos(a), 0,\
          0, 0,      0, 1,\
}}

#define MATRIX4X4_ROTATION_Z(a) {{\
    cos(a),  sin(a), 0, 0,\
   -sin(a),  cos(a), 0, 0,\
         0,       0, 1, 0,\
         0,       0, 0, 1,\
}}

#define MATRIX4X4_PERSPECTIVE_PROJECTION(fov, aspect, near, far) {{\
  (fov) / (aspect),     0,                                   0,                                       0,\
                 0, (fov),                                   0,                                       0,\
                 0,     0, ((far) + (near)) / ((near) - (far)), (2 * (far) * (near)) / ((near) - (far)),\
                 0,     0,                                  -1,                                       1,\
}}

void matrix4x4Identity(matrix4x4 *mat);
void matrix4x4Translation(matrix4x4 *mat, float x, float y, float z);
void matrix4x4Scaling(matrix4x4 *mat, float x, float y, float z);
void matrix4x4RotationX(matrix4x4 *mat, float a);
void matrix4x4RotationY(matrix4x4 *mat, float a);
void matrix4x4RotationZ(matrix4x4 *mat, float a);
void matrix4x4Multiply(matrix4x4 *mat, matrix4x4 *by);
void matrix4x4TransformPoint3d(matrix4x4 *mat, point3d *point);
void matrix4x4TransformVector3d(matrix4x4 *mat, vector3d *vec);
void matrix4x4PerspectiveProjection(matrix4x4 *mat, float fov, float aspect, float near, float far);
void matrix4x4Eye(matrix4x4 *mat, point3d *eye, point3d *center, vector3d *up);

#endif
