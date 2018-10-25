#ifndef DUST3D_MARK_ICON_CREATOR_H
#define DUST3D_MARK_ICON_CREATOR_H
#include <map>
#include <QIcon>
#include "document.h"

class MarkIconCreator
{
public:
    static QIcon createIcon(BoneMark boneMark);
private:
    static std::map<BoneMark, QIcon> m_iconMap;
    static int m_iconSize;
};

#endif
