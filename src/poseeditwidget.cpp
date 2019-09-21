#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QMenu>
#include <QWidgetAction>
#include <QLineEdit>
#include <QMessageBox>
#include <QFileDialog>
#include <QSpinBox>
#include <QSlider>
#include "theme.h"
#include "poseeditwidget.h"
#include "floatnumberwidget.h"
#include "version.h"
#include "poserconstruct.h"
#include "graphicscontainerwidget.h"
#include "documentwindow.h"
#include "shortcuts.h"
#include "imageforever.h"

float PoseEditWidget::m_defaultBlur = 0.5;

PoseEditWidget::PoseEditWidget(const Document *document, QWidget *parent) :
    QDialog(parent),
    m_document(document),
    m_poseDocument(new PoseDocument)
{
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    
    m_posePreviewManager = new PosePreviewManager();
    connect(m_posePreviewManager, &PosePreviewManager::renderDone, [=]() {
        if (m_closed) {
            close();
            return;
        }
        if (m_isPreviewDirty)
            updatePreview();
    });
    connect(m_posePreviewManager, &PosePreviewManager::resultPreviewMeshChanged, [=]() {
        m_previewWidget->updateMesh(m_posePreviewManager->takeResultPreviewMesh());
    });
    
    SkeletonGraphicsWidget *graphicsWidget = new SkeletonGraphicsWidget(m_poseDocument);
    graphicsWidget->setNodePositionModifyOnly(true);
    graphicsWidget->setBackgroundBlur(m_defaultBlur);
    m_poseGraphicsWidget = graphicsWidget;
    
    initShortCuts(this, graphicsWidget);

    GraphicsContainerWidget *containerWidget = new GraphicsContainerWidget;
    containerWidget->setGraphicsWidget(graphicsWidget);
    QGridLayout *containerLayout = new QGridLayout;
    containerLayout->setSpacing(0);
    containerLayout->setContentsMargins(1, 0, 0, 0);
    containerLayout->addWidget(graphicsWidget);
    containerWidget->setLayout(containerLayout);
    containerWidget->setMinimumSize(400, 400);
    
    m_previewWidget = new ModelWidget(this);
    m_previewWidget->setFixedSize(384, 384);
    m_previewWidget->enableMove(true);
    m_previewWidget->enableZoom(false);
    m_previewWidget->move(-64, 0);
    
    m_poseGraphicsWidget->setModelWidget(m_previewWidget);
    containerWidget->setModelWidget(m_previewWidget);
    
    connect(containerWidget, &GraphicsContainerWidget::containerSizeChanged,
        graphicsWidget, &SkeletonGraphicsWidget::canvasResized);
    
    connect(graphicsWidget, &SkeletonGraphicsWidget::moveNodeBy, m_poseDocument, &PoseDocument::moveNodeBy);
    connect(graphicsWidget, &SkeletonGraphicsWidget::setNodeOrigin, m_poseDocument, &PoseDocument::setNodeOrigin);
    connect(graphicsWidget, &SkeletonGraphicsWidget::groupOperationAdded, m_poseDocument, &PoseDocument::saveHistoryItem);
    connect(graphicsWidget, &SkeletonGraphicsWidget::undo, m_poseDocument, &PoseDocument::undo);
    connect(graphicsWidget, &SkeletonGraphicsWidget::redo, m_poseDocument, &PoseDocument::redo);
    connect(graphicsWidget, &SkeletonGraphicsWidget::paste, m_poseDocument, &PoseDocument::paste);
    connect(graphicsWidget, &SkeletonGraphicsWidget::switchChainSide, m_poseDocument, &PoseDocument::switchChainSide);
    
    connect(m_poseDocument, &PoseDocument::cleanup, graphicsWidget, &SkeletonGraphicsWidget::removeAllContent);
    
    connect(m_poseDocument, &PoseDocument::nodeAdded, graphicsWidget, &SkeletonGraphicsWidget::nodeAdded);
    connect(m_poseDocument, &PoseDocument::edgeAdded, graphicsWidget, &SkeletonGraphicsWidget::edgeAdded);
    connect(m_poseDocument, &PoseDocument::nodeOriginChanged, graphicsWidget, &SkeletonGraphicsWidget::nodeOriginChanged);
    connect(m_poseDocument, &PoseDocument::partVisibleStateChanged, graphicsWidget, &SkeletonGraphicsWidget::partVisibleStateChanged);
    
    connect(m_poseDocument, &PoseDocument::parametersChanged, this, [&]() {
        m_currentParameters.clear();
        m_poseDocument->toParameters(m_currentParameters);
        syncFrameFromCurrent();
        emit parametersAdjusted();
    });
    
    QSlider *opacitySlider = new QSlider(Qt::Horizontal);
    opacitySlider->setFixedWidth(100);
    opacitySlider->setMaximum(10);
    opacitySlider->setMinimum(0);
    opacitySlider->setValue(m_defaultBlur * 10);

    connect(opacitySlider, &QSlider::valueChanged, this, [=](int value) {
        graphicsWidget->setBackgroundBlur((float)value / 10);
    });
    
    auto updateSideVisibleButtonState = [=](QPushButton *button, SkeletonSide side) {
        this->updateSideButtonState(button, m_poseDocument->isSideVisible(side));
    };
    
    QPushButton *leftSideVisibleButton = new QPushButton(QChar('L'));
    leftSideVisibleButton->setToolTip(tr("Toggle Left Side Visibility"));
    initSideButton(leftSideVisibleButton);
    updateSideVisibleButtonState(leftSideVisibleButton, SkeletonSide::Left);
    connect(leftSideVisibleButton, &QPushButton::clicked, this, [=]() {
        m_poseDocument->setSideVisiableState(SkeletonSide::Left, !m_poseDocument->isSideVisible(SkeletonSide::Left));
    });
    
    QPushButton *middleSideVisibleButton = new QPushButton(QChar('M'));
    middleSideVisibleButton->setToolTip(tr("Toggle Middle Side Visibility"));
    initSideButton(middleSideVisibleButton);
    updateSideVisibleButtonState(middleSideVisibleButton, SkeletonSide::None);
    connect(middleSideVisibleButton, &QPushButton::clicked, this, [=]() {
        m_poseDocument->setSideVisiableState(SkeletonSide::None, !m_poseDocument->isSideVisible(SkeletonSide::None));
    });
    
    QPushButton *rightSideVisibleButton = new QPushButton(QChar('R'));
    rightSideVisibleButton->setToolTip(tr("Toggle Right Side Visibility"));
    initSideButton(rightSideVisibleButton);
    updateSideVisibleButtonState(rightSideVisibleButton, SkeletonSide::Right);
    connect(rightSideVisibleButton, &QPushButton::clicked, this, [=]() {
        m_poseDocument->setSideVisiableState(SkeletonSide::Right, !m_poseDocument->isSideVisible(SkeletonSide::Right));
    });
    
    connect(m_poseDocument, &PoseDocument::sideVisibleStateChanged, this, [=](SkeletonSide side) {
        switch (side) {
        case SkeletonSide::Left:
            updateSideVisibleButtonState(leftSideVisibleButton, side);
            break;
        case SkeletonSide::None:
            updateSideVisibleButtonState(middleSideVisibleButton, side);
            break;
        case SkeletonSide::Right:
            updateSideVisibleButtonState(rightSideVisibleButton, side);
            break;
        }
    });
    
    QHBoxLayout *sideButtonLayout = new QHBoxLayout;
    sideButtonLayout->setSpacing(0);
    sideButtonLayout->addStretch();
    sideButtonLayout->addWidget(rightSideVisibleButton);
    sideButtonLayout->addWidget(middleSideVisibleButton);
    sideButtonLayout->addWidget(leftSideVisibleButton);
    sideButtonLayout->addStretch();

    QHBoxLayout *sliderLayout = new QHBoxLayout;
    sliderLayout->addStretch();
    sliderLayout->addSpacing(50);
    sliderLayout->addWidget(new QLabel(tr("Dark")));
    sliderLayout->addWidget(opacitySlider);
    sliderLayout->addWidget(new QLabel(tr("Bright")));
    sliderLayout->addSpacing(50);
    sliderLayout->addStretch();
    
    QVBoxLayout *previewLayout = new QVBoxLayout;
    previewLayout->addStretch();
    previewLayout->addLayout(sideButtonLayout);
    previewLayout->addLayout(sliderLayout);
    previewLayout->addSpacing(20);
    
    QHBoxLayout *paramtersLayout = new QHBoxLayout;
    paramtersLayout->addWidget(containerWidget);
    
    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->addLayout(previewLayout);
    topLayout->addWidget(Theme::createVerticalLineWidget());
    topLayout->addLayout(paramtersLayout);
    topLayout->setStretch(2, 1);
    
    m_nameEdit = new QLineEdit;
    m_nameEdit->setFixedWidth(200);
    connect(m_nameEdit, &QLineEdit::textChanged, this, [=]() {
        setUnsaveState();
    });
    
    m_durationEdit = new QDoubleSpinBox();
    m_durationEdit->setDecimals(2);
    m_durationEdit->setMaximum(60);
    m_durationEdit->setMinimum(0);
    m_durationEdit->setSingleStep(0.1);
    m_durationEdit->setValue(m_duration);
    
    connect(m_durationEdit, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, [=](double value) {
        setDuration((float)value);
    });
    
    QPushButton *saveButton = new QPushButton(tr("Save"));
    connect(saveButton, &QPushButton::clicked, this, &PoseEditWidget::save);
    saveButton->setDefault(true);
    
    QPushButton *setPoseSettingButton = new QPushButton(Theme::awesome()->icon(fa::gear), tr(""));
    connect(setPoseSettingButton, &QPushButton::clicked, this, [=]() {
        showPoseSettingPopup(mapFromGlobal(QCursor::pos()));
    });
    
    QPushButton *changeReferenceSheet = new QPushButton(tr("Change Reference Sheet..."));
    connect(changeReferenceSheet, &QPushButton::clicked, this, &PoseEditWidget::changeTurnaround);
    connect(m_poseDocument, &PoseDocument::turnaroundChanged,
        graphicsWidget, &SkeletonGraphicsWidget::turnaroundChanged);
    
    m_framesSettingButton = new QPushButton();
    connect(m_framesSettingButton, &QPushButton::clicked, this, [=]() {
        showFramesSettingPopup(mapFromGlobal(QCursor::pos()));
    });
    
    m_currentFrameSlider = new QSlider(Qt::Horizontal);
    m_currentFrameSlider->setRange(0, m_frames.size() - 1);
    m_currentFrameSlider->setValue(m_currentFrame);
    //m_currentFrameSlider->hide();
    connect(m_currentFrameSlider, static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged), this, [=](int value) {
        setCurrentFrame(value);
    });
    
    connect(m_document, &Document::resultRigChanged, this, &PoseEditWidget::updatePoseDocument);
    
    QPushButton *moveToFirstFrameButton = new QPushButton(Theme::awesome()->icon(fa::angledoubleleft), "");
    connect(moveToFirstFrameButton, &QPushButton::clicked, this, [=]() {
        setCurrentFrame(0);
    });
    
    QPushButton *moveToPreviousFrameButton = new QPushButton(Theme::awesome()->icon(fa::angleleft), "");
    connect(moveToPreviousFrameButton, &QPushButton::clicked, this, [=]() {
        if (m_currentFrame > 0)
            setCurrentFrame(m_currentFrame - 1);
    });
    
    QPushButton *moveToNextFrameButton = new QPushButton(Theme::awesome()->icon(fa::angleright), "");
    connect(moveToNextFrameButton, &QPushButton::clicked, this, [=]() {
        if (m_currentFrame + 1 < (int)m_frames.size())
            setCurrentFrame(m_currentFrame + 1);
    });
    
    QPushButton *moveToLastFrameButton = new QPushButton(Theme::awesome()->icon(fa::angledoubleright), "");
    connect(moveToLastFrameButton, &QPushButton::clicked, this, [=]() {
        if (!m_frames.empty())
            setCurrentFrame(m_frames.size() - 1);
    });
    
    QPushButton *insertAfterFrameButton = new QPushButton(Theme::awesome()->icon(fa::plus), "");
    connect(insertAfterFrameButton, &QPushButton::clicked, this, &PoseEditWidget::insertFrameAfterCurrentFrame);
    
    QPushButton *deleteFrameButton = new QPushButton(Theme::awesome()->icon(fa::trash), "");
    connect(deleteFrameButton, &QPushButton::clicked, this, &PoseEditWidget::removeCurrentFrame);
    
    QHBoxLayout *timelineLayout = new QHBoxLayout;
    timelineLayout->addWidget(insertAfterFrameButton);
    timelineLayout->addWidget(moveToFirstFrameButton);
    timelineLayout->addWidget(moveToPreviousFrameButton);
    timelineLayout->addWidget(moveToNextFrameButton);
    timelineLayout->addWidget(moveToLastFrameButton);
    timelineLayout->addWidget(m_framesSettingButton);
    timelineLayout->addWidget(m_currentFrameSlider);
    timelineLayout->addWidget(deleteFrameButton);
    timelineLayout->setStretch(6, 1);
    
    QHBoxLayout *baseInfoLayout = new QHBoxLayout;
    baseInfoLayout->addWidget(new QLabel(tr("Name")));
    baseInfoLayout->addWidget(m_nameEdit);
    baseInfoLayout->addSpacing(10);
    baseInfoLayout->addWidget(new QLabel(tr("Duration")));
    baseInfoLayout->addWidget(m_durationEdit);
    baseInfoLayout->addStretch();
    baseInfoLayout->addWidget(setPoseSettingButton);
    baseInfoLayout->addWidget(changeReferenceSheet);
    baseInfoLayout->addWidget(saveButton);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(Theme::createHorizontalLineWidget());
    mainLayout->addLayout(timelineLayout);
    mainLayout->addLayout(baseInfoLayout);
    
    setLayout(mainLayout);
    
    connect(this, &PoseEditWidget::parametersAdjusted, this, &PoseEditWidget::updatePreview);
    connect(this, &PoseEditWidget::parametersAdjusted, [=]() {
        setUnsaveState();
    });
    connect(this, &PoseEditWidget::addPose, m_document, &Document::addPose);
    connect(this, &PoseEditWidget::renamePose, m_document, &Document::renamePose);
    connect(this, &PoseEditWidget::setPoseFrames, m_document, &Document::setPoseFrames);
    connect(this, &PoseEditWidget::setPoseTurnaroundImageId, m_document, &Document::setPoseTurnaroundImageId);
    connect(this, &PoseEditWidget::setPoseYtranslationScale, m_document, &Document::setPoseYtranslationScale);
    
    updatePoseDocument();
    updateTitle();
    updateFramesSettingButton();
    m_poseDocument->saveHistoryItem();
}

