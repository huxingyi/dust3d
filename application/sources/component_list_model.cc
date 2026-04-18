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
                if (nullptr != part && dust3d::PartTarget::Model != part->target) {
                    QPixmap badgedPixmap(pixmap);
                    QPainter painter(&badgedPixmap);
                    QString badgeText = dust3d::PartTarget::CutFace == part->target
                        ? tr("Cut Face")
                        : tr("Stitch");
                    QColor badgeColor = dust3d::PartTarget::CutFace == part->target
                        ? QColor(Theme::red.red(), Theme::red.green(), Theme::red.blue(), 210)
                        : QColor(Theme::blue.red(), Theme::blue.green(), Theme::blue.blue(), 210);
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
    // Use the default implementation from QAbstractListModel
    return QAbstractListModel::mimeData(indexes);
}

bool ComponentListModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int /*row*/, int /*column*/, const QModelIndex& /*parent*/)
{
    if (!data->hasFormat("application/x-qabstractitemmodeldatalist"))
        return false;

    // Accept the drop - actual reordering is handled in the view's dropEvent
    return true;
}
