#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>
#include "skeletonpartlistwidget.h"
#include "theme.h"

SkeletonPartWidget::SkeletonPartWidget(const SkeletonDocument *document, QUuid partId) :
    m_document(document),
    m_partId(partId)
{
    m_visibleButton = new QPushButton(QChar(fa::eye));
    m_visibleButton->setStyleSheet("QPushButton {border: none; background: none; color: #cccccc;}");
    m_visibleButton->setFont(Theme::awesome()->font(Theme::miniIconFontSize));
    m_visibleButton->setFixedSize(Theme::miniIconSize, Theme::miniIconSize);
    
    m_previewLabel = new QLabel;
    
    QHBoxLayout *miniToolLayout = new QHBoxLayout;
    miniToolLayout->setSpacing(0);
    miniToolLayout->setContentsMargins(0, 0, 0, 0);
    miniToolLayout->addSpacing(3);
    miniToolLayout->addWidget(m_visibleButton);
    miniToolLayout->addStretch();
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addLayout(miniToolLayout);
    mainLayout->addWidget(m_previewLabel);
    
    setLayout(mainLayout);
}

QLabel *SkeletonPartWidget::previewLabel()
{
    return m_previewLabel;
}

void SkeletonPartWidget::reload()
{
    const SkeletonPart *part = m_document->findPart(m_partId);
    if (!part) {
        qDebug() << "Part not found:" << m_partId;
        return;
    }
    //m_nameLabel->setText(part->name.isEmpty() ? part->id.toString() : part->name);
    m_previewLabel->setPixmap(QPixmap::fromImage(part->preview));
}

SkeletonPartListWidget::SkeletonPartListWidget(const SkeletonDocument *document) :
    m_document(document)
{
    setSelectionMode(QAbstractItemView::NoSelection);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSpacing(1);
    setContentsMargins(0, 0, 0, 0);
}

void SkeletonPartListWidget::partChanged(QUuid partId)
{
    auto itemIt = m_itemMap.find(partId);
    if (itemIt == m_itemMap.end()) {
        return;
    }
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

void SkeletonPartListWidget::partPreviewChanged(QUuid partid)
{
    const SkeletonPart *part = m_document->findPart(partid);
    if (!part) {
        qDebug() << "Part not found:" << partid;
        return;
    }
    auto item = m_itemMap.find(partid);
    if (item == m_itemMap.end()) {
        qDebug() << "Part item not found:" << partid;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second);
    widget->previewLabel()->setPixmap(QPixmap::fromImage(part->preview));
}
