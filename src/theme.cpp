#include "theme.h"

// 0xfc, 0x66, 0x21
// 252, 102, 33
// 0.99, 0.4, 0.13

QColor Theme::skeletonMasterNodeBorderColor = QColor(0xfc, 0x66, 0x21, 128);
QColor Theme::skeletonMasterNodeBorderHighlightColor = QColor(0xfc, 0x66, 0x21);
QColor Theme::skeletonMasterNodeFillColor = QColor(0xfc, 0x66, 0x21, 50);
int Theme::skeletonMasterNodeBorderSize = 7;
QColor Theme::skeletonSlaveNodeBorderColor = QColor(0xcc, 0xcc, 0xcc, 64);
QColor Theme::skeletonSlaveNodeBorderHighlightColor = QColor(0xcc, 0xcc, 0xcc);
QColor Theme::skeletonSlaveNodeFillColor = QColor(0xcc, 0xcc, 0xcc, 25);
int Theme::skeletonSlaveNodeBorderSize = 7;
QString Theme::tabButtonSelectedStylesheet = "QPushButton { color: #efefef; background-color: #fc6621; border: 0px; padding-top: 2px; padding-bottom: 2px; padding-left: 25px; padding-right: 25px;}";
QString Theme::tabButtonStylesheet = "QPushButton { color: #efefef; background-color: #353535; border: 0px; padding-top: 2px; padding-bottom: 2px; padding-left: 25px; padding-right: 25px;}";

