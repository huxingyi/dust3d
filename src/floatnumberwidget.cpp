#include <QtWidgets>
#include "floatnumberwidget.h"

FloatNumberWidget::FloatNumberWidget(QWidget *parent) :
    QWidget(parent)
{
    m_slider = new QSlider(Qt::Horizontal, this);
    m_slider->setRange(0, 100);
    m_slider->setFixedWidth(120);

    m_label = new QLabel(this);
    m_label->setAlignment(Qt::AlignLeft);

    connect(m_slider, &QAbstractSlider::valueChanged, [=](int value) {
        float fvalue = value / 100.0;
        updateValueLabel(fvalue);
        emit valueChanged(fvalue);
    });

    QBoxLayout *popupLayout = new QHBoxLayout(this);
    popupLayout->setMargin(2);
    popupLayout->addWidget(m_slider);
    popupLayout->addWidget(m_label);
}

void FloatNumberWidget::updateValueLabel(float value)
{
    QString valueString = QString().sprintf("%.2f", value);
    if (m_itemName.isEmpty())
        m_label->setText(valueString);
    else
        m_label->setText(m_itemName + ": " + valueString);
}

void FloatNumberWidget::setItemName(const QString &name)
{
    m_itemName = name;
    updateValueLabel(value());
}

void FloatNumberWidget::setRange(float min, float max)
{
    m_slider->setRange(min * 100, max * 100);
}

void FloatNumberWidget::increaseValue()
{
    m_slider->triggerAction(QSlider::SliderPageStepAdd);
}

void FloatNumberWidget::descreaseValue()
{
    m_slider->triggerAction(QSlider::SliderPageStepSub);
}

float FloatNumberWidget::value() const
{
    return m_slider->value() / 100.0;
}

void FloatNumberWidget::setValue(float value)
{
    m_slider->setValue(value * 100);
}
