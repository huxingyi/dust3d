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

#ifndef DUST3D_BASE_COLOR_H_
#define DUST3D_BASE_COLOR_H_

#include <dust3d/base/debug.h>
#include <iostream>
#include <string>

namespace dust3d {

class Color;

inline std::string to_string(const Color& color);

class Color {
public:
    inline Color()
        : m_data { 0.0, 0.0, 0.0, 1.0 }
    {
    }

    inline Color(double r, double g, double b)
        : m_data { r, g, b, 1.0 }
    {
    }

    inline Color(double r, double g, double b, double alpha)
        : m_data { r, g, b, alpha }
    {
    }

    inline Color(const std::string& name)
    {
        if (7 == name.size() && '#' == name[0]) {
            m_data[0] = strtoul(name.substr(1, 2).c_str(), nullptr, 16) / 255.0;
            m_data[1] = strtoul(name.substr(3, 2).c_str(), nullptr, 16) / 255.0;
            m_data[2] = strtoul(name.substr(5, 2).c_str(), nullptr, 16) / 255.0;
        } else if (9 == name.size() && '#' == name[0]) {
            m_data[3] = strtoul(name.substr(1, 2).c_str(), nullptr, 16) / 255.0;
            m_data[0] = strtoul(name.substr(3, 2).c_str(), nullptr, 16) / 255.0;
            m_data[1] = strtoul(name.substr(5, 2).c_str(), nullptr, 16) / 255.0;
            m_data[2] = strtoul(name.substr(7, 2).c_str(), nullptr, 16) / 255.0;
        }
    }

    inline double& operator[](size_t index)
    {
        return m_data[index];
    }

    inline const double& operator[](size_t index) const
    {
        return m_data[index];
    }

    inline const double& alpha() const
    {
        return m_data[3];
    }

    inline const double& r() const
    {
        return m_data[0];
    }

    inline const double& g() const
    {
        return m_data[1];
    }

    inline const double& b() const
    {
        return m_data[2];
    }

    inline const double& red() const
    {
        return m_data[0];
    }

    inline const double& green() const
    {
        return m_data[1];
    }

    inline const double& blue() const
    {
        return m_data[2];
    }

    inline void setRed(double r)
    {
        m_data[0] = r;
    }

    inline void setGreen(double g)
    {
        m_data[1] = g;
    }

    inline void setBlue(double b)
    {
        m_data[2] = b;
    }

    inline static Color createRed()
    {
        return Color(1.0, 0.0, 0.0);
    }

    inline static Color createWhite()
    {
        return Color(1.0, 1.0, 1.0);
    }

    inline static Color createBlack()
    {
        return Color(0.0, 0.0, 0.0);
    }

    inline static Color createTransparent()
    {
        return Color(0.0, 0.0, 0.0, 0.0);
    }

    inline std::string toString() const
    {
        return to_string(*this);
    }

private:
    double m_data[4] = { 0.0, 0.0, 0.0, 1.0 };
};

inline std::string to_string(const Color& color)
{
    static const char* digits = "0123456789ABCDEF";
    std::string name = "#00000000";

    int alpha = static_cast<int>(color.alpha() * 255);
    int r = static_cast<int>(color.r() * 255);
    int g = static_cast<int>(color.g() * 255);
    int b = static_cast<int>(color.b() * 255);

    name[1] = digits[(alpha & 0xf0) >> 4];
    name[2] = digits[alpha & 0x0f];
    name[3] = digits[(r & 0xf0) >> 4];
    name[4] = digits[r & 0x0f];
    name[5] = digits[(g & 0xf0) >> 4];
    name[6] = digits[g & 0x0f];
    name[7] = digits[(b & 0xf0) >> 4];
    name[8] = digits[b & 0x0f];

    return name;
}

inline Color operator*(const Color& color, double number)
{
    return Color(number * color[0], number * color[1], number * color[2], number * color[3]);
}

inline Color operator+(const Color& a, const Color& b)
{
    return Color(a[0] + b[0], a[1] + b[1], a[2] + b[2], a[3] + b[3]);
}

}

#endif
