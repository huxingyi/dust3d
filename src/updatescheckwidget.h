#ifndef DUST3D_UPDATES_CHECK_WIDGET_H
#define DUST3D_UPDATES_CHECK_WIDGET_H
#include <QDialog>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>

class UpdatesChecker;

class UpdatesCheckWidget : public QDialog
{
    Q_OBJECT
public:
    UpdatesCheckWidget();
    ~UpdatesCheckWidget();
public slots:
    void check();
    void checkFinished();
    void viewUpdates();
private:
    UpdatesChecker *m_updatesChecker = nullptr;
    QStackedWidget *m_stackedWidget = nullptr;
    QLabel *m_infoLabel = nullptr;
    QPushButton *m_viewButton = nullptr;
    QString m_viewUrl;
};

#endif
