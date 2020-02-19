#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QSlider>
#include <QMessageBox>
#include <QDebug>
#include <QStackedWidget>
#include "motioneditwidget.h"
#include "util.h"
#include "poselistwidget.h"
#include "motionlistwidget.h"
#include "version.h"
#include "tabwidget.h"
#include "flowlayout.h"
#include "proceduralanimation.h"

MotionEditWidget::MotionEditWidget(const Document *document, QWidget *parent) :
    QDialog(parent),
    m_document(document)
{
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    
    m_clipPlayer = new AnimationClipPlayer;

    m_timelineWidget = new MotionTimelineWidget(document, this);
    
    connect(m_timelineWidget, &MotionTimelineWidget::clipsChanged, this, &MotionEditWidget::setUnsavedState);
    connect(m_timelineWidget, &MotionTimelineWidget::clipsChanged, this, &MotionEditWidget::generatePreviews);
    
    m_previewWidget = new ModelWidget(this);
    m_previewWidget->setFixedSize(384, 384);
    m_previewWidget->enableMove(true);
    m_previewWidget->enableZoom(false);
    m_previewWidget->move(-64, 0);
    m_previewWidget->toggleWireframe();
    
    connect(m_clipPlayer, &AnimationClipPlayer::frameReadyToShow, this, [=]() {
        m_previewWidget->updateMesh(m_clipPlayer->takeFrameMesh());
    });
    
    PoseListWidget *poseListWidget = new PoseListWidget(document);
    poseListWidget->setCornerButtonVisible(true);
    poseListWidget->setHasContextMenu(false);
    QWidget *poseListContainerWidget = new QWidget;
    QGridLayout *poseListLayoutForContainer = new QGridLayout;
    poseListLayoutForContainer->addWidget(poseListWidget);
    poseListContainerWidget->setLayout(poseListLayoutForContainer);
    
    poseListContainerWidget->resize(512, Theme::posePreviewImageSize);
    
    connect(poseListWidget, &PoseListWidget::cornerButtonClicked, this, [=](QUuid poseId) {
        m_timelineWidget->addPose(poseId);
    });
    
    //FlowLayout *proceduralAnimationListLayout = new FlowLayout;
    //for (size_t i = 0; i < (int)ProceduralAnimation::Count - 1; ++i) {
    //    auto proceduralAnimation = (ProceduralAnimation)(i + 1);
    //    QString dispName = ProceduralAnimationToDispName(proceduralAnimation);
    //    QPushButton *addButton = new QPushButton(Theme::awesome()->icon(fa::plus), dispName);
    //    connect(addButton, &QPushButton::clicked, this, [=]() {
    //        m_timelineWidget->addProceduralAnimation(proceduralAnimation);
    //    });
    //    proceduralAnimationListLayout->addWidget(addButton);
    //}
    //QWidget *proceduralAnimationListContainerWidget = new QWidget;
    //proceduralAnimationListContainerWidget->setLayout(proceduralAnimationListLayout);
    
    //proceduralAnimationListContainerWidget->resize(512, Theme::motionPreviewImageSize);
    
    MotionListWidget *motionListWidget = new MotionListWidget(document);
    motionListWidget->setCornerButtonVisible(true);
    motionListWidget->setHasContextMenu(false);
    QWidget *motionListContainerWidget = new QWidget;
    QGridLayout *motionListLayoutForContainer = new QGridLayout;
    motionListLayoutForContainer->addWidget(motionListWidget);
    motionListContainerWidget->setLayout(motionListLayoutForContainer);
    
    motionListContainerWidget->resize(512, Theme::motionPreviewImageSize);
    
    QStackedWidget *stackedWidget = new QStackedWidget;
    stackedWidget->addWidget(poseListContainerWidget);
    //stackedWidget->addWidget(proceduralAnimationListContainerWidget);
    stackedWidget->addWidget(motionListContainerWidget);
    
    connect(motionListWidget, &MotionListWidget::cornerButtonClicked, this, [=](QUuid motionId) {
        m_timelineWidget->addMotion(motionId);
    });
    
    std::vector<QString> tabs = {
        tr("Poses"),
        //tr("Procedural Animations"),
        tr("Motions")
    };
    TabWidget *tabWidget = new TabWidget(tabs);
    tabWidget->setCurrentIndex(0);
    
    connect(tabWidget, &TabWidget::currentIndexChanged, stackedWidget, &QStackedWidget::setCurrentIndex);
    
    QVBoxLayout *motionEditLayout = new QVBoxLayout;
    motionEditLayout->addWidget(tabWidget);
    motionEditLayout->addWidget(stackedWidget);
    motionEditLayout->addStretch();
    motionEditLayout->addWidget(Theme::createHorizontalLineWidget());
    motionEditLayout->addWidget(m_timelineWidget);
    
    QSlider *speedModeSlider = new QSlider(Qt::Horizontal);
    speedModeSlider->setFixedWidth(100);
    speedModeSlider->setMaximum(2);
    speedModeSlider->setMinimum(0);
    speedModeSlider->setValue(1);
    
    connect(speedModeSlider, &QSlider::valueChanged, this, [=](int value) {
        m_clipPlayer->setSpeedMode((AnimationClipPlayer::SpeedMode)value);
    });
    
    QHBoxLayout *sliderLayout = new QHBoxLayout;
    sliderLayout->addStretch();
    sliderLayout->addSpacing(50);
    sliderLayout->addWidget(new QLabel(tr("Slow")));
    sliderLayout->addWidget(speedModeSlider);
    sliderLayout->addWidget(new QLabel(tr("Fast")));
    sliderLayout->addSpacing(50);
    sliderLayout->addStretch();
    
    QVBoxLayout *previewLayout = new QVBoxLayout;
    previewLayout->addStretch();
    previewLayout->addLayout(sliderLayout);
    previewLayout->addSpacing(20);
    
    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->addLayout(previewLayout);
    topLayout->addWidget(Theme::createVerticalLineWidget());
    topLayout->addLayout(motionEditLayout);
    topLayout->setStretch(2, 1);
    
    m_nameEdit = new QLineEdit;
    m_nameEdit->setFixedWidth(200);
    connect(m_nameEdit, &QLineEdit::textChanged, this, &MotionEditWidget::setUnsavedState);
    QPushButton *saveButton = new QPushButton(tr("Save"));
    connect(saveButton, &QPushButton::clicked, this, &MotionEditWidget::save);
    saveButton->setDefault(true);
    
    QHBoxLayout *baseInfoLayout = new QHBoxLayout;
    baseInfoLayout->addWidget(new QLabel(tr("Name")));
    baseInfoLayout->addWidget(m_nameEdit);
    baseInfoLayout->addStretch();
    baseInfoLayout->addWidget(saveButton);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(Theme::createHorizontalLineWidget());
    mainLayout->addLayout(baseInfoLayout);
    
    setLayout(mainLayout);
    
    connect(this, &MotionEditWidget::addMotion, m_document, &Document::addMotion);
    connect(this, &MotionEditWidget::renameMotion, m_document, &Document::renameMotion);
    connect(this, &MotionEditWidget::setMotionClips, m_document, &Document::setMotionClips);
    
    updateTitle();
}

