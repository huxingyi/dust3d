#include "horizontal_line_widget.h"
#include "theme.h"

HorizontalLineWidget::HorizontalLineWidget()
    : QWidget()
{
    setFixedHeight(1);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setStyleSheet(QString("background-color: ") + Theme::separator.name() + ";");
    setContentsMargins(0, 0, 0, 0);
}
