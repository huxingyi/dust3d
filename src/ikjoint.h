#ifndef IK_JOINT_H
#define IK_JOINT_H
#include "jointnodetree.h"

void moveIkJoints(const JointNodeTree &inputJointNodeTree, JointNodeTree &outputJointNodeTree,
    int startJointIndex, int endJointIndex, QVector3D destination);

#endif
