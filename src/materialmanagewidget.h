#ifndef DUST3D_MATERIAL_MANAGE_WIDGET_H
#define DUST3D_MATERIAL_MANAGE_WIDGET_H
#include <QWidget>
#include "document.h"
#include "materiallistwidget.h"

class MaterialManageWidget : public QWidget
{
    Q_OBJECT
signals:
    void registerDialog(QWidget *widget);
    void unregisterDialog(QWidget *widget);
public:
    MaterialManageWidget(const Document *document, QWidget *parent=nullptr);
    MaterialListWidget *materialListWidget();
protected:
    virtual QSize sizeHint() const;
public slots:
    void showAddMaterialDialog();
    void showMaterialDialog(QUuid materialId);
private:
    const Document *m_document = nullptr;
    MaterialListWidget *m_materialListWidget = nullptr;
};

#endif
