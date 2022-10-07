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

#ifndef DUST3D_BASE_DEBUG_H_
#define DUST3D_BASE_DEBUG_H_

#include <iostream>

namespace dust3d
{

class Debug
{
public:
    Debug() = default;
    
    Debug(Debug const &) = delete;
    
    Debug &operator=(Debug const &) = delete;
    
    Debug &operator=(Debug &&) = delete;
    
    Debug(Debug &&instance) noexcept 
        : m_firstItem(false)
    {
        instance.m_lastItem = false;
    }

    ~Debug() 
    {
        if (m_lastItem)
            std::cout << '\n';
    }
    
    template <typename T>
    friend Debug operator<<(Debug instance, const T &value) 
    {
        if (instance.m_firstItem)
            instance.m_firstItem = false;
        else
            std::cout << ' ';

        std::cout << value;
        return instance;
    }
    
private:
    bool m_firstItem = true;
    bool m_lastItem = true;
};

}

#ifndef dust3dDebug

#define DEBUG_STRINGIZE1(x) #x
#define DEBUG_STRINGIZE2(x) DEBUG_STRINGIZE1(x)

#define dust3dDebug dust3d::Debug() << __FILE__ "(" DEBUG_STRINGIZE2(__LINE__) ")#" << __func__ << ":"

#endif

#endif
