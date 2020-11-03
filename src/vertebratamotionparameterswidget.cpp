#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include "vertebratamotionparameterswidget.h"
#include "util.h"

std::map<QString, QString> VertebrataMotionParametersWidget::fromVertebrataMotionParameters(const VertebrataMotion::Parameters &from)
{
    std::map<QString, QString> parameters;
    
    parameters["stanceTime"] = QString::number(from.stanceTime);
    parameters["swingTime"] = QString::number(from.swingTime);
    parameters["hipHeight"] = QString::number(from.hipHeight);
    parameters["legSideIntval"] = QString::number(from.legSideIntval);
    parameters["legBalanceIntval"] = QString::number(from.legBalanceIntval);
    parameters["spineStability"] = QString::number(from.spineStability);
    parameters["cycles"] = QString::number(from.cycles);
    parameters["groundOffset"] = QString::number(from.groundOffset);
    parameters["tailLiftForce"] = QString::number(from.tailLiftForce);
    parameters["tailDragForce"] = QString::number(from.tailDragForce);
    
    return parameters;
}

VertebrataMotion::Parameters VertebrataMotionParametersWidget::toVertebrataMotionParameters(const std::map<QString, QString> &parameters)
{
    VertebrataMotion::Parameters vertebrataMotionParameters;
    
    if (parameters.end() != parameters.find("stanceTime"))
        vertebrataMotionParameters.stanceTime = valueOfKeyInMapOrEmpty(parameters, "stanceTime").toDouble();
    if (parameters.end() != parameters.find("swingTime"))
        vertebrataMotionParameters.swingTime = valueOfKeyInMapOrEmpty(parameters, "swingTime").toDouble();
    if (parameters.end() != parameters.find("hipHeight"))
        vertebrataMotionParameters.hipHeight = valueOfKeyInMapOrEmpty(parameters, "hipHeight").toDouble();
    if (parameters.end() != parameters.find("legSideIntval"))
        vertebrataMotionParameters.legSideIntval = valueOfKeyInMapOrEmpty(parameters, "legSideIntval").toDouble();
    if (parameters.end() != parameters.find("legBalanceIntval"))
        vertebrataMotionParameters.legBalanceIntval = valueOfKeyInMapOrEmpty(parameters, "legBalanceIntval").toDouble();
    if (parameters.end() != parameters.find("spineStability"))
        vertebrataMotionParameters.spineStability = valueOfKeyInMapOrEmpty(parameters, "spineStability").toDouble();
    if (parameters.end() != parameters.find("cycles"))
        vertebrataMotionParameters.cycles = valueOfKeyInMapOrEmpty(parameters, "cycles").toInt();
    if (parameters.end() != parameters.find("groundOffset"))
        vertebrataMotionParameters.groundOffset = valueOfKeyInMapOrEmpty(parameters, "groundOffset").toDouble();
    if (parameters.end() != parameters.find("tailLiftForce"))
        vertebrataMotionParameters.tailLiftForce = valueOfKeyInMapOrEmpty(parameters, "tailLiftForce").toDouble();
    if (parameters.end() != parameters.find("tailDragForce"))
        vertebrataMotionParameters.tailDragForce = valueOfKeyInMapOrEmpty(parameters, "tailDragForce").toDouble();
    
    return vertebrataMotionParameters;
}