MotionEditWidget::~MotionEditWidget()
{
    delete m_clipPlayer;
}

QSize MotionEditWidget::sizeHint() const
{
    return QSize(1024, 768);
}

void MotionEditWidget::reject()
{
    close();
}

void MotionEditWidget::closeEvent(QCloseEvent *event)
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
    if (nullptr != m_previewsGenerator) {
        event->ignore();
        return;
    }
    event->accept();
}

void MotionEditWidget::save()
{
    if (m_motionId.isNull()) {
        m_motionId = QUuid::createUuid();
        emit addMotion(m_motionId, m_nameEdit->text(), m_timelineWidget->clips());
    } else if (m_unsaved) {
        emit renameMotion(m_motionId, m_nameEdit->text());
        emit setMotionClips(m_motionId, m_timelineWidget->clips());
    }
    clearUnsaveState();
}

void MotionEditWidget::clearUnsaveState()
{
    m_unsaved = false;
    updateTitle();
}

void MotionEditWidget::setUnsavedState()
{
    m_unsaved = true;
    updateTitle();
}

void MotionEditWidget::setEditMotionId(QUuid motionId)
{
    if (m_motionId == motionId)
        return;
    
    m_motionId = motionId;
    updateTitle();
}

void MotionEditWidget::setEditMotionName(QString name)
{
    m_nameEdit->setText(name);
    updateTitle();
}

void MotionEditWidget::updateTitle()
{
    if (m_motionId.isNull()) {
        setWindowTitle(unifiedWindowTitle(tr("New") + (m_unsaved ? "*" : "")));
        return;
    }
    const Motion *motion = m_document->findMotion(m_motionId);
    if (nullptr == motion) {
        qDebug() << "Find motion failed:" << m_motionId;
        return;
    }
    setWindowTitle(unifiedWindowTitle(motion->name + (m_unsaved ? "*" : "")));
}

void MotionEditWidget::setEditMotionClips(std::vector<MotionClip> clips)
{
    m_timelineWidget->setClips(clips);
}

void MotionEditWidget::generatePreviews()
{
    if (nullptr != m_previewsGenerator) {
        m_isPreviewsObsolete = true;
        return;
    }
    
    m_isPreviewsObsolete = false;
    
    const std::vector<RiggerBone> *rigBones = m_document->resultRigBones();
    const std::map<int, RiggerVertexWeights> *rigWeights = m_document->resultRigWeights();
    
    if (nullptr == rigBones || nullptr == rigWeights) {
        return;
    }
    
    m_previewsGenerator = new MotionsGenerator(m_document->rigType, rigBones, rigWeights,
        m_document->currentRiggedOutcome());
    for (const auto &pose: m_document->poseMap)
        m_previewsGenerator->addPoseToLibrary(pose.first, pose.second.frames, pose.second.yTranslationScale);
    for (const auto &motion: m_document->motionMap)
        m_previewsGenerator->addMotionToLibrary(motion.first, motion.second.clips);
    m_previewsGenerator->addMotionToLibrary(QUuid(), m_timelineWidget->clips());
    m_previewsGenerator->addRequirement(QUuid());
    QThread *thread = new QThread;
    m_previewsGenerator->moveToThread(thread);
    connect(thread, &QThread::started, m_previewsGenerator, &MotionsGenerator::process);
    connect(m_previewsGenerator, &MotionsGenerator::finished, this, &MotionEditWidget::previewsReady);
    connect(m_previewsGenerator, &MotionsGenerator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void MotionEditWidget::previewsReady()
{
    auto resultPreviewMeshs = m_previewsGenerator->takeResultPreviewMeshs(QUuid());
    m_clipPlayer->updateFrameMeshes(resultPreviewMeshs);

    delete m_previewsGenerator;
    m_previewsGenerator = nullptr;
    
    if (m_closed) {
        close();
        return;
    }
    
    if (m_isPreviewsObsolete)
        generatePreviews();
}
