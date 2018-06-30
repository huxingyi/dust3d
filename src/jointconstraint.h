#ifndef JOINT_CONSTRAINT_H
#define JOINT_CONSTRAINT_H
#include <QVector3D>

// http://bulletphysics.org/mediawiki-1.5.8/index.php/Constraints

enum class JointConstraintType
{
    Hinge,
    ConeTwist
};

class JointConstraint
{
public:
    JointConstraint(JointConstraintType type=JointConstraintType::Hinge);
    void setHingeLimit(float minDegrees, float maxDegrees, const QVector3D &planeNormal);
    JointConstraintType type() const;
    float minHingeDegrees() const;
    float maxHingeDegrees() const;
    const const QVector3D &hingePlaneNormal() const;
private:
    JointConstraintType m_type;
    float m_minHingeDegrees = 0;
    float m_maxHingeDegrees = 0;
    QVector3D m_hingePlaneNormal;
};

#endif
