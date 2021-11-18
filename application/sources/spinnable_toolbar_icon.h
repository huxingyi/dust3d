#ifndef DUST3D_APPLICATION_SPINNABLE_TOOLBAR_ICON_H_
#define DUST3D_APPLICATION_SPINNABLE_TOOLBAR_ICON_H_

#include <QWidget>
#include "waitingspinnerwidget.h"
#include "toolbar_button.h"

class SpinnableToolbarIcon : public QWidget
{
public:
    SpinnableToolbarIcon(QWidget *parent=nullptr);
    void showSpinner(bool showSpinner=true);
    bool isSpinning();
private:
    WaitingSpinnerWidget *m_spinner = nullptr;
};

#endif
