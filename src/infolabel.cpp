#include <QHBoxLayout>
#include "infolabel.h"
#include "QtAwesome.h"
#include "theme.h"

InfoLabel::InfoLabel(const QString &text, QWidget *parent) :
    QWidget(parent)
{
    m_icon = new QLabel(QChar(fa::infocircle));
    Theme::initAwesomeLabel(m_icon);
    
    m_label = new QLabel(text);
    
    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addWidget(m_icon);
    mainLayout->addWidget(m_label);
    
    setLayout(mainLayout);
}

void InfoLabel::setText(const QString &text)
{
    m_label->setText(text);
}
