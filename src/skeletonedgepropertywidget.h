#ifndef SKELETON_EDGE_PROPERTY_WIDGET_H
#define SKELETON_EDGE_PROPERTY_WIDGET_H
#include <QWidget>
#include <QComboBox>
#include <QObject>
#include "skeletondocument.h"

class SkeletonEdgePropertyWidget : public QWidget
{
    Q_OBJECT
signals:
    void setEdgeBranchMode(QUuid edgeId, SkeletonEdgeBranchMode mode);
public slots:
    void showProperties(QUuid edgeId);
public:
    SkeletonEdgePropertyWidget(const SkeletonDocument *document);
    QUuid currentEdgeId();
private:
    void updateData();
private:
    QComboBox *m_branchModeComboBox;
    const SkeletonDocument *m_document;
    QUuid m_edgeId;
};

#endif
