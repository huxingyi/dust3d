#ifndef DUST3D_THEME_H
#define DUST3D_THEME_H
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
    static QWidget *createHorizontalLineWidget();
    static QWidget *createVerticalLineWidget();
    static int toolIconFontSize;
    static int toolIconSize;
    static int materialPreviewImageSize;
    static int cutFacePreviewImageSize;
    static int posePreviewImageSize;
    static int partPreviewImageSize;
    static int motionPreviewImageSize;
    static int miniIconFontSize;
    static int miniIconSize;
    static int sidebarPreferredWidth;
    static int normalButtonSize;
public:
    static void initAwesomeButton(QPushButton *button);
    static void initAwesomeLabel(QLabel *label);
    static void initAwesomeSmallButton(QPushButton *button);
    static void initAwesomeMiniButton(QPushButton *button);
    static void updateAwesomeMiniButton(QPushButton *button, QChar icon, bool highlighted, bool unnormal=false);
    static void initAwesomeToolButton(QPushButton *button);
    static void initAwesomeToolButtonWithoutFont(QPushButton *button);
    static void initAwsomeBaseSizes();
    static void initToolButton(QPushButton *button);
};

#endif
