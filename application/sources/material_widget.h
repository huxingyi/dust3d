#ifndef DUST3D_APPLICATION_MATERIAL_WIDGET_H_
#define DUST3D_APPLICATION_MATERIAL_WIDGET_H_

#include <QFrame>
#include <QLabel>
#include <QIcon>
#include <QPushButton>
#include "model_widget.h"

class Document;

class MaterialWidget : public QFrame
{
    Q_OBJECT
signals:
    void modifyMaterial(dust3d::Uuid materialId);
public:
    MaterialWidget(const Document *document, dust3d::Uuid materialId);
    static int preferredHeight();
    ModelWidget *previewWidget();
protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
public slots:
    void reload();
    void updatePreview(dust3d::Uuid materialId);
    void updateName(dust3d::Uuid materialId);
    void updateCheckedState(bool checked);
private:
    dust3d::Uuid m_materialId;
    const Document *m_document = nullptr;
    ModelWidget *m_previewWidget = nullptr;
    QLabel *m_nameLabel = nullptr;
};

#endif
