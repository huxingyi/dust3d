#ifndef EXPORT_PREVIEW_WIDGET_H
#define EXPORT_PREVIEW_WIDGET_H
#include <QDialog>
#include <QImage>
#include <QLabel>
#include <QPushButton>
#include <QShowEvent>
#include "modelwidget.h"
#include "waitingspinnerwidget.h"
#include "skeletondocument.h"

class ExportPreviewWidget : public QDialog
{
    Q_OBJECT
signals:
    void regenerate();
    void save();
public:
    ExportPreviewWidget(SkeletonDocument *document, QWidget *parent=nullptr);
public slots:
    void checkSpinner();
    void updateTexturePreview();
protected:
    void resizeEvent(QResizeEvent *event);
    void showEvent(QShowEvent *event);
private:
    void resizePreviewImage();
    void initAwesomeButton(QPushButton *button);
    void updateTexturePreviewImage(const QImage &image);
private:
    SkeletonDocument *m_document;
    QLabel *m_previewLabel;
    QImage m_previewImage;
    ModelWidget *m_textureRenderWidget;
    WaitingSpinnerWidget *m_spinnerWidget;
    QPushButton *m_saveButton;
};

#endif
