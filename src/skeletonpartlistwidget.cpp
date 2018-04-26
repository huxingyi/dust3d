#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>
#include <QMenu>
#include <QWidgetAction>
#include <QScrollBar>
#include <QColorDialog>
#include "skeletonpartlistwidget.h"
#include "theme.h"
#include "floatnumberwidget.h"

SkeletonPartWidget::SkeletonPartWidget(const SkeletonDocument *document, QUuid partId) :
    m_document(document),
    m_partId(partId)
{
    m_visibleButton = new QPushButton();
    initButton(m_visibleButton);
    updateVisibleButton();
    
    m_lockButton = new QPushButton();
    initButton(m_lockButton);
    updateLockButton();
    
    m_subdivButton = new QPushButton();
    initButton(m_subdivButton);
    updateSubdivButton();
    
    m_disableButton = new QPushButton();
    initButton(m_disableButton);
    updateDisableButton();
    
    m_xMirrorButton = new QPushButton();
    initButton(m_xMirrorButton);
    updateXmirrorButton();
    
    m_deformButton = new QPushButton();
    initButton(m_deformButton);
    updateDeformButton();
    
    m_roundButton = new QPushButton;
    initButton(m_roundButton);
    updateRoundButton();
    
    m_colorButton = new QPushButton;
    initButton(m_colorButton);
    updateColorButton();
    
    m_previewLabel = new QLabel;
    
    QHBoxLayout *miniTopToolLayout = new QHBoxLayout;
    miniTopToolLayout->setSpacing(0);
    miniTopToolLayout->setContentsMargins(0, 0, 0, 0);
    miniTopToolLayout->addSpacing(5);
    miniTopToolLayout->addWidget(m_visibleButton);
    miniTopToolLayout->addWidget(m_disableButton);
    miniTopToolLayout->addWidget(m_lockButton);
    miniTopToolLayout->addWidget(m_colorButton);
    miniTopToolLayout->addStretch();
    
    QHBoxLayout *miniBottomToolLayout = new QHBoxLayout;
    miniBottomToolLayout->setSpacing(0);
    miniBottomToolLayout->setContentsMargins(0, 0, 0, 0);
    miniBottomToolLayout->addSpacing(5);
    miniBottomToolLayout->addWidget(m_subdivButton);
    miniBottomToolLayout->addWidget(m_deformButton);
    miniBottomToolLayout->addWidget(m_xMirrorButton);
    miniBottomToolLayout->addWidget(m_roundButton);
    miniBottomToolLayout->addStretch();
    
    QWidget *hrLightWidget = new QWidget;
    hrLightWidget->setFixedHeight(1);
    hrLightWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    hrLightWidget->setStyleSheet(QString("background-color: #565656;"));
    hrLightWidget->setContentsMargins(0, 0, 0, 0);
    
    QWidget *hrWidget = new QWidget;
    hrWidget->setFixedHeight(1);
    hrWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    hrWidget->setStyleSheet(QString("background-color: #1a1a1a;"));
    hrWidget->setContentsMargins(0, 0, 0, 0);
    
    QVBoxLayout *toolsLayout = new QVBoxLayout;
    toolsLayout->setSpacing(0);
    toolsLayout->setContentsMargins(0, 0, 0, 0);
    toolsLayout->addSpacing(4);
    toolsLayout->addLayout(miniTopToolLayout);
    toolsLayout->addWidget(m_previewLabel);
    toolsLayout->addLayout(miniBottomToolLayout);
    toolsLayout->addSpacing(4);
    
    QWidget *backgroundWidget = new QWidget;
    backgroundWidget->setObjectName("background");
    m_backgroundWidget = backgroundWidget;
    backgroundWidget->setLayout(toolsLayout);
    
    QHBoxLayout *backgroundLayout = new QHBoxLayout;
    backgroundLayout->setSpacing(0);
    backgroundLayout->setContentsMargins(0, 0, 0, 0);
    backgroundLayout->addWidget(backgroundWidget);
    backgroundLayout->addSpacing(2);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(hrLightWidget);
    mainLayout->addSpacing(3);
    mainLayout->addLayout(backgroundLayout);
    mainLayout->addSpacing(3);
    mainLayout->addWidget(hrWidget);
    
    setLayout(mainLayout);
    
    connect(this, &SkeletonPartWidget::setPartLockState, m_document, &SkeletonDocument::setPartLockState);
    connect(this, &SkeletonPartWidget::setPartVisibleState, m_document, &SkeletonDocument::setPartVisibleState);
    connect(this, &SkeletonPartWidget::setPartSubdivState, m_document, &SkeletonDocument::setPartSubdivState);
    connect(this, &SkeletonPartWidget::setPartDisableState, m_document, &SkeletonDocument::setPartDisableState);
    connect(this, &SkeletonPartWidget::setPartXmirrorState, m_document, &SkeletonDocument::setPartXmirrorState);
    connect(this, &SkeletonPartWidget::setPartDeformThickness, m_document, &SkeletonDocument::setPartDeformThickness);
    connect(this, &SkeletonPartWidget::setPartDeformWidth, m_document, &SkeletonDocument::setPartDeformWidth);
    connect(this, &SkeletonPartWidget::setPartRoundState, m_document, &SkeletonDocument::setPartRoundState);
    connect(this, &SkeletonPartWidget::setPartColorState, m_document, &SkeletonDocument::setPartColorState);
    connect(this, &SkeletonPartWidget::checkPart, m_document, &SkeletonDocument::checkPart);
    connect(this, &SkeletonPartWidget::enableBackgroundBlur, m_document, &SkeletonDocument::enableBackgroundBlur);
    connect(this, &SkeletonPartWidget::disableBackgroundBlur, m_document, &SkeletonDocument::disableBackgroundBlur);
    
    connect(this, &SkeletonPartWidget::groupOperationAdded, m_document, &SkeletonDocument::saveSnapshot);
    
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
        emit groupOperationAdded();
    });
    
    connect(m_disableButton, &QPushButton::clicked, [=]() {
        const SkeletonPart *part = m_document->findPart(m_partId);
        if (!part) {
            qDebug() << "Part not found:" << m_partId;
            return;
        }
        emit setPartDisableState(m_partId, !part->disabled);
        emit groupOperationAdded();
    });
    
    connect(m_xMirrorButton, &QPushButton::clicked, [=]() {
        const SkeletonPart *part = m_document->findPart(m_partId);
        if (!part) {
            qDebug() << "Part not found:" << m_partId;
            return;
        }
        emit setPartXmirrorState(m_partId, !part->xMirrored);
        emit groupOperationAdded();
    });
    
    connect(m_deformButton, &QPushButton::clicked, [=]() {
        const SkeletonPart *part = m_document->findPart(m_partId);
        if (!part) {
            qDebug() << "Part not found:" << m_partId;
            return;
        }
        showDeformSettingPopup(mapFromGlobal(QCursor::pos()));
    });
    
    connect(m_roundButton, &QPushButton::clicked, [=]() {
        const SkeletonPart *part = m_document->findPart(m_partId);
        if (!part) {
            qDebug() << "Part not found:" << m_partId;
            return;
        }
        emit setPartRoundState(m_partId, !part->rounded);
        emit groupOperationAdded();
    });
    
    connect(m_colorButton, &QPushButton::clicked, [=]() {
        const SkeletonPart *part = m_document->findPart(m_partId);
        if (!part) {
            qDebug() << "Part not found:" << m_partId;
            return;
        }
        showColorSettingPopup(mapFromGlobal(QCursor::pos()));
    });
    
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &SkeletonPartWidget::customContextMenuRequested, this, &SkeletonPartWidget::showContextMenu);
}

