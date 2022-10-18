#ifndef DUST3D_APPLICATION_UPDATES_CHECK_WIDGET_H_
#define DUST3D_APPLICATION_UPDATES_CHECK_WIDGET_H_

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>

class UpdatesChecker;

class UpdatesCheckWidget : public QDialog {
    Q_OBJECT
public:
    UpdatesCheckWidget();
    ~UpdatesCheckWidget();
public slots:
    void check();
    void checkFinished();
    void viewUpdates();

private:
    UpdatesChecker* m_updatesChecker = nullptr;
    QStackedWidget* m_stackedWidget = nullptr;
    QLabel* m_infoLabel = nullptr;
    QPushButton* m_viewButton = nullptr;
    QString m_viewUrl;
};

#endif
