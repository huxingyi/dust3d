#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>
#include <QMenu>
#include <QWidgetAction>
#include "skeletonpartlistwidget.h"
#include "theme.h"
#include "floatnumberwidget.h"

SkeletonPartWidget::SkeletonPartWidget(const SkeletonDocument *document, QUuid partId) :
    m_document(document),
    m_partId(partId)
{
    m_visibleButton = new QPushButton();
    initButton(m_visibleButton);
    updateVisibleButton();
    
    m_lockButton = new QPushButton();
    initButton(m_lockButton);
    updateLockButton();
    
    m_subdivButton = new QPushButton();
    initButton(m_subdivButton);
    updateSubdivButton();
    
    m_disableButton = new QPushButton();
    initButton(m_disableButton);
    updateDisableButton();
    
    m_xMirrorButton = new QPushButton();
    initButton(m_xMirrorButton);
    updateXmirrorButton();
    
    m_deformButton = new QPushButton();
    initButton(m_deformButton);
    updateDeformButton();
    
    m_previewLabel = new QLabel;
    
    QHBoxLayout *miniTopToolLayout = new QHBoxLayout;
    miniTopToolLayout->setSpacing(0);
    miniTopToolLayout->setContentsMargins(0, 0, 0, 0);
    miniTopToolLayout->addWidget(m_visibleButton);
    miniTopToolLayout->addWidget(m_lockButton);
    miniTopToolLayout->addWidget(m_disableButton);
    miniTopToolLayout->addStretch();
    
    QHBoxLayout *miniBottomToolLayout = new QHBoxLayout;
    miniBottomToolLayout->setSpacing(0);
    miniBottomToolLayout->setContentsMargins(0, 0, 0, 0);
    miniBottomToolLayout->addWidget(m_subdivButton);
    miniBottomToolLayout->addWidget(m_deformButton);
    miniBottomToolLayout->addWidget(m_xMirrorButton);
    miniBottomToolLayout->addStretch();
    
    QWidget *hrWidget = new QWidget;
    hrWidget->setFixedHeight(1);
    hrWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    hrWidget->setStyleSheet(QString("background-color: #252525;"));
    hrWidget->setContentsMargins(0, 0, 0, 0);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addLayout(miniTopToolLayout);
    mainLayout->addWidget(m_previewLabel);
    mainLayout->addLayout(miniBottomToolLayout);
    mainLayout->addWidget(hrWidget);
    
    setLayout(mainLayout);
    
    connect(this, &SkeletonPartWidget::setPartLockState, m_document, &SkeletonDocument::setPartLockState);
    connect(this, &SkeletonPartWidget::setPartVisibleState, m_document, &SkeletonDocument::setPartVisibleState);
    connect(this, &SkeletonPartWidget::setPartSubdivState, m_document, &SkeletonDocument::setPartSubdivState);
    connect(this, &SkeletonPartWidget::setPartDisableState, m_document, &SkeletonDocument::setPartDisableState);
    connect(this, &SkeletonPartWidget::setPartXmirrorState, m_document, &SkeletonDocument::setPartXmirrorState);
    connect(this, &SkeletonPartWidget::setPartDeformThickness, m_document, &SkeletonDocument::setPartDeformThickness);
    connect(this, &SkeletonPartWidget::setPartDeformWidth, m_document, &SkeletonDocument::setPartDeformWidth);
    connect(this, &SkeletonPartWidget::checkPart, m_document, &SkeletonDocument::checkPart);
    
    connect(this, &SkeletonPartWidget::groupOperationAdded, m_document, &SkeletonDocument::saveSnapshot);
    
    connect(m_lockButton, &QPushButton::clicked, [=]() {
        const SkeletonPart *part = m_document->findPart(m_partId);
        if (!part) {
            qDebug() << "Part not found:" << m_partId;
            return;
        }
        emit setPartLockState(m_partId, !part->locked);
    });
    
    connect(m_visibleButton, &QPushButton::clicked, [=]() {
        const SkeletonPart *part = m_document->findPart(m_partId);
        if (!part) {
            qDebug() << "Part not found:" << m_partId;
            return;
        }
        emit setPartVisibleState(m_partId, !part->visible);
    });
    
    connect(m_subdivButton, &QPushButton::clicked, [=]() {
        const SkeletonPart *part = m_document->findPart(m_partId);
        if (!part) {
            qDebug() << "Part not found:" << m_partId;
            return;
        }
        emit setPartSubdivState(m_partId, !part->subdived);
        emit groupOperationAdded();
    });
    
    connect(m_disableButton, &QPushButton::clicked, [=]() {
        const SkeletonPart *part = m_document->findPart(m_partId);
        if (!part) {
            qDebug() << "Part not found:" << m_partId;
            return;
        }
        emit setPartDisableState(m_partId, !part->disabled);
        emit groupOperationAdded();
    });
    
    connect(m_xMirrorButton, &QPushButton::clicked, [=]() {
        const SkeletonPart *part = m_document->findPart(m_partId);
        if (!part) {
            qDebug() << "Part not found:" << m_partId;
            return;
        }
        emit setPartXmirrorState(m_partId, !part->xMirrored);
        emit groupOperationAdded();
    });
    
    connect(m_deformButton, &QPushButton::clicked, [=]() {
        const SkeletonPart *part = m_document->findPart(m_partId);
        if (!part) {
            qDebug() << "Part not found:" << m_partId;
            return;
        }
        showDeformSettingPopup(mapFromGlobal(QCursor::pos()));
        emit groupOperationAdded();
    });
}

void SkeletonPartWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    QWidget::mouseDoubleClickEvent(event);
    emit checkPart(m_partId);
}

void SkeletonPartWidget::initToolButton(QPushButton *button)
{
    button->setFont(Theme::awesome()->font(Theme::toolIconFontSize / 2));
    button->setFixedSize(Theme::toolIconSize / 2, Theme::toolIconSize / 2);
    button->setStyleSheet("QPushButton {color: #f7d9c8}");
    button->setFocusPolicy(Qt::NoFocus);
}

void SkeletonPartWidget::showDeformSettingPopup(const QPoint &pos)
{
    QMenu popupMenu;
    
    const SkeletonPart *part = m_document->findPart(m_partId);
    if (!part) {
        qDebug() << "Find part failed:" << m_partId;
        return;
    }
    
    QWidget *popup = new QWidget;
    
    FloatNumberWidget *thicknessWidget = new FloatNumberWidget;
    thicknessWidget->setRange(0, 2);
    thicknessWidget->setValue(part->deformThickness);
    
    connect(thicknessWidget, &FloatNumberWidget::valueChanged, [=](float value) {
        emit setPartDeformThickness(m_partId, value);
    });
    
    FloatNumberWidget *widthWidget = new FloatNumberWidget;
    widthWidget->setRange(0, 2);
    widthWidget->setValue(part->deformWidth);
    
    connect(widthWidget, &FloatNumberWidget::valueChanged, [=](float value) {
        emit setPartDeformWidth(m_partId, value);
    });
    
    QPushButton *thicknessEraser = new QPushButton(QChar(fa::eraser));
    initToolButton(thicknessEraser);
    
    connect(thicknessEraser, &QPushButton::clicked, [=]() {
        thicknessWidget->setValue(1.0);
    });
    
    QPushButton *widthEraser = new QPushButton(QChar(fa::eraser));
    initToolButton(widthEraser);
    
    connect(widthEraser, &QPushButton::clicked, [=]() {
        widthWidget->setValue(1.0);
    });
    
    QVBoxLayout *layout = new QVBoxLayout;
    QHBoxLayout *thicknessLayout = new QHBoxLayout;
    QHBoxLayout *widthLayout = new QHBoxLayout;
    thicknessLayout->addWidget(thicknessEraser);
    thicknessLayout->addWidget(thicknessWidget);
    widthLayout->addWidget(widthEraser);
    widthLayout->addWidget(widthWidget);
    layout->addLayout(thicknessLayout);
    layout->addLayout(widthLayout);
    
    popup->setLayout(layout);
    
    QWidgetAction *action = new QWidgetAction(this);
    action->setDefaultWidget(popup);
    
    popupMenu.addAction(action);
    
    popupMenu.exec(mapToGlobal(pos));
}

void SkeletonPartWidget::initButton(QPushButton *button)
{
    button->setFont(Theme::awesome()->font(Theme::miniIconFontSize));
    button->setFixedSize(Theme::miniIconSize, Theme::miniIconSize);
    button->setFocusPolicy(Qt::NoFocus);
}

void SkeletonPartWidget::updateButton(QPushButton *button, QChar icon, bool highlighted)
{
    button->setText(icon);
    if (highlighted)
        button->setStyleSheet("QPushButton {border: none; background: none; color: #925935;}");
    else
        button->setStyleSheet("QPushButton {border: none; background: none; color: #252525;}");
}

void SkeletonPartWidget::updatePreview()
{
    const SkeletonPart *part = m_document->findPart(m_partId);
    if (!part) {
        qDebug() << "Part not found:" << m_partId;
        return;
    }
    m_previewLabel->setPixmap(QPixmap::fromImage(part->preview));
}

void SkeletonPartWidget::updateLockButton()
{
    const SkeletonPart *part = m_document->findPart(m_partId);
    if (!part) {
        qDebug() << "Part not found:" << m_partId;
        return;
    }
    if (part->locked)
        updateButton(m_lockButton, QChar(fa::lock), true);
    else
        updateButton(m_lockButton, QChar(fa::unlock), false);
}

void SkeletonPartWidget::updateVisibleButton()
{
    const SkeletonPart *part = m_document->findPart(m_partId);
    if (!part) {
        qDebug() << "Part not found:" << m_partId;
        return;
    }
    if (part->visible)
        updateButton(m_visibleButton, QChar(fa::eye), false);
    else
        updateButton(m_visibleButton, QChar(fa::eyeslash), true);
}

