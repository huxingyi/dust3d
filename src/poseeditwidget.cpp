#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QMenu>
#include <QWidgetAction>
#include <QLineEdit>
#include <QMessageBox>
#include "theme.h"
#include "poseeditwidget.h"
#include "floatnumberwidget.h"
#include "version.h"

PoseEditWidget::PoseEditWidget(const SkeletonDocument *document, QWidget *parent) :
    QDialog(parent),
    m_document(document)
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
    
    std::map<QString, std::tuple<QPushButton *, PopupWidgetType>> buttons;
    buttons["Head"] = std::make_tuple(new QPushButton(tr("Head")), PopupWidgetType::PitchYawRoll);
    buttons["Neck"] = std::make_tuple(new QPushButton(tr("Neck")), PopupWidgetType::PitchYawRoll);
    buttons["RightUpperArm"] = std::make_tuple(new QPushButton(tr("UpperArm")), PopupWidgetType::PitchYawRoll);
    buttons["RightLowerArm"] = std::make_tuple(new QPushButton(tr("LowerArm")), PopupWidgetType::Intersection);
    buttons["RightHand"] = std::make_tuple(new QPushButton(tr("Hand")), PopupWidgetType::Intersection);
    buttons["LeftUpperArm"] = std::make_tuple(new QPushButton(tr("UpperArm")), PopupWidgetType::PitchYawRoll);
    buttons["LeftLowerArm"] = std::make_tuple(new QPushButton(tr("LowerArm")), PopupWidgetType::Intersection);
    buttons["LeftHand"] = std::make_tuple(new QPushButton(tr("Hand")), PopupWidgetType::Intersection);
    buttons["Chest"] = std::make_tuple(new QPushButton(tr("Chest")), PopupWidgetType::PitchYawRoll);
    buttons["Spine"] = std::make_tuple(new QPushButton(tr("Spine")), PopupWidgetType::PitchYawRoll);
    buttons["RightUpperLeg"] = std::make_tuple(new QPushButton(tr("UpperLeg")), PopupWidgetType::PitchYawRoll);
    buttons["RightLowerLeg"] = std::make_tuple(new QPushButton(tr("LowerLeg")), PopupWidgetType::Intersection);
    buttons["RightFoot"] = std::make_tuple(new QPushButton(tr("Foot")), PopupWidgetType::Intersection);
    buttons["LeftUpperLeg"] = std::make_tuple(new QPushButton(tr("UpperLeg")), PopupWidgetType::PitchYawRoll);
    buttons["LeftLowerLeg"] = std::make_tuple(new QPushButton(tr("LowerLeg")), PopupWidgetType::Intersection);
    buttons["LeftFoot"] = std::make_tuple(new QPushButton(tr("Foot")), PopupWidgetType::Intersection);
    
    QGridLayout *marksContainerLayout = new QGridLayout;
    marksContainerLayout->setContentsMargins(0, 0, 0, 0);
    marksContainerLayout->setSpacing(0);
    marksContainerLayout->addWidget(std::get<0>(buttons["Head"]), 0, 1);
    marksContainerLayout->addWidget(std::get<0>(buttons["Neck"]), 1, 1);
    marksContainerLayout->addWidget(std::get<0>(buttons["RightUpperArm"]), 1, 0);
    marksContainerLayout->addWidget(std::get<0>(buttons["RightLowerArm"]), 2, 0);
    marksContainerLayout->addWidget(std::get<0>(buttons["RightHand"]), 3, 0);
    marksContainerLayout->addWidget(std::get<0>(buttons["LeftUpperArm"]), 1, 2);
    marksContainerLayout->addWidget(std::get<0>(buttons["LeftLowerArm"]), 2, 2);
    marksContainerLayout->addWidget(std::get<0>(buttons["LeftHand"]), 3, 2);
    marksContainerLayout->addWidget(std::get<0>(buttons["Chest"]), 2, 1);
    marksContainerLayout->addWidget(std::get<0>(buttons["Spine"]), 3, 1);
    
    QGridLayout *lowerMarksContainerLayout = new QGridLayout;
    lowerMarksContainerLayout->setContentsMargins(0, 0, 0, 0);
    lowerMarksContainerLayout->setSpacing(0);
    lowerMarksContainerLayout->addWidget(std::get<0>(buttons["RightUpperLeg"]), 0, 1);
    lowerMarksContainerLayout->addWidget(std::get<0>(buttons["RightLowerLeg"]), 1, 1);
    lowerMarksContainerLayout->addWidget(std::get<0>(buttons["RightFoot"]), 2, 0);
    lowerMarksContainerLayout->addWidget(std::get<0>(buttons["LeftUpperLeg"]), 0, 2);
    lowerMarksContainerLayout->addWidget(std::get<0>(buttons["LeftLowerLeg"]), 1, 2);
    lowerMarksContainerLayout->addWidget(std::get<0>(buttons["LeftFoot"]), 2, 3);
    
    QFont buttonFont;
    buttonFont.setWeight(QFont::Light);
    buttonFont.setPixelSize(9);
    buttonFont.setBold(false);
    for (const auto &item: buttons) {
        QString boneName = item.first;
        QPushButton *buttonWidget = std::get<0>(item.second);
        PopupWidgetType widgetType = std::get<1>(item.second);
        buttonWidget->setFont(buttonFont);
        buttonWidget->setMaximumWidth(55);
        connect(buttonWidget, &QPushButton::clicked, [this, boneName, widgetType]() {
            emit showPopupAngleDialog(boneName, widgetType, mapFromGlobal(QCursor::pos()));
        });
    }
    
    m_previewWidget = new ModelWidget(this);
    m_previewWidget->setMinimumSize(128, 128);
    m_previewWidget->resize(384, 384);
    m_previewWidget->move(-64, -64+22);
    
    QVBoxLayout *markButtonsLayout = new QVBoxLayout;
    markButtonsLayout->addStretch();
    markButtonsLayout->addLayout(marksContainerLayout);
    markButtonsLayout->addLayout(lowerMarksContainerLayout);
    markButtonsLayout->addStretch();
    
    QHBoxLayout *paramtersLayout = new QHBoxLayout;
    paramtersLayout->setContentsMargins(256, 0, 0, 0);
    paramtersLayout->addStretch();
    paramtersLayout->addLayout(markButtonsLayout);
    paramtersLayout->addSpacing(20);
    
    m_nameEdit = new QLineEdit;
    connect(m_nameEdit, &QLineEdit::textChanged, this, [=]() {
        m_unsaved = true;
        updateTitle();
    });
    QPushButton *saveButton = new QPushButton(tr("Save"));
    connect(saveButton, &QPushButton::clicked, this, &PoseEditWidget::save);
    saveButton->setDefault(true);
    
    QHBoxLayout *baseInfoLayout = new QHBoxLayout;
    baseInfoLayout->addWidget(new QLabel(tr("Name")));
    baseInfoLayout->addWidget(m_nameEdit);
    baseInfoLayout->addStretch();
    baseInfoLayout->addWidget(saveButton);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(paramtersLayout);
    mainLayout->addWidget(Theme::createHorizontalLineWidget());
    mainLayout->addLayout(baseInfoLayout);
    
    setLayout(mainLayout);
    
    connect(m_document, &SkeletonDocument::resultRigChanged, this, &PoseEditWidget::updatePreview);
    connect(this, &PoseEditWidget::parametersAdjusted, this, &PoseEditWidget::updatePreview);
    connect(this, &PoseEditWidget::parametersAdjusted, [=]() {
        m_unsaved = true;
        updateTitle();
    });
    connect(this, &PoseEditWidget::addPose, m_document, &SkeletonDocument::addPose);
    connect(this, &PoseEditWidget::renamePose, m_document, &SkeletonDocument::renamePose);
    connect(this, &PoseEditWidget::setPoseParameters, m_document, &SkeletonDocument::setPoseParameters);
    
    updatePreview();
    updateTitle();
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
    if (m_openedMenuCount > 0) {
        event->ignore();
        return;
    }
    if (m_posePreviewManager->isRendering()) {
        event->ignore();
        return;
    }
    event->accept();
}

