#include <QGuiApplication>
#include <QMenu>
#include <QApplication>
#include "cutfacelistwidget.h"

CutFaceListWidget::CutFaceListWidget(const Document *document, QWidget *parent) :
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

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QTreeWidget::customContextMenuRequested, this, &CutFaceListWidget::showContextMenu);
    
    std::set<QUuid> cutFacePartIds;
    for (const auto &it: m_document->partMap) {
        if (PartTarget::CutFace == it.second.target) {
            if (cutFacePartIds.find(it.first) != cutFacePartIds.end())
                continue;
            cutFacePartIds.insert(it.first);
            m_cutFacePartIdList.push_back(it.first);
        }
        if (!it.second.cutFaceLinkedId.isNull()) {
            if (cutFacePartIds.find(it.second.cutFaceLinkedId) != cutFacePartIds.end())
                continue;
            cutFacePartIds.insert(it.second.cutFaceLinkedId);
            m_cutFacePartIdList.push_back(it.second.cutFaceLinkedId);
        }
    }

    reload();
}

bool CutFaceListWidget::isEmpty()
{
    return m_cutFacePartIdList.empty();
}

void CutFaceListWidget::enableMultipleSelection(bool enabled)
{
    m_multipleSelectionEnabled = enabled;
}

void CutFaceListWidget::updateCutFaceSelectState(QUuid partId, bool selected)
{
    auto findItemResult = m_itemMap.find(partId);
    if (findItemResult == m_itemMap.end()) {
        qDebug() << "Find part item failed:" << partId;
        return;
    }
    CutFaceWidget *cutFaceWidget = (CutFaceWidget *)itemWidget(findItemResult->second.first, findItemResult->second.second);
    cutFaceWidget->updateCheckedState(selected);
}

void CutFaceListWidget::selectCutFace(QUuid partId, bool multiple)
{
    if (multiple) {
        if (!m_currentSelectedPartId.isNull()) {
            m_selectedPartIds.insert(m_currentSelectedPartId);
            m_currentSelectedPartId = QUuid();
        }
        if (m_selectedPartIds.find(partId) != m_selectedPartIds.end()) {
            updateCutFaceSelectState(partId, false);
            m_selectedPartIds.erase(partId);
        } else {
            if (m_multipleSelectionEnabled || m_selectedPartIds.empty()) {
                updateCutFaceSelectState(partId, true);
                m_selectedPartIds.insert(partId);
            }
        }
        if (m_selectedPartIds.size() > 1) {
            return;
        }
        if (m_selectedPartIds.size() == 1)
            partId = *m_selectedPartIds.begin();
        else {
            partId = QUuid();
            emit currentSelectedCutFaceChanged(partId);
        }
    }
    if (!m_selectedPartIds.empty()) {
        for (const auto &id: m_selectedPartIds) {
            updateCutFaceSelectState(id, false);
        }
        m_selectedPartIds.clear();
    }
    if (m_currentSelectedPartId != partId) {
        if (!m_currentSelectedPartId.isNull()) {
            updateCutFaceSelectState(m_currentSelectedPartId, false);
        }
        m_currentSelectedPartId = partId;
        if (!m_currentSelectedPartId.isNull()) {
            updateCutFaceSelectState(m_currentSelectedPartId, true);
        }
        emit currentSelectedCutFaceChanged(m_currentSelectedPartId);
    }
}

void CutFaceListWidget::mousePressEvent(QMouseEvent *event)
{
    QModelIndex itemIndex = indexAt(event->pos());
    QTreeView::mousePressEvent(event);
    if (event->button() == Qt::LeftButton) {
        bool multiple = QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier);
        if (itemIndex.isValid()) {
            QTreeWidgetItem *item = itemFromIndex(itemIndex);
            auto partId = QUuid(item->data(itemIndex.column(), Qt::UserRole).toString());
            if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier)) {
                bool startAdd = false;
                bool stopAdd = false;
                std::vector<QUuid> waitQueue;
                for (const auto &childId: m_cutFacePartIdList) {
                    if (m_shiftStartPartId == childId || partId == childId) {
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
                    if (!m_selectedPartIds.empty()) {
                        for (const auto &id: m_selectedPartIds) {
                            updateCutFaceSelectState(id, false);
                        }
                        m_selectedPartIds.clear();
                    }
                    if (!m_currentSelectedPartId.isNull()) {
                        m_currentSelectedPartId = QUuid();
                    }
                    for (const auto &waitId: waitQueue) {
                        selectCutFace(waitId, true);
                    }
                }
                return;
            } else {
                m_shiftStartPartId = partId;
            }
            selectCutFace(partId, multiple);
            return;
        }
        if (!multiple)
            selectCutFace(QUuid());
    }
}

bool CutFaceListWidget::isCutFaceSelected(QUuid partId)
{
    return (m_currentSelectedPartId == partId ||
        m_selectedPartIds.find(partId) != m_selectedPartIds.end());
}

void CutFaceListWidget::showContextMenu(const QPoint &pos)
{
    if (!m_hasContextMenu)
        return;
}

void CutFaceListWidget::resizeEvent(QResizeEvent *event)
{
    QTreeWidget::resizeEvent(event);
    if (calculateColumnCount() != columnCount())
        reload();
}

int CutFaceListWidget::calculateColumnCount()
{
    if (nullptr == parentWidget())
        return 0;

    int columns = parentWidget()->width() / Theme::cutFacePreviewImageSize;
    if (0 == columns)
        columns = 1;
    return columns;
}

void CutFaceListWidget::reload()
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

    decltype(m_cutFacePartIdList.size()) cutFaceIndex = 0;
    while (cutFaceIndex < m_cutFacePartIdList.size()) {
        QTreeWidgetItem *item = new QTreeWidgetItem(this);
        item->setFlags((item->flags() | Qt::ItemIsEnabled) & ~(Qt::ItemIsSelectable) & ~(Qt::ItemIsEditable));
        for (int col = 0; col < columns && cutFaceIndex < m_cutFacePartIdList.size(); col++, cutFaceIndex++) {
            const auto &partId = m_cutFacePartIdList[cutFaceIndex];
            item->setSizeHint(col, QSize(columnWidth, CutFaceWidget::preferredHeight() + 2));
            item->setData(col, Qt::UserRole, partId.toString());
            CutFaceWidget *widget = new CutFaceWidget(m_document, partId);
            setItemWidget(item, col, widget);
            widget->reload();
            widget->updateCheckedState(isCutFaceSelected(partId));
            m_itemMap[partId] = std::make_pair(item, col);
        }
        invisibleRootItem()->addChild(item);
    }
}

void CutFaceListWidget::setHasContextMenu(bool hasContextMenu)
{
    m_hasContextMenu = hasContextMenu;
}

void CutFaceListWidget::removeAllContent()
{
    m_itemMap.clear();
    clear();
}
