#include <QGridLayout>
#include "combineeditwidget.h"

CombineEditWidget::CombineEditWidget(QWidget *parent) :
    QFrame(parent)
{
    m_modelingWidget = new ModelingWidget(this);
    
    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(m_modelingWidget, 0, 0, 1, 1);
    
    setLayout(mainLayout);
}

ModelingWidget *CombineEditWidget::modelingWidget()
{
    return m_modelingWidget;
}

void CombineEditWidget::resizeEvent(QResizeEvent *event)
{
    QFrame::resizeEvent(event);
    emit sizeChanged();
}


