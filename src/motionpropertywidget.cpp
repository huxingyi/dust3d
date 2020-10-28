#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QThread>
#include <QTimer>
#include <QFormLayout>
#include "motionpropertywidget.h"
#include "simpleshaderwidget.h"
#include "motionpreviewgenerator.h"
#include "simplerendermeshgenerator.h"

MotionPropertyWidget::~MotionPropertyWidget()
{
    delete m_bones;
    delete m_rigWeights;
    delete m_outcome;
}

MotionPropertyWidget::MotionPropertyWidget()
{ 
    m_modelRenderWidget = new SimpleShaderWidget;
    
    QFormLayout *parametersLayout = new QFormLayout;
    parametersLayout->setContentsMargins(10, 10, 10, 10);
    
    QDoubleSpinBox *stanceTimeBox = new QDoubleSpinBox;
    stanceTimeBox->setValue(m_parameters.stanceTime);
    stanceTimeBox->setSuffix(" s");
    parametersLayout->addRow(tr("Stance time: "), stanceTimeBox);
    connect(stanceTimeBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double value){
        m_parameters.stanceTime = value;
        emit parametersChanged();
    });
    
    QDoubleSpinBox *swingTimeBox = new QDoubleSpinBox;
    swingTimeBox->setValue(m_parameters.swingTime);
    swingTimeBox->setSuffix(" s");
    parametersLayout->addRow(tr("Swing time: "), swingTimeBox);
    connect(swingTimeBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double value){
        m_parameters.swingTime = value;
        emit parametersChanged();
    });
    
    QDoubleSpinBox *preferredHeightBox = new QDoubleSpinBox;
    preferredHeightBox->setValue(m_parameters.preferredHeight);
    parametersLayout->addRow(tr("Preferred height: "), preferredHeightBox);
    connect(preferredHeightBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double value){
        m_parameters.preferredHeight = value;
        emit parametersChanged();
    });
    
    QSpinBox *legSideIntvalBox = new QSpinBox;
    legSideIntvalBox->setValue(m_parameters.legSideIntval);
    parametersLayout->addRow(tr("Leg side intval: "), legSideIntvalBox);
    connect(legSideIntvalBox, QOverload<int>::of(&QSpinBox::valueChanged), [&](int value){
        m_parameters.legSideIntval = value;
        emit parametersChanged();
    });
    
    QSpinBox *legBalanceIntvalBox = new QSpinBox;
    legBalanceIntvalBox->setValue(m_parameters.legBalanceIntval);
    parametersLayout->addRow(tr("Leg balance intval: "), legBalanceIntvalBox);
    connect(legBalanceIntvalBox, QOverload<int>::of(&QSpinBox::valueChanged), [&](int value){
        m_parameters.legBalanceIntval = value;
        emit parametersChanged();
    });
    
    QDoubleSpinBox *spineStabilityBox = new QDoubleSpinBox;
    spineStabilityBox->setValue(m_parameters.spineStability);
    parametersLayout->addRow(tr("Spine stability: "), spineStabilityBox);
    connect(spineStabilityBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double value){
        m_parameters.spineStability = value;
        emit parametersChanged();
    });
    
    QDoubleSpinBox *groundOffsetBox = new QDoubleSpinBox;
    groundOffsetBox->setRange(-2.0, 2.0);
    groundOffsetBox->setValue(m_parameters.groundOffset);
    parametersLayout->addRow(tr("Ground offset: "), groundOffsetBox);
    connect(groundOffsetBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double value){
        m_parameters.groundOffset = value;
        emit parametersChanged();
    });
    
    QHBoxLayout *canvasLayout = new QHBoxLayout;
    canvasLayout->setSpacing(0);
    canvasLayout->setContentsMargins(0, 0, 0, 0);
    canvasLayout->addWidget(m_modelRenderWidget);
    canvasLayout->addLayout(parametersLayout);
    canvasLayout->addSpacing(3);
    canvasLayout->setStretch(0, 1);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(canvasLayout);
    
    QWidget *centralWidget = new QWidget;
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);
    
    QTimer *timer = new QTimer(this);
    timer->setInterval(17);
    connect(timer, &QTimer::timeout, [this] {
        if (m_renderQueue.size() > 600)
            return;
        if (this->m_frames.empty())
            return;
        if (this->m_frameIndex < this->m_frames.size()) {
            m_renderQueue.push(this->m_frames[this->m_frameIndex]);
            checkRenderQueue();
        }
        this->m_frameIndex = (this->m_frameIndex + 1) % this->m_frames.size();
    });
    timer->start();
    
    connect(this, &MotionPropertyWidget::parametersChanged, this, &MotionPropertyWidget::generatePreview);
    
    generatePreview();
}