QSize PoseEditWidget::sizeHint() const
{
    return QSize(0, 350);
}

PoseEditWidget::~PoseEditWidget()
{
    delete m_posePreviewManager;
}

void PoseEditWidget::updatePreview()
{
    if (m_closed)
        return;
    
    if (m_posePreviewManager->isRendering()) {
        m_isPreviewDirty = true;
        return;
    }
    
    const std::vector<AutoRiggerBone> *rigBones = m_document->resultRigBones();
    const std::map<int, AutoRiggerVertexWeights> *rigWeights = m_document->resultRigWeights();
    
    m_isPreviewDirty = false;
    
    if (nullptr == rigBones || nullptr == rigWeights) {
        return;
    }
    
    TetrapodPoser *poser = new TetrapodPoser(*rigBones);
    poser->parameters() = m_parameters;
    poser->commit();
    m_posePreviewManager->postUpdate(*poser, m_document->currentRiggedResultContext(), *rigWeights);
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
    const SkeletonPose *pose = m_document->findPose(m_poseId);
    if (nullptr == pose) {
        qDebug() << "Find pose failed:" << m_poseId;
        return;
    }
    setWindowTitle(unifiedWindowTitle(pose->name + (m_unsaved ? "*" : "")));
}

void PoseEditWidget::showPopupAngleDialog(QString boneName, PopupWidgetType popupWidgetType, QPoint pos)
{
    QMenu popupMenu;

    QWidget *popup = new QWidget;
    
    QVBoxLayout *layout = new QVBoxLayout;
    
    if (PopupWidgetType::PitchYawRoll == popupWidgetType) {
        FloatNumberWidget *pitchWidget = new FloatNumberWidget;
        pitchWidget->setItemName(tr("Pitch"));
        pitchWidget->setRange(-180, 180);
        pitchWidget->setValue(valueOfKeyInMapOrEmpty(m_parameters[boneName], "pitch").toFloat());
        connect(pitchWidget, &FloatNumberWidget::valueChanged, this, [=](float value) {
            m_parameters[boneName]["pitch"] = QString::number(value);
            emit parametersAdjusted();
        });
        QPushButton *pitchEraser = new QPushButton(QChar(fa::eraser));
        Theme::initAwesomeMiniButton(pitchEraser);
        connect(pitchEraser, &QPushButton::clicked, this, [=]() {
            pitchWidget->setValue(0.0);
        });
        QHBoxLayout *pitchLayout = new QHBoxLayout;
        pitchLayout->addWidget(pitchEraser);
        pitchLayout->addWidget(pitchWidget);
        layout->addLayout(pitchLayout);
        
        FloatNumberWidget *yawWidget = new FloatNumberWidget;
        yawWidget->setItemName(tr("Yaw"));
        yawWidget->setRange(-180, 180);
        yawWidget->setValue(valueOfKeyInMapOrEmpty(m_parameters[boneName], "yaw").toFloat());
        connect(yawWidget, &FloatNumberWidget::valueChanged, this, [=](float value) {
            m_parameters[boneName]["yaw"] = QString::number(value);
            emit parametersAdjusted();
        });
        QPushButton *yawEraser = new QPushButton(QChar(fa::eraser));
        Theme::initAwesomeMiniButton(yawEraser);
        connect(yawEraser, &QPushButton::clicked, this, [=]() {
            yawWidget->setValue(0.0);
        });
        QHBoxLayout *yawLayout = new QHBoxLayout;
        yawLayout->addWidget(yawEraser);
        yawLayout->addWidget(yawWidget);
        layout->addLayout(yawLayout);
        
        FloatNumberWidget *rollWidget = new FloatNumberWidget;
        rollWidget->setItemName(tr("Roll"));
        rollWidget->setRange(-180, 180);
        rollWidget->setValue(valueOfKeyInMapOrEmpty(m_parameters[boneName], "roll").toFloat());
        connect(rollWidget, &FloatNumberWidget::valueChanged, this, [=](float value) {
            m_parameters[boneName]["roll"] = QString::number(value);
            emit parametersAdjusted();
        });
        QPushButton *rollEraser = new QPushButton(QChar(fa::eraser));
        Theme::initAwesomeMiniButton(rollEraser);
        connect(rollEraser, &QPushButton::clicked, this, [=]() {
            rollWidget->setValue(0.0);
        });
        QHBoxLayout *rollLayout = new QHBoxLayout;
        rollLayout->addWidget(rollEraser);
        rollLayout->addWidget(rollWidget);
        layout->addLayout(rollLayout);
    } else {
        FloatNumberWidget *intersectionWidget = new FloatNumberWidget;
        intersectionWidget->setItemName(tr("Intersection"));
        intersectionWidget->setRange(-180, 180);
        intersectionWidget->setValue(valueOfKeyInMapOrEmpty(m_parameters[boneName], "intersection").toFloat());
        connect(intersectionWidget, &FloatNumberWidget::valueChanged, this, [=](float value) {
            m_parameters[boneName]["intersection"] = QString::number(value);
            emit parametersAdjusted();
        });
        QPushButton *intersectionEraser = new QPushButton(QChar(fa::eraser));
        Theme::initAwesomeMiniButton(intersectionEraser);
        connect(intersectionEraser, &QPushButton::clicked, this, [=]() {
            intersectionWidget->setValue(0.0);
        });
        QHBoxLayout *intersectionLayout = new QHBoxLayout;
        intersectionLayout->addWidget(intersectionEraser);
        intersectionLayout->addWidget(intersectionWidget);
        layout->addLayout(intersectionLayout);
    }
    
    popup->setLayout(layout);
    
    QWidgetAction action(this);
    action.setDefaultWidget(popup);
    
    popupMenu.addAction(&action);
    
    m_openedMenuCount++;
    popupMenu.exec(mapToGlobal(pos));
    m_openedMenuCount--;

    if (m_closed)
        close();
}

void PoseEditWidget::setEditPoseName(QString name)
{
    m_nameEdit->setText(name);
    updateTitle();
}

void PoseEditWidget::setEditParameters(std::map<QString, std::map<QString, QString>> parameters)
{
    m_parameters = parameters;
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
    }
    m_unsaved = false;
    close();
}
