#include "component_breadcrumb_widget.h"
#include "document.h"
#include "theme.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>

ComponentBreadcrumbWidget::ComponentBreadcrumbWidget(Document* document, QWidget* parent)
    : QWidget(parent)
    , m_document(document)
{
    setAcceptDrops(true);
    setFixedHeight(thumbnailSize() + 8);
    setVisible(false);
}

int ComponentBreadcrumbWidget::thumbnailSize() const
{
    return 24;
}

void ComponentBreadcrumbWidget::setCurrentComponentId(const dust3d::Uuid& componentId)
{
    if (m_currentComponentId == componentId)
        return;
    m_currentComponentId = componentId;
    setVisible(!m_currentComponentId.isNull());
    rebuildPath();
    update();
}

void ComponentBreadcrumbWidget::updateThumbnails()
{
    rebuildPath();
    update();
}

void ComponentBreadcrumbWidget::rebuildPath()
{
    m_items.clear();

    std::vector<dust3d::Uuid> path;
    dust3d::Uuid walkId = m_currentComponentId;
    while (true) {
        const Document::Component* parent = m_document->findComponentParent(walkId);
        if (nullptr == parent)
            break;
        path.push_back(walkId);
        walkId = parent->id;
    }
    path.push_back(dust3d::Uuid());

    std::reverse(path.begin(), path.end());

    int ts = thumbnailSize();
    int separatorWidth = 12;
    int x = 4;
    int y = (height() - ts) / 2;

    for (size_t i = 0; i < path.size(); ++i) {
        BreadcrumbItem item;
        item.componentId = path[i];

        const Document::Component* comp = m_document->findComponent(path[i]);
        if (nullptr != comp && !comp->previewPixmap.isNull()) {
            item.thumbnail = comp->previewPixmap.scaled(
                ts * devicePixelRatioF(), ts * devicePixelRatioF(),
                Qt::KeepAspectRatio, Qt::SmoothTransformation);
            item.thumbnail.setDevicePixelRatio(devicePixelRatioF());
        }

        item.rect = QRect(x, y, ts, ts);
        m_items.push_back(item);

        x += ts + separatorWidth;
    }
}

void ComponentBreadcrumbWidget::paintEvent(QPaintEvent*)
{
    if (m_items.empty())
        return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    int ts = thumbnailSize();

    for (size_t i = 0; i < m_items.size(); ++i) {
        const auto& item = m_items[i];

        if (m_hoveredDropIndex == (int)i) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(Theme::red.red(), Theme::red.green(), Theme::red.blue(), 80));
            painter.drawRoundedRect(item.rect.adjusted(-2, -2, 2, 2), 3, 3);
        }

        painter.setPen(Qt::NoPen);
        painter.setBrush(Theme::gray);
        painter.drawRoundedRect(item.rect, 3, 3);

        if (!item.thumbnail.isNull()) {
            painter.drawPixmap(item.rect, item.thumbnail);
        } else {
            painter.setPen(Qt::NoPen);
            painter.setBrush(Theme::altDarkBackground);
            painter.drawRoundedRect(item.rect, 3, 3);
        }

        bool isLast = (i == m_items.size() - 1);
        if (isLast) {
            painter.setPen(QPen(Theme::red, 1.5));
            painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(item.rect.adjusted(-1, -1, 1, 1), 3, 3);
        }

        if (i + 1 < m_items.size()) {
            int arrowX = item.rect.right() + 3;
            int arrowY = item.rect.center().y();
            painter.setPen(QPen(Theme::midGray, 1.5));
            painter.drawLine(arrowX, arrowY - 3, arrowX + 4, arrowY);
            painter.drawLine(arrowX, arrowY + 3, arrowX + 4, arrowY);
        }
    }
}

void ComponentBreadcrumbWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
        return;

    int index = itemAt(event->pos());
    if (index >= 0 && index < (int)m_items.size()) {
        emit navigateToComponent(m_items[index].componentId);
    }
}

int ComponentBreadcrumbWidget::itemAt(const QPoint& pos) const
{
    for (int i = 0; i < (int)m_items.size(); ++i) {
        if (m_items[i].rect.contains(pos))
            return i;
    }
    return -1;
}