QSize MotionPropertyWidget::sizeHint() const
{
    return QSize(600, 360);
}

void MotionPropertyWidget::updateBones(const std::vector<RiggerBone> *rigBones,
    const std::map<int, RiggerVertexWeights> *rigWeights,
    const Outcome *outcome)
{
    delete m_bones;
    m_bones = nullptr;
    
    delete m_rigWeights;
    m_rigWeights = nullptr;
    
    delete m_outcome;
    m_outcome = nullptr;
    
    if (nullptr != rigBones &&
            nullptr != rigWeights &&
            nullptr != outcome) {
        m_bones = new std::vector<RiggerBone>(*rigBones);
        m_rigWeights = new std::map<int, RiggerVertexWeights>(*rigWeights);
        m_outcome = new Outcome(*outcome);
        
        generatePreview();
    }
}

void MotionPropertyWidget::generatePreview()
{
    if (nullptr != m_previewGenerator) {
        m_isPreviewObsolete = true;
        return;
    }
    
    m_isPreviewObsolete = false;
    
    if (nullptr == m_bones || nullptr == m_rigWeights || nullptr == m_outcome)
        return;
    
    QThread *thread = new QThread;
    
    m_previewGenerator = new MotionPreviewGenerator(*m_bones, *m_rigWeights, *m_outcome, m_parameters);
    m_previewGenerator->moveToThread(thread);
    connect(thread, &QThread::started, m_previewGenerator, &MotionPreviewGenerator::process);
    connect(m_previewGenerator, &MotionPreviewGenerator::finished, this, &MotionPropertyWidget::previewReady);
    connect(m_previewGenerator, &MotionPreviewGenerator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void MotionPropertyWidget::previewReady()
{
    m_frames.clear();

    const auto &frames = m_previewGenerator->frames();
    for (const auto &frame: frames) {
        m_frames.push_back({frame.vertices, frame.faces, frame.cornerNormals});
    }
    
    delete m_previewGenerator;
    m_previewGenerator = nullptr;
    
    if (m_isPreviewObsolete)
        generatePreview();
}

void MotionPropertyWidget::checkRenderQueue()
{
    if (nullptr != m_renderMeshGenerator)
        return;
    
    if (m_renderQueue.empty())
        return;
    
    //qDebug() << "Generate render mesh...";
    
    QThread *thread = new QThread;
    
    const auto &item = m_renderQueue.front();
    m_renderMeshGenerator = new SimpleRenderMeshGenerator(item.vertices, item.faces, &item.cornerNormals);
    m_renderQueue.pop();
    m_renderMeshGenerator->moveToThread(thread);
    connect(thread, &QThread::started, m_renderMeshGenerator, &SimpleRenderMeshGenerator::process);
    connect(m_renderMeshGenerator, &SimpleRenderMeshGenerator::finished, this, &MotionPropertyWidget::renderMeshReady);
    connect(m_renderMeshGenerator, &SimpleRenderMeshGenerator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void MotionPropertyWidget::renderMeshReady()
{
    SimpleShaderMesh *renderMesh = m_renderMeshGenerator->takeRenderMesh();
    
    //qDebug() << "Render mesh ready";
    
    delete m_renderMeshGenerator;
    m_renderMeshGenerator = nullptr;
    
    m_modelRenderWidget->updateMesh(renderMesh);
    
    checkRenderQueue();
}
