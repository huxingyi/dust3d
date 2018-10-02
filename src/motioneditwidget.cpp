#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QDebug>
#include "motioneditwidget.h"
#include "dust3dutil.h"
#include "poselistwidget.h"
#include "version.h"

MotionEditWidget::MotionEditWidget(const SkeletonDocument *document, QWidget *parent) :
    QDialog(parent),
    m_document(document)
{
    m_clipPlayer = new AnimationClipPlayer;

    m_graphicsWidget = new InterpolationGraphicsWidget(this);
    m_graphicsWidget->setMinimumSize(256, 128);
    
    connect(m_graphicsWidget, &InterpolationGraphicsWidget::controlNodesChanged, this, &MotionEditWidget::setUnsavedState);
    connect(m_graphicsWidget, &InterpolationGraphicsWidget::controlNodesChanged, this, &MotionEditWidget::generatePreviews);
    connect(m_graphicsWidget, &InterpolationGraphicsWidget::removeKeyframe, this, [=](int index) {
        m_keyframes.erase(m_keyframes.begin() + index);
        syncKeyframesToGraphicsView();
        setUnsavedState();
        generatePreviews();
    });
    connect(m_graphicsWidget, &InterpolationGraphicsWidget::keyframeKnotChanged, this, [=](int index, float knot) {
        m_keyframes[index].first = knot;
        setUnsavedState();
        generatePreviews();
    });
    
    m_previewWidget = new ModelWidget(this);
    m_previewWidget->setMinimumSize(128, 128);
    m_previewWidget->resize(384, 384);
    m_previewWidget->move(-64, 0);
    
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
        addKeyframe(m_graphicsWidget->cursorKnot(), poseId);
    });
    
    //QLabel *interpolationTypeItemName = new QLabel(tr("Interpolation:"));
    //QLabel *interpolationTypeValue = new QLabel(tr("Spring"));
    
    //QPushButton *interpolationTypeButton = new QPushButton();
    //interpolationTypeButton->setText(tr("Browse..."));
    
    //QHBoxLayout *interpolationButtonsLayout = new QHBoxLayout;
    //interpolationButtonsLayout->addStretch();
    //interpolationButtonsLayout->addWidget(interpolationTypeItemName);
    //interpolationButtonsLayout->addWidget(interpolationTypeValue);
    //interpolationButtonsLayout->addWidget(interpolationTypeButton);
    //interpolationButtonsLayout->addStretch();
    
    QVBoxLayout *motionEditLayout = new QVBoxLayout;
    motionEditLayout->addWidget(poseListContainerWidget);
    motionEditLayout->addWidget(Theme::createHorizontalLineWidget());
    motionEditLayout->addWidget(m_graphicsWidget);
    //motionEditLayout->addLayout(interpolationButtonsLayout);
    
    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->setContentsMargins(256, 0, 0, 0);
    topLayout->addWidget(Theme::createVerticalLineWidget());
    topLayout->addLayout(motionEditLayout);
    
    m_nameEdit = new QLineEdit;
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
    
    connect(this, &MotionEditWidget::addMotion, m_document, &SkeletonDocument::addMotion);
    connect(this, &MotionEditWidget::renameMotion, m_document, &SkeletonDocument::renameMotion);
    connect(this, &MotionEditWidget::setMotionControlNodes, m_document, &SkeletonDocument::setMotionControlNodes);
    connect(this, &MotionEditWidget::setMotionKeyframes, m_document, &SkeletonDocument::setMotionKeyframes);
    
    updateTitle();
}

MotionEditWidget::~MotionEditWidget()
{
    delete m_clipPlayer;
}

