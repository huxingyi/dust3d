#include <QGridLayout>
#include <QDebug>
#include "skeletonpartlistwidget.h"
#include "theme.h"

SkeletonPartWidget::SkeletonPartWidget(const SkeletonDocument *document, QUuid partId) :
    m_document(document),
    m_partId(partId)
{
    m_visibleButton = new QPushButton(QChar(fa::eye));
    //m_visibleButton->setStyleSheet("QPushButton {border: none; background: none;}");
    m_visibleButton->setFont(Theme::awesome()->font(Theme::toolIconFontSize * 2 / 3));
    m_visibleButton->setFixedSize(Theme::toolIconSize * 2 / 3, Theme::toolIconSize * 2 / 3);
    
    m_previewLabel = new QLabel;
    
    //m_nameLabel = new QLabel;
    
    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(m_visibleButton, 0, 0);
    mainLayout->addWidget(m_previewLabel, 0, 1);
    //mainLayout->addWidget(m_nameLabel, 0, 2);
    
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