void SkeletonPartWidget::showContextMenu(const QPoint &pos)
{
    emit checkPart(m_partId);
    
    QMenu contextMenu(this);
    
    const SkeletonPart *part = m_document->findPart(m_partId);
    
    QAction hidePartAction(tr("Hide Part"), this);
    if (part && part->visible) {
        connect(&hidePartAction, &QAction::triggered, [=]() {
            emit setPartVisibleState(m_partId, false);
        });
        contextMenu.addAction(&hidePartAction);
    }
    
    QAction showPartAction(tr("Show Part"), this);
    if (part && !part->visible) {
        connect(&showPartAction, &QAction::triggered, [=]() {
            emit setPartVisibleState(m_partId, true);
        });
        contextMenu.addAction(&showPartAction);
    }
    
    QAction hideOtherPartsAction(tr("Hide Other Parts"), this);
    connect(&hideOtherPartsAction, &QAction::triggered, [=]() {
        for (const auto &it: m_document->partIds) {
            if (it != m_partId)
                emit setPartVisibleState(it, false);
        }
    });
    contextMenu.addAction(&hideOtherPartsAction);
    
    QAction showAllPartsAction(tr("Show All Parts"), this);
    connect(&showAllPartsAction, &QAction::triggered, [=]() {
        for (const auto &it: m_document->partIds) {
            emit setPartVisibleState(it, true);
        }
    });
    contextMenu.addAction(&showAllPartsAction);
    
    QAction hideAllPartsAction(tr("Hide All Parts"), this);
    connect(&hideAllPartsAction, &QAction::triggered, [=]() {
        for (const auto &it: m_document->partIds) {
            emit setPartVisibleState(it, false);
        }
    });
    contextMenu.addAction(&hideAllPartsAction);
    
    QAction lockAllPartsAction(tr("Lock All Parts"), this);
    connect(&lockAllPartsAction, &QAction::triggered, [=]() {
        for (const auto &it: m_document->partIds) {
            emit setPartLockState(it, true);
        }
    });
    contextMenu.addAction(&lockAllPartsAction);
    
    QAction unlockAllPartsAction(tr("Unlock All Parts"), this);
    connect(&unlockAllPartsAction, &QAction::triggered, [=]() {
        for (const auto &it: m_document->partIds) {
            emit setPartLockState(it, false);
        }
    });
    contextMenu.addAction(&unlockAllPartsAction);

    contextMenu.exec(mapToGlobal(pos));
}