QSize MotionEditWidget::sizeHint() const
{
    return QSize(800, 600);
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
            QMessageBox::Yes | QMessageBox::No);
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
    std::vector<HermiteControlNode> controlNodes;
    m_graphicsWidget->toNormalizedControlNodes(controlNodes);
    if (m_motionId.isNull()) {
        emit addMotion(m_nameEdit->text(), controlNodes, m_keyframes);
    } else if (m_unsaved) {
        emit renameMotion(m_motionId, m_nameEdit->text());
        emit setMotionControlNodes(m_motionId, controlNodes);
        emit setMotionKeyframes(m_motionId, m_keyframes);
    }
    m_unsaved = false;
    close();
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
    const SkeletonMotion *motion = m_document->findMotion(m_motionId);
    if (nullptr == motion) {
        qDebug() << "Find motion failed:" << m_motionId;
        return;
    }
    setWindowTitle(unifiedWindowTitle(motion->name + (m_unsaved ? "*" : "")));
}

void MotionEditWidget::setEditMotionControlNodes(std::vector<HermiteControlNode> controlNodes)
{
    m_graphicsWidget->setControlNodes(controlNodes);
}

void MotionEditWidget::syncKeyframesToGraphicsView()
{
    std::vector<std::pair<float, QString>> keyframesForGraphicsView;
    for (const auto &frame: m_keyframes) {
        QString poseName;
        const SkeletonPose *pose = m_document->findPose(frame.second);
        if (nullptr == pose) {
            qDebug() << "Find pose failed:" << frame.second;
        } else {
            poseName = pose->name;
        }
        keyframesForGraphicsView.push_back({frame.first, poseName});
    }
    m_graphicsWidget->setKeyframes(keyframesForGraphicsView);
}

void MotionEditWidget::setEditMotionKeyframes(std::vector<std::pair<float, QUuid>> keyframes)
{
    m_keyframes = keyframes;
    syncKeyframesToGraphicsView();
}

void MotionEditWidget::addKeyframe(float knot, QUuid poseId)
{
    m_keyframes.push_back({knot, poseId});
    std::sort(m_keyframes.begin(), m_keyframes.end(),
            [](const std::pair<float, QUuid> &first, const std::pair<float, QUuid> &second) {
        return first.first < second.first;
    });
    syncKeyframesToGraphicsView();
    setUnsavedState();
    generatePreviews();
}

void MotionEditWidget::updateKeyframeKnot(int index, float knot)
{
    if (index < 0 || index >= (int)m_keyframes.size())
        return;
    m_keyframes[index].first = knot;
    setUnsavedState();
    generatePreviews();
}

void MotionEditWidget::generatePreviews()
{
    if (nullptr != m_previewsGenerator) {
        m_isPreviewsObsolete = true;
        return;
    }
    
    m_isPreviewsObsolete = false;
    
    const std::vector<AutoRiggerBone> *rigBones = m_document->resultRigBones();
    const std::map<int, AutoRiggerVertexWeights> *rigWeights = m_document->resultRigWeights();
    
    if (nullptr == rigBones || nullptr == rigWeights) {
        return;
    }
    
    m_previewsGenerator = new MotionPreviewsGenerator(rigBones, rigWeights,
        m_document->currentRiggedResultContext());
    for (const auto &pose: m_document->poseMap)
        m_previewsGenerator->addPoseToLibrary(pose.first, pose.second.parameters);
    //for (const auto &motion: m_document->motionMap)
    //    m_previewsGenerator->addMotionToLibrary(motion.first, motion.second.controlNodes, motion.second.keyframes);
    std::vector<HermiteControlNode> controlNodes;
    m_graphicsWidget->toNormalizedControlNodes(controlNodes);
    m_previewsGenerator->addMotionToLibrary(QUuid(), controlNodes, m_keyframes);
    m_previewsGenerator->addPreviewRequirement(QUuid());
    QThread *thread = new QThread;
    m_previewsGenerator->moveToThread(thread);
    connect(thread, &QThread::started, m_previewsGenerator, &MotionPreviewsGenerator::process);
    connect(m_previewsGenerator, &MotionPreviewsGenerator::finished, this, &MotionEditWidget::previewsReady);
    connect(m_previewsGenerator, &MotionPreviewsGenerator::finished, thread, &QThread::quit);
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
