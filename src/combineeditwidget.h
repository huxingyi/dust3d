#ifndef COMBINE_EDIT_WIDGET_H
#define COMBINE_EDIT_WIDGET_H
#include <QFrame>
#include "modelingwidget.h"

class CombineEditWidget : public QFrame
{
    Q_OBJECT
signals:
    void sizeChanged();
public:
    CombineEditWidget(QWidget *parent = 0);
    ModelingWidget *modelingWidget();
protected:
    void resizeEvent(QResizeEvent *event);
private:
    ModelingWidget *m_modelingWidget;
};

#endif