void PoseEditWidget::initSideButton(QPushButton *button)
{
    QFont font;
    font.setWeight(QFont::Light);
    font.setPixelSize(Theme::toolIconFontSize);
    font.setBold(false);

    button->setFont(font);
    button->setFixedSize(Theme::toolIconSize, Theme::toolIconSize);
    button->setStyleSheet("QPushButton {color: #f7d9c8}");
    button->setFocusPolicy(Qt::NoFocus);
}

void PoseEditWidget::updateSideButtonState(QPushButton *button, bool visible)
{
    if (visible)
        button->setStyleSheet("QPushButton {color: #f7d9c8}");
    else
        button->setStyleSheet("QPushButton {color: #252525}");
}

void PoseEditWidget::showPoseSettingPopup(const QPoint &pos)
{
    QMenu popupMenu;
    
    QWidget *popup = new QWidget;
    
    FloatNumberWidget *yTranslationScaleWidget = new FloatNumberWidget;
    yTranslationScaleWidget->setItemName(tr("Height Adjustment Scale"));
    yTranslationScaleWidget->setRange(0, 1);
    yTranslationScaleWidget->setValue(m_yTranslationScale);
    
    connect(yTranslationScaleWidget, &FloatNumberWidget::valueChanged, [&](float value) {
        m_yTranslationScale = value;
        setUnsaveState();
    });
    
    QPushButton *yTranslationScaleEraser = new QPushButton(QChar(fa::eraser));
    Theme::initAwesomeToolButton(yTranslationScaleEraser);
    
    connect(yTranslationScaleEraser, &QPushButton::clicked, [=]() {
        yTranslationScaleWidget->setValue(1.0);
    });
    
    QHBoxLayout *yTranslationScaleLayout = new QHBoxLayout;
    yTranslationScaleLayout->addWidget(yTranslationScaleEraser);
    yTranslationScaleLayout->addWidget(yTranslationScaleWidget);
    
    popup->setLayout(yTranslationScaleLayout);
    
    QWidgetAction *action = new QWidgetAction(this);
    action->setDefaultWidget(popup);
    
    popupMenu.addAction(action);
    
    popupMenu.exec(mapToGlobal(pos));
}

