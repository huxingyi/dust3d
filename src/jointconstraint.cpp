#include "jointconstraint.h"

JointConstraint::JointConstraint(JointConstraintType type) :
    m_type(type)
{
}

void JointConstraint::setHingeLimit(float minDegrees, float maxDegrees, const QVector3D &planeNormal)
{
    m_minHingeDegrees = minDegrees;
    m_maxHingeDegrees = maxDegrees;
    m_hingePlaneNormal = planeNormal;
}

JointConstraintType JointConstraint::type() const
{
    return m_type;
}

float JointConstraint::minHingeDegrees() const
{
    return m_minHingeDegrees;
}

float JointConstraint::maxHingeDegrees() const
{
    return m_maxHingeDegrees;
}

const QVector3D &JointConstraint::hingePlaneNormal() const
{
    return m_hingePlaneNormal;
}
