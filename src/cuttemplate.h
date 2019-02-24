#ifndef CUT_TEMPLATE_H
#define CUT_TEMPLATE_H
#include <QObject>
#include <QString>
#include <QVector2D>
#include <vector>

enum class CutTemplate
{
    Quad = 0,
    //Octagon,
    Count
};

QString CutTemplateToDispName(CutTemplate cutTemplate);
#define IMPL_CutTemplateToDispName                                          \
QString CutTemplateToDispName(CutTemplate cutTemplate)                      \
{                                                                           \
    switch (cutTemplate) {                                                  \
        case CutTemplate::Quad:                                             \
            return QObject::tr("Quad");                                     \
        default:                                                            \
            return "";                                                      \
    }                                                                       \
}
std::vector<QVector2D> CutTemplateToPoints(CutTemplate cutTemplate);
#define TMPL_CutTemplateToPoints                                            \
std::vector<QVector2D> CutTemplateToPoints(CutTemplate cutTemplate)         \
{                                                                           \
    switch (cutTemplate) {                                                  \
        case CutTemplate::Quad:                                             \
            return {                                                        \
                {-1.0, -1.0},                                               \
                { 1.0, -1.0},                                               \
                { 1.0,  1.0},                                               \
                {-1.0,  1.0},                                               \
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

bool cutTemplatePointsCompare(const std::vector<QVector2D> &first, const std::vector<QVector2D> &second);
QString cutTemplatePointsToString(const std::vector<QVector2D> &points);
std::vector<QVector2D> cutTemplatePointsFromString(const QString &pointsString);
void normalizeCutTemplatePoints(std::vector<QVector2D> *points);

#endif
