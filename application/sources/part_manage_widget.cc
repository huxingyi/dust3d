#include <QVBoxLayout>
#include "part_manage_widget.h"
#include "component_preview_grid_widget.h"
#include "theme.h"

PartManageWidget::PartManageWidget(const Document *document, QWidget *parent):
    QWidget(parent)
{
    QHBoxLayout *toolsLayout = new QHBoxLayout;
    toolsLayout->setSpacing(0);
    toolsLayout->setMargin(0);

    QPushButton *lockButton = new QPushButton(QChar(fa::lock));
    Theme::initAwesomeToolButton(lockButton);

    QPushButton *unlockButton = new QPushButton(QChar(fa::unlock));
    Theme::initAwesomeToolButton(unlockButton);

    QPushButton *showButton = new QPushButton(QChar(fa::eye));
    Theme::initAwesomeToolButton(showButton);

    QPushButton *hideButton = new QPushButton(QChar(fa::eyeslash));
    Theme::initAwesomeToolButton(hideButton);

    toolsLayout->addWidget(showButton);
    toolsLayout->addWidget(hideButton);
    toolsLayout->addWidget(unlockButton);
    toolsLayout->addWidget(lockButton);
    toolsLayout->addStretch();

    ComponentPreviewGridWidget *componentPreviewGridWidget = new ComponentPreviewGridWidget(document);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(toolsLayout);
    mainLayout->addWidget(componentPreviewGridWidget);

    setLayout(mainLayout);
}
