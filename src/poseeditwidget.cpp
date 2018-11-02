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
#include "poserconstruct.h"

PoseEditWidget::PoseEditWidget(const Document *document, QWidget *parent) :
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
    
    m_previewWidget = new ModelWidget(this);
    m_previewWidget->setMinimumSize(128, 128);
    m_previewWidget->resize(384, 384);
    m_previewWidget->move(-64, -64+22);
    
    QHBoxLayout *paramtersLayout = new QHBoxLayout;
    paramtersLayout->setContentsMargins(0, 480, 0, 0);
    paramtersLayout->addStretch();
    paramtersLayout->addSpacing(20);
    
    m_nameEdit = new QLineEdit;
    m_nameEdit->setFixedWidth(200);
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
    
    connect(m_document, &Document::resultRigChanged, this, &PoseEditWidget::updatePreview);
    connect(this, &PoseEditWidget::parametersAdjusted, this, &PoseEditWidget::updatePreview);
    connect(this, &PoseEditWidget::parametersAdjusted, [=]() {
        m_unsaved = true;
        updateTitle();
    });
    connect(this, &PoseEditWidget::addPose, m_document, &Document::addPose);
    connect(this, &PoseEditWidget::renamePose, m_document, &Document::renamePose);
    connect(this, &PoseEditWidget::setPoseParameters, m_document, &Document::setPoseParameters);
    
    updateButtons();
    updatePreview();
    updateTitle();
}

void PoseEditWidget::updateButtons()
{
    delete m_buttonsContainer;
    m_buttonsContainer = new QWidget(this);
    m_buttonsContainer->resize(600, 500);
    m_buttonsContainer->move(256, 0);
    m_buttonsContainer->show();
    
    QGridLayout *marksContainerLayout = new QGridLayout;
    marksContainerLayout->setContentsMargins(0, 0, 0, 0);
    marksContainerLayout->setSpacing(2);
    
    QFont buttonFont;
    buttonFont.setWeight(QFont::Light);
    buttonFont.setPixelSize(7);
    buttonFont.setBold(false);
    
    std::map<QString, std::tuple<QPushButton *, PopupWidgetType>> buttons;
    const std::vector<RiggerBone> *rigBones = m_document->resultRigBones();
    if (nullptr != rigBones && !rigBones->empty()) {
        for (const auto &bone: *rigBones) {
            if (!bone.hasButton)
                continue;
            QPushButton *buttonWidget = new QPushButton(bone.name);
            PopupWidgetType widgetType = bone.buttonParameterType;
            buttonWidget->setFont(buttonFont);
            buttonWidget->setMaximumWidth(100);
            QString boneName = bone.name;
            connect(buttonWidget, &QPushButton::clicked, [this, boneName, widgetType]() {
                emit showPopupAngleDialog(boneName, widgetType, mapFromGlobal(QCursor::pos()));
            });
            marksContainerLayout->addWidget(buttonWidget, bone.button.first, bone.button.second);
        }
    }
    
    marksContainerLayout->setSizeConstraint(QLayout::SizeConstraint::SetMinimumSize);
    
    QVBoxLayout *mainLayouer = new QVBoxLayout;
    mainLayouer->addStretch();
    mainLayouer->addLayout(marksContainerLayout);
    mainLayouer->addStretch();
    
    m_buttonsContainer->setLayout(mainLayouer);
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
    return QSize(1024, 768);
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
    
    const std::vector<RiggerBone> *rigBones = m_document->resultRigBones();
    const std::map<int, RiggerVertexWeights> *rigWeights = m_document->resultRigWeights();
    
    m_isPreviewDirty = false;
    
    if (nullptr == rigBones || nullptr == rigWeights) {
        return;
    }
    
    Poser *poser = newPoser(m_document->rigType, *rigBones);
    if (nullptr == poser)
        return;
    
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
    } else if (PopupWidgetType::Intersection == popupWidgetType) {
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
    } else if (PopupWidgetType::Translation == popupWidgetType) {
        FloatNumberWidget *xWidget = new FloatNumberWidget;
        xWidget->setItemName(tr("X"));
        xWidget->setRange(-1, 1);
        xWidget->setValue(valueOfKeyInMapOrEmpty(m_parameters[boneName], "x").toFloat());
        connect(xWidget, &FloatNumberWidget::valueChanged, this, [=](float value) {
            m_parameters[boneName]["x"] = QString::number(value);
            emit parametersAdjusted();
        });
        QPushButton *xEraser = new QPushButton(QChar(fa::eraser));
        Theme::initAwesomeMiniButton(xEraser);
        connect(xEraser, &QPushButton::clicked, this, [=]() {
            xWidget->setValue(0.0);
        });
        QHBoxLayout *xLayout = new QHBoxLayout;
        xLayout->addWidget(xEraser);
        xLayout->addWidget(xWidget);
        layout->addLayout(xLayout);
        
        FloatNumberWidget *yWidget = new FloatNumberWidget;
        yWidget->setItemName(tr("Y"));
        yWidget->setRange(-1, 1);
        yWidget->setValue(valueOfKeyInMapOrEmpty(m_parameters[boneName], "y").toFloat());
        connect(yWidget, &FloatNumberWidget::valueChanged, this, [=](float value) {
            m_parameters[boneName]["y"] = QString::number(value);
            emit parametersAdjusted();
        });
        QPushButton *yEraser = new QPushButton(QChar(fa::eraser));
        Theme::initAwesomeMiniButton(yEraser);
        connect(yEraser, &QPushButton::clicked, this, [=]() {
            yWidget->setValue(0.0);
        });
        QHBoxLayout *yLayout = new QHBoxLayout;
        yLayout->addWidget(yEraser);
        yLayout->addWidget(yWidget);
        layout->addLayout(yLayout);
        
        FloatNumberWidget *zWidget = new FloatNumberWidget;
        zWidget->setItemName(tr("Z"));
        zWidget->setRange(-1, 1);
        zWidget->setValue(valueOfKeyInMapOrEmpty(m_parameters[boneName], "z").toFloat());
        connect(zWidget, &FloatNumberWidget::valueChanged, this, [=](float value) {
            m_parameters[boneName]["z"] = QString::number(value);
            emit parametersAdjusted();
        });
        QPushButton *zEraser = new QPushButton(QChar(fa::eraser));
        Theme::initAwesomeMiniButton(zEraser);
        connect(zEraser, &QPushButton::clicked, this, [=]() {
            zWidget->setValue(0.0);
        });
        QHBoxLayout *zLayout = new QHBoxLayout;
        zLayout->addWidget(zEraser);
        zLayout->addWidget(zWidget);
        layout->addLayout(zLayout);
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
