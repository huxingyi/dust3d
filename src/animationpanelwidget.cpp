#include <QHBoxLayout>
#include <QPushButton>
#include "animationpanelwidget.h"
#include "version.h"

AnimationPanelWidget::AnimationPanelWidget(SkeletonDocument *document, QWidget *parent) :
    QWidget(parent),
    m_document(document),
    m_animationClipGenerator(nullptr)
{
    connect(&m_clipPlayer, &AnimationClipPlayer::frameReadyToShow, this, &AnimationPanelWidget::frameReadyToShow);
    
    QVBoxLayout *buttonsLayout = new QVBoxLayout;
    buttonsLayout->addStretch();
    
    QPushButton *resetButton = new QPushButton(tr("Reset"));
    connect(resetButton, &QPushButton::clicked, this, &AnimationPanelWidget::reset);
    buttonsLayout->addWidget(resetButton);
    
    buttonsLayout->addSpacing(10);
    
    for (const auto &clipName: AnimationClipGenerator::supportedClipNames) {
        QPushButton *clipButton = new QPushButton(QObject::tr(qPrintable(clipName + " (Experiment)")));
        connect(clipButton, &QPushButton::clicked, [=] {
            generateClip(clipName);
        });
        buttonsLayout->addWidget(clipButton);
    }
    buttonsLayout->addStretch();
    
    QVBoxLayout *controlLayout = new QVBoxLayout;
    controlLayout->setSpacing(0);
    controlLayout->setContentsMargins(0, 0, 0, 0);
    controlLayout->addLayout(buttonsLayout);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(controlLayout);
    
    setLayout(mainLayout);
    
    setMinimumWidth(200);
    
    setWindowTitle(APP_NAME);
}

AnimationPanelWidget::~AnimationPanelWidget()
{
}

void AnimationPanelWidget::sourceMeshChanged()
{
    if (m_nextMotionName.isEmpty() && m_lastMotionName.isEmpty())
        return;
    
    if (!m_nextMotionName.isEmpty()) {
        generateClip(m_nextMotionName);
        return;
    }
    
    if (!m_lastMotionName.isEmpty()) {
        generateClip(m_lastMotionName);
        return;
    }
}

void AnimationPanelWidget::hideEvent(QHideEvent *event)
{
    reset();
}

void AnimationPanelWidget::reset()
{
    m_lastMotionName.clear();
    m_clipPlayer.clear();
    emit panelClosed();
}

void AnimationPanelWidget::generateClip(QString motionName)
{
    if (nullptr != m_animationClipGenerator) {
        m_nextMotionName = motionName;
        return;
    }
    m_lastMotionName = motionName;
    m_nextMotionName.clear();
    
    qDebug() << "Animation clip generating..";
    
    QThread *thread = new QThread;
    
    std::map<QString, QString> parameters;
    m_animationClipGenerator = new AnimationClipGenerator(m_document->currentPostProcessedResultContext(),
        m_document->currentJointNodeTree(),
        motionName, parameters, true);
    m_animationClipGenerator->moveToThread(thread);
    connect(thread, &QThread::started, m_animationClipGenerator, &AnimationClipGenerator::process);
    connect(m_animationClipGenerator, &AnimationClipGenerator::finished, this, &AnimationPanelWidget::clipReady);
    connect(m_animationClipGenerator, &AnimationClipGenerator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void AnimationPanelWidget::clipReady()
{
    auto frameMeshes = m_animationClipGenerator->takeFrameMeshes();
    m_clipPlayer.updateFrameMeshes(frameMeshes);

    delete m_animationClipGenerator;
    m_animationClipGenerator = nullptr;
   
    qDebug() << "Animation clip generation done";
    
    if (!m_nextMotionName.isEmpty())
        generateClip(m_nextMotionName);
}

MeshLoader *AnimationPanelWidget::takeFrameMesh()
{
    return m_clipPlayer.takeFrameMesh();
}
