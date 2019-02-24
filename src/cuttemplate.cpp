#include <QStringList>
#include "cuttemplate.h"

IMPL_CutTemplateToDispName
TMPL_CutTemplateToPoints

static const auto g_defaultCutTemplate = CutTemplateToPoints(CutTemplate::Quad);

bool cutTemplatePointsCompare(const std::vector<QVector2D> &first, const std::vector<QVector2D> &second)
{
    if (first.size() != second.size())
        return false;
    for (size_t i = 0; i < first.size(); ++i) {
        if (!qFuzzyCompare(first[i], second[i]))
            return false;
    }
    return true;
}

QString cutTemplatePointsToString(const std::vector<QVector2D> &points)
{
    QStringList items;
    for (const auto &point: points) {
        items.append(QString::number(point.x()));
        items.append(QString::number(point.y()));
    }
    return items.join(",");
}

std::vector<QVector2D> cutTemplatePointsFromString(const QString &pointsString)
{
    std::vector<float> numbers;
    for (const auto &item: pointsString.split(",")) {
        numbers.push_back(item.toFloat());
    }
    if (0 != numbers.size() % 2)
        return g_defaultCutTemplate;
    size_t pointsNum = numbers.size() / 2;
    if (pointsNum < 3)
        return g_defaultCutTemplate;
    size_t numberIndex = 0;
    std::vector<QVector2D> points;
    for (size_t i = 0; i < pointsNum; ++i) {
        const auto &x = numbers[numberIndex++];
        const auto &y = numbers[numberIndex++];
        points.push_back({x, y});
    }
    return points;
}

void normalizeCutTemplatePoints(std::vector<QVector2D> *points)
{
    QVector2D center;
    if (nullptr == points || points->empty())
        return;
    float xLow = std::numeric_limits<float>::max();
    float xHigh = std::numeric_limits<float>::lowest();
    float yLow = std::numeric_limits<float>::max();
    float yHigh = std::numeric_limits<float>::lowest();
    for (const auto &position: *points) {
        if (position.x() < xLow)
            xLow = position.x();
        else if (position.x() > xHigh)
            xHigh = position.x();
        if (position.y() < yLow)
            yLow = position.y();
        else if (position.y() > yHigh)
            yHigh = position.y();
    }
    float xMiddle = (xHigh + xLow) * 0.5;
    float yMiddle = (yHigh + yLow) * 0.5;
    float xSize = xHigh - xLow;
    float ySize = yHigh - yLow;
    float longSize = ySize;
    if (xSize > longSize)
        longSize = xSize;
    if (qFuzzyIsNull(longSize))
        longSize = 0.000001;
    for (auto &position: *points) {
        position.setX((position.x() - xMiddle) * 2 / longSize);
        position.setY((position.y() - yMiddle) * 2 / longSize);
    }
}
