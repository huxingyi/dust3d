#include <QDebug>
#include <QPalette>
#include <QFont>
#include <QGridLayout>
#include "theme.h"
#include "statusbarlabel.h"

StatusBarLabel::StatusBarLabel(QWidget *parent) :
    QWidget(parent)
{
    setAttribute(Qt::WA_Hover);
    
    setAutoFillBackground(true);
    
    QFont font;
    font.setWeight(QFont::Light);
    font.setBold(false);
    setFont(font);
    
    m_label = new QLabel;
    
    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(15, 3, 15, 5);
    mainLayout->addWidget(m_label);
    
    setLayout(mainLayout);
    
    updateBackground();
}

void StatusBarLabel::setSelected(bool selected)
{
    if (m_selected == selected)
        return;
    
    m_selected = selected;
    updateBackground();
}

void StatusBarLabel::updateText(const QString &text)
{
    m_label->setText(text);
}

bool StatusBarLabel::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::HoverEnter: {
            hoverEnter(static_cast<QHoverEvent*>(event));
            return true;
        }
        break;
    case QEvent::HoverLeave: {
            hoverLeave(static_cast<QHoverEvent*>(event));
            return true;
        }
        break;
    case QEvent::HoverMove: {
            hoverMove(static_cast<QHoverEvent*>(event));
            return true;
        }
        break;
    default:
        break;
    }
    return QWidget::event(event);
}

void StatusBarLabel::mousePressEvent(QMouseEvent *event)
{
    if (m_selected)
        return;
    
    emit clicked();
}

void StatusBarLabel::enterEvent(QEvent *event)
{
}

void StatusBarLabel::leaveEvent(QEvent *event)
{
}


void StatusBarLabel::hoverEnter(QHoverEvent *event)
{
    m_hovered = true;
    updateBackground();
}

void StatusBarLabel::hoverLeave(QHoverEvent *event)
{
    m_hovered = false;
    updateBackground();
}

void StatusBarLabel::hoverMove(QHoverEvent *event)
{
}

void StatusBarLabel::updateBackground()
{
    if (m_selected) {
        setPalette(Theme::statusBarActivePalette);
    } else if (m_hovered) {
        setPalette(Theme::statusBarHoverPalette);
    } else {
        setPalette(Theme::statusBarNormalPalette);
    }
}

