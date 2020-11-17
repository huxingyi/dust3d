#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QThread>
#include <QTimer>
#include <QFormLayout>
#include <QMessageBox>
#include <QScrollArea>
#include "motioneditwidget.h"
#include "simpleshaderwidget.h"
#include "simplerendermeshgenerator.h"
#include "motionsgenerator.h"
#include "util.h"
#include "version.h"
#include "vertebratamotionparameterswidget.h"

MotionEditWidget::~MotionEditWidget()
{
    for (auto &it: m_frames)
        delete it;
    while (!m_renderQueue.empty()) {
        delete m_renderQueue.front();
        m_renderQueue.pop();
    }
    delete m_bones;
    delete m_rigWeights;
    delete m_object;
}

MotionEditWidget::MotionEditWidget()
{ 
    m_modelRenderWidget = new SimpleShaderWidget;
    
    m_parametersArea = new QScrollArea;
    m_parametersArea->setFrameShape(QFrame::NoFrame);
    
    QHBoxLayout *canvasLayout = new QHBoxLayout;
    canvasLayout->setSpacing(0);
    canvasLayout->setContentsMargins(0, 0, 0, 0);
    canvasLayout->addWidget(m_modelRenderWidget);
    canvasLayout->addWidget(m_parametersArea);
    canvasLayout->addSpacing(3);
    canvasLayout->setStretch(0, 1);
    
    m_nameEdit = new QLineEdit;
    connect(m_nameEdit, &QLineEdit::textChanged, this, [=]() {
        m_name = m_nameEdit->text();
        m_unsaved = true;
        updateTitle();
    });
    QPushButton *saveButton = new QPushButton(tr("Save"));
    connect(saveButton, &QPushButton::clicked, this, &MotionEditWidget::save);
    saveButton->setDefault(true);
    
    QHBoxLayout *baseInfoLayout = new QHBoxLayout;
    baseInfoLayout->addWidget(new QLabel(tr("Name")));
    baseInfoLayout->addWidget(m_nameEdit);
    baseInfoLayout->addStretch();
    baseInfoLayout->addWidget(saveButton);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(canvasLayout);
    mainLayout->addWidget(Theme::createHorizontalLineWidget());
    mainLayout->addLayout(baseInfoLayout);
    
    QWidget *centralWidget = new QWidget;
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);
    
    QTimer *timer = new QTimer(this);
    timer->setInterval(17);
    connect(timer, &QTimer::timeout, [this] {
        if (m_renderQueue.size() > 600) {
            checkRenderQueue();
            return;
        }
        if (this->m_frames.empty())
            return;
        if (this->m_frameIndex < this->m_frames.size()) {
            m_renderQueue.push(new SimpleShaderMesh(*this->m_frames[this->m_frameIndex]));
            checkRenderQueue();
        }
        this->m_frameIndex = (this->m_frameIndex + 1) % this->m_frames.size();
    });
    timer->start();
    
    connect(this, &MotionEditWidget::parametersChanged, this, &MotionEditWidget::updateParameters);
    
    updateParametersArea();
    generatePreview();
    updateTitle();
}

void MotionEditWidget::updateParametersArea()
{
    VertebrataMotionParametersWidget *widget = new VertebrataMotionParametersWidget(m_parameters);
    connect(widget, &VertebrataMotionParametersWidget::parametersChanged, this, [=]() {
        this->m_parameters = widget->getParameters();
        emit parametersChanged();
    });
    m_parametersArea->setWidget(widget);
}

QSize MotionEditWidget::sizeHint() const
{
    return QSize(650, 460);
}

void MotionEditWidget::updateParameters()
{
    m_unsaved = true;
    generatePreview();
    updateTitle();
}

void MotionEditWidget::updateTitle()
{
    if (m_motionId.isNull()) {
        setWindowTitle(unifiedWindowTitle(tr("New") + (m_unsaved ? "*" : "")));
        return;
    }
    setWindowTitle(unifiedWindowTitle(m_name + (m_unsaved ? "*" : "")));
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
    if (nullptr != m_previewGenerator) {
        event->ignore();
        return;
    }
    event->accept();
}

void MotionEditWidget::setEditMotionName(const QString &name)
{
    m_name = name;
    m_nameEdit->setText(name);
    updateTitle();
}

void MotionEditWidget::clearUnsaveState()
{
    m_unsaved = false;
    updateTitle();
}

void MotionEditWidget::setEditMotionId(const QUuid &motionId)
{
    if (m_motionId == motionId)
        return;

    m_motionId = motionId;
    updateTitle();
}

void MotionEditWidget::setEditMotionParameters(const std::map<QString, QString> &parameters)
{
    m_parameters = parameters;
    updateParametersArea();
    generatePreview();
}

void MotionEditWidget::save()
{
    if (m_motionId.isNull()) {
        m_motionId = QUuid::createUuid();
        emit addMotion(m_motionId, m_name, m_parameters);
    } else if (m_unsaved) {
        emit renameMotion(m_motionId, m_name);
        emit setMotionParameters(m_motionId, m_parameters);
    }
    clearUnsaveState();
}

void MotionEditWidget::updateBones(RigType rigType,
    const std::vector<RiggerBone> *rigBones,
    const std::map<int, RiggerVertexWeights> *rigWeights,
    const Object *object)
{
    m_rigType = rigType;
    
    delete m_bones;
    m_bones = nullptr;
    
    delete m_rigWeights;
    m_rigWeights = nullptr;
    
    delete m_object;
    m_object = nullptr;
    
    if (nullptr != rigBones &&
            nullptr != rigWeights &&
            nullptr != object) {
        m_bones = new std::vector<RiggerBone>(*rigBones);
        m_rigWeights = new std::map<int, RiggerVertexWeights>(*rigWeights);
        m_object = new Object(*object);
        
        generatePreview();
    }
}

void MotionEditWidget::generatePreview()
{
    if (nullptr != m_previewGenerator) {
        m_isPreviewObsolete = true;
        return;
    }
    
    m_isPreviewObsolete = false;
    
    if (RigType::None == m_rigType || nullptr == m_bones || nullptr == m_rigWeights || nullptr == m_object)
        return;
    
    QThread *thread = new QThread;
    
    m_previewGenerator = new MotionsGenerator(m_rigType, *m_bones, *m_rigWeights, *m_object);
    m_previewGenerator->enablePreviewMeshes();
    m_previewGenerator->addMotion(QUuid(), m_parameters);
    m_previewGenerator->moveToThread(thread);
    connect(thread, &QThread::started, m_previewGenerator, &MotionsGenerator::process);
    connect(m_previewGenerator, &MotionsGenerator::finished, this, &MotionEditWidget::previewReady);
    connect(m_previewGenerator, &MotionsGenerator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void MotionEditWidget::previewReady()
{
    for (auto &it: m_frames)
        delete it;
    m_frames.clear();
    
    std::vector<std::pair<float, SimpleShaderMesh *>> frames = m_previewGenerator->takeResultPreviewMeshes(QUuid());
    for (const auto &frame: frames) {
        m_frames.push_back(frame.second);
    }
    
    delete m_previewGenerator;
    m_previewGenerator = nullptr;
    
    if (m_closed) {
        close();
        return;
    }
    
    if (m_isPreviewObsolete)
        generatePreview();
}

void MotionEditWidget::checkRenderQueue()
{
    if (m_renderQueue.empty())
        return;
    
    SimpleShaderMesh *mesh = m_renderQueue.front();
    m_renderQueue.pop();
    m_modelRenderWidget->updateMesh(mesh);
}
