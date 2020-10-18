/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright (C) 2013-2020 Mattia Basaglia
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include "QtColorWidgets/color_delegate.hpp"
#include "QtColorWidgets/color_selector.hpp"
#include "QtColorWidgets/color_dialog.hpp"
#include <QPainter>
#include <QMouseEvent>

namespace color_widgets {

ColorDelegate::ColorDelegate(QWidget *parent) :
    QAbstractItemDelegate(parent)
{
}

void ColorDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                           const QModelIndex &index) const
{
    if (index.data().canConvert<QColor>())
    {
        QStyleOptionFrame panel;
        panel.initFrom(option.widget);
        if (option.widget->isEnabled())
            panel.state = QStyle::State_Enabled;
        panel.rect = option.rect;
        panel.lineWidth = 2;
        panel.midLineWidth = 0;
        panel.state |= QStyle::State_Sunken;
        option.widget->style()->drawPrimitive(QStyle::PE_Frame, &panel, painter, nullptr);
        QRect r = option.widget->style()->subElementRect(QStyle::SE_FrameContents, &panel, nullptr);
        painter->setClipRect(r);
        painter->fillRect(option.rect, index.data().value<QColor>());
    }
}

bool ColorDelegate::editorEvent(QEvent* event,
                                QAbstractItemModel* model,
                                const QStyleOptionViewItem& option,
                                const QModelIndex& index)
{

    if ( event->type() == QEvent::MouseButtonRelease && index.data().canConvert<QColor>())
    {
        QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);

        if ( mouse_event->button() == Qt::LeftButton &&
            ( index.flags() & Qt::ItemIsEditable) )
        {
            ColorDialog *editor = new ColorDialog(const_cast<QWidget*>(option.widget));
            connect(this, &QObject::destroyed, editor, &QObject::deleteLater);
            editor->setMinimumSize(editor->sizeHint());
            auto original_color = index.data().value<QColor>();
            editor->setColor(original_color);
            auto set_color = [model, index](const QColor& color){
                model->setData(index, QVariant(color));
            };
            connect(editor, &ColorDialog::colorSelected, this, set_color);
            editor->show();
        }

        return true;
    }

    return QAbstractItemDelegate::editorEvent(event, model, option, index);
}


QSize ColorDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index)
    Q_UNUSED(option)
    return QSize(24,16);
}

} // namespace color_widgets
