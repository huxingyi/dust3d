#include "cutfacewidget.h"

CutFaceWidget::CutFaceWidget(const Document *document, QUuid partId) :
    m_partId(partId),
    m_document(document)
{
    setObjectName("CutFaceFrame");

    m_previewWidget = new ModelWidget(this);
    m_previewWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_previewWidget->setFixedSize(Theme::cutFacePreviewImageSize, Theme::cutFacePreviewImageSize);
    m_previewWidget->enableMove(false);
    m_previewWidget->enableZoom(false);
    m_previewWidget->setXRotation(0);
    m_previewWidget->setYRotation(0);
    m_previewWidget->setZRotation(0);

    setFixedSize(Theme::cutFacePreviewImageSize, CutFaceWidget::preferredHeight());

    connect(document, &Document::partPreviewChanged, this, &CutFaceWidget::updatePreview);
}

void CutFaceWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    m_previewWidget->move((width() - Theme::cutFacePreviewImageSize) / 2, 0);
}

int CutFaceWidget::preferredHeight()
{
    return Theme::cutFacePreviewImageSize;
}

void CutFaceWidget::reload()
{
    updatePreview(m_partId);
}

void CutFaceWidget::updatePreview(QUuid partId)
{
    if (partId != m_partId)
        return;
    const SkeletonPart *part = m_document->findPart(m_partId);
    if (!part) {
        qDebug() << "Part not found:" << m_partId;
        return;
    }
    Model *previewMesh = part->takePreviewMesh();
    m_previewWidget->updateMesh(previewMesh);
}

void CutFaceWidget::updateCheckedState(bool checked)
{
    if (checked)
        setStyleSheet("#CutFaceFrame {border: 1px solid " + Theme::red.name() + ";}");
    else
        setStyleSheet("#CutFaceFrame {border: 1px solid transparent;}");
}

ModelWidget *CutFaceWidget::previewWidget()
{
    return m_previewWidget;
}
