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

    auto createButton = [](QChar icon) {
        QPushButton *button = new QPushButton(icon);
        button->setFixedSize(Theme::toolIconSize, Theme::toolIconSize);
        return button;
    };

    QPushButton *lockButton = createButton(QChar(fa::lock));
    QPushButton *unlockButton = createButton(QChar(fa::unlock));
    QPushButton *showButton = createButton(QChar(fa::eye));
    QPushButton *hideButton = createButton(QChar(fa::eyeslash));

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
