#ifndef DUST3D_RAGDOLL_H
#define DUST3D_RAGDOLL_H
#include <QObject>
#include <BulletDynamics/Dynamics/btDynamicsWorld.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>
#include <BulletDynamics/ConstraintSolver/btTypedConstraint.h>
#include <LinearMath/btVector3.h>
#include <LinearMath/btScalar.h>
#include <BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h>
#include <BulletCollision/BroadphaseCollision/btDbvtBroadphase.h>
#include <BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <QString>
#include <map>
#include <tuple>
#include <QStringList>
#include <QColor>
#include "rigger.h"
#include "jointnodetree.h"

class RagDoll : public QObject
{
    Q_OBJECT
public:
    RagDoll(const std::vector<RiggerBone> *rigBones,
        const JointNodeTree *initialJointNodeTree=nullptr);
    ~RagDoll();
    bool stepSimulation(float amount);
    const JointNodeTree &getStepJointNodeTree();
    const std::vector<std::tuple<QVector3D, QVector3D, float, float, QColor>> &getStepBonePositions();

private:
    btDefaultCollisionConfiguration *m_collisionConfiguration = nullptr;
    btCollisionDispatcher *m_collisionDispather = nullptr;
    btDbvtBroadphase *m_broadphase = nullptr;
    btSequentialImpulseConstraintSolver *m_constraintSolver = nullptr;
    btDynamicsWorld *m_world = nullptr;
    btCollisionShape *m_groundShape = nullptr;
    btRigidBody *m_groundBody = nullptr;
    float m_groundY = 0;
    std::vector<std::pair<QVector3D, QVector3D>> m_boneInitialPositions;
    
    std::map<QString, btCollisionShape *> m_boneShapes;
    std::map<QString, btRigidBody *> m_boneBodies;
    std::vector<btTypedConstraint *> m_boneConstraints;
    
    std::map<QString, QVector3D> m_boneMiddleMap;
    std::map<QString, float> m_boneLengthMap;
    std::map<QString, float> m_boneRadiusMap;
    
    JointNodeTree m_jointNodeTree;
    JointNodeTree m_stepJointNodeTree;
    std::vector<RiggerBone> m_bones;
    std::vector<std::tuple<QVector3D, QVector3D, float, float, QColor>> m_stepBonePositions;
    
    std::map<QString, int> m_boneNameToIndexMap;
    std::map<QString, std::vector<QString>> m_chains;
 
    btRigidBody *createRigidBody(btScalar mass, const btTransform &startTransform, btCollisionShape *shape);
    void createDynamicsWorld();
    void addConstraint(const RiggerBone &child, const RiggerBone &parent, bool isBorrowedParent=false);
};

#endif
