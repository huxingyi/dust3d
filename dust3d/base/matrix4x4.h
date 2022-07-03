/*
 *  Copyright (c) 2016-2021 Jeremy HU <jeremy-at-dust3d dot org>. All rights reserved. 
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:

 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.

 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#ifndef DUST3D_BASE_MATRIX4X4_H_
#define DUST3D_BASE_MATRIX4X4_H_

#include <cstring>
#include <memory>
#include <dust3d/base/vector3.h>
#include <dust3d/base/quaternion.h>

namespace dust3d
{
    
class Matrix4x4;
    
inline Vector3 operator*(const Matrix4x4 &m, const Vector3 &v);

class Matrix4x4
{
public:
    // | M00  M10  M20  M30 |
    // | M01  M11  M21  M31 |
    // | M02  M12  M22  M32 |
    // | M03  M13  M23  M33 |
    
    // | 0  4  8  12 |
    // | 1  5  9  13 |
    // | 2  6  10 14 |
    // | 3  7  11 15 |
          
    const int M00 = 0;
    const int M01 = 1;
    const int M02 = 2;
    const int M03 = 3;
    const int M10 = 4;
    const int M11 = 5;
    const int M12 = 6;
    const int M13 = 7;
    const int M20 = 8;
    const int M21 = 9;
    const int M22 = 10;
    const int M23 = 11;
    const int M30 = 12;
    const int M31 = 13;
    const int M32 = 14;
    const int M33 = 15;

    inline Matrix4x4() = default;
    
    // https://en.wikipedia.org/wiki/Rotation_matrix#Rotation_matrix_from_axis_and_angle
    inline void rotate(const Vector3 &axis, double angle)
    {
        double cosine = std::cos(angle);
        double sine = std::sin(angle);
        double oneMinusCosine = 1.0 - cosine;
        Matrix4x4 by;
        double *byData = by.data();
        byData[M00] = cosine + axis.x() * axis.x() * oneMinusCosine;
        byData[M10] = axis.x() * axis.y() * oneMinusCosine - axis.z() * sine;
        byData[M20] = axis.x() * axis.z() * oneMinusCosine + axis.y() * sine;
        byData[M30] = 0.0;
        byData[M01] = axis.y() * axis.x() * oneMinusCosine + axis.z() * sine;
        byData[M11] = cosine + axis.y() * axis.y() * oneMinusCosine;
        byData[M21] = axis.y() * axis.z() * oneMinusCosine - axis.x() * sine;
        byData[M31] = 0.0;
        byData[M02] = axis.z() * axis.x() * oneMinusCosine - axis.y() * sine;
        byData[M12] = axis.z() * axis.y() * oneMinusCosine + axis.x() * sine;
        byData[M22] = cosine + axis.z() * axis.z() * oneMinusCosine;
        byData[M32] = 0.0;
        byData[M03] = 0.0;
        byData[M13] = 0.0;
        byData[M23] = 0.0;
        byData[M33] = 1.0;
       *this *= by;
    }
    
    inline const double *constData() const
    {
        return &m_data[0];
    }
    
    inline double *data()
    {
        return &m_data[0];
    }
    
    inline void rotate(const Quaternion &q)
    {
        Matrix4x4 tmp;
        double *tmpData = tmp.data();
        double xx = q.x() + q.x();
        double yy = q.y() + q.y();
        double zz = q.z() + q.z();
        double xxw = xx * q.w();
        double yyw = yy * q.w();
        double zzw = zz * q.w();
        double xxx = xx * q.x();
        double xxy = xx * q.y();
        double xxz = xx * q.z();
        double yyy = yy * q.y();
        double yyz = yy * q.z();
        double zzz = zz * q.z();
        tmpData[M00] = 1.0f - (yyy + zzz);
        tmpData[M10] =         xxy - zzw;
        tmpData[M20] =         xxz + yyw;
        tmpData[M30] = 0.0f;
        tmpData[M01] =         xxy + zzw;
        tmpData[M11] = 1.0f - (xxx + zzz);
        tmpData[M21] =         yyz - xxw;
        tmpData[M31] = 0.0f;
        tmpData[M02] =         xxz - yyw;
        tmpData[M12] =         yyz + xxw;
        tmpData[M22] = 1.0f - (xxx + yyy);
        tmpData[M32] = 0.0f;
        tmpData[M03] = 0.0f;
        tmpData[M13] = 0.0f;
        tmpData[M23] = 0.0f;
        tmpData[M33] = 1.0f;
        *this *= tmp;
    }
    
    inline Matrix4x4 &translate(const Vector3 &v)
    {
        m_data[M30] += v.x();
        m_data[M31] += v.y();
        m_data[M32] += v.z();
        return *this;
    }
    
    inline Matrix4x4 inverted() const
    {
        Matrix4x4 m;
        double *dest = m.data();
        dest[0] = m_data[5] * m_data[10] * m_data[15] - 
            m_data[5] * m_data[11] * m_data[14] - 
            m_data[9] * m_data[6] * m_data[15] + 
            m_data[9] * m_data[7] * m_data[14] +
            m_data[13] * m_data[6] * m_data[11] - 
            m_data[13] * m_data[7] * m_data[10];
        dest[4] = -m_data[4] * m_data[10] * m_data[15] + 
            m_data[4] * m_data[11] * m_data[14] + 
            m_data[8] * m_data[6] * m_data[15] - 
            m_data[8] * m_data[7] * m_data[14] - 
            m_data[12] * m_data[6] * m_data[11] + 
            m_data[12] * m_data[7] * m_data[10];
        dest[8] = m_data[4] * m_data[9] * m_data[15] - 
            m_data[4] * m_data[11] * m_data[13] - 
            m_data[8] * m_data[5] * m_data[15] + 
            m_data[8] * m_data[7] * m_data[13] + 
            m_data[12] * m_data[5] * m_data[11] - 
            m_data[12] * m_data[7] * m_data[9];
        dest[12] = -m_data[4] * m_data[9] * m_data[14] + 
            m_data[4] * m_data[10] * m_data[13] +
            m_data[8] * m_data[5] * m_data[14] - 
            m_data[8] * m_data[6] * m_data[13] - 
            m_data[12] * m_data[5] * m_data[10] + 
            m_data[12] * m_data[6] * m_data[9];
        dest[1] = -m_data[1] * m_data[10] * m_data[15] + 
            m_data[1] * m_data[11] * m_data[14] + 
            m_data[9] * m_data[2] * m_data[15] - 
            m_data[9] * m_data[3] * m_data[14] - 
            m_data[13] * m_data[2] * m_data[11] + 
            m_data[13] * m_data[3] * m_data[10];
        dest[5] = m_data[0] * m_data[10] * m_data[15] - 
            m_data[0] * m_data[11] * m_data[14] - 
            m_data[8] * m_data[2] * m_data[15] + 
            m_data[8] * m_data[3] * m_data[14] + 
            m_data[12] * m_data[2] * m_data[11] - 
            m_data[12] * m_data[3] * m_data[10];
        dest[9] = -m_data[0] * m_data[9] * m_data[15] + 
            m_data[0] * m_data[11] * m_data[13] + 
            m_data[8] * m_data[1] * m_data[15] - 
            m_data[8] * m_data[3] * m_data[13] - 
            m_data[12] * m_data[1] * m_data[11] + 
            m_data[12] * m_data[3] * m_data[9];
        dest[13] = m_data[0] * m_data[9] * m_data[14] - 
            m_data[0] * m_data[10] * m_data[13] - 
            m_data[8] * m_data[1] * m_data[14] + 
            m_data[8] * m_data[2] * m_data[13] + 
            m_data[12] * m_data[1] * m_data[10] - 
            m_data[12] * m_data[2] * m_data[9];
        dest[2] = m_data[1] * m_data[6] * m_data[15] - 
            m_data[1] * m_data[7] * m_data[14] - 
            m_data[5] * m_data[2] * m_data[15] + 
            m_data[5] * m_data[3] * m_data[14] + 
            m_data[13] * m_data[2] * m_data[7] - 
            m_data[13] * m_data[3] * m_data[6];
        dest[6] = -m_data[0] * m_data[6] * m_data[15] + 
            m_data[0] * m_data[7] * m_data[14] + 
            m_data[4] * m_data[2] * m_data[15] - 
            m_data[4] * m_data[3] * m_data[14] - 
            m_data[12] * m_data[2] * m_data[7] + 
            m_data[12] * m_data[3] * m_data[6];
        dest[10] = m_data[0] * m_data[5] * m_data[15] - 
            m_data[0] * m_data[7] * m_data[13] - 
            m_data[4] * m_data[1] * m_data[15] + 
            m_data[4] * m_data[3] * m_data[13] + 
            m_data[12] * m_data[1] * m_data[7] - 
            m_data[12] * m_data[3] * m_data[5];
        dest[14] = -m_data[0] * m_data[5] * m_data[14] + 
            m_data[0] * m_data[6] * m_data[13] + 
            m_data[4] * m_data[1] * m_data[14] - 
            m_data[4] * m_data[2] * m_data[13] - 
            m_data[12] * m_data[1] * m_data[6] + 
            m_data[12] * m_data[2] * m_data[5];
        dest[3] = -m_data[1] * m_data[6] * m_data[11] + 
            m_data[1] * m_data[7] * m_data[10] + 
            m_data[5] * m_data[2] * m_data[11] - 
            m_data[5] * m_data[3] * m_data[10] - 
            m_data[9] * m_data[2] * m_data[7] + 
            m_data[9] * m_data[3] * m_data[6];
        dest[7] = m_data[0] * m_data[6] * m_data[11] - 
            m_data[0] * m_data[7] * m_data[10] - 
            m_data[4] * m_data[2] * m_data[11] + 
            m_data[4] * m_data[3] * m_data[10] + 
            m_data[8] * m_data[2] * m_data[7] - 
            m_data[8] * m_data[3] * m_data[6];
        dest[11] = -m_data[0] * m_data[5] * m_data[11] + 
            m_data[0] * m_data[7] * m_data[9] + 
            m_data[4] * m_data[1] * m_data[11] - 
            m_data[4] * m_data[3] * m_data[9] - 
            m_data[8] * m_data[1] * m_data[7] + 
            m_data[8] * m_data[3] * m_data[5];
        dest[15] = m_data[0] * m_data[5] * m_data[10] - 
            m_data[0] * m_data[6] * m_data[9] - 
            m_data[4] * m_data[1] * m_data[10] + 
            m_data[4] * m_data[2] * m_data[9] + 
            m_data[8] * m_data[1] * m_data[6] - 
            m_data[8] * m_data[2] * m_data[5];

        double det = m_data[0] * dest[0] + 
            m_data[1] * dest[4] + 
            m_data[2] * dest[8] + 
            m_data[3] * dest[12];

        if (Math::isZero(det))
            return Matrix4x4();

        det = 1.0 / det;

        for (size_t i = 0; i < 16; ++i)
            dest[i] *= det;
    }
    
    inline Matrix4x4 &operator*=(const Matrix4x4 &by)
    {
        double tmpData[16];
        memcpy(tmpData, m_data, sizeof(tmpData));
        const double *byData = by.constData();
        m_data[0] = tmpData[0] * byData[0] +
            tmpData[4] * byData[1] +
            tmpData[8] * byData[2] +
            tmpData[12] * byData[3];
        m_data[4] = tmpData[0] * byData[4] +
            tmpData[4] * byData[5] +
            tmpData[8] * byData[6] +
            tmpData[12] * byData[7];
        m_data[8] = tmpData[0] * byData[8] +
            tmpData[4] * byData[9] +
            tmpData[8] * byData[10] +
            tmpData[12] * byData[11];
        m_data[12] = tmpData[0] * byData[12] +
            tmpData[4] * byData[13] +
            tmpData[8] * byData[14] +
            tmpData[12] * byData[15];

        m_data[1] = tmpData[1] * byData[0] +
            tmpData[5] * byData[1] +
            tmpData[9] * byData[2] +
            tmpData[13] * byData[3];
        m_data[5] = tmpData[1] * byData[4] +
            tmpData[5] * byData[5] +
            tmpData[9] * byData[6] +
            tmpData[13] * byData[7];
        m_data[9] = tmpData[1] * byData[8] +
            tmpData[5] * byData[9] +
            tmpData[9] * byData[10] +
            tmpData[13] * byData[11];
        m_data[13] = tmpData[1] * byData[12] +
            tmpData[5] * byData[13] +
            tmpData[9] * byData[14] +
            tmpData[13] * byData[15];

        m_data[2] = tmpData[2] * byData[0] +
            tmpData[6] * byData[1] +
            tmpData[10] * byData[2] +
            tmpData[14] * byData[3];
        m_data[6] = tmpData[2] * byData[4] +
            tmpData[6] * byData[5] +
            tmpData[10] * byData[6] +
            tmpData[14] * byData[7];
        m_data[10] = tmpData[2] * byData[8] +
            tmpData[6] * byData[9] +
            tmpData[10] * byData[10] +
            tmpData[14] * byData[11];
        m_data[14] = tmpData[2] * byData[12] +
            tmpData[6] * byData[13] +
            tmpData[10] * byData[14] +
            tmpData[14] * byData[15];

        m_data[3] = tmpData[3] * byData[0] +
            tmpData[7] * byData[1] +
            tmpData[11] * byData[2] +
            tmpData[15] * byData[3];
        m_data[7] = tmpData[3] * byData[4] +
            tmpData[7] * byData[5] +
            tmpData[11] * byData[6] +
            tmpData[15] * byData[7];
        m_data[11] = tmpData[3] * byData[8] +
            tmpData[7] * byData[9] +
            tmpData[11] * byData[10] +
            tmpData[15] * byData[11];
        m_data[15] = tmpData[3] * byData[12] +
            tmpData[7] * byData[13] +
            tmpData[11] * byData[14] +
            tmpData[15] * byData[15];
        return *this;
    }

private:
    double m_data[16] = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    };
};

inline Vector3 operator*(const Matrix4x4 &m, const Vector3 &v)
{
    const double *data = m.constData();
    return Vector3((v.x() * data[0]) + (v.y() * data[4]) + (v.z() * data[8]),
        (v.x() * data[1]) + (v.y() * data[5]) + (v.z() * data[9]),
        (v.x() * data[2]) + (v.y() * data[6]) + (v.z() * data[10]));
}

}

#endif
