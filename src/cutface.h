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
    UserDefined,
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
    if (face == "UserDefined")                                              \
        return CutFace::UserDefined;                                        \
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
        case CutFace::UserDefined:                                          \
            return "UserDefined";                                           \
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

void normalizeCutFacePoints(std::vector<QVector2D> *points);
void cutFacePointsFromNodes(std::vector<QVector2D> &points, const std::vector<std::tuple<float, float, float>> &nodes);

#endif
