#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QAction>
#include <QMenu>
#include <QWidgetAction>
#include <QColorDialog>
#include <QSizePolicy>
#include "skeletonpartwidget.h"
#include "theme.h"
#include "floatnumberwidget.h"

SkeletonPartWidget::SkeletonPartWidget(const SkeletonDocument *document, QUuid partId) :
    m_document(document),
    m_partId(partId)
{
    QSizePolicy retainSizePolicy = sizePolicy();
    retainSizePolicy.setRetainSizeWhenHidden(true);

    m_visibleButton = new QPushButton();
    m_visibleButton->setSizePolicy(retainSizePolicy);
    initButton(m_visibleButton);
    
    m_lockButton = new QPushButton();
    m_lockButton->setSizePolicy(retainSizePolicy);
    initButton(m_lockButton);
    
    m_subdivButton = new QPushButton();
    m_subdivButton->setSizePolicy(retainSizePolicy);
    initButton(m_subdivButton);
    
    m_disableButton = new QPushButton();
    m_disableButton->setSizePolicy(retainSizePolicy);
    initButton(m_disableButton);
    
    m_xMirrorButton = new QPushButton();
    m_xMirrorButton->setSizePolicy(retainSizePolicy);
    initButton(m_xMirrorButton);
    
    m_deformButton = new QPushButton();
    m_deformButton->setSizePolicy(retainSizePolicy);
    initButton(m_deformButton);
    
    m_roundButton = new QPushButton;
    m_roundButton->setSizePolicy(retainSizePolicy);
    initButton(m_roundButton);
    
    m_colorButton = new QPushButton;
    m_colorButton->setSizePolicy(retainSizePolicy);
    initButton(m_colorButton);
    
    m_previewLabel = new QLabel;
    m_previewLabel->setFixedSize(Theme::previewImageSize, Theme::previewImageSize);
    
    QWidget *hrLightWidget = new QWidget;
    hrLightWidget->setFixedHeight(1);
    hrLightWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    //hrLightWidget->setStyleSheet(QString("background-color: #565656;"));
    hrLightWidget->setContentsMargins(0, 0, 0, 0);
    
    QWidget *hrWidget = new QWidget;
    hrWidget->setFixedHeight(1);
    hrWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    //hrWidget->setStyleSheet(QString("background-color: #1a1a1a;"));
    hrWidget->setContentsMargins(0, 0, 0, 0);
    
    QGridLayout *toolsLayout = new QGridLayout;
    toolsLayout->setSpacing(0);
    toolsLayout->setContentsMargins(0, 0, 5, 0);
    int row = 0;
    int col = 0;
    toolsLayout->addWidget(m_visibleButton, row, col++, Qt::AlignBottom);
    toolsLayout->addWidget(m_lockButton, row, col++, Qt::AlignBottom);
    toolsLayout->addWidget(m_disableButton, row, col++, Qt::AlignBottom);
    toolsLayout->addWidget(m_colorButton, row, col++, Qt::AlignBottom);
    row++;
    col = 0;
    toolsLayout->addWidget(m_subdivButton, row, col++, Qt::AlignTop);
    toolsLayout->addWidget(m_deformButton, row, col++, Qt::AlignTop);
    toolsLayout->addWidget(m_xMirrorButton, row, col++, Qt::AlignTop);
    toolsLayout->addWidget(m_roundButton, row, col++, Qt::AlignTop);
    
    QHBoxLayout *previewAndToolsLayout = new QHBoxLayout;
    previewAndToolsLayout->setSpacing(0);
    previewAndToolsLayout->setContentsMargins(0, 0, 0, 0);
    previewAndToolsLayout->addWidget(m_previewLabel);
    previewAndToolsLayout->addLayout(toolsLayout);
    previewAndToolsLayout->setStretch(0, 0);
    previewAndToolsLayout->setStretch(1, 0);
    
    QWidget *backgroundWidget = new QWidget;
    backgroundWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    backgroundWidget->setFixedSize(Theme::previewImageSize + Theme::miniIconSize * 4 + 5, Theme::previewImageSize);
    backgroundWidget->setObjectName("background");
    m_backgroundWidget = backgroundWidget;
    backgroundWidget->setLayout(previewAndToolsLayout);
    
    QHBoxLayout *backgroundLayout = new QHBoxLayout;
    backgroundLayout->setSpacing(0);
    backgroundLayout->setContentsMargins(0, 0, 0, 0);
    backgroundLayout->addWidget(backgroundWidget);
    backgroundLayout->addSpacing(10);

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
    
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    setFixedSize(preferredSize());
    
    updateAllButtons();
}

QSize SkeletonPartWidget::preferredSize()
{
    return QSize(Theme::previewImageSize + Theme::miniIconSize * 4 + 5 + 2, Theme::previewImageSize + 6);
}

void SkeletonPartWidget::updateAllButtons()
{
    updateVisibleButton();
    updateLockButton();
    updateSubdivButton();
    updateDisableButton();
    updateXmirrorButton();
    updateDeformButton();
    updateRoundButton();
    updateColorButton();
}

void SkeletonPartWidget::updateCheckedState(bool checked)
{
    if (checked)
        m_backgroundWidget->setStyleSheet("QWidget#background {border: 1px solid #f7d9c8;}");
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
    QColor color;
    if (highlighted)
        color = QColor("#fc6621");
    else
        color = QColor("#525252");

    color = color.toHsv();
    color.setHsv(color.hue(), color.saturation() / 5, color.value() * 2 / 3);
    color = color.toRgb();

    button->setStyleSheet("QPushButton {border: none; background: none; color: " + color.name() + ";}");
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
