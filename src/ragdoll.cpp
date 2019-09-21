#define _USE_MATH_DEFINES
#include <cmath>
#include <LinearMath/btDefaultMotionState.h>
#include <LinearMath/btAlignedAllocator.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>
#include <BulletDynamics/ConstraintSolver/btHingeConstraint.h>
#include <BulletDynamics/ConstraintSolver/btConeTwistConstraint.h>
#include <BulletDynamics/ConstraintSolver/btPoint2PointConstraint.h>
#include <BulletDynamics/ConstraintSolver/btFixedConstraint.h>
#include <BulletDynamics/ConstraintSolver/btTypedConstraint.h>
#include <QQuaternion>
#include <QtMath>
#include <QMatrix4x4>
#include "ragdoll.h"
#include "poser.h"

RagDoll::RagDoll(const std::vector<RiggerBone> *rigBones) :
    m_jointNodeTree(rigBones),
    m_stepJointNodeTree(rigBones)
{
    if (nullptr == rigBones || rigBones->empty())
        return;
    
    m_bones = *rigBones;
    
    for (const auto &bone: m_bones) {
        auto radius = qMax(bone.headRadius, bone.tailRadius);
        m_stepBonePositions.push_back(std::make_tuple(bone.headPosition, bone.tailPosition, radius));
        float groundY = bone.headPosition.y() - radius;
        if (groundY < m_groundY)
            m_groundY = groundY;
        groundY = bone.tailPosition.y() - radius;
        if (groundY < m_groundY)
            m_groundY = groundY;
    }
    
    createDynamicsWorld();
    
    std::vector<QString> boneNames;
    for (const auto &bone: *rigBones) {
        m_boneNameToIndexMap[bone.name] = bone.index;
        boneNames.push_back(bone.name);
    }
    Poser::fetchChains(boneNames, m_chains);
    
    for (const auto &bone: *rigBones) {
        float radius = (bone.headRadius + bone.tailRadius) * 0.5;
        float height = bone.headPosition.distanceToPoint(bone.tailPosition);
        QVector3D middlePosition = (bone.headPosition + bone.tailPosition) * 0.5;
        m_boneLengthMap[bone.name] = height;
        m_boneRadiusMap[bone.name] = radius;
        m_boneMiddleMap[bone.name] = middlePosition;
    }
    
    std::set<std::pair<QString, QString>> constraintPairs;
    
    for (const auto &bone: m_bones) {
        float height = m_boneLengthMap[bone.name];
        float radius = m_boneRadiusMap[bone.name];
        float mass = 1.0;

        btCollisionShape *shape = new btCapsuleShape(btScalar(radius), btScalar(height));
        shape->setUserIndex(bone.index);
        
        m_boneShapes[bone.name] = shape;
        
        btTransform transform;
        const auto &middlePosition = m_boneMiddleMap[bone.name];
        transform.setIdentity();
        transform.setOrigin(btVector3(
            btScalar(middlePosition.x()),
            btScalar(middlePosition.y()),
            btScalar(middlePosition.z())
        ));
        QVector3D to = (bone.tailPosition - bone.headPosition).normalized();
        QVector3D from = QVector3D(0, 1, 0);
        QQuaternion rotation = QQuaternion::rotationTo(from, to);
        btQuaternion btRotation(btScalar(rotation.x()), btScalar(rotation.y()), btScalar(rotation.z()),
            btScalar(rotation.scalar()));
        transform.getBasis().setRotation(btRotation);
        
        btRigidBody *body = createRigidBody(btScalar(mass), transform, shape);
        m_boneBodies[bone.name] = body;
    }
    
    // Setup some damping on the m_bodies
    for (const auto &bone: m_bones) {
        m_boneBodies[bone.name]->setDamping(btScalar(0.05), btScalar(0.85));
        m_boneBodies[bone.name]->setDeactivationTime(btScalar(0.8));
        m_boneBodies[bone.name]->setSleepingThresholds(btScalar(1.6), btScalar(2.5));
    }
    
    for (const auto &it: m_chains) {
        for (size_t i = 1; i < it.second.size(); ++i) {
            const auto &parent = (*rigBones)[m_boneNameToIndexMap[it.second[i - 1]]];
            const auto &child = (*rigBones)[m_boneNameToIndexMap[it.second[i]]];
            if (constraintPairs.find(std::make_pair(parent.name, child.name)) == constraintPairs.end()) {
                constraintPairs.insert(std::make_pair(parent.name, child.name));
                if (parent.name.startsWith("Spine")) {
                    addFixedConstraint(parent, child);
                } else {
                    addFreeConstraint(parent, child);
                }
            }
        }
    }
    
    for (const auto &parent: m_bones) {
        for (const auto &childIndex: parent.children) {
            const auto &child = m_bones[childIndex];
            if (parent.children.size() <= 1 && !child.name.startsWith("Virtual"))
                continue;
            if (constraintPairs.find(std::make_pair(parent.name, child.name)) == constraintPairs.end()) {
                constraintPairs.insert(std::make_pair(parent.name, child.name));
                addFixedConstraint(parent, child);
            }
        }
    }
    
    for (const auto &parent: m_bones) {
        for (const auto &childIndex: parent.children) {
            const auto &child = m_bones[childIndex];
            if (constraintPairs.find(std::make_pair(parent.name, child.name)) == constraintPairs.end()) {
                constraintPairs.insert(std::make_pair(parent.name, child.name));
                addFreeConstraint(parent, child);
            }
        }
    }
    
    for (const auto &bone: m_bones) {
        for (const auto &childIndex: bone.children) {
            for (const auto &otherIndex: bone.children) {
                if (childIndex == otherIndex)
                    continue;
                m_boneBodies[m_bones[childIndex].name]->setIgnoreCollisionCheck(m_boneBodies[m_bones[otherIndex].name], true);
            }
        }
    }
    
    for (const auto &it: m_chains) {
        const auto &name = it.second[0];
        const auto &leftBody = m_boneBodies.find(name);
        if (leftBody == m_boneBodies.end())
            continue;
        const QString left = "Left";
        if (name.startsWith(left)) {
            QString pairedName = "Right" + name.mid(left.length());
            const auto &rightBody = m_boneBodies.find(pairedName);
            if (rightBody == m_boneBodies.end())
                continue;
            leftBody->second->setIgnoreCollisionCheck(rightBody->second, true);
            rightBody->second->setIgnoreCollisionCheck(leftBody->second, true);
        }
    }
}