void SkeletonPartWidget::updateCheckedState(bool checked)
{
    if (checked)
        m_backgroundWidget->setStyleSheet("QWidget#background {border: 1px solid #fc6621;}");
    else
        m_backgroundWidget->setStyleSheet("QWidget#background {border: 1px solid transparent;}");
}

void SkeletonPartWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    QWidget::mouseDoubleClickEvent(event);
    emit checkPart(m_partId);
}

void SkeletonPartWidget::initToolButtonWithoutFont(QPushButton *button)
{
    button->setFixedSize(Theme::toolIconSize / 2, Theme::toolIconSize / 2);
    button->setStyleSheet("QPushButton {color: #f7d9c8}");
    button->setFocusPolicy(Qt::NoFocus);
}

void SkeletonPartWidget::initToolButton(QPushButton *button)
{
    button->setFont(Theme::awesome()->font(Theme::toolIconFontSize / 2));
    initToolButtonWithoutFont(button);
}

void SkeletonPartWidget::showColorSettingPopup(const QPoint &pos)
{
    QMenu popupMenu;
    
    const SkeletonPart *part = m_document->findPart(m_partId);
    if (!part) {
        qDebug() << "Find part failed:" << m_partId;
        return;
    }
    
    QWidget *popup = new QWidget;
    
    QPushButton *colorEraser = new QPushButton(QChar(fa::eraser));
    initToolButton(colorEraser);
    
    //QLabel *colorPreviewLabel = new QLabel;
    //colorPreviewLabel->setFixedSize(Theme::miniIconSize, Theme::miniIconSize);
    //colorPreviewLabel->setAutoFillBackground(true);
    
    QPushButton *pickButton = new QPushButton();
    initToolButtonWithoutFont(pickButton);
    QPalette palette = pickButton->palette();
    QColor choosenColor = part->hasColor ? part->color : Theme::white;
    palette.setColor(QPalette::Window, choosenColor);
    palette.setColor(QPalette::Button, choosenColor);
    pickButton->setPalette(palette);
    
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(colorEraser);
    layout->addWidget(pickButton);
    
    connect(colorEraser, &QPushButton::clicked, [=]() {
        emit setPartColorState(m_partId, false, Theme::white);
    });
    
    connect(pickButton, &QPushButton::clicked, [=]() {
        emit disableBackgroundBlur();
        QColor color = QColorDialog::getColor(part->color, this);
        emit enableBackgroundBlur();
        if(color.isValid()) {
            emit setPartColorState(m_partId, true, color);
        }
    });
    
    popup->setLayout(layout);
    
    QWidgetAction *action = new QWidgetAction(this);
    action->setDefaultWidget(popup);
    
    popupMenu.addAction(action);
    
    popupMenu.exec(mapToGlobal(pos));
}

void SkeletonPartWidget::showDeformSettingPopup(const QPoint &pos)
{
    QMenu popupMenu;
    
    const SkeletonPart *part = m_document->findPart(m_partId);
    if (!part) {
        qDebug() << "Find part failed:" << m_partId;
        return;
    }
    
    QWidget *popup = new QWidget;
    
    FloatNumberWidget *thicknessWidget = new FloatNumberWidget;
    thicknessWidget->setRange(0, 2);
    thicknessWidget->setValue(part->deformThickness);
    
    connect(thicknessWidget, &FloatNumberWidget::valueChanged, [=](float value) {
        emit setPartDeformThickness(m_partId, value);
    });
    
    FloatNumberWidget *widthWidget = new FloatNumberWidget;
    widthWidget->setRange(0, 2);
    widthWidget->setValue(part->deformWidth);
    
    connect(widthWidget, &FloatNumberWidget::valueChanged, [=](float value) {
        emit setPartDeformWidth(m_partId, value);
    });
    
    QPushButton *thicknessEraser = new QPushButton(QChar(fa::eraser));
    initToolButton(thicknessEraser);
    
    connect(thicknessEraser, &QPushButton::clicked, [=]() {
        thicknessWidget->setValue(1.0);
    });
    
    QPushButton *widthEraser = new QPushButton(QChar(fa::eraser));
    initToolButton(widthEraser);
    
    connect(widthEraser, &QPushButton::clicked, [=]() {
        widthWidget->setValue(1.0);
    });
    
    QVBoxLayout *layout = new QVBoxLayout;
    QHBoxLayout *thicknessLayout = new QHBoxLayout;
    QHBoxLayout *widthLayout = new QHBoxLayout;
    thicknessLayout->addWidget(thicknessEraser);
    thicknessLayout->addWidget(thicknessWidget);
    widthLayout->addWidget(widthEraser);
    widthLayout->addWidget(widthWidget);
    layout->addLayout(thicknessLayout);
    layout->addLayout(widthLayout);
    
    popup->setLayout(layout);
    
    QWidgetAction *action = new QWidgetAction(this);
    action->setDefaultWidget(popup);
    
    popupMenu.addAction(action);
    
    popupMenu.exec(mapToGlobal(pos));
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
        button->setStyleSheet("QPushButton {border: none; background: none; color: #fc6621;}");
    else
        button->setStyleSheet("QPushButton {border: none; background: none; color: #525252;}");
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
        updateButton(m_subdivButton, QChar(fa::circleo), true);
    else
        updateButton(m_subdivButton, QChar(fa::squareo), false);
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
        updateButton(m_xMirrorButton, QChar(fa::balancescale), true);
    else
        updateButton(m_xMirrorButton, QChar(fa::balancescale), false);
}

