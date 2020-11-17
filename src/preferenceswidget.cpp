#include <QFormLayout>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QColorDialog>
#include "preferenceswidget.h"
#include "util.h"
#include "preferences.h"
#include "theme.h"

PreferencesWidget::PreferencesWidget(const Document *document, QWidget *parent) :
    QDialog(parent),
    m_document(document)
{
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    
    QPushButton *colorEraser = new QPushButton(QChar(fa::eraser));
    Theme::initAwesomeToolButton(colorEraser);
    
    QPushButton *pickButton = new QPushButton();
    Theme::initAwesomeToolButtonWithoutFont(pickButton);
    
    QHBoxLayout *colorLayout = new QHBoxLayout;
    colorLayout->addWidget(colorEraser);
    colorLayout->addWidget(pickButton);
    colorLayout->addStretch();
    
    auto updatePickButtonColor = [=]() {
        QPalette palette = pickButton->palette();
        QColor choosenColor = Preferences::instance().partColor();
        palette.setColor(QPalette::Window, choosenColor);
        palette.setColor(QPalette::Button, choosenColor);
        pickButton->setPalette(palette);
        pickButton->update();
    };
    
    connect(colorEraser, &QPushButton::clicked, [=]() {
        Preferences::instance().setPartColor(Qt::white);
        updatePickButtonColor();
    });
    
    connect(pickButton, &QPushButton::clicked, [=]() {
        emit disableBackgroundBlur();
        QColor color = QColorDialog::getColor(Preferences::instance().partColor(), this);
        if (color.isValid()) {
            Preferences::instance().setPartColor(color);
            updatePickButtonColor();
            raise();
        }
        emit enableBackgroundBlur();
    });
    
    QComboBox *combineModeSelectBox = new QComboBox;
    for (size_t i = 0; i < (size_t)CombineMode::Count; ++i) {
        CombineMode mode = (CombineMode)i;
        combineModeSelectBox->addItem(CombineModeToDispName(mode));
    }
    connect(combineModeSelectBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [=](int index) {
        Preferences::instance().setComponentCombineMode((CombineMode)index);
    });
    
    QCheckBox *flatShadingBox = new QCheckBox();
    Theme::initCheckbox(flatShadingBox);
    connect(flatShadingBox, &QCheckBox::stateChanged, this, [=]() {
        Preferences::instance().setFlatShading(flatShadingBox->isChecked());
    });
    
    QCheckBox *toonShadingBox = new QCheckBox();
    Theme::initCheckbox(toonShadingBox);
    connect(toonShadingBox, &QCheckBox::stateChanged, this, [=]() {
        Preferences::instance().setToonShading(toonShadingBox->isChecked());
    });
    
    QComboBox *toonLineSelectBox = new QComboBox;
    for (size_t i = 0; i < (size_t)ToonLine::Count; ++i) {
        ToonLine toonLine = (ToonLine)i;
        toonLineSelectBox->addItem(ToonLineToDispName(toonLine));
    }
    connect(toonLineSelectBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [=](int index) {
        Preferences::instance().setToonLine((ToonLine)index);
    });
    
    QHBoxLayout *toonShadingLayout = new QHBoxLayout;
    toonShadingLayout->addWidget(toonShadingBox);
    toonShadingLayout->addWidget(toonLineSelectBox);
    toonShadingLayout->addStretch();
    
    QComboBox *textureSizeSelectBox = new QComboBox;
    textureSizeSelectBox->addItem("512");
    textureSizeSelectBox->addItem("1024");
    textureSizeSelectBox->addItem("2048");
    textureSizeSelectBox->addItem("4096");
    connect(textureSizeSelectBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [=](int index) {
        Preferences::instance().setTextureSize(textureSizeSelectBox->itemText(index).toInt());
    });
    
    QCheckBox *scriptEnabledBox = new QCheckBox();
    Theme::initCheckbox(scriptEnabledBox);
    connect(scriptEnabledBox, &QCheckBox::stateChanged, this, [=]() {
        Preferences::instance().setScriptEnabled(scriptEnabledBox->isChecked());
    });
    
    QFormLayout *formLayout = new QFormLayout;
    formLayout->addRow(tr("Part color:"), colorLayout);
    formLayout->addRow(tr("Combine mode:"), combineModeSelectBox);
    formLayout->addRow(tr("Flat shading:"), flatShadingBox);
    formLayout->addRow(tr("Toon shading:"), toonShadingLayout);
    formLayout->addRow(tr("Texture size:"), textureSizeSelectBox);
    formLayout->addRow(tr("Script:"), scriptEnabledBox);
    
    auto loadFromPreferences = [=]() {
        updatePickButtonColor();
        combineModeSelectBox->setCurrentIndex((int)Preferences::instance().componentCombineMode());
        flatShadingBox->setChecked(Preferences::instance().flatShading());
        toonLineSelectBox->setCurrentIndex((int)Preferences::instance().toonLine());
        toonShadingBox->setChecked(Preferences::instance().toonShading());
        textureSizeSelectBox->setCurrentIndex(
            textureSizeSelectBox->findText(QString::number(Preferences::instance().textureSize()))
        );
        scriptEnabledBox->setChecked(Preferences::instance().scriptEnabled());
    };
    
    loadFromPreferences();
    
    QPushButton *resetButton = new QPushButton(tr("Reset"));
    connect(resetButton, &QPushButton::clicked, this, [=]() {
        Preferences::instance().reset();
        loadFromPreferences();
    });
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(formLayout);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(resetButton);
    
    setLayout(mainLayout);
    
    setWindowTitle(unifiedWindowTitle(tr("Preferences")));
}
