#include "component_list_model.h"
#include "theme.h"
#include <QAbstractListModel>
#include <QGuiApplication>
#include <QMimeData>
#include <QPainter>
#include <dust3d/base/debug.h>

ComponentListModel::ComponentListModel(const Document* document, QObject* parent)
    : QAbstractListModel(parent)
    , m_document(document)
{
    connect(m_document, &Document::componentPreviewPixmapChanged, [this](const dust3d::Uuid& componentId) {
        auto findIndex = this->m_componentIdToIndexMap.find(componentId);
        if (findIndex != this->m_componentIdToIndexMap.end()) {
            //dust3dDebug << "dataChanged:" << findIndex->second.row();
            emit this->dataChanged(findIndex->second, findIndex->second);
        }
    });
    connect(m_document, &Document::cleanup, [this]() {
        this->setListingComponentId(dust3d::Uuid());
        this->reload();
    });
    connect(m_document, &Document::componentChildrenChanged, [this](const dust3d::Uuid& componentId) {
        if (componentId != this->listingComponentId())
            return;
        this->reload();
    });
    connect(this, &ComponentListModel::listingComponentChanged, m_document, &Document::setCurrentCanvasComponentId);
}

void ComponentListModel::reload()
{
    beginResetModel();
    m_componentIdToIndexMap.clear();
    const Document::Component* listingComponent = m_document->findComponent(m_listingComponentId);
    if (nullptr != listingComponent) {
        for (int i = 0; i < (int)listingComponent->childrenIds.size(); ++i) {
            m_componentIdToIndexMap[listingComponent->childrenIds[i]] = createIndex(i, 0);
        }
    }
    endResetModel();
    emit layoutChanged();
}

QModelIndex ComponentListModel::componentIdToIndex(const dust3d::Uuid& componentId) const
{
    auto findIndex = m_componentIdToIndexMap.find(componentId);
    if (findIndex == m_componentIdToIndexMap.end())
        return QModelIndex();
    return findIndex->second;
}

int ComponentListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    const Document::Component* listingComponent = m_document->findComponent(m_listingComponentId);
    if (nullptr == listingComponent)
        return 0;
    return (int)listingComponent->childrenIds.size();
}

int ComponentListModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return 1;
}

const Document::Component* ComponentListModel::modelIndexToComponent(const QModelIndex& index) const
{
    const Document::Component* listingComponent = m_document->findComponent(m_listingComponentId);
    if (nullptr == listingComponent)
        return nullptr;
    if (index.row() >= (int)listingComponent->childrenIds.size()) {
        dust3dDebug << "Component list row is out of range, size:" << listingComponent->childrenIds.size() << "row:" << index.row();
        return nullptr;
    }
    const auto& componentId = listingComponent->childrenIds[index.row()];
    const Document::Component* component = m_document->findComponent(componentId);
    if (nullptr == component) {
        dust3dDebug << "Component not found:" << componentId.toString();
        return nullptr;
    }
    return component;
}

const dust3d::Uuid ComponentListModel::modelIndexToComponentId(const QModelIndex& index) const
{
    const Document::Component* listingComponent = m_document->findComponent(m_listingComponentId);
    if (nullptr == listingComponent)
        return dust3d::Uuid();
    if (index.row() >= (int)listingComponent->childrenIds.size()) {
        dust3dDebug << "Component list row is out of range, size:" << listingComponent->childrenIds.size() << "row:" << index.row();
        return dust3d::Uuid();
    }
    const auto& componentId = listingComponent->childrenIds[index.row()];
    return componentId;
}

