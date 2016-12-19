#ifndef __MATRIX_H__
#define __MATRIX_H__

// Modified from http://wiki.unity3d.com/index.php?title=Matrix

typedef struct matrix {
  float data[16];
} matrix;

matrix *matrixLoadIdentity(matrix *mat);
matrix *matrixTranslate(matrix *mat, float x, float y, float z);
matrix *matrixRotateX(matrix *mat, float degree);
matrix *matrixRotateY(matrix *mat, float degree);
matrix *matrixRotateZ(matrix *mat, float degree);
matrix *matrixScale(matrix *mat, float x, float y, float z);
float *matrixTransformVector(matrix *mat, float *vec);
matrix *matrixAppend(matrix *mat, matrix *matB);

#endif
