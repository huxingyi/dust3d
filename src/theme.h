#ifndef THEME_H
#define THEME_H
#include <QColor>
#include <QString>
#include <map>

class Theme
{
public:
    static QColor red;
    static QColor green;
    static QColor blue;
    static float normalAlpha;
    static float checkedAlpha;
    static float branchAlpha;
    static float fillAlpha;
    static int skeletonNodeBorderSize;
    static int skeletonEdgeWidth;
    static QString tabButtonSelectedStylesheet;
    static QString tabButtonStylesheet;
    static std::map<QString, QString> nextSideColorNameMap;
    static std::map<QString, QColor> sideColorNameToColorMap;
};

#endif