void RagDoll::addFreeConstraint(const RiggerBone &parent, const RiggerBone &child)
{
    btRigidBody *parentBoneBody = m_boneBodies[parent.name];
    btRigidBody *childBoneBody = m_boneBodies[child.name];
    
    if (nullptr == parentBoneBody || nullptr == childBoneBody)
        return;
    
    float parentLength = m_boneLengthMap[parent.name];
    float childLength = m_boneLengthMap[child.name];
    const btVector3 btPivotA(0, parentLength * 0.5, 0.0f);
    const btVector3 btPivotB(0, -childLength * 0.5, 0.0f);
    
    btPoint2PointConstraint *constraint = new btPoint2PointConstraint(*parentBoneBody, *childBoneBody,
        btPivotA, btPivotB);
    m_world->addConstraint(constraint, true);
    m_boneConstraints.push_back(constraint);
}

void RagDoll::addFixedConstraint(const RiggerBone &parent, const RiggerBone &child)
{
    btRigidBody *parentBoneBody = m_boneBodies[parent.name];
    btRigidBody *childBoneBody = m_boneBodies[child.name];
    
    if (nullptr == parentBoneBody || nullptr == childBoneBody)
        return;
    
    float parentLength = m_boneLengthMap[parent.name];
    float childLength = m_boneLengthMap[child.name];
    const btVector3 btPivotA(0, parentLength * 0.5, 0.0f);
    const btVector3 btPivotB(0, -childLength * 0.5, 0.0f);
    
    btTransform localA;
    btTransform localB;
    
    btFixedConstraint *constraint = nullptr;
    
    localA.setIdentity();
    localB.setIdentity();
    localA.setOrigin(btPivotA);
    localB.setOrigin(btPivotB);
    
    constraint = new btFixedConstraint(*parentBoneBody, *childBoneBody, localA, localB);
    m_world->addConstraint(constraint, true);
    m_boneConstraints.push_back(constraint);
}

RagDoll::~RagDoll()
{
    for (auto &constraint: m_boneConstraints) {
        delete constraint;
    }
    m_boneConstraints.clear();
    for (auto &body: m_boneBodies) {
        if (nullptr == body.second)
            continue;
        m_world->removeRigidBody(body.second);
        delete body.second->getMotionState();
        delete body.second;
    }
    m_boneBodies.clear();
    for (auto &shape: m_boneShapes) {
        delete shape.second;
    }
    m_boneShapes.clear();
    
    delete m_groundShape;
    delete m_groundBody;
    
    delete m_world;
    
    delete m_collisionConfiguration;
    delete m_collisionDispather;
    delete m_broadphase;
    delete m_constraintSolver;
}

void RagDoll::createDynamicsWorld()
{
    m_collisionConfiguration = new btDefaultCollisionConfiguration();
    m_collisionDispather = new btCollisionDispatcher(m_collisionConfiguration);
    m_broadphase = new btDbvtBroadphase();
    m_constraintSolver = new btSequentialImpulseConstraintSolver();
    
    m_world = new btDiscreteDynamicsWorld(m_collisionDispather, m_broadphase, m_constraintSolver, m_collisionConfiguration);
    m_world->setGravity(btVector3(0, -10, 0));
    
    m_world->getSolverInfo().m_solverMode |= SOLVER_ENABLE_FRICTION_DIRECTION_CACHING;
    m_world->getSolverInfo().m_numIterations = 5;
    
    m_groundShape = new btBoxShape(btVector3(btScalar(250.), btScalar(250.), btScalar(250.)));

    btTransform groundTransform;
    groundTransform.setIdentity();
    groundTransform.setOrigin(btVector3(0, m_groundY - 250, 0));
    m_groundBody = createRigidBody(0, groundTransform, m_groundShape);
}

