#ifndef MARK_ICON_CREATOR_H
#define MARK_ICON_CREATOR_H
#include <map>
#include <QIcon>
#include "skeletondocument.h"

class MarkIconCreator
{
public:
    static QIcon createIcon(SkeletonBoneMark boneMark);
private:
    static std::map<SkeletonBoneMark, QIcon> m_iconMap;
    static int m_iconSize;
};

#endif
