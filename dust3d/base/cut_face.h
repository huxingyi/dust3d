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

#ifndef DUST3D_BASE_CUT_FACE_H_
#define DUST3D_BASE_CUT_FACE_H_

#include <string>
#include <vector>
#include <tuple>
#include <dust3d/base/vector2.h>

namespace dust3d
{
    
enum class CutFace
{
    Quad = 0,
    Pentagon,
    Hexagon,
    Triangle,
    UserDefined,
    Count
};

CutFace CutFaceFromString(const char *faceString);
#define IMPL_CutFaceFromString                                              \
CutFace CutFaceFromString(const char *faceString)                           \
{                                                                           \
    std::string face = faceString;                                              \
    if (face == "Quad")                                                     \
        return CutFace::Quad;                                               \
    if (face == "Pentagon")                                                 \
        return CutFace::Pentagon;                                           \
    if (face == "Hexagon")                                                  \
        return CutFace::Hexagon;                                            \
    if (face == "Triangle")                                                 \
        return CutFace::Triangle;                                           \
    if (face == "UserDefined")                                              \
        return CutFace::UserDefined;                                        \
    return CutFace::Quad;                                                   \
}
std::string CutFaceToString(CutFace cutFace);
#define IMPL_CutFaceToString                                                \
std::string CutFaceToString(CutFace cutFace)                                    \
{                                                                           \
    switch (cutFace) {                                                      \
        case CutFace::Quad:                                                 \
            return "Quad";                                                  \
        case CutFace::Pentagon:                                             \
            return "Pentagon";                                              \
        case CutFace::Hexagon:                                              \
            return "Hexagon";                                               \
        case CutFace::Triangle:                                             \
            return "Triangle";                                              \
        case CutFace::UserDefined:                                          \
            return "UserDefined";                                           \
        default:                                                            \
            return "";                                                      \
    }                                                                       \
}
std::vector<Vector2> CutFaceToPoints(CutFace cutFace);
#define TMPL_CutFaceToPoints                                                \
std::vector<Vector2> CutFaceToPoints(CutFace cutFace)                     \
{                                                                           \
    switch (cutFace) {                                                      \
        case CutFace::Quad:                                                 \
            return {                                                        \
                {(float)-1.0, (float)-1.0},                                 \
                { (float)1.0, (float)-1.0},                                 \
                { (float)1.0,  (float)1.0},                                 \
                {(float)-1.0,  (float)1.0},                                 \
            };                                                              \
        case CutFace::Triangle:                                             \
            return {                                                        \
                {(float)-1.1527, (float)-0.6655},                           \
                { (float)1.1527, (float)-0.6655},                           \
                {    (float)0.0,  (float)1.33447},                          \
            };                                                              \
        case CutFace::Pentagon:                                             \
            return {                                                        \
                { (float)-0.6498, (float)-0.8944},                          \
                {  (float)0.6498, (float)-0.8944},                          \
                { (float)1.05146,  (float)0.34164},                         \
                {     (float)0.0,  (float)1.10557},                         \
                {(float)-1.05146,  (float)0.34164},                         \
            };                                                              \
        case CutFace::Hexagon:                                              \
            return {                                                        \
                { (float)-0.577, (float)-1.0},                              \
                {  (float)0.577, (float)-1.0},                              \
                { (float)1.1547,  (float)0.0},                              \
                {  (float)0.577,  (float)1.0},                              \
                { (float)-0.577,  (float)1.0},                              \
                {(float)-1.1547,  (float)0.0},                              \
            };                                                              \
        default:                                                            \
            return {                                                        \
                {(float)-1.0, (float)-1.0},                                 \
                { (float)1.0, (float)-1.0},                                 \
                { (float)1.0,  (float)1.0},                                 \
                {(float)-1.0,  (float)1.0},                                 \
            };                                                              \
    }                                                                       \
}

void normalizeCutFacePoints(std::vector<Vector2> *points);
void cutFacePointsFromNodes(std::vector<Vector2> &points, const std::vector<std::tuple<float, float, float, std::string>> &nodes, bool isRing=false,
    std::vector<std::string> *pointsIds=nullptr);

}

#endif