QVariant ComponentListModel::data(const QModelIndex& index, int role) const
{
    switch (role) {
    case Qt::ToolTipRole: {
        const Document::Component* component = modelIndexToComponent(index);
        if (nullptr != component) {
            return component->name;
        }
    } break;
    case Qt::DecorationRole: {
        const Document::Component* component = modelIndexToComponent(index);
        if (nullptr != component) {
            QPixmap pixmap;
            if (0 == component->previewPixmap.width()) {
                static QPixmap s_emptyPixmap;
                if (0 == s_emptyPixmap.width()) {
                    qreal dpr = qApp->devicePixelRatio();
                    int physicalSize = qRound(Theme::partPreviewImageSize * dpr);
                    QImage image(physicalSize, physicalSize, QImage::Format_ARGB32);
                    image.fill(Qt::transparent);
                    s_emptyPixmap = QPixmap::fromImage(image);
                    s_emptyPixmap.setDevicePixelRatio(dpr);
                }
                pixmap = s_emptyPixmap;
            } else {
                pixmap = component->previewPixmap;
            }
            if (!component->linkToPartId.isNull()) {
                const Document::Part* part = m_document->findPart(component->linkToPartId);
                if (nullptr != part && (dust3d::PartTarget::Model != part->target || dust3d::PartGenerator::None != part->generator)) {
                    QPixmap badgedPixmap(pixmap);
                    QPainter painter(&badgedPixmap);
                    QString badgeText;
                    QColor badgeColor;
                    if (dust3d::PartGenerator::Imported == part->generator) {
                        badgeText = tr("Imported");
                        badgeColor = QColor(Theme::green.red(), Theme::green.green(), Theme::green.blue(), 210);
                    } else if (dust3d::PartGenerator::Rock == part->generator) {
                        badgeText = tr("Rock");
                        badgeColor = QColor(Theme::green.red(), Theme::green.green(), Theme::green.blue(), 210);
                    } else if (dust3d::PartTarget::CutFace == part->target) {
                        badgeText = tr("Cut Face");
                        badgeColor = QColor(Theme::red.red(), Theme::red.green(), Theme::red.blue(), 210);
                    } else if (dust3d::PartTarget::StitchingLine == part->target) {
                        // Compute sequence number among stitching line siblings
                        int seqNum = 0, seqTotal = 0;
                        const Document::Component* listingComp = m_document->findComponent(m_listingComponentId);
                        if (listingComp) {
                            for (const auto& sibId : listingComp->childrenIds) {
                                const Document::Component* sib = m_document->findComponent(sibId);
                                if (!sib)
                                    continue;
                                const Document::Part* sibPart = m_document->findPart(sib->linkToPartId);
                                if (sibPart && dust3d::PartTarget::StitchingLine == sibPart->target) {
                                    ++seqTotal;
                                    if (sibId == component->id)
                                        seqNum = seqTotal;
                                }
                            }
                        }
                        badgeText = tr("Stitching Line");
                        badgeColor = QColor(Theme::blue.red(), Theme::blue.green(), Theme::blue.blue(), 210);
                        // Draw sequence number in top-right corner
                        if (seqTotal > 1 && seqNum > 0) {
                            QString seqText = QString::number(seqNum);
                            QFont seqFont = painter.font();
                            seqFont.setPixelSize(QGuiApplication::font().pixelSize() * 0.7);
                            seqFont.setBold(true);
                            painter.setFont(seqFont);
                            QFontMetrics seqFm(seqFont);
                            int seqW = seqFm.horizontalAdvance(seqText) + 6;
                            int seqH = seqFm.height() + 4;
                            int logW = qRound(badgedPixmap.width() / badgedPixmap.devicePixelRatio());
                            int sx = logW - seqW - 4;
                            int sy = 4;
                            painter.fillRect(sx, sy, seqW, seqH,
                                QColor(Theme::blue.red(), Theme::blue.green(), Theme::blue.blue(), 220));
                            painter.setPen(Qt::white);
                            painter.drawText(sx + 3, sy + 2 + seqFm.ascent(), seqText);
                        }
                    } else if (dust3d::PartTarget::StitchingLoop == part->target) {
                        badgeText = tr("Stitching Loop");
                        badgeColor = QColor(Theme::blue.red(), Theme::blue.green(), Theme::blue.blue(), 210);
                    } else {
                        badgeText = tr("Unknown");
                        badgeColor = QColor(128, 128, 128, 210);
                    }
                    QFont font = painter.font();
                    font.setPixelSize(QGuiApplication::font().pixelSize() * 0.63);
                    font.setBold(true);
                    painter.setFont(font);
                    QFontMetrics fm(font);
                    int textWidth = fm.horizontalAdvance(badgeText);
                    int padding = 2;
                    int bw = textWidth + padding * 2;
                    int bh = fm.height() + padding * 2;
                    int logicalWidth = qRound(badgedPixmap.width() / badgedPixmap.devicePixelRatio());
                    int logicalHeight = qRound(badgedPixmap.height() / badgedPixmap.devicePixelRatio());
                    int x = 2;
                    int y = logicalHeight - bh - 5;
                    painter.fillRect(x, y, logicalWidth - 4, bh, badgeColor);
                    painter.setPen(Qt::white);
                    painter.drawText(x + padding, y + padding + fm.ascent(), badgeText);
                    badgedPixmap.setDevicePixelRatio(pixmap.devicePixelRatio());
                    return badgedPixmap;
                }
            }
            return pixmap;
        }
    } break;
    }
    return QVariant();
}

