#include <QPushButton>
#include <QDebug>
#include "tabwidget.h"
#include "flowlayout.h"
#include "theme.h"

TabWidget::TabWidget(const std::vector<QString> &items, QWidget *parent) :
    QWidget(parent)
{
    FlowLayout *mainLayout = new FlowLayout;
    
    for (size_t i = 0; i < items.size(); ++i) {
        QPushButton *button = new QPushButton(items[i]);
        button->setAutoFillBackground(true);
        button->setFlat(true);
        connect(button, &QPushButton::clicked, this, [=]() {
            itemClicked(i);
        });
        m_buttons.push_back(button);
        mainLayout->addWidget(button);
        updateButtonLook(i, false);
    }
    
    setLayout(mainLayout);
}

void TabWidget::updateButtonLook(int index, bool checked)
{
    QPushButton *button = m_buttons[index];
    //button->setStyleSheet("QPushButton {background-color: black; color: " + (checked ? Theme::red.name() : Theme::white.name()) + ";}");
    button->setStyleSheet(checked ? Theme::tabButtonSelectedStylesheet : Theme::tabButtonStylesheet);
}

void TabWidget::setCurrentIndex(int index)
{
    if (m_currentIndex == index)
        return;
    if (index >= (int)m_buttons.size()) {
        qDebug() << "Try to set a invalid index:" << index << "total:" << m_buttons.size();
        return;
    }
    if (-1 != m_currentIndex)
        updateButtonLook(m_currentIndex, false);
    m_currentIndex = index;
    updateButtonLook(m_currentIndex, true);
    emit currentIndexChanged(m_currentIndex);
}

int TabWidget::currentIndex()
{
    return m_currentIndex;
}

void TabWidget::itemClicked(int index)
{
    setCurrentIndex(index);
}
