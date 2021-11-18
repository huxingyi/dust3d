#include <QSvgWidget>
#include <QGridLayout>
#include "toolbar_button.h"
#include "theme.h"

ToolbarButton::ToolbarButton(const QString &filename, QWidget *parent) :
    QPushButton(parent)
{
    auto margin = Theme::toolIconSize / 5;
    
    setFixedSize(Theme::toolIconSize, Theme::toolIconSize);
    setFocusPolicy(Qt::NoFocus);
    
    QSvgWidget *svgWidget = new QSvgWidget(filename);
    m_iconWidget = svgWidget;
    
    QGridLayout *containerLayout = new QGridLayout;
    containerLayout->setSpacing(0);
    containerLayout->setContentsMargins(margin, margin, margin, margin);
    containerLayout->addWidget(svgWidget);
    
    setLayout(containerLayout);
}

void ToolbarButton::setIcon(const QString &filename)
{
    m_iconWidget->load(filename);
}
