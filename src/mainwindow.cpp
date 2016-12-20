#include <QtGui>
#include <QHBoxLayout>
#include <QDesktopWidget>
#include "mainwindow.h"
#include "render.h"

MainWindow::MainWindow(void) {
	render = new Render;
	int size = QDesktopWidget().availableGeometry(this).size().height() * 0.7;
  resize(QSize(size, size));
	QHBoxLayout *mainLayout = new QHBoxLayout;
  mainLayout->addWidget(render);
  setLayout(mainLayout);
  setWindowTitle(tr("Dust3D Experiment"));
}

void MainWindow::keyPressEvent(QKeyEvent *e) {
  if (e->key() == Qt::Key_Escape) {
  	close();
  } else {
    QWidget::keyPressEvent(e);
  }
}
