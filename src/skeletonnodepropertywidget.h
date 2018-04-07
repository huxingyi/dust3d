#ifndef SKELETON_NODE_PROPERTY_WIDGET_H
#define SKELETON_NODE_PROPERTY_WIDGET_H
#include <QWidget>
#include <QDoubleSpinBox>
#include <QComboBox>
#include "skeletondocument.h"

class SkeletonNodePropertyWidget : public QWidget
{
    Q_OBJECT
signals:
    void setNodeRootMarkMode(QUuid nodeId, SkeletonNodeRootMarkMode mode);
public slots:
    void showProperties(QUuid nodeId);
public:
    SkeletonNodePropertyWidget(const SkeletonDocument *document);
    QUuid currentNodeId();
private:
    void updateData();
private:
    QComboBox *m_rootMarkModeComboBox;
    const SkeletonDocument *m_document;
    QUuid m_nodeId;
};

#endif
