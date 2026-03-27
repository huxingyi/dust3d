#include "export_progress_widget.h"
#include <QApplication>
#include <QVBoxLayout>

ExportProgressWidget::ExportProgressWidget(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Exporting..."));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setModal(true);
    setFixedSize(320, 100);

    m_stepLabel = new QLabel(tr("Preparing..."));
    m_stepLabel->setAlignment(Qt::AlignCenter);

    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->addWidget(m_stepLabel);
    layout->addWidget(m_progressBar);
    setLayout(layout);
}

void ExportProgressWidget::updateProgress(const QString& step, int current, int total)
{
    m_stepLabel->setText(step);
    if (total > 0)
        m_progressBar->setValue((int)(100.0 * current / total));
    else
        m_progressBar->setValue(0);
    QApplication::processEvents();
}

void ExportProgressWidget::setStep(const QString& step)
{
    m_stepLabel->setText(step);
    QApplication::processEvents();
}
