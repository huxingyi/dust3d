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

#ifndef DUST3D_BASE_PART_TARGET_H_
#define DUST3D_BASE_PART_TARGET_H_

#include <string>

namespace dust3d
{

enum class PartTarget
{
    Model = 0,
    CutFace,
    StitchingLine,
    Count
};
PartTarget PartTargetFromString(const char *targetString);
#define IMPL_PartTargetFromString                                               \
PartTarget PartTargetFromString(const char *targetString)                       \
{                                                                               \
    std::string target = targetString;                                          \
    if (target == "Model")                                                      \
        return PartTarget::Model;                                               \
    if (target == "CutFace")                                                    \
        return PartTarget::CutFace;                                             \
    if (target == "StitchingLine")                                              \
        return PartTarget::StitchingLine;                                       \
    return PartTarget::Model;                                                   \
}
const char *PartTargetToString(PartTarget target);
#define IMPL_PartTargetToString                                                 \
const char *PartTargetToString(PartTarget target)                               \
{                                                                               \
    switch (target) {                                                           \
        case PartTarget::Model:                                                 \
            return "Model";                                                     \
        case PartTarget::CutFace:                                               \
            return "CutFace";                                                   \
        case PartTarget::StitchingLine:                                         \
            return "StitchingLine";                                             \
        default:                                                                \
            return "Model";                                                     \
    }                                                                           \
}
std::string PartTargetToDispName(PartTarget target);
#define IMPL_PartTargetToDispName                                               \
std::string PartTargetToDispName(PartTarget target)                             \
{                                                                               \
    switch (target) {                                                           \
        case PartTarget::Model:                                                 \
            return std::string("Model");                                        \
        case PartTarget::CutFace:                                               \
            return std::string("Cut Face");                                     \
        case PartTarget::StitchingLine:                                         \
            return std::string("Stitching Line");                               \
        default:                                                                \
            return std::string("Model");                                        \
    }                                                                           \
}

}

#endif
