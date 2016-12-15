#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include "glwidget.h"

class MainWindow : public QWidget 
{
	Q_OBJECT

public:
	MainWindow(void);

protected:
    void keyPressEvent(QKeyEvent *event);

private:
	GLWidget *glWidget;
};

#endif
