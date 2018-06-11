#include <QHBoxLayout>
#include <QPushButton>
#include "animationpanelwidget.h"
#include "version.h"

AnimationPanelWidget::AnimationPanelWidget(SkeletonDocument *document, QWidget *parent) :
    QWidget(parent),
    m_document(document),
    m_animationClipGenerator(nullptr),
    m_lastFrameMesh(nullptr),
    m_sourceMeshReady(false)
{
    QHBoxLayout *moveControlButtonLayout = new QHBoxLayout;
    QHBoxLayout *fightControlButtonLayout = new QHBoxLayout;
    
    QPushButton *resetButton = new QPushButton(tr("Reset"));
    connect(resetButton, &QPushButton::clicked, [=] {
        m_lastMotionName.clear();
        emit panelClosed();
    });
    
    QPushButton *walkButton = new QPushButton(tr("Walk"));
    connect(walkButton, &QPushButton::clicked, [=] {
        generateClip("Walk");
    });
    
    QPushButton *runButton = new QPushButton(tr("Run"));
    connect(runButton, &QPushButton::clicked, [=] {
        generateClip("Run");
    });
    
    QPushButton *attackButton = new QPushButton(tr("Attack"));
    connect(attackButton, &QPushButton::clicked, [=] {
        generateClip("Attack");
    });
    
    QPushButton *hurtButton = new QPushButton(tr("Hurt"));
    connect(hurtButton, &QPushButton::clicked, [=] {
        generateClip("Hurt");
    });
    
    QPushButton *dieButton = new QPushButton(tr("Die"));
    connect(dieButton, &QPushButton::clicked, [=] {
        generateClip("Die");
    });
    
    moveControlButtonLayout->addStretch();
    moveControlButtonLayout->addWidget(resetButton);
    moveControlButtonLayout->addWidget(walkButton);
    moveControlButtonLayout->addWidget(runButton);
    moveControlButtonLayout->addStretch();
    
    fightControlButtonLayout->addStretch();
    fightControlButtonLayout->addWidget(attackButton);
    fightControlButtonLayout->addWidget(hurtButton);
    fightControlButtonLayout->addWidget(dieButton);
    fightControlButtonLayout->addStretch();
    
    QVBoxLayout *controlLayout = new QVBoxLayout;
    controlLayout->setSpacing(0);
    controlLayout->setContentsMargins(0, 0, 0, 0);
    controlLayout->addLayout(moveControlButtonLayout);
    controlLayout->addLayout(fightControlButtonLayout);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(controlLayout);
    
    setLayout(mainLayout);
    
    m_countForFrame.start();
    
    setWindowTitle(APP_NAME);
}

AnimationPanelWidget::~AnimationPanelWidget()
{
    delete m_lastFrameMesh;
    for (auto &it: m_frameMeshes) {
        delete it.second;
    }
}

void AnimationPanelWidget::sourceMeshChanged()
{
    m_sourceMeshReady = true;
    if (m_nextMotionName.isEmpty())
        return;
        
    generateClip(m_nextMotionName);
}

void AnimationPanelWidget::hideEvent(QHideEvent *event)
{
    m_lastMotionName.clear();
    emit panelClosed();
}

void AnimationPanelWidget::generateClip(QString motionName)
{
    if (nullptr != m_animationClipGenerator || !m_sourceMeshReady) {
        m_nextMotionName = motionName;
        return;
    }
    m_lastMotionName = motionName;
    m_nextMotionName.clear();
    
    qDebug() << "Animation clip generating..";
    
    QThread *thread = new QThread;
    
    std::map<QString, QString> parameters;
    m_animationClipGenerator = new AnimationClipGenerator(m_document->currentPostProcessedResultContext(),
        m_nextMotionName, parameters);
    m_animationClipGenerator->moveToThread(thread);
    connect(thread, &QThread::started, m_animationClipGenerator, &AnimationClipGenerator::process);
    connect(m_animationClipGenerator, &AnimationClipGenerator::finished, this, &AnimationPanelWidget::clipReady);
    connect(m_animationClipGenerator, &AnimationClipGenerator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void AnimationPanelWidget::clipReady()
{
    m_frameMeshes = m_animationClipGenerator->takeFrameMeshes();

    delete m_animationClipGenerator;
    m_animationClipGenerator = nullptr;
    
    m_countForFrame.restart();
    
    if (!m_frameMeshes.empty())
        QTimer::singleShot(m_frameMeshes[0].first, this, &AnimationPanelWidget::frameReadyToShow);

    qDebug() << "Animation clip generation done";
    
    if (!m_nextMotionName.isEmpty())
        generateClip(m_nextMotionName);
}

MeshLoader *AnimationPanelWidget::takeFrameMesh()
{
    if (m_lastMotionName.isEmpty())
        return m_document->takeResultMesh();
    
    if (m_frameMeshes.empty()) {
        if (nullptr != m_lastFrameMesh)
            return new MeshLoader(*m_lastFrameMesh);
        return nullptr;
    }
    int millis = m_frameMeshes[0].first - m_countForFrame.elapsed();
    if (millis > 0) {
        QTimer::singleShot(millis, this, &AnimationPanelWidget::frameReadyToShow);
        if (nullptr != m_lastFrameMesh)
            return new MeshLoader(*m_lastFrameMesh);
        return nullptr;
    }
    MeshLoader *mesh = m_frameMeshes[0].second;
    m_frameMeshes.erase(m_frameMeshes.begin());
    m_countForFrame.restart();
    if (!m_frameMeshes.empty()) {
        QTimer::singleShot(m_frameMeshes[0].first, this, &AnimationPanelWidget::frameReadyToShow);
    }
    delete m_lastFrameMesh;
    m_lastFrameMesh = new MeshLoader(*mesh);
    return mesh;
}
