#ifndef DUST3D_APPLICATION_EXPORT_PROGRESS_WIDGET_H_
#define DUST3D_APPLICATION_EXPORT_PROGRESS_WIDGET_H_

#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include <QString>

class ExportProgressWidget : public QDialog {
    Q_OBJECT
public:
    ExportProgressWidget(QWidget* parent = nullptr);
    void updateProgress(const QString& step, int current, int total);
    void setStep(const QString& step);

private:
    QLabel* m_stepLabel = nullptr;
    QProgressBar* m_progressBar = nullptr;
};

#endif
