#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H
#include <QMainWindow>
#include <QString>
#include <QVBoxLayout>
#include <QPushButton>

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow();
public slots:
    void updatePageButtons();
private:
    QPushButton *m_partsPageButton;
    QPushButton *m_combinePageButton;
};

#endif
