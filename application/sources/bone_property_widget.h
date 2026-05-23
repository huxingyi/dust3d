#ifndef DUST3D_APPLICATION_BONE_PROPERTY_WIDGET_H_
#define DUST3D_APPLICATION_BONE_PROPERTY_WIDGET_H_

#include <QWidget>

class Document;

class BonePropertyWidget : public QWidget {
    Q_OBJECT
signals:
    void setHeadHasEyelids(bool hasEyelids);
    void groupOperationAdded();

public:
    BonePropertyWidget(Document* document, QWidget* parent = nullptr);
};

#endif
