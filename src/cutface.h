#ifndef CUT_FACE_H
#define CUT_FACE_H
#include <QObject>
#include <QString>
#include <QVector2D>
#include <vector>

enum class CutFace
{
    Quad = 0,
    Pentagon,
    Hexagon,
    Triangle,
    Count
};

CutFace CutFaceFromString(const char *faceString);
#define IMPL_CutFaceFromString                                              \
CutFace CutFaceFromString(const char *faceString)                           \
{                                                                           \
    QString face = faceString;                                              \
    if (face == "Quad")                                                     \
        return CutFace::Quad;                                               \
    if (face == "Pentagon")                                                 \
        return CutFace::Pentagon;                                           \
    if (face == "Hexagon")                                                  \
        return CutFace::Hexagon;                                            \
    if (face == "Triangle")                                                 \
        return CutFace::Triangle;                                           \
    return CutFace::Quad;                                                   \
}
QString CutFaceToString(CutFace cutFace);
#define IMPL_CutFaceToString                                                \
QString CutFaceToString(CutFace cutFace)                                    \
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
        default:                                                            \
            return "";                                                      \
    }                                                                       \
}
std::vector<QVector2D> CutFaceToPoints(CutFace cutFace);
#define TMPL_CutFaceToPoints                                                \
std::vector<QVector2D> CutFaceToPoints(CutFace cutFace)                     \
{                                                                           \
    switch (cutFace) {                                                      \
        case CutFace::Quad:                                                 \
            return {                                                        \
                {-1.0, -1.0},                                               \
                { 1.0, -1.0},                                               \
                { 1.0,  1.0},                                               \
                {-1.0,  1.0},                                               \
            };                                                              \
        case CutFace::Triangle:                                             \
            return {                                                        \
                {-1.1547, -1.0},                                            \
                { 1.1547, -1.0},                                            \
                {    0.0,  1.0},                                            \
            };                                                              \
        case CutFace::Pentagon:                                             \
            return {                                                        \
                { -0.6498, -0.8944},                                        \
                {  0.6498, -0.8944},                                        \
                { 1.05146,  0.34164},                                       \
                {     0.0,  1.10557},                                       \
                {-1.05146,  0.34164},                                       \
            };                                                              \
        case CutFace::Hexagon:                                              \
            return {                                                        \
                { -0.577, -1.0},                                            \
                {  0.577, -1.0},                                            \
                { 1.1547,  0.0},                                            \
                {  0.577,  1.0},                                            \
                { -0.577,  1.0},                                            \
                {-1.1547,  0.0},                                            \
            };                                                              \
        default:                                                            \
            return {                                                        \
                {-1.0, -1.0},                                               \
                { 1.0, -1.0},                                               \
                { 1.0,  1.0},                                               \
                {-1.0,  1.0},                                               \
            };                                                              \
    }                                                                       \
}

void normalizeCutFacePoints(std::vector<QVector2D> *points);

#endif
