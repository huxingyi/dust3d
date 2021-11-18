#include "spinnable_toolbar_icon.h"
#include "theme.h"

SpinnableToolbarIcon::SpinnableToolbarIcon(QWidget *parent) :
    QWidget(parent)
{
    setFixedSize(Theme::toolIconSize, Theme::toolIconSize);

    m_spinner = new WaitingSpinnerWidget(this);
    m_spinner->setColor(Theme::white);
    m_spinner->setInnerRadius(Theme::toolIconSize / 8);
    m_spinner->setLineLength(Theme::toolIconSize / 4);
    m_spinner->setNumberOfLines(9);
    m_spinner->hide();
}

void SpinnableToolbarIcon::showSpinner(bool showSpinner)
{
    if (showSpinner) {
        m_spinner->start();
        m_spinner->show();
    } else {
        m_spinner->stop();
        m_spinner->hide();
    }
}

bool SpinnableToolbarIcon::isSpinning()
{
    return m_spinner->isVisible();
}
