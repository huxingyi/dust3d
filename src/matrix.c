#include <string.h>
#include "matrix.h"

void matrixIdentity(matrix *mat) {
    matrix identity = MATRIX_IDENTITY;
    memcpy(mat, &identity, sizeof(matrix));
}

void matrixTranslation(matrix *mat, float x, float y, float z) {
    matrix translation = MATRIX_TRANSLATTION(x, y, z);
    memcpy(mat, &translation, sizeof(matrix));
}

void matrixScaling(matrix *mat, float x, float y, float z) {
    matrix scaling = MAXTRIX_SCALING(x, y, z);
    memcpy(mat, &scaling, sizeof(matrix));
}

void matrixRotationX(matrix *mat, float a) {
    matrix rotationX = MAXTRIX_ROTATION_X(a);
    memcpy(mat, &rotationX, sizeof(matrix));
}

void matrixRotationY(matrix *mat, float a) {
    matrix rotationY = MAXTRIX_ROTATION_Y(a);
    memcpy(mat, &rotationY, sizeof(matrix));
}

void matrixRotationZ(matrix *mat, float a) {
    matrix rotationZ = MAXTRIX_ROTATION_Z(a);
    memcpy(mat, &rotationZ, sizeof(matrix));
}

void matrixMultiply(matrix *mat, matrix *by) {
    matrix tmp;
    memcpy(&tmp, mat, sizeof(matrix));
    mat->data[0] = tmp.data[0] * by->data[0] +
        tmp.data[4] * by->data[1] +
        tmp.data[8] * by->data[2] +
        tmp.data[12] * by->data[3];
    mat->data[4] = tmp.data[0] * by->data[4] +
        tmp.data[4] * by->data[5] +
        tmp.data[8] * by->data[6] +
        tmp.data[12] * by->data[7];
    mat->data[8] = tmp.data[0] * by->data[8] +
        tmp.data[4] * by->data[9] +
        tmp.data[8] * by->data[10] +
        tmp.data[12] * by->data[11];
    mat->data[12] = tmp.data[0] * by->data[12] +
        tmp.data[4] * by->data[13] +
        tmp.data[8] * by->data[14] +
        tmp.data[12] * by->data[15];

    mat->data[1] = tmp.data[1] * by->data[0] +
        tmp.data[5] * by->data[1] +
        tmp.data[9] * by->data[2] +
        tmp.data[13] * by->data[3];
    mat->data[5] = tmp.data[1] * by->data[4] +
        tmp.data[5] * by->data[5] +
        tmp.data[9] * by->data[6] +
        tmp.data[13] * by->data[7];
    mat->data[9] = tmp.data[1] * by->data[8] +
        tmp.data[5] * by->data[9] +
        tmp.data[9] * by->data[10] +
        tmp.data[13] * by->data[11];
    mat->data[13] = tmp.data[1] * by->data[12] +
        tmp.data[5] * by->data[13] +
        tmp.data[9] * by->data[14] +
        tmp.data[13] * by->data[15];

    mat->data[2] = tmp.data[2] * by->data[0] +
        tmp.data[6] * by->data[1] +
        tmp.data[10] * by->data[2] +
        tmp.data[14] * by->data[3];
    mat->data[6] = tmp.data[2] * by->data[4] +
        tmp.data[6] * by->data[5] +
        tmp.data[10] * by->data[6] +
        tmp.data[14] * by->data[7];
    mat->data[10] = tmp.data[2] * by->data[8] +
        tmp.data[6] * by->data[9] +
        tmp.data[10] * by->data[10] +
        tmp.data[14] * by->data[11];
    mat->data[14] = tmp.data[2] * by->data[12] +
        tmp.data[6] * by->data[13] +
        tmp.data[10] * by->data[14] +
        tmp.data[14] * by->data[15];

    mat->data[3] = tmp.data[3] * by->data[0] +
        tmp.data[7] * by->data[1] +
        tmp.data[11] * by->data[2] +
        tmp.data[15] * by->data[3];
    mat->data[7] = tmp.data[3] * by->data[4] +
        tmp.data[7] * by->data[5] +
        tmp.data[11] * by->data[6] +
        tmp.data[15] * by->data[7];
    mat->data[11] = tmp.data[3] * by->data[8] +
        tmp.data[7] * by->data[9] +
        tmp.data[11] * by->data[10] +
        tmp.data[15] * by->data[11];
    mat->data[15] = tmp.data[3] * by->data[12] +
        tmp.data[7] * by->data[13] +
        tmp.data[11] * by->data[14] +
        tmp.data[15] * by->data[15];
}

void matrixTransformVector(matrix *mat, float vec[3]) {
    float x = (vec[0] * mat->data[0]) +
        (vec[1] * mat->data[4]) + (vec[2] * mat->data[8]) + mat->data[12];
    float y = (vec[0] * mat->data[1]) +
        (vec[1] * mat->data[5]) + (vec[2] * mat->data[9]) + mat->data[13];
    float z = (vec[0] * mat->data[2]) +
        (vec[1] * mat->data[6]) + (vec[2] * mat->data[10]) + mat->data[14];
    vec[0] = x;
    vec[1] = y;
    vec[2] = z;
}

void matrixTransformXYZ(matrix *mat, float *x, float *y, float *z) {
    float vec[3] = {*x, *y, *z};
    matrixTransformVector(mat, vec);
    *x = vec[0];
    *y = vec[1];
    *z = vec[2];
}
