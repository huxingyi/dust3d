#ifndef DUST3D_MATERIAL_WIDGET_H
#define DUST3D_MATERIAL_WIDGET_H
#include <QFrame>
#include <QLabel>
#include <QIcon>
#include "document.h"
#include "modelwidget.h"

class MaterialWidget : public QFrame
{
    Q_OBJECT
signals:
    void modifyMaterial(QUuid materialId);
    void cornerButtonClicked(QUuid materialId);
public:
    MaterialWidget(const Document *document, QUuid materialId);
    static int preferredHeight();
    ModelWidget *previewWidget();
protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
public slots:
    void reload();
    void updatePreview(QUuid materialId);
    void updateName(QUuid materialId);
    void updateCheckedState(bool checked);
    void setCornerButtonVisible(bool visible);
private:
    QUuid m_materialId;
    const Document *m_document = nullptr;
    ModelWidget *m_previewWidget = nullptr;
    QLabel *m_nameLabel = nullptr;
    QPushButton *m_cornerButton = nullptr;
};

#endif
