#ifndef DUST3D_ADVANCE_SETTING_WIDGET_H
#define DUST3D_ADVANCE_SETTING_WIDGET_H
#include <QDialog>
#include "document.h"

class AdvanceSettingWidget : public QDialog
{
    Q_OBJECT
signals:
    void enableWeld(bool enabled);
public:
    AdvanceSettingWidget(const Document *document, QWidget *parent=nullptr);
private:
    const Document *m_document = nullptr;
};

#endif
