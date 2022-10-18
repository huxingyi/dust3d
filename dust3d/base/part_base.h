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

#ifndef DUST3D_BASE_PART_BASE_H_
#define DUST3D_BASE_PART_BASE_H_

#include <string>

namespace dust3d {

enum class PartBase {
    XYZ = 0,
    Average,
    YZ,
    XY,
    ZX,
    Count
};
PartBase PartBaseFromString(const char* baseString);
#define IMPL_PartBaseFromString                         \
    PartBase PartBaseFromString(const char* baseString) \
    {                                                   \
        std::string base = baseString;                  \
        if (base == "XYZ")                              \
            return PartBase::XYZ;                       \
        if (base == "Average")                          \
            return PartBase::Average;                   \
        if (base == "YZ")                               \
            return PartBase::YZ;                        \
        if (base == "XY")                               \
            return PartBase::XY;                        \
        if (base == "ZX")                               \
            return PartBase::ZX;                        \
        return PartBase::XYZ;                           \
    }
const char* PartBaseToString(PartBase base);
#define IMPL_PartBaseToString                   \
    const char* PartBaseToString(PartBase base) \
    {                                           \
        switch (base) {                         \
        case PartBase::XYZ:                     \
            return "XYZ";                       \
        case PartBase::Average:                 \
            return "Average";                   \
        case PartBase::YZ:                      \
            return "YZ";                        \
        case PartBase::XY:                      \
            return "XY";                        \
        case PartBase::ZX:                      \
            return "ZX";                        \
        default:                                \
            return "XYZ";                       \
        }                                       \
    }
std::string PartBaseToDispName(PartBase base);
#define IMPL_PartBaseToDispName                   \
    std::string PartBaseToDispName(PartBase base) \
    {                                             \
        switch (base) {                           \
        case PartBase::XYZ:                       \
            return std::string("Dynamic");        \
        case PartBase::Average:                   \
            return std::string("Average");        \
        case PartBase::YZ:                        \
            return std::string("Side Plane");     \
        case PartBase::XY:                        \
            return std::string("Front Plane");    \
        case PartBase::ZX:                        \
            return std::string("Top Plane");      \
        default:                                  \
            return std::string("Dynamic");        \
        }                                         \
    }

}

#endif
