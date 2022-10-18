#ifndef DUST3D_APPLICATION_MATERIAL_MANAGE_WIDGET_H_
#define DUST3D_APPLICATION_MATERIAL_MANAGE_WIDGET_H_

#include "material_list_widget.h"
#include <QWidget>

class Document;

class MaterialManageWidget : public QWidget {
    Q_OBJECT
signals:
    void registerDialog(QWidget* widget);
    void unregisterDialog(QWidget* widget);

public:
    MaterialManageWidget(const Document* document, QWidget* parent = nullptr);
    MaterialListWidget* materialListWidget();

protected:
    virtual QSize sizeHint() const;
public slots:
    void showAddMaterialDialog();
    void showMaterialDialog(dust3d::Uuid materialId);

private:
    const Document* m_document = nullptr;
    MaterialListWidget* m_materialListWidget = nullptr;
};

#endif
