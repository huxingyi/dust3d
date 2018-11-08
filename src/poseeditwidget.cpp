#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QMenu>
#include <QWidgetAction>
#include <QLineEdit>
#include <QMessageBox>
#include <QFileDialog>
#include "theme.h"
#include "poseeditwidget.h"
#include "floatnumberwidget.h"
#include "version.h"
#include "poserconstruct.h"
#include "graphicscontainerwidget.h"
#include "documentwindow.h"
#include "shortcuts.h"
#include "imageforever.h"

PoseEditWidget::PoseEditWidget(const Document *document, QWidget *parent) :
    QDialog(parent),
    m_document(document),
    m_poseDocument(new PoseDocument)
{
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
    
    m_previewWidget = new ModelWidget(containerWidget);
    m_previewWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_previewWidget->setMinimumSize(DocumentWindow::m_modelRenderWidgetInitialSize, DocumentWindow::m_modelRenderWidgetInitialSize);
    m_previewWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_previewWidget->move(DocumentWindow::m_modelRenderWidgetInitialX, DocumentWindow::m_modelRenderWidgetInitialY);
    
    m_poseGraphicsWidget->setModelWidget(m_previewWidget);
    containerWidget->setModelWidget(m_previewWidget);
    
    connect(containerWidget, &GraphicsContainerWidget::containerSizeChanged,
        graphicsWidget, &SkeletonGraphicsWidget::canvasResized);
    
    connect(graphicsWidget, &SkeletonGraphicsWidget::moveNodeBy, m_poseDocument, &PoseDocument::moveNodeBy);
    connect(graphicsWidget, &SkeletonGraphicsWidget::setNodeOrigin, m_poseDocument, &PoseDocument::setNodeOrigin);
    connect(graphicsWidget, &SkeletonGraphicsWidget::groupOperationAdded, m_poseDocument, &PoseDocument::saveHistoryItem);
    connect(graphicsWidget, &SkeletonGraphicsWidget::undo, m_poseDocument, &PoseDocument::undo);
    connect(graphicsWidget, &SkeletonGraphicsWidget::redo, m_poseDocument, &PoseDocument::redo);
    
    connect(m_poseDocument, &PoseDocument::cleanup, graphicsWidget, &SkeletonGraphicsWidget::removeAllContent);
    
    connect(m_poseDocument, &PoseDocument::nodeAdded, graphicsWidget, &SkeletonGraphicsWidget::nodeAdded);
    connect(m_poseDocument, &PoseDocument::edgeAdded, graphicsWidget, &SkeletonGraphicsWidget::edgeAdded);
    connect(m_poseDocument, &PoseDocument::nodeOriginChanged, graphicsWidget, &SkeletonGraphicsWidget::nodeOriginChanged);
    
    connect(m_poseDocument, &PoseDocument::parametersChanged, this, [&]() {
        m_parameters.clear();
        m_poseDocument->toParameters(m_parameters);
        emit parametersAdjusted();
    });
    
    QHBoxLayout *paramtersLayout = new QHBoxLayout;
    paramtersLayout->addWidget(containerWidget);
    
    m_nameEdit = new QLineEdit;
    m_nameEdit->setFixedWidth(200);
    connect(m_nameEdit, &QLineEdit::textChanged, this, [=]() {
        m_unsaved = true;
        updateTitle();
    });
    QPushButton *saveButton = new QPushButton(tr("Save"));
    connect(saveButton, &QPushButton::clicked, this, &PoseEditWidget::save);
    saveButton->setDefault(true);
    
    QPushButton *changeReferenceSheet = new QPushButton(tr("Change Reference Sheet..."));
    connect(changeReferenceSheet, &QPushButton::clicked, this, &PoseEditWidget::changeTurnaround);
    connect(m_poseDocument, &PoseDocument::turnaroundChanged,
        graphicsWidget, &SkeletonGraphicsWidget::turnaroundChanged);
    
    connect(m_document, &Document::resultRigChanged, this, &PoseEditWidget::updatePoseDocument);
    
    QHBoxLayout *baseInfoLayout = new QHBoxLayout;
    baseInfoLayout->addWidget(new QLabel(tr("Name")));
    baseInfoLayout->addWidget(m_nameEdit);
    baseInfoLayout->addWidget(changeReferenceSheet);
    baseInfoLayout->addStretch();
    baseInfoLayout->addWidget(saveButton);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(paramtersLayout);
    mainLayout->addWidget(Theme::createHorizontalLineWidget());
    mainLayout->addLayout(baseInfoLayout);
    
    setLayout(mainLayout);
    
    connect(m_document, &Document::resultRigChanged, this, &PoseEditWidget::updatePoseDocument);
    connect(this, &PoseEditWidget::parametersAdjusted, this, &PoseEditWidget::updatePreview);
    connect(this, &PoseEditWidget::parametersAdjusted, [=]() {
        m_unsaved = true;
        updateTitle();
    });
    connect(this, &PoseEditWidget::addPose, m_document, &Document::addPose);
    connect(this, &PoseEditWidget::renamePose, m_document, &Document::renamePose);
    connect(this, &PoseEditWidget::setPoseParameters, m_document, &Document::setPoseParameters);
    connect(this, &PoseEditWidget::setPoseAttributes, m_document, &Document::setPoseAttributes);
    
    updatePoseDocument();
    updateTitle();
    m_poseDocument->saveHistoryItem();
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
    m_imageId = ImageForever::add(&image);
    m_attributes["canvasImageId"] = m_imageId.toString();
    m_poseDocument->updateTurnaround(image);
}

QUuid PoseEditWidget::findImageIdFromAttributes(const std::map<QString, QString> &attributes)
{
    auto findImageIdResult = attributes.find("canvasImageId");
    if (findImageIdResult == attributes.end())
        return QUuid();
    return QUuid(findImageIdResult->second);
}

void PoseEditWidget::updatePoseDocument()
{
    m_poseDocument->fromParameters(m_document->resultRigBones(), m_parameters);
    QUuid imageId = findImageIdFromAttributes(m_attributes);
    auto image = ImageForever::get(imageId);
    if (nullptr != image)
        m_poseDocument->updateTurnaround(*image);
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
            QMessageBox::Yes | QMessageBox::No);
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
    
    m_parameters.clear();
    m_poseDocument->toParameters(m_parameters);
    
    poser->parameters() = m_parameters;
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

void PoseEditWidget::setEditParameters(std::map<QString, std::map<QString, QString>> parameters)
{
    m_parameters = parameters;
    updatePoseDocument();
    updatePreview();
    m_poseDocument->saveHistoryItem();
}

void PoseEditWidget::setEditAttributes(std::map<QString, QString> attributes)
{
    m_attributes = attributes;
    updatePoseDocument();
    updatePreview();
}

void PoseEditWidget::clearUnsaveState()
{
    m_unsaved = false;
    updateTitle();
}

void PoseEditWidget::save()
{
    if (m_poseId.isNull()) {
        emit addPose(m_nameEdit->text(), m_parameters);
    } else if (m_unsaved) {
        emit renamePose(m_poseId, m_nameEdit->text());
        emit setPoseParameters(m_poseId, m_parameters);
        emit setPoseAttributes(m_poseId, m_attributes);
    }
    m_unsaved = false;
    close();
}
