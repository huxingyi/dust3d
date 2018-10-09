#ifndef MATERIAL_WIDGET_H
#define MATERIAL_WIDGET_H
#include <QFrame>
#include <QLabel>
#include <QIcon>
#include "skeletondocument.h"
#include "modelwidget.h"

class MaterialWidget : public QFrame
{
    Q_OBJECT
signals:
    void modifyMaterial(QUuid materialId);
    void cornerButtonClicked(QUuid materialId);
public:
    MaterialWidget(const SkeletonDocument *document, QUuid materialId);
    static int preferredHeight();
    ModelWidget *previewWidget();
protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
public slots:
    void reload();
    void updatePreview();
    void updateName();
    void updateCheckedState(bool checked);
    void setCornerButtonVisible(bool visible);
private:
    QUuid m_materialId;
    const SkeletonDocument *m_document = nullptr;
    ModelWidget *m_previewWidget = nullptr;
    QLabel *m_nameLabel = nullptr;
    QPushButton *m_cornerButton = nullptr;
};

#endif
