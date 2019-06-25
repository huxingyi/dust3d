#include <QGuiApplication>
#include <QMenu>
#include <QXmlStreamWriter>
#include <QClipboard>
#include <QApplication>
#include "snapshotxml.h"
#include "poselistwidget.h"

PoseListWidget::PoseListWidget(const Document *document, QWidget *parent) :
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
    
    connect(document, &Document::poseListChanged, this, &PoseListWidget::reload);
    connect(document, &Document::cleanup, this, &PoseListWidget::removeAllContent);
    
    connect(this, &PoseListWidget::removePose, document, &Document::removePose);
    
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QTreeWidget::customContextMenuRequested, this, &PoseListWidget::showContextMenu);
    
    reload();
}

void PoseListWidget::poseRemoved(QUuid poseId)
{
    if (m_currentSelectedPoseId == poseId)
        m_currentSelectedPoseId = QUuid();
    m_selectedPoseIds.erase(poseId);
    m_itemMap.erase(poseId);
}

void PoseListWidget::updatePoseSelectState(QUuid poseId, bool selected)
{
    auto findItemResult = m_itemMap.find(poseId);
    if (findItemResult == m_itemMap.end()) {
        qDebug() << "Find pose item failed:" << poseId;
        return;
    }
    PoseWidget *poseWidget = (PoseWidget *)itemWidget(findItemResult->second.first, findItemResult->second.second);
    poseWidget->updateCheckedState(selected);
    if (m_cornerButtonVisible) {
        poseWidget->setCornerButtonVisible(selected);
    }
}

void PoseListWidget::selectPose(QUuid poseId, bool multiple)
{
    if (multiple) {
        if (!m_currentSelectedPoseId.isNull()) {
            m_selectedPoseIds.insert(m_currentSelectedPoseId);
            m_currentSelectedPoseId = QUuid();
        }
        if (m_selectedPoseIds.find(poseId) != m_selectedPoseIds.end()) {
            updatePoseSelectState(poseId, false);
            m_selectedPoseIds.erase(poseId);
        } else {
            updatePoseSelectState(poseId, true);
            m_selectedPoseIds.insert(poseId);
        }
        if (m_selectedPoseIds.size() > 1) {
            return;
        }
        if (m_selectedPoseIds.size() == 1)
            poseId = *m_selectedPoseIds.begin();
        else
            poseId = QUuid();
    }
    if (!m_selectedPoseIds.empty()) {
        for (const auto &id: m_selectedPoseIds) {
            updatePoseSelectState(id, false);
        }
        m_selectedPoseIds.clear();
    }
    if (m_currentSelectedPoseId != poseId) {
        if (!m_currentSelectedPoseId.isNull()) {
            updatePoseSelectState(m_currentSelectedPoseId, false);
        }
        m_currentSelectedPoseId = poseId;
        if (!m_currentSelectedPoseId.isNull()) {
            updatePoseSelectState(m_currentSelectedPoseId, true);
        }
    }
}

void PoseListWidget::mousePressEvent(QMouseEvent *event)
{
    QModelIndex itemIndex = indexAt(event->pos());
    QTreeView::mousePressEvent(event);
    if (event->button() == Qt::LeftButton) {
        bool multiple = QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier);
        if (itemIndex.isValid()) {
            QTreeWidgetItem *item = itemFromIndex(itemIndex);
            auto poseId = QUuid(item->data(itemIndex.column(), Qt::UserRole).toString());
            if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier)) {
                bool startAdd = false;
                bool stopAdd = false;
                std::vector<QUuid> waitQueue;
                for (const auto &childId: m_document->poseIdList) {
                    if (m_shiftStartPoseId == childId || poseId == childId) {
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
                    if (!m_selectedPoseIds.empty()) {
                        for (const auto &id: m_selectedPoseIds) {
                            updatePoseSelectState(id, false);
                        }
                        m_selectedPoseIds.clear();
                    }
                    if (!m_currentSelectedPoseId.isNull()) {
                        m_currentSelectedPoseId = QUuid();
                    }
                    for (const auto &waitId: waitQueue) {
                        selectPose(waitId, true);
                    }
                }
                return;
            } else {
                m_shiftStartPoseId = poseId;
            }
            selectPose(poseId, multiple);
            return;
        }
        if (!multiple)
            selectPose(QUuid());
    }
}

bool PoseListWidget::isPoseSelected(QUuid poseId)
{
    return (m_currentSelectedPoseId == poseId ||
        m_selectedPoseIds.find(poseId) != m_selectedPoseIds.end());
}

