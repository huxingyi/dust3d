#ifndef DUST3D_PREFERENCES_WIDGET_H
#define DUST3D_PREFERENCES_WIDGET_H
#include <QDialog>
#include "document.h"

class PreferencesWidget : public QDialog
{
    Q_OBJECT
signals:
    void enableWeld(bool enabled);
    void enableBackgroundBlur();
    void disableBackgroundBlur();
public:
    PreferencesWidget(const Document *document, QWidget *parent=nullptr);
private:
    const Document *m_document = nullptr;
};

#endif
