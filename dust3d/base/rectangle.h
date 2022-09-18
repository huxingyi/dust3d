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

#ifndef DUST3D_BASE_RECTANGLE_H_
#define DUST3D_BASE_RECTANGLE_H_

#include <vector>
#include <algorithm>

namespace dust3d
{

class Rectangle
{
public:
    inline Rectangle() :
        m_data {0.0, 0.0, 0.0, 0.0}
    {
    }
    
    inline Rectangle(double x, double y, double width, double height) :
        m_data {x, y, width, height}
    {
    }
    
    inline double &operator[](size_t index)
    {
        return m_data[index];
    }
    
    inline const double &operator[](size_t index) const
    {
        return m_data[index];
    }
    
    inline const double &x() const
    {
        return m_data[0];
    }
    
    inline const double &y() const
    {
        return m_data[1];
    }
    
    inline const double &left() const
    {
        return m_data[0];
    }
    
    inline double right() const
    {
        return left() + width();
    }
    
    inline const double &top() const
    {
        return m_data[1];
    }
    
    inline double bottom() const
    {
        return top() + height();
    }
    
    inline const double &width() const
    {
        return m_data[2];
    }
    
    inline const double &height() const
    {
        return m_data[3];
    }
    
    inline Rectangle intersected(const Rectangle &other) const
    {
        const auto &rectangle1 = *this;
        const auto &rectangle2 = other;
        
        if (rectangle2.left() >= rectangle1.right())
            return Rectangle();
        
        if (rectangle2.right() <= rectangle1.left())
            return Rectangle();

        if (rectangle2.top() >= rectangle1.bottom())
            return Rectangle();
        
        if (rectangle2.bottom() <= rectangle1.top())
            return Rectangle();
        
        double left = std::max(rectangle1.left(), rectangle2.left());
        double width = std::min(rectangle1.right(), rectangle2.right()) - left;
        double top = std::max(rectangle1.top(), rectangle2.top());
        double height = std::min(rectangle1.bottom(), rectangle2.bottom()) - top;
        
        return Rectangle(left, top, width, height);
    }
    
    inline bool contains(double x, double y) const
    {
        return (x >= left() && x < right()) &&
            (y >= top() && y < bottom());
    }
    
private:
    double m_data[4] = {0.0, 0.0, 0.0, 0.0};
};

}

#endif