void PoseEditWidget::showFramesSettingPopup(const QPoint &pos)
{
    QMenu popupMenu;
    
    QWidget *popup = new QWidget;
    
    QSpinBox *framesEdit = new QSpinBox();
    framesEdit->setMaximum(m_frames.size());
    framesEdit->setMinimum(1);
    framesEdit->setSingleStep(1);
    framesEdit->setValue(m_frames.size());
    
    connect(framesEdit, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [=](int value) {
        setCurrentFrame(value);
    });
    
    QFormLayout *formLayout = new QFormLayout;
    formLayout->addRow(tr("Frames:"), framesEdit);
    
    popup->setLayout(formLayout);
    
    QWidgetAction *action = new QWidgetAction(this);
    action->setDefaultWidget(popup);
    
    popupMenu.addAction(action);
    
    popupMenu.exec(mapToGlobal(pos));
}

void PoseEditWidget::updateFramesSettingButton()
{
    m_currentFrameSlider->setRange(0, m_frames.size() - 1);
    if (m_currentFrame != m_currentFrameSlider->value())
        m_currentFrameSlider->setValue(m_currentFrame);
    m_framesSettingButton->setText(tr("Frame: %1 / %2").arg(QString::number(m_currentFrame + 1).rightJustified(2, ' ')).arg(QString::number(m_frames.size()).leftJustified(2, ' ')));
}

