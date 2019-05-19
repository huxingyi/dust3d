#ifndef DUST3D_CUT_FACE_WIDGET_H
#define DUST3D_CUT_FACE_WIDGET_H
#include <QFrame>
#include <QLabel>
#include <QIcon>
#include "document.h"
#include "modelwidget.h"

class CutFaceWidget : public QFrame
{
    Q_OBJECT
public:
    CutFaceWidget(const Document *document, QUuid partId);
    static int preferredHeight();
    ModelWidget *previewWidget();
protected:
    void resizeEvent(QResizeEvent *event) override;
public slots:
    void reload();
    void updatePreview(QUuid partId);
    void updateCheckedState(bool checked);
private:
    QUuid m_partId;
    const Document *m_document = nullptr;
    ModelWidget *m_previewWidget = nullptr;
};

#endif