void SkeletonPartWidget::updateDeformButton()
{
    const SkeletonPart *part = m_document->findPart(m_partId);
    if (!part) {
        qDebug() << "Part not found:" << m_partId;
        return;
    }
    if (part->deformAdjusted())
        updateButton(m_deformButton, QChar(fa::handlizardo), true);
    else
        updateButton(m_deformButton, QChar(fa::handlizardo), false);
}

void SkeletonPartWidget::updateRoundButton()
{
    const SkeletonPart *part = m_document->findPart(m_partId);
    if (!part) {
        qDebug() << "Part not found:" << m_partId;
        return;
    }
    if (part->rounded)
        updateButton(m_roundButton, QChar(fa::magnet), true);
    else
        updateButton(m_roundButton, QChar(fa::magnet), false);
}

void SkeletonPartWidget::updateColorButton()
{
    const SkeletonPart *part = m_document->findPart(m_partId);
    if (!part) {
        qDebug() << "Part not found:" << m_partId;
        return;
    }
    if (part->hasColor)
        updateButton(m_colorButton, QChar(fa::eyedropper), true);
    else
        updateButton(m_colorButton, QChar(fa::eyedropper), false);
}

void SkeletonPartWidget::reload()
{
    updatePreview();
    updateLockButton();
    updateVisibleButton();
    updateSubdivButton();
    updateDisableButton();
    updateXmirrorButton();
    updateDeformButton();
    updateRoundButton();
    updateColorButton();
}

SkeletonPartListWidget::SkeletonPartListWidget(const SkeletonDocument *document, QWidget *parent) :
    QListWidget(parent),
    m_document(document)
{
    setSelectionMode(QAbstractItemView::NoSelection);
    setFocusPolicy(Qt::NoFocus);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setSpacing(0);
    setContentsMargins(0, 0, 0, 0);
    
    setFixedWidth(Theme::previewImageSize + Theme::miniIconSize);
    setMinimumHeight(Theme::previewImageSize + 3);
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
    
    int row = 0;
    
    for (auto partIdIt = m_document->partIds.begin(); partIdIt != m_document->partIds.end(); partIdIt++, row++) {
        QUuid partId = *partIdIt;
        QListWidgetItem *item = new QListWidgetItem(this);
        item->setSizeHint(QSize(width(), Theme::previewImageSize));
        item->setData(Qt::UserRole, QVariant(row));
        item->setBackground(QWidget::palette().color(QPalette::Button));
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

void SkeletonPartListWidget::partDeformChanged(QUuid partId)
{
    auto item = m_itemMap.find(partId);
    if (item == m_itemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second);
    widget->updateDeformButton();
}

void SkeletonPartListWidget::partRoundStateChanged(QUuid partId)
{
    auto item = m_itemMap.find(partId);
    if (item == m_itemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second);
    widget->updateRoundButton();
}

void SkeletonPartListWidget::partColorStateChanged(QUuid partId)
{
    auto item = m_itemMap.find(partId);
    if (item == m_itemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second);
    widget->updateColorButton();
}

void SkeletonPartListWidget::partChecked(QUuid partId)
{
    auto item = m_itemMap.find(partId);
    if (item == m_itemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second);
    widget->updateCheckedState(true);
}

void SkeletonPartListWidget::partUnchecked(QUuid partId)
{
    auto item = m_itemMap.find(partId);
    if (item == m_itemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second);
    widget->updateCheckedState(false);
}

QSize SkeletonPartListWidget::sizeHint() const {
    return QSize(Theme::previewImageSize, Theme::previewImageSize * 5.5);
}
