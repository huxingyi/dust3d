#include <QPixmap>
#include <QPainter>
#include "markiconcreator.h"
#include "theme.h"

std::map<SkeletonBoneMark, QIcon> MarkIconCreator::m_iconMap;
int MarkIconCreator::m_iconSize = 40;

QIcon MarkIconCreator::createIcon(SkeletonBoneMark boneMark)
{
    if (m_iconMap.find(boneMark) == m_iconMap.end()) {
        QPixmap pixmap(MarkIconCreator::m_iconSize, MarkIconCreator::m_iconSize);
        pixmap.fill(Qt::transparent);
        QColor color = SkeletonBoneMarkToColor(boneMark);
        QPainter painter(&pixmap);
        painter.setBrush(QBrush(color));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(0, 0, MarkIconCreator::m_iconSize, MarkIconCreator::m_iconSize);
        QIcon icon(pixmap);
        m_iconMap[boneMark] = icon;
    }
    return m_iconMap[boneMark];
}
