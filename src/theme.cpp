#include "theme.h"

// Red
// 0xfc, 0x66, 0x21
// 252, 102, 33
// 0.99, 0.4, 0.13

// Green
// 0xaa, 0xeb, 0xc4

// Blue
// 0x2a, 0x5a, 0xac

// White
// 0xf7, 0xd9, 0xc8

QColor Theme::red = QColor(0xfc, 0x66, 0x21);
QColor Theme::green = QColor(0xaa, 0xeb, 0xc4);
QColor Theme::blue = QColor(0x2a, 0x5a, 0xac);
float Theme::normalAlpha = 128.0 / 255;
float Theme::branchAlpha = 64.0 / 255;
float Theme::checkedAlpha = 1.0;
float Theme::fillAlpha = 50.0 / 255;
int Theme::skeletonNodeBorderSize = 1;
int Theme::skeletonEdgeWidth = 1;

std::map<QString, QString> createSideColorNameMap() {
    std::map<QString, QString> map;
    map["red"] = "green";
    map["green"] = "red";
    return map;
}

std::map<QString, QColor> createSideColorNameToColorMap() {
    std::map<QString, QColor> map;
    map["red"] = Theme::red;
    map["green"] = Theme::green;
    return map;
}

std::map<QString, QString> Theme::nextSideColorNameMap = createSideColorNameMap();
std::map<QString, QColor> Theme::sideColorNameToColorMap = createSideColorNameToColorMap();

QString Theme::tabButtonSelectedStylesheet = "QPushButton { color: #efefef; background-color: #fc6621; border: 0px; padding-top: 2px; padding-bottom: 2px; padding-left: 25px; padding-right: 25px;}";
QString Theme::tabButtonStylesheet = "QPushButton { color: #efefef; background-color: #353535; border: 0px; padding-top: 2px; padding-bottom: 2px; padding-left: 25px; padding-right: 25px;}";