static bool hasDragComponentIds(const QMimeData* mimeData)
{
    return mimeData->hasFormat("application/x-dust3d-component-ids");
}

static std::vector<dust3d::Uuid> parseDragComponentIds(const QMimeData* mimeData)
{
    std::vector<dust3d::Uuid> ids;
    QByteArray data = mimeData->data("application/x-dust3d-component-ids");
    if (data.isEmpty())
        return ids;
    for (const auto& part : data.split(';')) {
        dust3d::Uuid id(std::string(part.constData(), part.size()));
        if (!id.isNull())
            ids.push_back(id);
    }
    return ids;
}

void ComponentBreadcrumbWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (hasDragComponentIds(event->mimeData()))
        event->acceptProposedAction();
}

void ComponentBreadcrumbWidget::dragMoveEvent(QDragMoveEvent* event)
{
    if (!hasDragComponentIds(event->mimeData()))
        return;

    event->acceptProposedAction();
    int oldHovered = m_hoveredDropIndex;
    m_hoveredDropIndex = itemAt(event->pos());
    if (m_hoveredDropIndex != oldHovered)
        update();
}

void ComponentBreadcrumbWidget::dragLeaveEvent(QDragLeaveEvent*)
{
    m_hoveredDropIndex = -1;
    update();
}

void ComponentBreadcrumbWidget::dropEvent(QDropEvent* event)
{
    int index = itemAt(event->pos());
    m_hoveredDropIndex = -1;
    update();

    if (index < 0 || index >= (int)m_items.size()) {
        event->ignore();
        return;
    }

    std::vector<dust3d::Uuid> componentIds = parseDragComponentIds(event->mimeData());
    if (componentIds.empty()) {
        event->ignore();
        return;
    }

    const dust3d::Uuid& targetParentId = m_items[index].componentId;

    // Filter out components that are already direct children of the target
    std::vector<dust3d::Uuid> toMove;
    for (const auto& id : componentIds) {
        dust3d::Uuid parentId = m_document->findComponentParentId(id);
        if (parentId != targetParentId)
            toMove.push_back(id);
    }

    if (toMove.empty()) {
        event->accept();
        return;
    }

    const Document::Component* targetComponent = m_document->findComponent(targetParentId);

    QDialog dlg(this);
    dlg.setWindowTitle(tr("Move into Group"));

    QVBoxLayout* vLayout = new QVBoxLayout(&dlg);

    QLabel* questionLabel = new QLabel(tr("Do you want to move the selected component(s) into the group?"), &dlg);
    questionLabel->setWordWrap(true);
    vLayout->addWidget(questionLabel);

    QHBoxLayout* previewLayout = new QHBoxLayout;
    previewLayout->setSpacing(8);

    auto makePreviewLabel = [&dlg](const QPixmap& pixmap) -> QLabel* {
        QLabel* lbl = new QLabel(&dlg);
        lbl->setFixedSize(64, 64);
        lbl->setAlignment(Qt::AlignCenter);
        if (!pixmap.isNull())
            lbl->setPixmap(pixmap.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        else
            lbl->setText(tr("?"));
        return lbl;
    };

    const Document::Component* srcComponent = m_document->findComponent(toMove.front());
    QPixmap srcPixmap = srcComponent ? srcComponent->previewPixmap : QPixmap();
    previewLayout->addWidget(makePreviewLabel(srcPixmap));

    QLabel* arrowLabel = new QLabel(QString::fromUtf8("\xe2\x86\x92"), &dlg);
    arrowLabel->setAlignment(Qt::AlignCenter);
    previewLayout->addWidget(arrowLabel);

    QPixmap targetPixmap = targetComponent ? targetComponent->previewPixmap : QPixmap();
    previewLayout->addWidget(makePreviewLabel(targetPixmap));
    previewLayout->addStretch();
    vLayout->addLayout(previewLayout);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Yes | QDialogButtonBox::No, &dlg);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    vLayout->addWidget(buttons);

    if (dlg.exec() == QDialog::Accepted) {
        for (const auto& componentId : toMove)
            m_document->moveComponent(componentId, targetParentId);
        m_document->saveSnapshot();
    }

    event->accept();
}
