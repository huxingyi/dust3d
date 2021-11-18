#include <QHBoxLayout>
#include "info_label.h"
#include "theme.h"

InfoLabel::InfoLabel(const QString &text, QWidget *parent) :
    QWidget(parent)
{
    m_icon = new QLabel(QChar(fa::infocircle));
    Theme::initAwesomeLabel(m_icon);
    
    m_label = new QLabel(text);
    m_label->setWordWrap(true);
    
    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addWidget(m_icon);
    mainLayout->addWidget(m_label);
    mainLayout->addStretch();
    
    setLayout(mainLayout);
}

void InfoLabel::setText(const QString &text)
{
    m_label->setText(text);
}
