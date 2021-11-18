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

#ifndef DUST3D_BASE_QUATERNION_H_
#define DUST3D_BASE_QUATERNION_H_

#include <dust3d/base/vector3.h>

namespace dust3d
{
    
class Quaternion;

inline Quaternion operator*(const Quaternion &q, const double &number);
inline Quaternion operator*(const double &number, const Quaternion &q);
inline Quaternion operator+(const Quaternion &q, const Quaternion &p);
inline Quaternion operator/(const Quaternion &q, const double &number);
inline Quaternion operator/(const double &number, const Quaternion &q);

class Quaternion
{
public:
    inline Quaternion() = default;
    
    inline Quaternion(double w, double x, double y, double z) :
        m_data {w, x, y, z}
    {
    }
    
    inline Quaternion(const Quaternion &q) :
        m_data {q.w(), q.x(), q.y(), q.z()}
    {
    }

    inline const double &x() const
    {
        return m_data[1];
    }
    
    inline const double &y() const
    {
        return m_data[2];
    }
    
    inline const double &z() const
    {
        return m_data[3];
    }
    
    inline const double &w() const
    {
        return m_data[0];
    }
    
    inline Quaternion &Quaternion::operator*=(const double &number) 
    {
        m_data[0] *= number;
        m_data[1] *= number;
        m_data[2] *= number;
        m_data[3] *= number;
        return *this;
    }

    inline static Quaternion slerp(const Quaternion &a, const Quaternion &b, double t)
    {
        if (t <= 0.0)
            return a;
        
        if (t >= 1.0)
            return b;
        
        double dot = a.w() * b.w() + a.x() * b.x() * a.y() * b.y() + a.z() * b.z();
        Quaternion to(b);
        if (dot < 0.0) {
            dot *= -1.0;
            to *= -1.0;
        }
        
        double angle = std::acos(dot);
        double sine = std::sin(angle);
        if (Math::isZero(sine)) {
            return a * (1.0 - t) + b * t;
        }
        
        return (((Quaternion)(a * std::sin(angle * (1.0f - t)))) + 
            ((Quaternion)(to * std::sin(angle * t)))) / sine;
    }
    
    inline static Quaternion fromAxisAndAngle(const Vector3 &axis, double angle)
    {
        Vector3 axisNormalized = axis.normalized();
        double halfAngle = angle * 0.5;
        double sine = std::sin(halfAngle);
        return Quaternion(std::cos(halfAngle), 
            axisNormalized.x() * sine, axisNormalized.y() * sine, axisNormalized.z() * sine);
    }
    
    inline static Quaternion rotationTo(const Vector3 &from, const Vector3 &to)
    {
        Vector3 a = from.normalized();
        Vector3 b = to.normalized();
        double d = Vector3::dotProduct(a, b) + 1.0;
        if (Math::isZero(d)) {
            Vector3 axis = Vector3::crossProduct(Vector3(1.0, 0.0, 0.0), a).normalized();
            if (axis.isZero())
                axis = Vector3::crossProduct(Vector3(0.0, 1.0, 0.0), a).normalized();
            return Quaternion(0.0, axis.x(), axis.y(), axis.z());
        }
        d = std::sqrt(d * 2.0);
        Vector3 v = Vector3::crossProduct(a, b) / d;
        return Quaternion(d * 0.5, v.x(), v.y(), v.z()).normalized();
    }
    
    inline Quaternion normalized() const
    {
        double length2 = x() * x() + y() * y() + z() * z() + w() * w();
        double length = std::sqrt(length2);
        if (Math::isZero(length))
            return Quaternion();
        
        return Quaternion(w() / length, x() / length, y() / length, z() / length);
    }
private:
    double m_data[4] = {0.0};
};

inline Quaternion operator*(const Quaternion &q, const double &number) 
{
    return Quaternion(q.w() * number, q.x() * number, q.y() * number, q.z() * number);
}

inline Quaternion operator*(const double &number, const Quaternion &q) 
{
    return Quaternion(q.w() * number, q.x() * number, q.y() * number, q.z() * number);
}

inline Quaternion operator+(const Quaternion &q, const Quaternion &p)
{
    return Quaternion(p.w() + q.w(), p.x() + q.x(), p.y() + q.y(), p.z() + q.z());
}

inline Quaternion operator/(const Quaternion &q, const double &number) 
{
    return Quaternion(q.w() / number, q.x() / number, q.y() / number, q.z() / number);
}

inline Quaternion operator/(const double &number, const Quaternion &q)
{
    return Quaternion(q.w() / number, q.x() / number, q.y() / number, q.z() / number);
}

}

#endif
