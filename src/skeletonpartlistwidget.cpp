#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>
#include "skeletonpartlistwidget.h"
#include "theme.h"

SkeletonPartWidget::SkeletonPartWidget(const SkeletonDocument *document, QUuid partId) :
    m_document(document),
    m_partId(partId)
{
    m_visibleButton = new QPushButton();
    updateButton(m_visibleButton, QChar(fa::eye), false);
    initButton(m_visibleButton);
    
    m_lockButton = new QPushButton();
    updateButton(m_lockButton, QChar(fa::unlock), false);
    initButton(m_lockButton);
    
    m_subdivButton = new QPushButton();
    updateButton(m_subdivButton, QChar(fa::cube), false);
    initButton(m_subdivButton);
    
    m_disableButton = new QPushButton();
    updateButton(m_disableButton, QChar(fa::toggleon), false);
    initButton(m_disableButton);
    
    m_xMirrorButton = new QPushButton();
    updateButton(m_xMirrorButton, QChar(fa::quoteleft), false);
    initButton(m_xMirrorButton);
    
    m_zMirrorButton = new QPushButton();
    updateButton(m_zMirrorButton, QChar(fa::quoteright), false);
    initButton(m_zMirrorButton);
    
    m_previewLabel = new QLabel;
    
    QHBoxLayout *miniTopToolLayout = new QHBoxLayout;
    miniTopToolLayout->setSpacing(0);
    miniTopToolLayout->setContentsMargins(0, 0, 0, 0);
    miniTopToolLayout->addWidget(m_visibleButton);
    miniTopToolLayout->addWidget(m_lockButton);
    miniTopToolLayout->addWidget(m_subdivButton);
    miniTopToolLayout->addStretch();
    
    QHBoxLayout *miniBottomToolLayout = new QHBoxLayout;
    miniBottomToolLayout->setSpacing(0);
    miniBottomToolLayout->setContentsMargins(0, 0, 0, 0);
    miniBottomToolLayout->addWidget(m_disableButton);
    miniBottomToolLayout->addWidget(m_xMirrorButton);
    miniBottomToolLayout->addWidget(m_zMirrorButton);
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
    connect(this, &SkeletonPartWidget::setPartZmirrorState, m_document, &SkeletonDocument::setPartZmirrorState);
    
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
    });
    
    connect(m_disableButton, &QPushButton::clicked, [=]() {
        const SkeletonPart *part = m_document->findPart(m_partId);
        if (!part) {
            qDebug() << "Part not found:" << m_partId;
            return;
        }
        emit setPartDisableState(m_partId, !part->disabled);
    });
    
    connect(m_xMirrorButton, &QPushButton::clicked, [=]() {
        const SkeletonPart *part = m_document->findPart(m_partId);
        if (!part) {
            qDebug() << "Part not found:" << m_partId;
            return;
        }
        emit setPartXmirrorState(m_partId, !part->xMirrored);
    });
    
    connect(m_zMirrorButton, &QPushButton::clicked, [=]() {
        const SkeletonPart *part = m_document->findPart(m_partId);
        if (!part) {
            qDebug() << "Part not found:" << m_partId;
            return;
        }
        emit setPartZmirrorState(m_partId, !part->zMirrored);
    });
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
        button->setStyleSheet("QPushButton {border: none; background: none; color: #f7d9c8;}");
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
        updateButton(m_subdivButton, QChar(fa::connectdevelop), true);
    else
        updateButton(m_subdivButton, QChar(fa::cube), false);
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
        updateButton(m_xMirrorButton, QChar(fa::quoteleft), true);
    else
        updateButton(m_xMirrorButton, QChar(fa::quoteleft), false);
}

void SkeletonPartWidget::updateZmirrorButton()
{
    const SkeletonPart *part = m_document->findPart(m_partId);
    if (!part) {
        qDebug() << "Part not found:" << m_partId;
        return;
    }
    if (part->zMirrored)
        updateButton(m_zMirrorButton, QChar(fa::quoteright), true);
    else
        updateButton(m_zMirrorButton, QChar(fa::quoteright), false);
}

void SkeletonPartWidget::reload()
{
    updatePreview();
    updateLockButton();
    updateVisibleButton();
    updateSubdivButton();
    updateDisableButton();
    updateXmirrorButton();
    updateZmirrorButton();
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
    
    for (auto partIdIt = m_document->partIds.begin(); partIdIt != m_document->partIds.end(); partIdIt++) {
        QUuid partId = *partIdIt;
        QListWidgetItem *item = new QListWidgetItem(this);
        item->setSizeHint(QSize(width(), Theme::previewImageSize));
        addItem(item);
        SkeletonPartWidget *widget = new SkeletonPartWidget(m_document, partId);
        setItemWidget(item, widget);
        widget->reload();
        m_itemMap[partId] = item;
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

void SkeletonPartListWidget::partZmirrorStateChanged(QUuid partId)
{
    auto item = m_itemMap.find(partId);
    if (item == m_itemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second);
    widget->updateZmirrorButton();
}
