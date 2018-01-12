#include <string.h>
#include "matrix4x4.h"

void matrix4x4Identity(matrix4x4 *mat) {
    *mat = ((matrix4x4)MATRIX4X4_IDENTITY);
}

void matrix4x4Translation(matrix4x4 *mat, float x, float y, float z) {
    *mat = ((matrix4x4)MATRIX4X4_TRANSLATTION(x, y, z));
}

void matrix4x4Scaling(matrix4x4 *mat, float x, float y, float z) {
    *mat = ((matrix4x4)MATRIX4X4_SCALING(x, y, z));
}

void matrix4x4RotationX(matrix4x4 *mat, float a) {
    *mat = ((matrix4x4)MATRIX4X4_ROTATION_X(a));
}

void matrix4x4RotationY(matrix4x4 *mat, float a) {
    *mat = ((matrix4x4)MATRIX4X4_ROTATION_Y(a));
}

void matrix4x4RotationZ(matrix4x4 *mat, float a) {
    *mat = ((matrix4x4)MATRIX4X4_ROTATION_Z(a));
}

void matrix4x4Multiply(matrix4x4 *mat, matrix4x4 *by) {
    matrix4x4 tmp;
    memcpy(&tmp, mat, sizeof(matrix4x4));
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

void matrix4x4TransformPoint3d(matrix4x4 *mat, point3d *point) {
    float x = (point->x * mat->data[0]) +
        (point->y * mat->data[4]) + (point->z * mat->data[8]) + mat->data[12];
    float y = (point->x * mat->data[1]) +
        (point->y * mat->data[5]) + (point->z * mat->data[9]) + mat->data[13];
    float z = (point->x * mat->data[2]) +
        (point->y * mat->data[6]) + (point->z * mat->data[10]) + mat->data[14];
    point->x = x;
    point->y = y;
    point->z = z;
}

void matrix4x4TransformVector3d(matrix4x4 *mat, vector3d *vec) {
    float x = (vec->x * mat->data[0]) +
        (vec->y * mat->data[4]) + (vec->z * mat->data[8]);
    float y = (vec->x * mat->data[1]) +
        (vec->y * mat->data[5]) + (vec->z * mat->data[9]);
    float z = (vec->x * mat->data[2]) +
        (vec->y * mat->data[6]) + (vec->z * mat->data[10]);
    vec->x = x;
    vec->y = y;
    vec->z = z;
}

void matrix4x4PerspectiveProjection(matrix4x4 *mat, float fov, float aspect, float near, float far) {
    *mat = ((matrix4x4)MATRIX4X4_PERSPECTIVE_PROJECTION(fov, aspect, near, far));
}

void matrix4x4Eye(matrix4x4 *mat, point3d *eye, point3d *center, vector3d *up) {
    vector3d right, direct, upNormalized;
    float u, v, w;
    vector3dSegment(eye, center, &direct);
    vector3dNormalize(&direct);
    upNormalized = *up;
    vector3dNormalize(&upNormalized);
    vector3dCrossProduct(&direct, &upNormalized, &right);
    vector3dNormalize(&right);
    vector3dCrossProduct(&right, &direct, &upNormalized);
    u = -vector3dDotProduct(&right, eye);
    v = -vector3dDotProduct(&upNormalized, eye);
    w = vector3dDotProduct(&direct, eye);
    *mat = ((matrix4x4){{
                right.x,        right.y,        right.z, u,
         upNormalized.x, upNormalized.y, upNormalized.z, v,
              -direct.x,      -direct.y,      -direct.z, w,
                      0,              0,              0, 1,
    }});
}


