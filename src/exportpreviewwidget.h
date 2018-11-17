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
    void saveAsGlb();
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
    void resizePreviewImages();
    void initAwesomeButton(QPushButton *button);
private:
    Document *m_document;
    QLabel *m_colorPreviewLabel;
    QLabel *m_normalPreviewLabel;
    QLabel *m_metalnessRoughnessAmbientOcclusionPreviewLabel;
    QImage m_colorImage;
    QImage m_normalImage;
    QImage m_metalnessRoughnessAmbientOcclusionImage;
    //ModelWidget *m_textureRenderWidget;
    WaitingSpinnerWidget *m_spinnerWidget;
    QPushButton *m_saveButton;
};

#endif
