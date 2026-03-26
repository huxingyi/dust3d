#include "animation_manage_widget.h"
#include "document.h"
#include "theme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>

AnimationManageWidget::AnimationManageWidget(Document* document, QWidget* parent)
    : QWidget(parent)
    , m_document(document)
    , m_modelWidget(new ModelWidget(this))
    , m_frameTimer(new QTimer(this))
{
    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(5);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    m_modelWidget->setMinimumHeight(280);
    m_modelWidget->disableCullFace();
    m_modelWidget->enableZoom(true);
    m_modelWidget->enableMove(true);
    m_modelWidget->setMoveAndZoomByWindow(false);

    mainLayout->addWidget(m_modelWidget);
    mainLayout->addStretch();

    setLayout(mainLayout);

    createParameterWidgets();

    m_frameTimer->setInterval(100);
    connect(m_frameTimer, &QTimer::timeout, this, &AnimationManageWidget::onAnimationFrameTimeout);

    if (m_document) {
        connect(m_document, &Document::resultRigChanged, this, &AnimationManageWidget::onResultRigChanged);
    }
}

void AnimationManageWidget::createParameterWidgets()
{
    QVBoxLayout* settingsLayout = new QVBoxLayout;

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

    settingsLayout->addLayout(makeSliderRow("Step Length", m_stepLengthSlider, 100));
    settingsLayout->addLayout(makeSliderRow("Step Height", m_stepHeightSlider, 100));
    settingsLayout->addLayout(makeSliderRow("Body Bob", m_bodyBobSlider, 100));
    settingsLayout->addLayout(makeSliderRow("Gait Speed", m_gaitSpeedSlider, 100));

    m_useFabrikCheck = new QCheckBox("Use FABRIK IK");
    m_useFabrikCheck->setChecked(true);
    m_planeStabilizationCheck = new QCheckBox("Plane Stabilization");
    m_planeStabilizationCheck->setChecked(true);

    settingsLayout->addWidget(m_useFabrikCheck);
    settingsLayout->addWidget(m_planeStabilizationCheck);

    auto connectAll = [&]() {
        connect(m_stepLengthSlider, &QSlider::valueChanged, this, &AnimationManageWidget::triggerPreviewRegeneration);
        connect(m_stepHeightSlider, &QSlider::valueChanged, this, &AnimationManageWidget::triggerPreviewRegeneration);
        connect(m_bodyBobSlider, &QSlider::valueChanged, this, &AnimationManageWidget::triggerPreviewRegeneration);
        connect(m_gaitSpeedSlider, &QSlider::valueChanged, this, &AnimationManageWidget::triggerPreviewRegeneration);
        connect(m_useFabrikCheck, &QCheckBox::toggled, this, &AnimationManageWidget::triggerPreviewRegeneration);
        connect(m_planeStabilizationCheck, &QCheckBox::toggled, this, &AnimationManageWidget::triggerPreviewRegeneration);
    };

    connectAll();

    // Insert parameter widgets above the model widget
    QVBoxLayout* topLayout = qobject_cast<QVBoxLayout*>(layout());
    if (topLayout) {
        topLayout->insertLayout(0, settingsLayout);
    }

    // Initialize internal parameter structure
    m_animationParams.useFabrikIk = true;
    m_animationParams.planeStabilization = true;
    m_animationParams.stepLengthFactor = 1.0;
    m_animationParams.stepHeightFactor = 1.0;
    m_animationParams.bodyBobFactor = 1.0;
    m_animationParams.gaitSpeedFactor = 1.0;
}

void AnimationManageWidget::triggerPreviewRegeneration()
{
    if (!m_stepLengthSlider || !m_stepHeightSlider || !m_bodyBobSlider || !m_gaitSpeedSlider || !m_useFabrikCheck || !m_planeStabilizationCheck)
        return;

    m_animationParams.stepLengthFactor = m_stepLengthSlider->value() / 100.0;
    m_animationParams.stepHeightFactor = m_stepHeightSlider->value() / 100.0;
    m_animationParams.bodyBobFactor = m_bodyBobSlider->value() / 100.0;
    m_animationParams.gaitSpeedFactor = m_gaitSpeedSlider->value() / 100.0;
    m_animationParams.useFabrikIk = m_useFabrikCheck->isChecked();
    m_animationParams.planeStabilization = m_planeStabilizationCheck->isChecked();

    onResultRigChanged();
}

AnimationManageWidget::~AnimationManageWidget()
{
    stopAnimationLoop();

    if (m_animationThread) {
        m_animationThread->quit();
        m_animationThread->wait(1000);
        // thread object is deleted by deleteLater after finished
        m_animationThread = nullptr;
    }
}
void AnimationManageWidget::onResultRigChanged()
{
    qDebug() << "AnimationManageWidget: resultRigChanged";

    stopAnimationLoop();
    m_animationFrames.clear();
    m_currentFrame = 0;

    m_animationParams.stepLengthFactor = m_stepLengthSlider->value() / 100.0;
    m_animationParams.stepHeightFactor = m_stepHeightSlider->value() / 100.0;
    m_animationParams.bodyBobFactor = m_bodyBobSlider->value() / 100.0;
    m_animationParams.gaitSpeedFactor = m_gaitSpeedSlider->value() / 100.0;
    m_animationParams.useFabrikIk = m_useFabrikCheck->isChecked();
    m_animationParams.planeStabilization = m_planeStabilizationCheck->isChecked();

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

    dust3d::Object* rigObject = m_document->takeRigObject();
    m_animationWorker->setRigObject(std::unique_ptr<dust3d::Object>(rigObject));

    if (m_animationThread) {
        m_animationThread->quit();
        m_animationThread->wait(1000);
        m_animationThread = nullptr;
    }

    m_animationThread = new QThread;
    m_animationWorker->moveToThread(m_animationThread);

    m_animationWorkerBusy = true;

    connect(m_animationThread, &QThread::started, m_animationWorker.get(), &AnimationPreviewWorker::process);
    connect(m_animationWorker.get(), &AnimationPreviewWorker::finished, this, &AnimationManageWidget::onAnimationPreviewReady);
    connect(m_animationWorker.get(), &AnimationPreviewWorker::finished, m_animationThread, &QThread::quit);
    connect(m_animationThread, &QThread::finished, m_animationThread, &QThread::deleteLater);

    m_animationThread->start();
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

    if (m_animationThread) {
        m_animationThread = nullptr; // deleted by deleteLater
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

