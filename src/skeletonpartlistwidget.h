#ifndef SKELETON_PART_LIST_WIDGET_H
#define SKELETON_PART_LIST_WIDGET_H
#include <QListWidget>
#include <map>
#include <QLabel>
#include <QPushButton>
#include "skeletondocument.h"

class SkeletonPartWidget : public QWidget
{
    Q_OBJECT
public:
    SkeletonPartWidget(const SkeletonDocument *document, QUuid partId);
    void reload();
    QLabel *previewLabel();
private:
    const SkeletonDocument *m_document;
    QUuid m_partId;
    QLabel *m_previewLabel;
    QPushButton *m_visibleButton;
    QLabel *m_nameLabel;
};

class SkeletonPartListWidget : public QListWidget
{
    Q_OBJECT
public:
    SkeletonPartListWidget(const SkeletonDocument *document);
public slots:
    void partChanged(QUuid partId);
    void partListChanged();
    void partPreviewChanged(QUuid partid);
private:
    const SkeletonDocument *m_document;
    std::map<QUuid, QListWidgetItem *> m_itemMap;
};

#endif
