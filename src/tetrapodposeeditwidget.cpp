#include <QVBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QMenu>
#include <QWidgetAction>
#include "theme.h"
#include "tetrapodposeeditwidget.h"
#include "floatnumberwidget.h"

TetrapodPoseEditWidget::TetrapodPoseEditWidget(const SkeletonDocument *document, QWidget *parent) :
    QWidget(parent),
    m_document(document)
{
    m_posePreviewManager = new PosePreviewManager();
    connect(m_posePreviewManager, &PosePreviewManager::renderDone, [=]() {
        if (m_isPreviewDirty)
            updatePreview();
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
    buttonFont.setPixelSize(7);
    buttonFont.setBold(false);
    for (const auto &item: buttons) {
        QString boneName = item.first;
        QPushButton *buttonWidget = std::get<0>(item.second);
        PopupWidgetType widgetType = std::get<1>(item.second);
        buttonWidget->setFont(buttonFont);
        buttonWidget->setMaximumWidth(45);
        connect(buttonWidget, &QPushButton::clicked, [this, boneName, widgetType]() {
            emit showPopupAngleDialog(boneName, widgetType, mapFromGlobal(QCursor::pos()));
        });
    }
    
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addLayout(marksContainerLayout);
    layout->addLayout(lowerMarksContainerLayout);
    layout->addStretch();
    
    setLayout(layout);
    
    connect(m_document, &SkeletonDocument::resultRigChanged, this, &TetrapodPoseEditWidget::updatePreview);
}

TetrapodPoseEditWidget::~TetrapodPoseEditWidget()
{
    delete m_posePreviewManager;
}

void TetrapodPoseEditWidget::updatePreview()
{
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
    
    delete m_poser;
    m_poser = new TetrapodPoser(*rigBones);
    m_poser->parameters() = m_parameters;
    m_poser->commit();
    m_posePreviewManager->postUpdate(*m_poser, m_document->currentRiggedResultContext(), *rigWeights);
}

void TetrapodPoseEditWidget::showPopupAngleDialog(QString boneName, PopupWidgetType popupWidgetType, QPoint pos)
{
    QMenu popupMenu;

    QWidget *popup = new QWidget;
    
    QVBoxLayout *layout = new QVBoxLayout;
    
    if (PopupWidgetType::PitchYawRoll == popupWidgetType) {
        FloatNumberWidget *pitchWidget = new FloatNumberWidget;
        pitchWidget->setItemName(tr("Pitch"));
        pitchWidget->setRange(-180, 180);
        pitchWidget->setValue(valueOfKeyInMapOrEmpty(m_parameters[boneName], "pitch").toFloat());
        connect(pitchWidget, &FloatNumberWidget::valueChanged, [=](float value) {
            m_parameters[boneName]["pitch"] = QString::number(value);
            updatePreview();
        });
        QPushButton *pitchEraser = new QPushButton(QChar(fa::eraser));
        Theme::initAwesomeMiniButton(pitchEraser);
        connect(pitchEraser, &QPushButton::clicked, [=]() {
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
        connect(yawWidget, &FloatNumberWidget::valueChanged, [=](float value) {
            m_parameters[boneName]["yaw"] = QString::number(value);
            updatePreview();
        });
        QPushButton *yawEraser = new QPushButton(QChar(fa::eraser));
        Theme::initAwesomeMiniButton(yawEraser);
        connect(yawEraser, &QPushButton::clicked, [=]() {
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
        connect(rollWidget, &FloatNumberWidget::valueChanged, [=](float value) {
            m_parameters[boneName]["roll"] = QString::number(value);
            updatePreview();
        });
        QPushButton *rollEraser = new QPushButton(QChar(fa::eraser));
        Theme::initAwesomeMiniButton(rollEraser);
        connect(rollEraser, &QPushButton::clicked, [=]() {
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
        connect(intersectionWidget, &FloatNumberWidget::valueChanged, [=](float value) {
            m_parameters[boneName]["intersection"] = QString::number(value);
            updatePreview();
        });
        QPushButton *intersectionEraser = new QPushButton(QChar(fa::eraser));
        Theme::initAwesomeMiniButton(intersectionEraser);
        connect(intersectionEraser, &QPushButton::clicked, [=]() {
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
    
    popupMenu.exec(mapToGlobal(pos));
}
