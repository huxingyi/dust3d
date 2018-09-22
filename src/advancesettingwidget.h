#ifndef ADVANCE_SETTING_WIDGET_H
#define ADVANCE_SETTING_WIDGET_H
#include <QDialog>
#include "skeletondocument.h"

class AdvanceSettingWidget : public QDialog
{
    Q_OBJECT
signals:
    void enableWeld(bool enabled);
public:
    AdvanceSettingWidget(const SkeletonDocument *document, QWidget *parent=nullptr);
private:
    const SkeletonDocument *m_document = nullptr;
};

#endif
