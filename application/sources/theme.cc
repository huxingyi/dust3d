#include "theme.h"
#include <QApplication>
#include <QFont>
#include <QFontMetrics>
#include <QPalette>
#include <QStyleFactory>

QColor Theme::white = Qt::white; //QColor(0xf7, 0xd9, 0xc8);
QColor Theme::red = QColor(0xfc, 0x66, 0x21);
QColor Theme::green = QColor(0xaa, 0xeb, 0xc4);
QColor Theme::blue = QColor(0x0d, 0xa9, 0xf1);
QColor Theme::black = QColor(25, 25, 25);
QColor Theme::gray = QColor(0x32, 0x32, 0x32);
QColor Theme::darkBackground = QColor(0x25, 0x25, 0x25);
QColor Theme::altDarkBackground = QColor(53, 53, 53);
QColor Theme::separator = QColor(0x4a, 0x4a, 0x4a);
QColor Theme::link = QColor(42, 130, 218);
QColor Theme::disabledIcon = QColor(0xcc, 0xcc, 0xcc);
QColor Theme::buttonDimmed = QColor(0x52, 0x52, 0x52);
QColor Theme::buttonDisabled = QColor(42, 42, 42);
QColor Theme::midGray = QColor(128, 128, 128);
float Theme::normalAlpha = 96.0 / 255;
float Theme::checkedAlpha = 1.0;
float Theme::edgeAlpha = 1.0;
float Theme::fillAlpha = 50.0 / 255;
int Theme::toolIconFontSize = 0;
int Theme::toolIconSize = 0;
int Theme::miniIconFontSize = 0;
int Theme::miniIconSize = 0;
int Theme::partPreviewImageSize = 0;
int Theme::sidebarPreferredWidth = 0;
int Theme::previewIconBorderSize = 0;
int Theme::previewIconMargin = 0;
int Theme::previewIconBorderRadius = 0;

void Theme::initialize()
{
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, Theme::darkBackground);
    darkPalette.setColor(QPalette::WindowText, Theme::white);
    darkPalette.setColor(QPalette::Base, Theme::black);
    darkPalette.setColor(QPalette::AlternateBase, Theme::altDarkBackground);
    darkPalette.setColor(QPalette::Text, Theme::white);
    darkPalette.setColor(QPalette::Disabled, QPalette::Text, Theme::darkBackground);
    darkPalette.setColor(QPalette::Button, Theme::altDarkBackground);
    darkPalette.setColor(QPalette::ButtonText, Theme::white);
    darkPalette.setColor(QPalette::BrightText, Theme::red);
    darkPalette.setColor(QPalette::Link, Theme::link);
    darkPalette.setColor(QPalette::Highlight, Theme::red);
    darkPalette.setColor(QPalette::HighlightedText, Theme::darkBackground);
    QApplication::setPalette(darkPalette);

    QFont font;
    font.setWeight(QFont::Light);
    font.setBold(false);
    font.setPixelSize(11);
    QApplication::setFont(font);

    QFontMetrics fontMetrics(QApplication::font());
    Theme::toolIconFontSize = fontMetrics.height();
    Theme::toolIconSize = (int)(Theme::toolIconFontSize * 1.5);
    Theme::miniIconFontSize = (int)(Theme::toolIconFontSize * 0.85);
    Theme::miniIconSize = (int)(Theme::miniIconFontSize * 1.5);
    Theme::partPreviewImageSize = (Theme::miniIconSize * 2.3);
    Theme::sidebarPreferredWidth = Theme::partPreviewImageSize * 4.5;
    Theme::previewIconBorderSize = std::max(1, Theme::partPreviewImageSize / 20);
    Theme::previewIconMargin = std::max(1, Theme::previewIconBorderSize / 2);
    Theme::previewIconBorderRadius = std::max(3, Theme::partPreviewImageSize / 10);

    int controlHeight = Theme::toolIconSize;

    // Minimal style: all controls share the same compact height
    QString minimalStyle = QString(
        "QPushButton { padding: 0px 4px; max-height: %1px; }"
        "QPushButton:disabled { color: %2; }"
        "QComboBox { padding: 0px 4px; max-height: %1px; }"
        "QCheckBox { spacing: 4px; }"
        "QLineEdit { padding: 0px 4px; max-height: %1px; }"
        "QProgressBar { max-height: 4px; }"
        "QLabel { padding: 0px; }"
        "QSpinBox { padding: 0px 4px; max-height: %1px; }"
        "QDoubleSpinBox { padding: 0px 4px; max-height: %1px; }"
        "QFrame#separatorLine { background-color: %3; min-height: 1px; max-height: 1px; border: none; }")
                               .arg(controlHeight)
                               .arg(Theme::darkBackground.name())
                               .arg(Theme::separator.name());
    qApp->setStyleSheet(minimalStyle);

    Theme::awesome();
}