void PoseEditWidget::ensureEnoughFrames()
{
    if (m_currentFrame >= (int)m_frames.size()) {
        m_frames.resize(m_currentFrame + 1);
        setUnsaveState();
        updateFramesSettingButton();
    }
}

void PoseEditWidget::syncFrameFromCurrent()
{
    ensureEnoughFrames();
    m_frames[m_currentFrame] = {m_currentAttributes, m_currentParameters};
    updateFramesDurations();
}

void PoseEditWidget::updateFramesDurations()
{
    if (m_frames.empty())
        return;
    
    float frameDuration = m_duration / m_frames.size();
    for (auto &frame: m_frames)
        frame.first["duration"] = QString::number(frameDuration);
}

void PoseEditWidget::setDuration(float duration)
{
    if (qFuzzyCompare(duration, m_duration))
        return;
    
    m_duration = duration;
    setUnsaveState();
    updateFramesDurations();
}

void PoseEditWidget::setCurrentFrame(int frame)
{
    if (m_currentFrame == frame)
        return;
    m_currentFrame = frame;
    ensureEnoughFrames();
    updateFramesSettingButton();
    m_currentAttributes = m_frames[m_currentFrame].first;
    m_currentParameters = m_frames[m_currentFrame].second;
    updatePoseDocument();
}

