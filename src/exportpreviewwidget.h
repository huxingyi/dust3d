#ifndef DUST3D_EXPORT_PREVIEW_WIDGET_H
#define DUST3D_EXPORT_PREVIEW_WIDGET_H
#include <QDialog>
#include <QImage>
#include <QLabel>
#include <QPushButton>
#include <QShowEvent>
#include "modelwidget.h"
#include "waitingspinnerwidget.h"
#include "document.h"

class ExportPreviewWidget : public QDialog
{
    Q_OBJECT
signals:
    void regenerate();
    void saveAsGltf();
    void saveAsFbx();
public:
    ExportPreviewWidget(Document *document, QWidget *parent=nullptr);
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
    Document *m_document;
    QLabel *m_previewLabel;
    QImage m_previewImage;
    ModelWidget *m_textureRenderWidget;
    WaitingSpinnerWidget *m_spinnerWidget;
    QPushButton *m_saveButton;
};

#endif
