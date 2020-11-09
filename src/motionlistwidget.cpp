#include <QGuiApplication>
#include <QMenu>
#include <QXmlStreamWriter>
#include <QClipboard>
#include <QApplication>
#include "snapshotxml.h"
#include "motionlistwidget.h"

MotionListWidget::MotionListWidget(const Document *document, QWidget *parent) :
    QTreeWidget(parent),
    m_document(document)
{
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setFocusPolicy(Qt::NoFocus);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    setAutoScroll(false);
    
    setHeaderHidden(true);

    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, Qt::transparent);
    palette.setColor(QPalette::Base, Qt::transparent);
    setPalette(palette);
    
    setStyleSheet("QTreeView {qproperty-indentation: 0;}");
    
    setContentsMargins(0, 0, 0, 0);
    
    connect(document, &Document::motionListChanged, this, &MotionListWidget::reload);
    connect(document, &Document::cleanup, this, &MotionListWidget::removeAllContent);
    
    connect(this, &MotionListWidget::removeMotion, document, &Document::removeMotion);
    
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QTreeWidget::customContextMenuRequested, this, &MotionListWidget::showContextMenu);
    
    reload();
}

void MotionListWidget::motionRemoved(QUuid motionId)
{
    if (m_currentSelectedMotionId == motionId)
        m_currentSelectedMotionId = QUuid();
    m_selectedMotionIds.erase(motionId);
    m_itemMap.erase(motionId);
}

void MotionListWidget::updateMotionSelectState(QUuid motionId, bool selected)
{
    auto findItemResult = m_itemMap.find(motionId);
    if (findItemResult == m_itemMap.end()) {
        qDebug() << "Find motion item failed:" << motionId;
        return;
    }
    MotionWidget *motionWidget = (MotionWidget *)itemWidget(findItemResult->second.first, findItemResult->second.second);
    motionWidget->updateCheckedState(selected);
    if (m_cornerButtonVisible) {
        motionWidget->setCornerButtonVisible(selected);
    }
}

void MotionListWidget::selectMotion(QUuid motionId, bool multiple)
{
    if (multiple) {
        if (!m_currentSelectedMotionId.isNull()) {
            m_selectedMotionIds.insert(m_currentSelectedMotionId);
            m_currentSelectedMotionId = QUuid();
        }
        if (m_selectedMotionIds.find(motionId) != m_selectedMotionIds.end()) {
            updateMotionSelectState(motionId, false);
            m_selectedMotionIds.erase(motionId);
        } else {
            updateMotionSelectState(motionId, true);
            m_selectedMotionIds.insert(motionId);
        }
        if (m_selectedMotionIds.size() > 1) {
            return;
        }
        if (m_selectedMotionIds.size() == 1)
            motionId = *m_selectedMotionIds.begin();
        else
            motionId = QUuid();
    }
    if (!m_selectedMotionIds.empty()) {
        for (const auto &id: m_selectedMotionIds) {
            updateMotionSelectState(id, false);
        }
        m_selectedMotionIds.clear();
    }
    if (m_currentSelectedMotionId != motionId) {
        if (!m_currentSelectedMotionId.isNull()) {
            updateMotionSelectState(m_currentSelectedMotionId, false);
        }
        m_currentSelectedMotionId = motionId;
        if (!m_currentSelectedMotionId.isNull()) {
            updateMotionSelectState(m_currentSelectedMotionId, true);
        }
    }
}

void MotionListWidget::mousePressEvent(QMouseEvent *event)
{
    QModelIndex itemIndex = indexAt(event->pos());
    QTreeView::mousePressEvent(event);
    if (event->button() == Qt::LeftButton) {
        bool multiple = QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier);
        if (itemIndex.isValid()) {
            QTreeWidgetItem *item = itemFromIndex(itemIndex);
            auto motionId = QUuid(item->data(itemIndex.column(), Qt::UserRole).toString());
            if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier)) {
                bool startAdd = false;
                bool stopAdd = false;
                std::vector<QUuid> waitQueue;
                for (const auto &motionIt: m_document->motionMap) {
                    const auto &childId = motionIt.first;
                    if (m_shiftStartMotionId == childId || motionId == childId) {
                        if (startAdd) {
                            stopAdd = true;
                        } else {
                            startAdd = true;
                        }
                    }
                    if (startAdd)
                        waitQueue.push_back(childId);
                    if (stopAdd)
                        break;
                }
                if (stopAdd && !waitQueue.empty()) {
                    if (!m_selectedMotionIds.empty()) {
                        for (const auto &id: m_selectedMotionIds) {
                            updateMotionSelectState(id, false);
                        }
                        m_selectedMotionIds.clear();
                    }
                    if (!m_currentSelectedMotionId.isNull()) {
                        m_currentSelectedMotionId = QUuid();
                    }
                    for (const auto &waitId: waitQueue) {
                        selectMotion(waitId, true);
                    }
                }
                return;
            } else {
                m_shiftStartMotionId = motionId;
            }
            selectMotion(motionId, multiple);
            return;
        }
        if (!multiple)
            selectMotion(QUuid());
    }
}

bool MotionListWidget::isMotionSelected(QUuid motionId)
{
    return (m_currentSelectedMotionId == motionId ||
        m_selectedMotionIds.find(motionId) != m_selectedMotionIds.end());
}