void PoseEditWidget::insertFrameAfterCurrentFrame()
{
    int currentFrame = m_currentFrame;
    m_frames.resize(m_frames.size() + 1);
    updateFramesDurations();
    if (-1 != currentFrame) {
        for (int index = m_frames.size() - 1; index > currentFrame; --index) {
            m_frames[index] = m_frames[index - 1];
        }
    }
    setUnsaveState();
    setCurrentFrame(currentFrame + 1);
}

void PoseEditWidget::removeCurrentFrame()
{
    if (m_frames.size() <= 1)
        return;
    
    int currentFrame = m_currentFrame;
    if (-1 != currentFrame) {
        for (int index = currentFrame + 1; index < (int)m_frames.size(); ++index) {
            m_frames[index - 1] = m_frames[index];
        }
        m_frames.resize(m_frames.size() - 1);
    }
    updateFramesDurations();
    setUnsaveState();
    if (currentFrame - 1 >= 0)
        setCurrentFrame(currentFrame - 1);
    else if (currentFrame < (int)m_frames.size()) {
        m_currentFrame = -1;
        setCurrentFrame(currentFrame);
    } else
        setCurrentFrame(0);
}

void PoseEditWidget::changeTurnaround()
{
    QString fileName = QFileDialog::getOpenFileName(this, QString(), QString(),
        tr("Image Files (*.png *.jpg *.bmp)")).trimmed();
    if (fileName.isEmpty())
        return;
    QImage image;
    if (!image.load(fileName))
        return;
    auto newImageId = ImageForever::add(&image);
    if (m_imageId == newImageId)
        return;
    setUnsaveState();
    m_imageId = newImageId;
    m_poseDocument->updateTurnaround(image);
}

void PoseEditWidget::updatePoseDocument()
{
    m_otherFramesParameters.clear();
    for (int i = 0; i < (int)m_frames.size(); ++i) {
        if (m_currentFrame == i)
            continue;
        m_otherFramesParameters.push_back(m_frames[i].second);
    }
    m_poseDocument->updateOtherFramesParameters(m_otherFramesParameters);
    m_poseDocument->fromParameters(m_document->resultRigBones(), m_currentParameters);
    m_poseDocument->clearHistories();
    m_poseDocument->saveHistoryItem();
    updatePreview();
}

void PoseEditWidget::reject()
{
    close();
}