void SkeletonPartWidget::updateSubdivButton()
{
    const SkeletonPart *part = m_document->findPart(m_partId);
    if (!part) {
        qDebug() << "Part not found:" << m_partId;
        return;
    }
    if (part->subdived)
        updateButton(m_subdivButton, QChar(fa::circle), true);
    else
        updateButton(m_subdivButton, QChar(fa::square), false);
}

void SkeletonPartWidget::updateDisableButton()
{
    const SkeletonPart *part = m_document->findPart(m_partId);
    if (!part) {
        qDebug() << "Part not found:" << m_partId;
        return;
    }
    if (part->disabled)
        updateButton(m_disableButton, QChar(fa::unlink), true);
    else
        updateButton(m_disableButton, QChar(fa::link), false);
}

void SkeletonPartWidget::updateXmirrorButton()
{
    const SkeletonPart *part = m_document->findPart(m_partId);
    if (!part) {
        qDebug() << "Part not found:" << m_partId;
        return;
    }
    if (part->xMirrored)
        updateButton(m_xMirrorButton, QChar(fa::balancescale), true);
    else
        updateButton(m_xMirrorButton, QChar(fa::balancescale), false);
}

void SkeletonPartWidget::updateDeformButton()
{
    const SkeletonPart *part = m_document->findPart(m_partId);
    if (!part) {
        qDebug() << "Part not found:" << m_partId;
        return;
    }
    if (part->deformAdjusted())
        updateButton(m_deformButton, QChar(fa::handlizardo), true);
    else
        updateButton(m_deformButton, QChar(fa::handlizardo), false);
}

void SkeletonPartWidget::reload()
{
    updatePreview();
    updateLockButton();
    updateVisibleButton();
    updateSubdivButton();
    updateDisableButton();
    updateXmirrorButton();
    updateDeformButton();
}

SkeletonPartListWidget::SkeletonPartListWidget(const SkeletonDocument *document) :
    m_document(document)
{
    setSelectionMode(QAbstractItemView::NoSelection);
    setFocusPolicy(Qt::NoFocus);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSpacing(0);
    setContentsMargins(0, 0, 0, 0);
}

void SkeletonPartListWidget::partChanged(QUuid partId)
{
    auto item = m_itemMap.find(partId);
    if (item == m_itemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second);
    widget->reload();
}

void SkeletonPartListWidget::partListChanged()
{
    clear();
    m_itemMap.clear();
    
    int row = 0;
    
    for (auto partIdIt = m_document->partIds.begin(); partIdIt != m_document->partIds.end(); partIdIt++, row++) {
        QUuid partId = *partIdIt;
        QListWidgetItem *item = new QListWidgetItem(this);
        item->setSizeHint(QSize(width(), Theme::previewImageSize));
        item->setData(Qt::UserRole, QVariant(row));
        updateItemBackground(item, false);
        addItem(item);
        SkeletonPartWidget *widget = new SkeletonPartWidget(m_document, partId);
        setItemWidget(item, widget);
        widget->reload();
        m_itemMap[partId] = item;
    }
}

void SkeletonPartListWidget::updateItemBackground(QListWidgetItem *item, bool checked)
{
    if (checked) {
        QColor color = Theme::green;
        color.setAlphaF(Theme::fillAlpha);
        item->setBackground(color);
    } else {
        int row = item->data(Qt::UserRole).toInt();
        item->setBackground(0 == row % 2 ? Theme::dark : Theme::altDark);
    }
}

void SkeletonPartListWidget::partPreviewChanged(QUuid partId)
{
    auto item = m_itemMap.find(partId);
    if (item == m_itemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second);
    widget->updatePreview();
}

void SkeletonPartListWidget::partLockStateChanged(QUuid partId)
{
    auto item = m_itemMap.find(partId);
    if (item == m_itemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second);
    widget->updateLockButton();
}

void SkeletonPartListWidget::partVisibleStateChanged(QUuid partId)
{
    auto item = m_itemMap.find(partId);
    if (item == m_itemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second);
    widget->updateVisibleButton();
}

void SkeletonPartListWidget::partSubdivStateChanged(QUuid partId)
{
    auto item = m_itemMap.find(partId);
    if (item == m_itemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second);
    widget->updateSubdivButton();
}

void SkeletonPartListWidget::partDisableStateChanged(QUuid partId)
{
    auto item = m_itemMap.find(partId);
    if (item == m_itemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second);
    widget->updateDisableButton();
}

void SkeletonPartListWidget::partXmirrorStateChanged(QUuid partId)
{
    auto item = m_itemMap.find(partId);
    if (item == m_itemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second);
    widget->updateXmirrorButton();
}

void SkeletonPartListWidget::partDeformChanged(QUuid partId)
{
    auto item = m_itemMap.find(partId);
    if (item == m_itemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second);
    widget->updateDeformButton();
}

void SkeletonPartListWidget::partChecked(QUuid partId)
{
    auto item = m_itemMap.find(partId);
    if (item == m_itemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    updateItemBackground(item->second, true);
}

void SkeletonPartListWidget::partUnchecked(QUuid partId)
{
    auto item = m_itemMap.find(partId);
    if (item == m_itemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    updateItemBackground(item->second, false);
}

