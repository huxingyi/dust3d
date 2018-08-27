#ifndef THEME_H
#define THEME_H
#include <QColor>
#include <QString>
#include <map>
#include <QPushButton>
#include <QLabel>
#include "QtAwesome.h"

class Theme
{
public:
    static QColor red;
    static QColor green;
    static QColor blue;
    static QColor white;
    static QColor black;
    static QColor dark;
    static QColor altDark;
    static QColor broken;
    static float normalAlpha;
    static float checkedAlpha;
    static float branchAlpha;
    static float fillAlpha;
    static float edgeAlpha;
    static int skeletonNodeBorderSize;
    static int skeletonEdgeWidth;
    static QString tabButtonSelectedStylesheet;
    static QString tabButtonStylesheet;
    static std::map<QString, QString> nextSideColorNameMap;
    static std::map<QString, QColor> sideColorNameToColorMap;
    static QtAwesome *awesome();
    static int toolIconFontSize;
    static int toolIconSize;
    static int previewImageRenderSize;
    static int previewImageSize;
    static int miniIconFontSize;
    static int miniIconSize;
public:
    static void initAwesomeButton(QPushButton *button);
    static void initAwesomeLabel(QLabel *label);
    static void initAwesomeSmallButton(QPushButton *button);
};

#endif
