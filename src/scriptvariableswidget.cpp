#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QDebug>
#include "scriptvariableswidget.h"
#include "util.h"
#include "theme.h"

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
        auto value = valueOfKeyInMapOrEmpty(variable.second, "value").toFloat();
        qDebug() << "Script variable, name:" << name << "value:" << value;
        QDoubleSpinBox *inputBox = new QDoubleSpinBox;
        inputBox->setValue(value);
        connect(inputBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, [=](double toValue) {
            emit updateVariableValue(name, QString::number(toValue));
        });
        formLayout->addRow(name, inputBox);
    }
    widget->setLayout(formLayout);
    
    setWidget(widget);
}

QSize ScriptVariablesWidget::sizeHint() const
{
    return QSize(Theme::sidebarPreferredWidth, 0);
}
