#ifndef DUST3D_MATRIX_H
#define DUST3D_MATRIX_H
#include <math.h>

typedef struct matrix {
  float data[16];
} matrix;

#define MATRIX_IDENTITY {{\
    1, 0, 0, 0,\
    0, 1, 0, 0,\
    0, 0, 1, 0,\
    0, 0, 0, 1,\
}}

#define MATRIX_TRANSLATTION(x, y, z) {{\
    1, 0, 0, 0,\
    0, 1, 0, 0,\
    0, 0, 1, 0,\
    x, y, z, 1,\
}}

#define MAXTRIX_SCALING(x, y, z) {{\
    x, 0, 0, 0,\
    0, y, 0, 0,\
    0, 0, z, 0,\
    0, 0, 0, 1,\
}}

#define MAXTRIX_ROTATION_X(a) {{\
    1,       0,       0, 0,\
    0,  cos(a),  sin(a), 0,\
    0, -sin(a),  cos(a), 0,\
    0,       0,       0, 1,\
}}

#define MAXTRIX_ROTATION_Y(a) {{\
     cos(a), 0, sin(a), 0,\
          0, 1,      0, 0,\
     sin(a), 0, cos(a), 0,\
          0, 0,      0, 1,\
}}

#define MAXTRIX_ROTATION_Z(a) {{\
    cos(a),  sin(a), 0, 0,\
   -sin(a),  cos(a), 0, 0,\
         0,       0, 1, 0,\
         0,       0, 0, 1,\
}}

void matrixIdentity(matrix *mat);
void matrixTranslation(matrix *mat, float x, float y, float z);
void matrixScaling(matrix *mat, float x, float y, float z);
void matrixRotationX(matrix *mat, float a);
void matrixRotationY(matrix *mat, float a);
void matrixRotationZ(matrix *mat, float a);
void matrixMultiply(matrix *mat, matrix *by);
void matrixTransformVector(matrix *mat, float vec[3]);
void matrixTransformXYZ(matrix *mat, float *x, float *y, float *z);

#endif
