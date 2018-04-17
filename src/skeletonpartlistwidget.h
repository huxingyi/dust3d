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
signals:
    void setPartLockState(QUuid partId, bool locked);
    void setPartVisibleState(QUuid partId, bool visible);
    void setPartSubdivState(QUuid partId, bool subdived);
    void setPartDisableState(QUuid partId, bool disabled);
    void setPartXmirrorState(QUuid partId, bool mirrored);
    void setPartZmirrorState(QUuid partId, bool mirrored);
    void setPartThickness(QUuid partId, float thickness);
public:
    SkeletonPartWidget(const SkeletonDocument *document, QUuid partId);
    void reload();
    void updatePreview();
    void updateLockButton();
    void updateVisibleButton();
    void updateSubdivButton();
    void updateDisableButton();
    void updateXmirrorButton();
    void updateZmirrorButton();
    void updateThicknessButton();
public slots:
    void showThicknessSettingPopup(const QPoint &pos);
private:
    const SkeletonDocument *m_document;
    QUuid m_partId;
    QLabel *m_previewLabel;
    QPushButton *m_visibleButton;
    QPushButton *m_lockButton;
    QPushButton *m_subdivButton;
    QPushButton *m_disableButton;
    QPushButton *m_xMirrorButton;
    QPushButton *m_zMirrorButton;
    QPushButton *m_thicknessButton;
    QLabel *m_nameLabel;
private:
    void initButton(QPushButton *button);
    void updateButton(QPushButton *button, QChar icon, bool highlighted);
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
    void partLockStateChanged(QUuid partId);
    void partVisibleStateChanged(QUuid partId);
    void partSubdivStateChanged(QUuid partId);
    void partDisableStateChanged(QUuid partId);
    void partXmirrorStateChanged(QUuid partId);
    void partThicknessChanged(QUuid partId);
private:
    const SkeletonDocument *m_document;
    std::map<QUuid, QListWidgetItem *> m_itemMap;
};

#endif