void PoseEditWidget::closeEvent(QCloseEvent *event)
{
    if (m_unsaved && !m_closed) {
        QMessageBox::StandardButton answer = QMessageBox::question(this,
            APP_NAME,
            tr("Do you really want to close while there are unsaved changes?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (answer != QMessageBox::Yes) {
            event->ignore();
            return;
        }
    }
    m_closed = true;
    hide();
    if (m_posePreviewManager->isRendering()) {
        event->ignore();
        return;
    }
    event->accept();
}

QSize PoseEditWidget::sizeHint() const
{
    return QSize(1024, 768);
}

PoseEditWidget::~PoseEditWidget()
{
    delete m_posePreviewManager;
    delete m_poseDocument;
}

void PoseEditWidget::updatePreview()
{
    if (m_closed)
        return;
    
    if (m_posePreviewManager->isRendering()) {
        m_isPreviewDirty = true;
        return;
    }
    
    const std::vector<RiggerBone> *rigBones = m_document->resultRigBones();
    const std::map<int, RiggerVertexWeights> *rigWeights = m_document->resultRigWeights();
    
    m_isPreviewDirty = false;
    
    if (nullptr == rigBones || nullptr == rigWeights) {
        return;
    }
    
    Poser *poser = newPoser(m_document->rigType, *rigBones);
    if (nullptr == poser)
        return;
    
    poser->parameters() = m_currentParameters;
    poser->commit();
    m_posePreviewManager->postUpdate(*poser, m_document->currentRiggedOutcome(), *rigWeights);
    delete poser;
}

void PoseEditWidget::setEditPoseId(QUuid poseId)
{
    if (m_poseId == poseId)
        return;
    
    m_poseId = poseId;
    updateTitle();
}

void PoseEditWidget::updateTitle()
{
    if (m_poseId.isNull()) {
        setWindowTitle(unifiedWindowTitle(tr("New") + (m_unsaved ? "*" : "")));
        return;
    }
    const Pose *pose = m_document->findPose(m_poseId);
    if (nullptr == pose) {
        qDebug() << "Find pose failed:" << m_poseId;
        return;
    }
    setWindowTitle(unifiedWindowTitle(pose->name + (m_unsaved ? "*" : "")));
}

void PoseEditWidget::setEditPoseName(QString name)
{
    m_nameEdit->setText(name);
    updateTitle();
}

void PoseEditWidget::setEditPoseFrames(std::vector<std::pair<std::map<QString, QString>, std::map<QString, std::map<QString, QString>>>> frames)
{
    m_frames = frames;
    if (!m_frames.empty()) {
        m_currentFrame = 0;
        const auto &frame = m_frames[m_currentFrame];
        m_currentAttributes = frame.first;
        m_currentParameters = frame.second;
    }
    float totalDuration = 0;
    for (const auto &frame: m_frames) {
        float frameDuration = valueOfKeyInMapOrEmpty(frame.first, "duration").toFloat();
        totalDuration += frameDuration;
    }
    if (qFuzzyIsNull(totalDuration))
        totalDuration = 1.0;
    m_durationEdit->setValue(totalDuration);
    updatePoseDocument();
    updatePreview();
    updateFramesSettingButton();
    m_poseDocument->saveHistoryItem();
}

void PoseEditWidget::setEditPoseTurnaroundImageId(QUuid imageId)
{
    m_imageId = imageId;
    const auto &image = ImageForever::get(m_imageId);
    if (nullptr == image)
        return;
    m_poseDocument->updateTurnaround(*image);
}

void PoseEditWidget::setEditPoseYtranslationScale(float yTranslationScale)
{
    m_yTranslationScale = yTranslationScale;
}

void PoseEditWidget::clearUnsaveState()
{
    m_unsaved = false;
    updateTitle();
}

void PoseEditWidget::setUnsaveState()
{
    m_unsaved = true;
    updateTitle();
}

void PoseEditWidget::save()
{
    if (m_poseId.isNull()) {
        m_poseId = QUuid::createUuid();
        emit addPose(m_poseId, m_nameEdit->text(), m_frames, m_imageId, m_yTranslationScale);
    } else if (m_unsaved) {
        emit renamePose(m_poseId, m_nameEdit->text());
        emit setPoseFrames(m_poseId, m_frames);
        emit setPoseTurnaroundImageId(m_poseId, m_imageId);
        emit setPoseYtranslationScale(m_poseId, m_yTranslationScale);
    }
    clearUnsaveState();
}
