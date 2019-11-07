#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QDebug>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QColorDialog>
#include "scriptvariableswidget.h"
#include "util.h"
#include "theme.h"
#include "floatnumberwidget.h"
#include "intnumberwidget.h"

ScriptVariablesWidget::ScriptVariablesWidget(const Document *document,
        QWidget *parent) :
    QScrollArea(parent),
    m_document(document)
{
    connect(this, &ScriptVariablesWidget::updateVariableValue, m_document, &Document::updateVariableValue);

    setWidgetResizable(true);
    
    reload();
}

void ScriptVariablesWidget::reload()
{
    QWidget *widget = new QWidget;
    QFormLayout *formLayout = new QFormLayout;
    for (const auto &variable: m_document->variables()) {
        auto name = variable.first;
        auto input = valueOfKeyInMapOrEmpty(variable.second, "input");
        if ("Float" == input) {
            auto minValue = valueOfKeyInMapOrEmpty(variable.second, "minValue").toFloat();
            auto maxValue = valueOfKeyInMapOrEmpty(variable.second, "maxValue").toFloat();
            auto value = valueOfKeyInMapOrEmpty(variable.second, "value").toFloat();
            auto defaultValue = valueOfKeyInMapOrEmpty(variable.second, "defaultValue").toFloat();
            FloatNumberWidget *floatWidget = new FloatNumberWidget;
            floatWidget->setItemName(name);
            floatWidget->setRange(minValue, maxValue);
            floatWidget->setValue(value);
            
            connect(floatWidget, &FloatNumberWidget::valueChanged, [=](float toValue) {
                emit updateVariableValue(name, QString::number(toValue));
            });
            
            QPushButton *floatEraser = new QPushButton(QChar(fa::eraser));
            Theme::initAwesomeToolButton(floatEraser);
            
            connect(floatEraser, &QPushButton::clicked, [=]() {
                floatWidget->setValue(defaultValue);
            });
            
            QHBoxLayout *floatLayout = new QHBoxLayout;
            floatLayout->addWidget(floatEraser);
            floatLayout->addWidget(floatWidget);
            
            formLayout->addRow(floatLayout);
        } else if ("Int" == input) {
            auto minValue = valueOfKeyInMapOrEmpty(variable.second, "minValue").toInt();
            auto maxValue = valueOfKeyInMapOrEmpty(variable.second, "maxValue").toInt();
            auto value = valueOfKeyInMapOrEmpty(variable.second, "value").toInt();
            auto defaultValue = valueOfKeyInMapOrEmpty(variable.second, "defaultValue").toInt();
            IntNumberWidget *intWidget = new IntNumberWidget;
            intWidget->setItemName(name);
            intWidget->setRange(minValue, maxValue);
            intWidget->setValue(value);
            
            connect(intWidget, &IntNumberWidget::valueChanged, [=](int toValue) {
                emit updateVariableValue(name, QString::number(toValue));
            });
            
            QPushButton *intEraser = new QPushButton(QChar(fa::eraser));
            Theme::initAwesomeToolButton(intEraser);
            
            connect(intEraser, &QPushButton::clicked, [=]() {
                intWidget->setValue(defaultValue);
            });
            
            QHBoxLayout *intLayout = new QHBoxLayout;
            intLayout->addWidget(intEraser);
            intLayout->addWidget(intWidget);
            
            formLayout->addRow(intLayout);
        } else if ("Check" == input) {
            auto value = isTrueValueString(valueOfKeyInMapOrEmpty(variable.second, "value"));
            auto defaultValue = isTrueValueString(valueOfKeyInMapOrEmpty(variable.second, "defaultValue"));
            
            QCheckBox *checkBox = new QCheckBox;
            Theme::initCheckbox(checkBox);
            checkBox->setText(name);
            checkBox->setChecked(value);
            
            connect(checkBox, &QCheckBox::stateChanged, [=](int) {
                emit updateVariableValue(name, checkBox->isChecked() ? "true" : "false");
            });
            
            QPushButton *checkEraser = new QPushButton(QChar(fa::eraser));
            Theme::initAwesomeToolButton(checkEraser);
            
            connect(checkEraser, &QPushButton::clicked, [=]() {
                checkBox->setChecked(defaultValue);
            });
            
            QHBoxLayout *checkLayout = new QHBoxLayout;
            checkLayout->addWidget(checkEraser);
            checkLayout->addWidget(checkBox);
            
            formLayout->addRow(checkLayout);
        } else if ("Select" == input) {
            auto value = valueOfKeyInMapOrEmpty(variable.second, "value").toInt();
            auto defaultValue = valueOfKeyInMapOrEmpty(variable.second, "defaultValue").toInt();
            auto length = valueOfKeyInMapOrEmpty(variable.second, "length").toInt();
            
            QComboBox *selectBox = new QComboBox;
            selectBox->setEditable(false);
            selectBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
            for (auto i = 0; i < length; ++i) {
                selectBox->addItem(valueOfKeyInMapOrEmpty(variable.second, "option" + QString::number(i)));
            }
            selectBox->setCurrentIndex(value);
            
            connect(selectBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [=](int index) {
                emit updateVariableValue(name, QString::number(index));
            });
            
            QPushButton *selectEraser = new QPushButton(QChar(fa::eraser));
            Theme::initAwesomeToolButton(selectEraser);
            
            connect(selectEraser, &QPushButton::clicked, [=]() {
                selectBox->setCurrentIndex(defaultValue);
            });
            
            QLabel *selectLabel = new QLabel;
            selectLabel->setText(name);
            
            QHBoxLayout *selectLayout = new QHBoxLayout;
            selectLayout->addWidget(selectEraser);
            selectLayout->addWidget(selectBox);
            selectLayout->addWidget(selectLabel);
            
            formLayout->addRow(selectLayout);
        } else if ("Color" == input) {
            auto value = valueOfKeyInMapOrEmpty(variable.second, "value");
            auto defaultValue = valueOfKeyInMapOrEmpty(variable.second, "defaultValue");
            
            QPushButton *colorEraser = new QPushButton(QChar(fa::eraser));
            Theme::initAwesomeToolButton(colorEraser);
            
            QPushButton *pickButton = new QPushButton();
            Theme::initAwesomeToolButtonWithoutFont(pickButton);
            
            auto updatePickButtonColor = [=](const QColor &color) {
                QPalette palette = pickButton->palette();
                palette.setColor(QPalette::Window, color);
                palette.setColor(QPalette::Button, color);
                pickButton->setPalette(palette);
            };
            updatePickButtonColor(QColor(value));

            connect(colorEraser, &QPushButton::clicked, [=]() {
                updatePickButtonColor(defaultValue);
                emit updateVariableValue(name, defaultValue);
            });
            
            connect(pickButton, &QPushButton::clicked, [=]() {
                QColor color = QColorDialog::getColor(value, this);
                if (color.isValid()) {
                    updatePickButtonColor(color.name());
                    emit updateVariableValue(name, color.name());
                }
            });
            
            QLabel *selectLabel = new QLabel;
            selectLabel->setText(name);
            
            QHBoxLayout *colorLayout = new QHBoxLayout;
            colorLayout->addWidget(colorEraser);
            colorLayout->addWidget(pickButton);
            colorLayout->addWidget(selectLabel);
            colorLayout->addStretch();
            
            formLayout->addRow(colorLayout);
        }
    }
    widget->setLayout(formLayout);
    
    setWidget(widget);
}

QSize ScriptVariablesWidget::sizeHint() const
{
    return QSize(Theme::sidebarPreferredWidth, 0);
}
