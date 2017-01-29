#include "matrix.h"
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.1415926
#endif

#define DEG2RAD (M_PI / 180.0)

matrix *matrixLoadIdentity(matrix *mat) {
  memset(mat->data, 0, sizeof(mat->data));
  mat->data[0] = 1;
  mat->data[5] = 1;
  mat->data[10] = 1;
  mat->data[15] = 1;
  return mat;
}

matrix *matrixTranslate(matrix *mat, float x, float y, float z) {
  matrixLoadIdentity(mat);
  mat->data[12] = x;
  mat->data[13] = y;
  mat->data[14] = z;
  return mat;
}

//
// matrixRotate modified from http://www.gamedev.net/topic/600537-instead-of-glrotatef-build-a-matrix/
//

matrix *matrixRotate(matrix *mat, float degree, float x, float y, float z) {
  float c;
  float s;
  float length;
  
  matrixLoadIdentity(mat);
  
  if (degree <= 0) {
    return mat;
  }
  
  length = sqrt(x * x + y * y + z * z);
  
  c = cos(degree * DEG2RAD);
  s = sin(degree * DEG2RAD);
  
  mat->data[0] = x * x * (1 - c) + c;
	mat->data[4] = x * y * (1 - c) - z * s;
	mat->data[8] = x * z * (1 - c) + y * s;

	mat->data[1] = y * x * (1 - c) + z * s;
	mat->data[5] = y * y * (1 - c) + c;
	mat->data[9] = y * z * (1 - c) - x * s;

	mat->data[2] = x * z * (1 - c) - y * s;
	mat->data[6] = y * z * (1 - c) + x * s;
	mat->data[10] = z * z * (1 - c) + c;
  
  return mat;

}

matrix *matrixRotateX(matrix *mat, float degree) {
  float c;
  float s;
  
  matrixLoadIdentity(mat);
  
  if (degree <= 0) {
    return mat;
  }
  
  c = cos(degree * DEG2RAD);
  s = sin(degree * DEG2RAD);

  mat->data[5] = c;
  mat->data[6] = s;
  mat->data[9] = -s;
  mat->data[10] = c;
  
  return mat;
}

matrix *matrixRotateY(matrix *mat, float degree) {
  float c;
  float s;
  
  matrixLoadIdentity(mat);
  
  if (degree <= 0) {
    return mat;
  }
  
  c = cos(degree * DEG2RAD);
  s = sin(degree * DEG2RAD);

  mat->data[0] = c;
  mat->data[2] = -s;
  mat->data[8] = s;
  mat->data[10] = c;
  
  return mat;
}

matrix *matrixRotateZ(matrix *mat, float degree) {
  float c;
  float s;
  
  matrixLoadIdentity(mat);
  
  if (degree <= 0) {
    return mat;
  }
  
  c = cos(degree * DEG2RAD);
  s = sin(degree * DEG2RAD);

  mat->data[0] = c;
  mat->data[1] = s;
  mat->data[4] = -s;
  mat->data[5] = c;
  
  return mat;
}

matrix *matrixScale(matrix *mat, float x, float y, float z) {
  matrixLoadIdentity(mat);
  mat->data[0] = x;
  mat->data[5] = y;
  mat->data[10] = z;
  return mat;
}

float *matrixTransformVector(matrix *mat, float *vec) {
  float x = (vec[0] * mat->data[0]) + 
      (vec[1] * mat->data[4]) + (vec[2] * mat->data[8]) + mat->data[12];
  float y = (vec[0] * mat->data[1]) + 
      (vec[1] * mat->data[5]) + (vec[2] * mat->data[9]) + mat->data[13];
  float z = (vec[0] * mat->data[2]) + 
      (vec[1] * mat->data[6]) + (vec[2] * mat->data[10]) + mat->data[14];
  vec[0] = x;
  vec[1] = y;
  vec[2] = z;
  return vec;
}

matrix *matrixAppend(matrix *mat, matrix *matB) {
  matrix structMatA;
  matrix *matA = &structMatA;
  
  memcpy(matA, mat, sizeof(matrix));
  
  mat->data[0] = matA->data[0] * matB->data[0] + 
      matA->data[4] * matB->data[1] + 
      matA->data[8] * matB->data[2] + 
      matA->data[12] * matB->data[3];
  mat->data[4] = matA->data[0] * matB->data[4] + 
      matA->data[4] * matB->data[5] + 
      matA->data[8] * matB->data[6] + 
      matA->data[12] * matB->data[7];
  mat->data[8] = matA->data[0] * matB->data[8] + 
      matA->data[4] * matB->data[9] + 
      matA->data[8] * matB->data[10] + 
      matA->data[12] * matB->data[11];
  mat->data[12] = matA->data[0] * matB->data[12] + 
      matA->data[4] * matB->data[13] + 
      matA->data[8] * matB->data[14] + 
      matA->data[12] * matB->data[15];

  mat->data[1] = matA->data[1] * matB->data[0] + 
      matA->data[5] * matB->data[1] + 
      matA->data[9] * matB->data[2] + 
      matA->data[13] * matB->data[3];
  mat->data[5] = matA->data[1] * matB->data[4] + 
      matA->data[5] * matB->data[5] + 
      matA->data[9] * matB->data[6] + 
      matA->data[13] * matB->data[7];
  mat->data[9] = matA->data[1] * matB->data[8] + 
      matA->data[5] * matB->data[9] + 
      matA->data[9] * matB->data[10] + 
      matA->data[13] * matB->data[11];
  mat->data[13] = matA->data[1] * matB->data[12] + 
      matA->data[5] * matB->data[13] + 
      matA->data[9] * matB->data[14] + 
      matA->data[13] * matB->data[15];

  mat->data[2] = matA->data[2] * matB->data[0] + 
      matA->data[6] * matB->data[1] + 
      matA->data[10] * matB->data[2] + 
      matA->data[14] * matB->data[3];
  mat->data[6] = matA->data[2] * matB->data[4] + 
      matA->data[6] * matB->data[5] + 
      matA->data[10] * matB->data[6] + 
      matA->data[14] * matB->data[7];
  mat->data[10] = matA->data[2] * matB->data[8] + 
      matA->data[6] * matB->data[9] + 
      matA->data[10] * matB->data[10] + 
      matA->data[14] * matB->data[11];
  mat->data[14] = matA->data[2] * matB->data[12] + 
      matA->data[6] * matB->data[13] + 
      matA->data[10] * matB->data[14] + 
      matA->data[14] * matB->data[15];

  mat->data[3] = matA->data[3] * matB->data[0] + 
      matA->data[7] * matB->data[1] + 
      matA->data[11] * matB->data[2] + 
      matA->data[15] * matB->data[3];
  mat->data[7] = matA->data[3] * matB->data[4] + 
      matA->data[7] * matB->data[5] + 
      matA->data[11] * matB->data[6] + 
      matA->data[15] * matB->data[7];
  mat->data[11] = matA->data[3] * matB->data[8] + 
      matA->data[7] * matB->data[9] + 
      matA->data[11] * matB->data[10] + 
      matA->data[15] * matB->data[11];
  mat->data[15] = matA->data[3] * matB->data[12] + 
      matA->data[7] * matB->data[13] + 
      matA->data[11] * matB->data[14] + 
      matA->data[15] * matB->data[15];
      
  return mat;
}

