#include "animation_manage_widget.h"
#include "document.h"
#include "theme.h"
#include "toolbar_button.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QComboBox>
#include <QGroupBox>

AnimationManageWidget::AnimationManageWidget(Document* document, QWidget* parent)
    : QWidget(parent)
    , m_document(document)
    , m_modelWidget(new WorldWidget(this))
    , m_frameTimer(new QTimer(this))
{
    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(5);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    m_modelWidget->setMinimumHeight(280);
    m_modelWidget->enableZoom(true);
    m_modelWidget->enableMove(true);
    m_modelWidget->setMoveAndZoomByWindow(false);

    setLayout(mainLayout);

    createParameterWidgets();

    mainLayout->addStretch();

    m_frameTimer->setInterval(100);
    connect(m_frameTimer, &QTimer::timeout, this, &AnimationManageWidget::onAnimationFrameTimeout);

    if (m_document) {
        connect(m_document, &Document::resultRigChanged, this, &AnimationManageWidget::onResultRigChanged);
    }
}

void AnimationManageWidget::createParameterWidgets()
{
    QVBoxLayout* topLayout = qobject_cast<QVBoxLayout*>(layout());
    if (!topLayout)
        return;

    // Animation name selector + add button (outside groupbox)
    QHBoxLayout* animationLayout = new QHBoxLayout;
    m_animationNameCombo = new QComboBox;
    m_addAnimationButton = new ToolbarButton(":/resources/toolbar_add.svg");
    m_addAnimationButton->setToolTip(tr("Add new animation"));

    animationLayout->addWidget(m_animationNameCombo);
    animationLayout->addWidget(m_addAnimationButton);
    topLayout->addLayout(animationLayout);

    // Groupbox containing model widget and parameter controls
    QGroupBox* groupBox = new QGroupBox(tr("Parameters (New)"));
    QVBoxLayout* groupBoxLayout = new QVBoxLayout;
    groupBox->setLayout(groupBoxLayout);

    groupBoxLayout->addWidget(m_modelWidget);

    auto makeSliderRow = [&](const QString& labelText, QSlider* slider, int value) {
        QHBoxLayout* row = new QHBoxLayout;
        QLabel* label = new QLabel(labelText);
        slider->setOrientation(Qt::Horizontal);
        slider->setRange(25, 200);
        slider->setValue(value);
        slider->setSingleStep(1);
        row->addWidget(label);
        row->addWidget(slider);
        return row;
    };

    m_stepLengthSlider = new QSlider;
    m_stepHeightSlider = new QSlider;
    m_bodyBobSlider = new QSlider;
    m_gaitSpeedSlider = new QSlider;

    groupBoxLayout->addLayout(makeSliderRow("Step Length", m_stepLengthSlider, 100));
    groupBoxLayout->addLayout(makeSliderRow("Step Height", m_stepHeightSlider, 100));
    groupBoxLayout->addLayout(makeSliderRow("Body Bob", m_bodyBobSlider, 100));
    groupBoxLayout->addLayout(makeSliderRow("Gait Speed", m_gaitSpeedSlider, 100));

    m_useFabrikCheck = new QCheckBox("Use FABRIK IK");
    m_useFabrikCheck->setChecked(true);
    m_planeStabilizationCheck = new QCheckBox("Plane Stabilization");
    m_planeStabilizationCheck->setChecked(true);
    m_hideBonesCheck = new QCheckBox("Hide Bones");
    m_hideBonesCheck->setChecked(false);
    m_hidePartsCheck = new QCheckBox("Hide Parts");
    m_hidePartsCheck->setChecked(false);

    groupBoxLayout->addWidget(m_useFabrikCheck);
    groupBoxLayout->addWidget(m_planeStabilizationCheck);
    groupBoxLayout->addWidget(m_hideBonesCheck);
    groupBoxLayout->addWidget(m_hidePartsCheck);

    topLayout->addWidget(groupBox);

    auto connectAll = [&]() {
        connect(m_stepLengthSlider, &QSlider::valueChanged, this, &AnimationManageWidget::triggerPreviewRegeneration);
        connect(m_stepHeightSlider, &QSlider::valueChanged, this, &AnimationManageWidget::triggerPreviewRegeneration);
        connect(m_bodyBobSlider, &QSlider::valueChanged, this, &AnimationManageWidget::triggerPreviewRegeneration);
        connect(m_gaitSpeedSlider, &QSlider::valueChanged, this, &AnimationManageWidget::triggerPreviewRegeneration);
        connect(m_useFabrikCheck, &QCheckBox::toggled, this, &AnimationManageWidget::triggerPreviewRegeneration);
        connect(m_planeStabilizationCheck, &QCheckBox::toggled, this, &AnimationManageWidget::triggerPreviewRegeneration);
        connect(m_hideBonesCheck, &QCheckBox::toggled, this, &AnimationManageWidget::triggerPreviewRegeneration);
        connect(m_hidePartsCheck, &QCheckBox::toggled, this, &AnimationManageWidget::triggerPreviewRegeneration);
    };

    connectAll();

    if (m_document) {
        updateAnimationNameForRigType(m_document->getRigType());
        connect(m_document, &Document::rigTypeChanged, this, &AnimationManageWidget::onRigTypeChanged);
    } else {
        updateAnimationNameForRigType(QString());
    }

    // Initialize internal parameter structure from widgets
    updateAnimationParamsFromWidgets();
}

