#ifndef BONE_EXPORT_WIDGET_H
#define BONE_EXPORT_WIDGET_H
#include <QWidget>
#include "modelwidget.h"

class BoneExportWidget : public QWidget
{
    Q_OBJECT
public:
    BoneExportWidget();
private:
    ModelWidget *m_boneModelWidget;
};

#endif
