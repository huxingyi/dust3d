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
QColor Theme::white = QColor(0xf7, 0xd9, 0xc8);
QColor Theme::black = QColor(0x25,0x25,0x25);
QColor Theme::dark = QColor(0x19,0x19,0x19);
QColor Theme::altDark = QColor(0x16,0x16,0x16);
QColor Theme::broken = QColor(0xff,0xff,0xff);
float Theme::normalAlpha = 96.0 / 255;
float Theme::branchAlpha = 64.0 / 255;
float Theme::checkedAlpha = 1.0;
float Theme::edgeAlpha = 1.0;
float Theme::fillAlpha = 50.0 / 255;
int Theme::skeletonNodeBorderSize = 0;
int Theme::skeletonEdgeWidth = 0;
int Theme::toolIconFontSize = 16;
int Theme::toolIconSize = 24;
int Theme::miniIconFontSize = 9;
int Theme::miniIconSize = 15;
int Theme::previewImageSize = (Theme::miniIconSize * 3);
int Theme::previewImageRenderSize = 75;

QtAwesome *Theme::awesome()
{
    static QtAwesome *s_awesome = nullptr;
    if (nullptr == s_awesome) {
        s_awesome = new QtAwesome();
        s_awesome->initFontAwesome();
        s_awesome->setDefaultOption("color", Theme::white);
        s_awesome->setDefaultOption("color-disabled", QColor(0xcc, 0xcc, 0xcc));
        s_awesome->setDefaultOption("color-active", Theme::white);
        s_awesome->setDefaultOption("color-selected", Theme::white);
    }
    return s_awesome;
}

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

void Theme::initAwesomeButton(QPushButton *button)
{
    button->setFont(Theme::awesome()->font(Theme::toolIconFontSize));
    button->setFixedSize(Theme::toolIconSize, Theme::toolIconSize);
    button->setStyleSheet("QPushButton {color: #f7d9c8}");
    button->setFocusPolicy(Qt::NoFocus);
}

void Theme::initAwesomeSmallButton(QPushButton *button)
{
    button->setFont(Theme::awesome()->font(Theme::toolIconFontSize * 0.7));
    button->setFixedSize(Theme::toolIconSize * 0.75, Theme::toolIconSize * 0.75);
    button->setStyleSheet("QPushButton {color: #f7d9c8}");
    button->setFocusPolicy(Qt::NoFocus);
}

void Theme::initAwesomeLabel(QLabel *label)
{
    label->setFont(Theme::awesome()->font(Theme::toolIconFontSize));
    label->setStyleSheet("QLabel {color: #f7d9c8}");
}


