#include "horizontal_line_widget.h"

HorizontalLineWidget::HorizontalLineWidget()
    : QWidget()
{
    setFixedHeight(1);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setStyleSheet(QString("background-color: #565656;"));
    setContentsMargins(0, 0, 0, 0);
}
