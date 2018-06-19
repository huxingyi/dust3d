#ifndef RAGDOLL_H
#define RAGDOLL_H
#if USE_BULLET
#include "BulletCollision/CollisionShapes/btCapsuleShape.h"
#include "BulletCollision/CollisionShapes/btStaticPlaneShape.h"
#include "BulletCollision/CollisionShapes/btMultiSphereShape.h"
#include "BulletCollision/CollisionShapes/btCompoundShape.h"
#include "BulletCollision/CollisionShapes/btSphereShape.h"
#include "BulletDynamics/Dynamics/btRigidBody.h"
#include "BulletDynamics/ConstraintSolver/btTypedConstraint.h"
#include "BulletDynamics/ConstraintSolver/btHingeConstraint.h"
#include "BulletDynamics/ConstraintSolver/btPoint2PointConstraint.h"
#include "BulletDynamics/ConstraintSolver/btGeneric6DofConstraint.h"
#include "BulletDynamics/ConstraintSolver/btConeTwistConstraint.h"
#include "BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h"
#include "BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h"
#include "BulletCollision/BroadphaseCollision/btDbvtBroadphase.h"
#include "BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h"
#include "LinearMath/btDefaultMotionState.h"
#include "jointnodetree.h"
#include "rigframe.h"

class Ragdoll
{
public:
    Ragdoll(const JointNodeTree &poseJointNodeTree, const JointNodeTree &ragdollJointNodeTree);
    ~Ragdoll();
    void prepare();
    void stepSimulation(float amount);
    void saveFrame(RigFrame &frame);
    void setGroundPlaneY(float y);
private:
    void release();
    void collectParts(std::vector<std::vector<int>> &parts);
    void collectPartFrom(int jointIndex, std::vector<int> &part, std::set<int> &visited);
private:
    JointNodeTree m_poseJointNodeTree;
    JointNodeTree m_ragdollJointNodeTree;
    std::vector<btCollisionShape *> m_shapes;
    std::vector<btTypedConstraint *> m_constraints;
    std::vector<btDefaultMotionState *> m_motionStates;
    std::vector<btRigidBody *> m_bodies;
    std::vector<btSphereShape *> m_childShapes;
    std::vector<std::vector<int>> m_parts;
    btDefaultCollisionConfiguration *m_collisionConfiguration = nullptr;
    btCollisionDispatcher *m_collisionDispather = nullptr;
    btDbvtBroadphase *m_broadphase = nullptr;
    btSequentialImpulseConstraintSolver *m_constraintSolver = nullptr;
    btDiscreteDynamicsWorld *m_world = nullptr;
    btCollisionShape *m_groundShape = nullptr;
    btDefaultMotionState *m_groundMotionState = nullptr;
    btRigidBody *m_groundRigidBody = nullptr;
    bool m_prepared = false;
    RigFrame m_frame;
    float m_groundPlaneY = -1.5;
    bool m_hasMainPart = false;
};

#endif
#endif