void AnimationManageWidget::updateAnimationNameForRigType(const QString& rigType)
{
    if (!m_animationNameCombo || !m_addAnimationButton)
        return;

    m_animationNameCombo->clear();
    if (rigType.compare("Fly", Qt::CaseInsensitive) == 0) {
        m_animationNameCombo->addItem("FlyWalk");
        m_animationNameCombo->setEnabled(true);
        m_addAnimationButton->setEnabled(true);
    } else {
        m_animationNameCombo->addItem("N/A");
        m_animationNameCombo->setEnabled(false);
        m_addAnimationButton->setEnabled(false);
    }
}

void AnimationManageWidget::onRigTypeChanged(const QString& rigType)
{
    updateAnimationNameForRigType(rigType);
    triggerPreviewRegeneration();
}

void AnimationManageWidget::updateAnimationParamsFromWidgets()
{
    if (!m_stepLengthSlider || !m_stepHeightSlider || !m_bodyBobSlider || !m_gaitSpeedSlider || !m_useFabrikCheck || !m_planeStabilizationCheck)
        return;

    m_animationParams.stepLengthFactor = m_stepLengthSlider->value() / 100.0;
    m_animationParams.stepHeightFactor = m_stepHeightSlider->value() / 100.0;
    m_animationParams.bodyBobFactor = m_bodyBobSlider->value() / 100.0;
    m_animationParams.gaitSpeedFactor = m_gaitSpeedSlider->value() / 10.0;
    m_animationParams.useFabrikIk = m_useFabrikCheck->isChecked();
    m_animationParams.planeStabilization = m_planeStabilizationCheck->isChecked();
}

void AnimationManageWidget::triggerPreviewRegeneration()
{
    if (!m_stepLengthSlider || !m_stepHeightSlider || !m_bodyBobSlider || !m_gaitSpeedSlider || !m_useFabrikCheck || !m_planeStabilizationCheck || !m_hideBonesCheck || !m_hidePartsCheck)
        return;

    updateAnimationParamsFromWidgets();

    onResultRigChanged();
}

AnimationManageWidget::~AnimationManageWidget()
{
    stopAnimationLoop();
}
void AnimationManageWidget::onResultRigChanged()
{
    qDebug() << "AnimationManageWidget: resultRigChanged";

    stopAnimationLoop();
    m_animationFrames.clear();
    m_currentFrame = 0;

    updateAnimationParamsFromWidgets();

    if (m_animationWorkerBusy) {
        m_animationRegenerationPending = true;
        return;
    }

    if (!m_animationWorker) {
        m_animationWorker = std::make_unique<AnimationPreviewWorker>();
    }

    const RigStructure& actualRig = m_document->getActualRigStructure();
    if (actualRig.bones.empty()) {
        qWarning() << "AnimationManageWidget: no rig bones available";
        return;
    }

    m_animationWorker->setParameters(actualRig, 30, 1.0f, m_animationParams);
    m_animationWorker->setHideBones(m_hideBonesCheck ? m_hideBonesCheck->isChecked() : false);
    m_animationWorker->setHideParts(m_hidePartsCheck ? m_hidePartsCheck->isChecked() : false);

    dust3d::Object* rigObject = m_document->takeRigObject();
    m_animationWorker->setRigObject(std::unique_ptr<dust3d::Object>(rigObject));

    auto thread = new QThread;
    m_animationWorker->moveToThread(thread);

    m_animationWorkerBusy = true;

    connect(thread, &QThread::started, m_animationWorker.get(), &AnimationPreviewWorker::process);
    connect(m_animationWorker.get(), &AnimationPreviewWorker::finished, this, &AnimationManageWidget::onAnimationPreviewReady);
    connect(m_animationWorker.get(), &AnimationPreviewWorker::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);

    thread->start();
}

void AnimationManageWidget::onAnimationPreviewReady()
{
    m_animationWorkerBusy = false;

    if (!m_animationWorker) {
        qWarning() << "AnimationManageWidget: preview worker finished but missing worker";
    } else {
        m_animationFrames = m_animationWorker->takePreviewMeshes();
        m_animationWorker.reset();
    }

    if (m_animationFrames.empty()) {
        qWarning() << "AnimationManageWidget: no preview frames generated";
        if (m_animationRegenerationPending) {
            m_animationRegenerationPending = false;
            onResultRigChanged();
        }
        return;
    }

    qDebug() << "AnimationManageWidget: received" << m_animationFrames.size() << "frames";

    startAnimationLoop();

    if (m_animationRegenerationPending) {
        m_animationRegenerationPending = false;
        onResultRigChanged();
    }
}

void AnimationManageWidget::onAnimationFrameTimeout()
{
    if (m_animationFrames.empty() || !m_modelWidget)
        return;

    if (m_currentFrame < 0 || m_currentFrame >= (int)m_animationFrames.size())
        m_currentFrame = 0;

    ModelMesh* frameMesh = new ModelMesh(m_animationFrames[m_currentFrame]);
    m_modelWidget->updateMesh(frameMesh);

    m_currentFrame = (m_currentFrame + 1) % (int)m_animationFrames.size();
}

void AnimationManageWidget::startAnimationLoop()
{
    if (!m_frameTimer)
        return;

    if (!m_frameTimer->isActive()) {
        m_frameTimer->start();
    }
}

void AnimationManageWidget::stopAnimationLoop()
{
    if (m_frameTimer && m_frameTimer->isActive()) {
        m_frameTimer->stop();
    }
    m_currentFrame = 0;
}