void PoseListWidget::showContextMenu(const QPoint &pos)
{
    if (!m_hasContextMenu)
        return;
    
    QMenu contextMenu(this);
    
    std::set<QUuid> unorderedPoseIds = m_selectedPoseIds;
    if (!m_currentSelectedPoseId.isNull())
        unorderedPoseIds.insert(m_currentSelectedPoseId);
    
    std::vector<QUuid> poseIds;
    for (const auto &cand: m_document->poseIdList) {
        if (unorderedPoseIds.find(cand) != unorderedPoseIds.end())
            poseIds.push_back(cand);
    }
    
    QAction modifyAction(tr("Modify"), this);
    if (poseIds.size() == 1) {
        connect(&modifyAction, &QAction::triggered, this, [=]() {
            emit modifyPose(*poseIds.begin());
        });
        contextMenu.addAction(&modifyAction);
    }
    
    QAction copyAction(tr("Copy"), this);
    if (!poseIds.empty()) {
        connect(&copyAction, &QAction::triggered, this, &PoseListWidget::copy);
        contextMenu.addAction(&copyAction);
    }
    
    QAction pasteAction(tr("Paste"), this);
    if (m_document->hasPastablePosesInClipboard()) {
        connect(&pasteAction, &QAction::triggered, m_document, &Document::paste);
        contextMenu.addAction(&pasteAction);
    }
    
    QAction deleteAction(tr("Delete"), this);
    if (!poseIds.empty()) {
        connect(&deleteAction, &QAction::triggered, [=]() {
            for (const auto &poseId: poseIds)
                emit removePose(poseId);
        });
        contextMenu.addAction(&deleteAction);
    }
    
    contextMenu.exec(mapToGlobal(pos));
}

void PoseListWidget::resizeEvent(QResizeEvent *event)
{
    QTreeWidget::resizeEvent(event);
    if (calculateColumnCount() != columnCount())
        reload();
}

int PoseListWidget::calculateColumnCount()
{
    if (nullptr == parentWidget())
        return 0;
    
    int columns = parentWidget()->width() / Theme::posePreviewImageSize;
    if (0 == columns)
        columns = 1;
    return columns;
}

void PoseListWidget::reload()
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
    
    std::vector<QUuid> orderedPoseIdList = m_document->poseIdList;
    std::sort(orderedPoseIdList.begin(), orderedPoseIdList.end(), [&](const QUuid &firstPoseId, const QUuid &secondPoseId) {
        const auto *firstPose = m_document->findPose(firstPoseId);
        const auto *secondPose = m_document->findPose(secondPoseId);
        if (nullptr == firstPose || nullptr == secondPose)
            return false;
        return QString::compare(firstPose->name, secondPose->name, Qt::CaseInsensitive) < 0;
    });
    
    decltype(orderedPoseIdList.size()) poseIndex = 0;
    while (poseIndex < orderedPoseIdList.size()) {
        QTreeWidgetItem *item = new QTreeWidgetItem(this);
        item->setFlags((item->flags() | Qt::ItemIsEnabled) & ~(Qt::ItemIsSelectable) & ~(Qt::ItemIsEditable));
        for (int col = 0; col < columns && poseIndex < orderedPoseIdList.size(); col++, poseIndex++) {
            const auto &poseId = orderedPoseIdList[poseIndex];
            item->setSizeHint(col, QSize(columnWidth, PoseWidget::preferredHeight() + 2));
            item->setData(col, Qt::UserRole, poseId.toString());
            PoseWidget *widget = new PoseWidget(m_document, poseId);
            connect(widget, &PoseWidget::modifyPose, this, &PoseListWidget::modifyPose);
            connect(widget, &PoseWidget::cornerButtonClicked, this, &PoseListWidget::cornerButtonClicked);
            setItemWidget(item, col, widget);
            widget->reload();
            widget->updateCheckedState(isPoseSelected(poseId));
            m_itemMap[poseId] = std::make_pair(item, col);
        }
        invisibleRootItem()->addChild(item);
    }
}

void PoseListWidget::setCornerButtonVisible(bool visible)
{
    m_cornerButtonVisible = visible;
}

void PoseListWidget::setHasContextMenu(bool hasContextMenu)
{
    m_hasContextMenu = hasContextMenu;
}

void PoseListWidget::removeAllContent()
{
    m_itemMap.clear();
    clear();
}

void PoseListWidget::copy()
{
    if (m_selectedPoseIds.empty() && m_currentSelectedPoseId.isNull())
        return;
    
    std::set<QUuid> limitPoseIds = m_selectedPoseIds;
    if (!m_currentSelectedPoseId.isNull())
        limitPoseIds.insert(m_currentSelectedPoseId);
    
    std::set<QUuid> emptySet;
    
    Snapshot snapshot;
    m_document->toSnapshot(&snapshot, emptySet, DocumentToSnapshotFor::Poses,
        limitPoseIds);
    QString snapshotXml;
    QXmlStreamWriter xmlStreamWriter(&snapshotXml);
    saveSkeletonToXmlStream(&snapshot, &xmlStreamWriter);
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(snapshotXml);
}
