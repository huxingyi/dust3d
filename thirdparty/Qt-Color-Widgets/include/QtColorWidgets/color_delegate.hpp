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
#ifndef COLOR_DELEGATE_HPP
#define COLOR_DELEGATE_HPP

#include "colorwidgets_global.hpp"

#include <QAbstractItemDelegate>

namespace color_widgets {

/**
    Delegate to use a ColorSelector in a color list
*/
class QCP_EXPORT ColorDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    explicit ColorDelegate(QWidget *parent = 0);

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option,
                    const QModelIndex &index) const Q_DECL_OVERRIDE;

    bool editorEvent(QEvent* event,
                     QAbstractItemModel* model,
                     const QStyleOptionViewItem & option,
                     const QModelIndex & index) override;

    virtual QSize sizeHint(const QStyleOptionViewItem &option,
                           const QModelIndex &index) const Q_DECL_OVERRIDE;
};

} // namespace color_widgets

#endif // COLOR_DELEGATE_HPP