void MotionListWidget::showContextMenu(const QPoint &pos)
{
    if (!m_hasContextMenu)
        return;
    
    QMenu contextMenu(this);
    
    std::set<QUuid> unorderedMotionIds = m_selectedMotionIds;
    if (!m_currentSelectedMotionId.isNull())
        unorderedMotionIds.insert(m_currentSelectedMotionId);
    
    std::vector<QUuid> motionIds;
    for (const auto &cand: m_document->motionMap) {
        if (unorderedMotionIds.find(cand.first) != unorderedMotionIds.end())
            motionIds.push_back(cand.first);
    }
    
    QAction modifyAction(tr("Modify"), this);
    if (motionIds.size() == 1) {
        connect(&modifyAction, &QAction::triggered, this, [=]() {
            emit modifyMotion(*motionIds.begin());
        });
        contextMenu.addAction(&modifyAction);
    }
    
    QAction copyAction(tr("Copy"), this);
    if (!motionIds.empty()) {
        connect(&copyAction, &QAction::triggered, this, &MotionListWidget::copy);
        contextMenu.addAction(&copyAction);
    }
    
    QAction pasteAction(tr("Paste"), this);
    if (m_document->hasPastableMotionsInClipboard()) {
        connect(&pasteAction, &QAction::triggered, m_document, &Document::paste);
        contextMenu.addAction(&pasteAction);
    }
    
    QAction deleteAction(tr("Delete"), this);
    if (!motionIds.empty()) {
        connect(&deleteAction, &QAction::triggered, [=]() {
            for (const auto &motionId: motionIds)
                emit removeMotion(motionId);
        });
        contextMenu.addAction(&deleteAction);
    }
    
    contextMenu.exec(mapToGlobal(pos));
}

void MotionListWidget::resizeEvent(QResizeEvent *event)
{
    QTreeWidget::resizeEvent(event);
    if (calculateColumnCount() != columnCount())
        reload();
}

int MotionListWidget::calculateColumnCount()
{
    if (nullptr == parentWidget())
        return 0;
    
    int columns = parentWidget()->width() / Theme::motionPreviewImageSize;
    if (0 == columns)
        columns = 1;
    return columns;
}

void MotionListWidget::reload()
{
    removeAllContent();
    
    int columns = calculateColumnCount();
    if (0 == columns)
        return;
    
    int columnWidth = parentWidget()->width() / columns;
    
    //qDebug() << "parentWidth:" << parentWidget()->width() << "columnWidth:" << columnWidth << "columns:" << columns;
    
    setColumnCount(columns);
    for (int i = 0; i < columns; i++)
        setColumnWidth(i, columnWidth);
    
    std::vector<QUuid> orderedMotionIdList;
    for (const auto &motionIt: m_document->motionMap)
        orderedMotionIdList.push_back(motionIt.first);
    std::sort(orderedMotionIdList.begin(), orderedMotionIdList.end(), [&](const QUuid &firstMotionId, const QUuid &secondMotionId) {
        const auto *firstMotion = m_document->findMotion(firstMotionId);
        const auto *secondMotion = m_document->findMotion(secondMotionId);
        if (nullptr == firstMotion || nullptr == secondMotion)
            return false;
        return QString::compare(firstMotion->name, secondMotion->name, Qt::CaseInsensitive) < 0;
    });
    
    decltype(orderedMotionIdList.size()) motionIndex = 0;
    while (motionIndex < orderedMotionIdList.size()) {
        QTreeWidgetItem *item = new QTreeWidgetItem(this);
        item->setFlags((item->flags() | Qt::ItemIsEnabled) & ~(Qt::ItemIsSelectable) & ~(Qt::ItemIsEditable));
        for (int col = 0; col < columns && motionIndex < orderedMotionIdList.size(); col++, motionIndex++) {
            const auto &motionId = orderedMotionIdList[motionIndex];
            item->setSizeHint(col, QSize(columnWidth, MotionWidget::preferredHeight() + 2));
            item->setData(col, Qt::UserRole, motionId.toString());
            MotionWidget *widget = new MotionWidget(m_document, motionId);
            connect(widget, &MotionWidget::modifyMotion, this, &MotionListWidget::modifyMotion);
            connect(widget, &MotionWidget::cornerButtonClicked, this, &MotionListWidget::cornerButtonClicked);
            setItemWidget(item, col, widget);
            widget->reload();
            widget->updateCheckedState(isMotionSelected(motionId));
            m_itemMap[motionId] = std::make_pair(item, col);
        }
        invisibleRootItem()->addChild(item);
    }
}

void MotionListWidget::setCornerButtonVisible(bool visible)
{
    m_cornerButtonVisible = visible;
}

void MotionListWidget::setHasContextMenu(bool hasContextMenu)
{
    m_hasContextMenu = hasContextMenu;
}

void MotionListWidget::removeAllContent()
{
    m_itemMap.clear();
    clear();
}

void MotionListWidget::copy()
{
    if (m_selectedMotionIds.empty() && m_currentSelectedMotionId.isNull())
        return;
    
    std::set<QUuid> limitMotionIds = m_selectedMotionIds;
    if (!m_currentSelectedMotionId.isNull())
        limitMotionIds.insert(m_currentSelectedMotionId);
    
    std::set<QUuid> emptySet;
    
    Snapshot snapshot;
    m_document->toSnapshot(&snapshot, emptySet, DocumentToSnapshotFor::Motions,
        emptySet, limitMotionIds);
    QString snapshotXml;
    QXmlStreamWriter xmlStreamWriter(&snapshotXml);
    saveSkeletonToXmlStream(&snapshot, &xmlStreamWriter);
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(snapshotXml);
}