VertebrataMotionParametersWidget::VertebrataMotionParametersWidget(const std::map<QString, QString> &parameters) :
    m_parameters(parameters)
{
    m_vertebrataMotionParameters = toVertebrataMotionParameters(m_parameters);
    
    QFormLayout *parametersLayout = new QFormLayout;
    parametersLayout->setSpacing(0);
    
    QDoubleSpinBox *stanceTimeBox = new QDoubleSpinBox;
    stanceTimeBox->setValue(m_vertebrataMotionParameters.stanceTime);
    stanceTimeBox->setSuffix(" s");
    parametersLayout->addRow(tr("Stance time: "), stanceTimeBox);
    connect(stanceTimeBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double value){
        m_vertebrataMotionParameters.stanceTime = value;
        m_parameters = fromVertebrataMotionParameters(m_vertebrataMotionParameters);
        emit parametersChanged();
    });
    
    QDoubleSpinBox *swingTimeBox = new QDoubleSpinBox;
    swingTimeBox->setValue(m_vertebrataMotionParameters.swingTime);
    swingTimeBox->setSuffix(" s");
    parametersLayout->addRow(tr("Swing time: "), swingTimeBox);
    connect(swingTimeBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double value){
        m_vertebrataMotionParameters.swingTime = value;
        m_parameters = fromVertebrataMotionParameters(m_vertebrataMotionParameters);
        emit parametersChanged();
    });
    
    QDoubleSpinBox *hipHeightBox = new QDoubleSpinBox;
    hipHeightBox->setValue(m_vertebrataMotionParameters.hipHeight);
    parametersLayout->addRow(tr("Hip height: "), hipHeightBox);
    connect(hipHeightBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double value){
        m_vertebrataMotionParameters.hipHeight = value;
        m_parameters = fromVertebrataMotionParameters(m_vertebrataMotionParameters);
        emit parametersChanged();
    });
    
    QDoubleSpinBox *armLengthBox = new QDoubleSpinBox;
    armLengthBox->setValue(m_vertebrataMotionParameters.armLength);
    parametersLayout->addRow(tr("Arm length: "), armLengthBox);
    connect(armLengthBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double value){
        m_vertebrataMotionParameters.armLength = value;
        m_parameters = fromVertebrataMotionParameters(m_vertebrataMotionParameters);
        emit parametersChanged();
    });
    
    QDoubleSpinBox *legSideIntvalBox = new QDoubleSpinBox;
    legSideIntvalBox->setValue(m_vertebrataMotionParameters.legSideIntval);
    parametersLayout->addRow(tr("Leg side intval: "), legSideIntvalBox);
    connect(legSideIntvalBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double value){
        m_vertebrataMotionParameters.legSideIntval = value;
        m_parameters = fromVertebrataMotionParameters(m_vertebrataMotionParameters);
        emit parametersChanged();
    });
    
    QDoubleSpinBox *legBalanceIntvalBox = new QDoubleSpinBox;
    legBalanceIntvalBox->setValue(m_vertebrataMotionParameters.legBalanceIntval);
    parametersLayout->addRow(tr("Leg balance intval: "), legBalanceIntvalBox);
    connect(legBalanceIntvalBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double value){
        m_vertebrataMotionParameters.legBalanceIntval = value;
        m_parameters = fromVertebrataMotionParameters(m_vertebrataMotionParameters);
        emit parametersChanged();
    });
    
    QDoubleSpinBox *spineStabilityBox = new QDoubleSpinBox;
    spineStabilityBox->setValue(m_vertebrataMotionParameters.spineStability);
    parametersLayout->addRow(tr("Spine stability: "), spineStabilityBox);
    connect(spineStabilityBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double value){
        m_vertebrataMotionParameters.spineStability = value;
        m_parameters = fromVertebrataMotionParameters(m_vertebrataMotionParameters);
        emit parametersChanged();
    });
    
    QSpinBox *cyclesBox = new QSpinBox;
    cyclesBox->setValue(m_vertebrataMotionParameters.cycles);
    parametersLayout->addRow(tr("Cycles: "), cyclesBox);
    connect(cyclesBox, QOverload<int>::of(&QSpinBox::valueChanged), [&](int value){
        m_vertebrataMotionParameters.cycles = value;
        m_parameters = fromVertebrataMotionParameters(m_vertebrataMotionParameters);
        emit parametersChanged();
    });
    
    QDoubleSpinBox *groundOffsetBox = new QDoubleSpinBox;
    groundOffsetBox->setRange(-2.0, 2.0);
    groundOffsetBox->setValue(m_vertebrataMotionParameters.groundOffset);
    parametersLayout->addRow(tr("Ground offset: "), groundOffsetBox);
    connect(groundOffsetBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double value){
        m_vertebrataMotionParameters.groundOffset = value;
        m_parameters = fromVertebrataMotionParameters(m_vertebrataMotionParameters);
        emit parametersChanged();
    });
    
    QDoubleSpinBox *tailLiftForceBox = new QDoubleSpinBox;
    tailLiftForceBox->setRange(-10.0, 10.0);
    tailLiftForceBox->setValue(m_vertebrataMotionParameters.tailLiftForce);
    tailLiftForceBox->setSuffix(" g");
    parametersLayout->addRow(tr("Tail lift force: "), tailLiftForceBox);
    connect(tailLiftForceBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double value){
        m_vertebrataMotionParameters.tailLiftForce = value;
        m_parameters = fromVertebrataMotionParameters(m_vertebrataMotionParameters);
        emit parametersChanged();
    });
    
    QDoubleSpinBox *tailDragForceBox = new QDoubleSpinBox;
    tailDragForceBox->setValue(m_vertebrataMotionParameters.tailDragForce);
    tailDragForceBox->setSuffix(" g");
    parametersLayout->addRow(tr("Tail drag force: "), tailDragForceBox);
    connect(tailDragForceBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double value){
        m_vertebrataMotionParameters.tailDragForce = value;
        m_parameters = fromVertebrataMotionParameters(m_vertebrataMotionParameters);
        emit parametersChanged();
    });
    
    setLayout(parametersLayout);
}
