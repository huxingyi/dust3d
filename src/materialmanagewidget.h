#ifndef MATERIAL_MANAGE_WIDGET_H
#define MATERIAL_MANAGE_WIDGET_H
#include <QWidget>
#include "skeletondocument.h"
#include "materiallistwidget.h"

class MaterialManageWidget : public QWidget
{
    Q_OBJECT
signals:
    void registerDialog(QWidget *widget);
    void unregisterDialog(QWidget *widget);
public:
    MaterialManageWidget(const SkeletonDocument *document, QWidget *parent=nullptr);
    MaterialListWidget *materialListWidget();
protected:
    virtual QSize sizeHint() const;
public slots:
    void showAddMaterialDialog();
    void showMaterialDialog(QUuid materialId);
private:
    const SkeletonDocument *m_document = nullptr;
    MaterialListWidget *m_materialListWidget = nullptr;
};

#endif