QtAwesome* Theme::awesome()
{
    static QtAwesome* s_awesome = nullptr;
    if (nullptr == s_awesome) {
        s_awesome = new QtAwesome();
        s_awesome->initFontAwesome();
        s_awesome->setDefaultOption("color", Theme::white);
        s_awesome->setDefaultOption("color-disabled", Theme::disabledIcon);
        s_awesome->setDefaultOption("color-active", Theme::white);
        s_awesome->setDefaultOption("color-selected", Theme::white);
    }
    return s_awesome;
}

void Theme::initAwesomeButton(QPushButton* button)
{
    button->setFont(Theme::awesome()->font(Theme::toolIconFontSize));
    button->setFixedSize(Theme::toolIconSize, Theme::toolIconSize);
    button->setFocusPolicy(Qt::NoFocus);
}

void Theme::initAwesomeLabel(QLabel* label)
{
    label->setFont(Theme::awesome()->font(Theme::toolIconFontSize));
    label->setStyleSheet("QLabel {color: " + Theme::white.name() + "}");
}

void Theme::initAwesomeMiniButton(QPushButton* button)
{
    button->setFont(Theme::awesome()->font(Theme::miniIconFontSize));
    button->setFixedSize(Theme::miniIconSize, Theme::miniIconSize);
    button->setFocusPolicy(Qt::NoFocus);
}

void Theme::updateAwesomeMiniButton(QPushButton* button, QChar icon, bool highlighted, bool enabled, bool unnormal)
{
    button->setText(icon);
    QColor color;
    bool needDesaturation = true;

    if (highlighted) {
        if (unnormal) {
            color = Theme::blue;
            needDesaturation = false;
        } else {
            color = Theme::white;
        }
    } else {
        color = Theme::buttonDimmed;
    }

    if (needDesaturation) {
        color = color.toHsv();
        color.setHsv(color.hue(), color.saturation() / 5, color.value() * 2 / 3);
        color = color.toRgb();
    }

    if (!enabled) {
        color = Theme::buttonDisabled;
    }

    button->setStyleSheet("QPushButton {border: none; background: none; color: " + color.name() + ";}");
}

void Theme::initAwesomeToolButtonWithoutFont(QPushButton* button)
{
    button->setFixedSize(Theme::miniIconSize, Theme::miniIconSize);
    button->setFocusPolicy(Qt::NoFocus);
}

void Theme::initAwesomeToolButton(QPushButton* button)
{
    button->setFont(Theme::awesome()->font(Theme::miniIconFontSize));
    Theme::initAwesomeToolButtonWithoutFont(button);
}

void Theme::initToolButton(QPushButton* button)
{
    QFont font = button->font();
    font.setWeight(QFont::Light);
    font.setBold(false);
    button->setFont(font);
    button->setFixedHeight(Theme::toolIconSize);
    button->setFocusPolicy(Qt::NoFocus);
}

void Theme::initCheckbox(QCheckBox* checkbox)
{
    QPalette palette = checkbox->palette();
#if QT_VERSION < 0x060000
    palette.setColor(QPalette::Background, Theme::white);
#endif
    checkbox->setPalette(palette);
}

void Theme::initIconButton(QPushButton* button)
{
    button->setFixedSize(Theme::toolIconSize, Theme::toolIconSize);
}

void Theme::initLineEdit(QLineEdit* edit)
{
    edit->setStyleSheet("QLineEdit { background-color: " + Theme::black.name() + " }");
}