bool RagDoll::stepSimulation(float amount)
{
    bool positionChanged = false;
    m_world->stepSimulation(btScalar(amount));
    
    if (m_boneBodies.empty())
        return positionChanged;
    
    m_stepJointNodeTree = m_jointNodeTree;
    
    auto updateToNewPosition = [&](const QString &boneName, const btTransform &btWorldTransform, int jointIndex) {
        float halfHeight = m_boneLengthMap[boneName] * 0.5;
        btVector3 oldBoneHead(btScalar(0.0f), btScalar(-halfHeight), btScalar(0.0f));
        btVector3 oldBoneTail(btScalar(0.0f), btScalar(halfHeight), btScalar(0.0f));
        btVector3 newBoneHead = btWorldTransform * oldBoneHead;
        btVector3 newBoneTail = btWorldTransform * oldBoneTail;
        std::get<0>(m_stepBonePositions[jointIndex]) = QVector3D(newBoneHead.x(), newBoneHead.y(), newBoneHead.z());
        std::get<1>(m_stepBonePositions[jointIndex]) = QVector3D(newBoneTail.x(), newBoneTail.y(), newBoneTail.z());
    };
    for (const auto &it: m_boneShapes) {
        btTransform btWorldTransform;
        const auto *body = m_boneBodies[it.first];
        if (nullptr == body)
            continue;
        if (body->isActive()) {
            positionChanged = true;
        }
        if (body->getMotionState()) {
            body->getMotionState()->getWorldTransform(btWorldTransform);
        } else {
            btWorldTransform = body->getWorldTransform();
        }
        int jointIndex = it.second->getUserIndex();
        if (-1 == jointIndex)
            continue;
        updateToNewPosition(it.first, btWorldTransform, jointIndex);
    }
    
    if (!m_bones[0].children.empty()) {
        QVector3D sumOfRootBoneTail;
        for (const auto &childIndex: m_bones[0].children) {
            sumOfRootBoneTail += std::get<0>(m_stepBonePositions[childIndex]);
        }
        QVector3D modelTranslation = sumOfRootBoneTail / m_bones[0].children.size();
        m_stepJointNodeTree.addTranslation(0,
            QVector3D(0, modelTranslation.y() - m_stepJointNodeTree.nodes()[0].translation.y(), 0));
    }
    
    std::vector<QVector3D> directions(m_stepBonePositions.size());
    for (size_t index = 0; index < m_stepBonePositions.size(); ++index) {
        const auto &bone = m_bones[index];
        directions[index] = bone.tailPosition - bone.headPosition;
    }
    std::function<void(size_t index, const QQuaternion &rotation)> rotateChildren;
    rotateChildren = [&](size_t index, const QQuaternion &rotation) {
        const auto &bone = m_bones[index];
        for (const auto &childIndex: bone.children) {
            directions[childIndex] = rotation.rotatedVector(directions[childIndex]);
            rotateChildren(childIndex, rotation);
        }
    };
    for (size_t index = 1; index < m_stepBonePositions.size(); ++index) {
        if (m_bones[index].name.startsWith("Virtual") ||
                m_bones[index].name.startsWith("Spine01"))
            continue;
        QQuaternion rotation;
        const auto &oldDirection = directions[index];
        QVector3D newDirection = std::get<1>(m_stepBonePositions[index]) - std::get<0>(m_stepBonePositions[index]);
        rotation = QQuaternion::rotationTo(oldDirection.normalized(), newDirection.normalized());
        m_stepJointNodeTree.updateRotation(index, rotation);
        rotateChildren(index, rotation);
    }
    
    m_stepJointNodeTree.recalculateTransformMatrices();
    return positionChanged;
}

const JointNodeTree &RagDoll::getStepJointNodeTree()
{
    return m_stepJointNodeTree;
}

const std::vector<std::tuple<QVector3D, QVector3D, float>> &RagDoll::getStepBonePositions()
{
    return m_stepBonePositions;
}

btRigidBody *RagDoll::createRigidBody(btScalar mass, const btTransform &startTransform, btCollisionShape *shape)
{
    bool isDynamic = !qFuzzyIsNull(mass);

    btVector3 localInertia(0, 0, 0);
    if (isDynamic)
        shape->calculateLocalInertia(mass, localInertia);

    btDefaultMotionState *myMotionState = new btDefaultMotionState(startTransform);

    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, shape, localInertia);
    btRigidBody *body = new btRigidBody(rbInfo);
    body->setFriction(1.0f);
    body->setRollingFriction(1.0f);
    body->setSpinningFriction(1.0f);

    m_world->addRigidBody(body);

    return body;
}