void ComponentListModel::setListingComponentId(const dust3d::Uuid& componentId)
{
    if (m_listingComponentId == componentId)
        return;
    m_listingComponentId = componentId;
    reload();
    emit listingComponentChanged(m_listingComponentId);
}

const dust3d::Uuid ComponentListModel::listingComponentId() const
{
    return m_listingComponentId;
}

Qt::DropActions ComponentListModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

Qt::ItemFlags ComponentListModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);
    if (index.isValid()) {
        // Allow items to be dragged and dropped
        return defaultFlags | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    }
    // Root index - allow drop
    return defaultFlags | Qt::ItemIsDropEnabled;
}

QStringList ComponentListModel::mimeTypes() const
{
    return QStringList() << "application/x-qabstractitemmodeldatalist";
}

QMimeData* ComponentListModel::mimeData(const QModelIndexList& indexes) const
{
    QMimeData* data = QAbstractListModel::mimeData(indexes);
    QByteArray encodedIds;
    for (const auto& index : indexes) {
        dust3d::Uuid componentId = modelIndexToComponentId(index);
        if (!componentId.isNull()) {
            if (!encodedIds.isEmpty())
                encodedIds.append(';');
            encodedIds.append(componentId.toString().c_str());
        }
    }
    data->setData("application/x-dust3d-component-ids", encodedIds);
    return data;
}

bool ComponentListModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int /*row*/, int /*column*/, const QModelIndex& /*parent*/)
{
    if (!data->hasFormat("application/x-qabstractitemmodeldatalist"))
        return false;

    // Accept the drop - actual reordering is handled in the view's dropEvent
    return true;
}

bool ComponentListModel::hasUngroupedStitchingParts() const
{
    // Only relevant at root level
    if (!m_listingComponentId.isNull())
        return false;
    const Document::Component* listingComponent = m_document->findComponent(m_listingComponentId);
    if (!listingComponent)
        return false;
    for (const auto& childId : listingComponent->childrenIds) {
        const Document::Component* child = m_document->findComponent(childId);
        if (!child)
            continue;
        const Document::Part* part = m_document->findPart(child->linkToPartId);
        if (part && (dust3d::PartTarget::StitchingLine == part->target || dust3d::PartTarget::StitchingLoop == part->target))
            return true;
    }
    return false;
}

bool ComponentListModel::hasStitchingLoopInfo() const
{
    const Document::Component* listingComponent = m_document->findComponent(m_listingComponentId);
    if (!listingComponent)
        return false;
    for (const auto& childId : listingComponent->childrenIds) {
        const Document::Component* child = m_document->findComponent(childId);
        if (!child)
            continue;
        const Document::Part* part = m_document->findPart(child->linkToPartId);
        if (part && dust3d::PartTarget::StitchingLoop == part->target)
            return true;
    }
    return false;
}

bool ComponentListModel::hasStitchingLineOrdering() const
{
    int count = 0;
    const Document::Component* listingComponent = m_document->findComponent(m_listingComponentId);
    if (!listingComponent)
        return false;
    for (const auto& childId : listingComponent->childrenIds) {
        const Document::Component* child = m_document->findComponent(childId);
        if (!child)
            continue;
        const Document::Part* part = m_document->findPart(child->linkToPartId);
        if (part && dust3d::PartTarget::StitchingLine == part->target)
            ++count;
    }
    return count > 1;
}
