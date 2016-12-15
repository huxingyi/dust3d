#include <QtGui>
#include <QHBoxLayout>
#include <QDesktopWidget>
#include "mainwindow.h"

MainWindow::MainWindow(void) 
{
	glWidget = new GLWidget;

    resize(QDesktopWidget().availableGeometry(this).size() * 0.7);

	QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addWidget(glWidget);
    setLayout(mainLayout);

    setWindowTitle(tr("DUST3D"));
}

void MainWindow::keyPressEvent(QKeyEvent *e) 
{
    if (e->key() == Qt::Key_Escape) 
    {
    	close();
    } 
    else 
    {
        QWidget::keyPressEvent(e);
    }
}

