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
    
    m_previewLabel = new QLabel;
    
    QHBoxLayout *miniToolLayout = new QHBoxLayout;
    miniToolLayout->setSpacing(0);
    miniToolLayout->setContentsMargins(0, 0, 0, 0);
    miniToolLayout->addSpacing(5);
    miniToolLayout->addWidget(m_visibleButton);
    miniToolLayout->addWidget(m_lockButton);
    miniToolLayout->addWidget(m_subdivButton);
    miniToolLayout->addStretch();
    
    QWidget *hrWidget = new QWidget;
    hrWidget->setFixedHeight(1);
    hrWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    hrWidget->setStyleSheet(QString("background-color: #252525;"));
    hrWidget->setContentsMargins(0, 0, 0, 0);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addLayout(miniToolLayout);
    mainLayout->addWidget(m_previewLabel);
    mainLayout->addWidget(hrWidget);
    
    setLayout(mainLayout);
    
    connect(this, &SkeletonPartWidget::setPartLockState, m_document, &SkeletonDocument::setPartLockState);
    connect(this, &SkeletonPartWidget::setPartVisibleState, m_document, &SkeletonDocument::setPartVisibleState);
    connect(this, &SkeletonPartWidget::setPartSubdivState, m_document, &SkeletonDocument::setPartSubdivState);
    
    connect(m_lockButton, &QPushButton::clicked, [=]() {
        if (m_lockButton->text() == QChar(fa::lock)) {
            emit setPartLockState(m_partId, false);
        } else {
            emit setPartLockState(m_partId, true);
        }
    });
    
    connect(m_visibleButton, &QPushButton::clicked, [=]() {
        if (m_visibleButton->text() == QChar(fa::eye)) {
            emit setPartVisibleState(m_partId, false);
        } else {
            emit setPartVisibleState(m_partId, true);
        }
    });
    
    connect(m_subdivButton, &QPushButton::clicked, [=]() {
        if (m_subdivButton->text() == QChar(fa::cube)) {
            emit setPartSubdivState(m_partId, true);
        } else {
            emit setPartSubdivState(m_partId, false);
        }
    });
}

void SkeletonPartWidget::initButton(QPushButton *button)
{
    button->setFont(Theme::awesome()->font(Theme::miniIconFontSize));
    button->setFixedSize(Theme::miniIconSize, Theme::miniIconSize);
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

void SkeletonPartWidget::reload()
{
    updatePreview();
    updateLockButton();
    updateVisibleButton();
    updateSubdivButton();
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
