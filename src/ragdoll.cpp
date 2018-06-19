#if USE_BULLET
#include "ragdoll.h"

Ragdoll::Ragdoll(const JointNodeTree &poseJointNodeTree, const JointNodeTree &ragdollJointNodeTree) :
    m_poseJointNodeTree(poseJointNodeTree),
    m_ragdollJointNodeTree(ragdollJointNodeTree)
{
}

Ragdoll::~Ragdoll()
{
    release();
}

void Ragdoll::setGroundPlaneY(float y)
{
    m_groundPlaneY = y;
}

void Ragdoll::release()
{
    delete m_world;
    m_world = nullptr;
    
    delete m_groundShape;
    m_groundShape = nullptr;
    
    delete m_groundMotionState;
    m_groundMotionState = nullptr;
    
    delete m_groundRigidBody;
    m_groundRigidBody = nullptr;
    
    for (auto &it: m_constraints)
        delete it;
    m_constraints.clear();
    
    for (auto &it: m_bodies)
        delete it;
    m_bodies.clear();
    
    for (auto &it: m_shapes)
        delete it;
    m_shapes.clear();
    
    for (auto &it: m_childShapes)
        delete it;
    m_childShapes.clear();
    
    for (auto &it: m_motionStates)
        delete it;
    m_motionStates.clear();
    
    delete m_collisionConfiguration;
    delete m_collisionDispather;
    delete m_broadphase;
    delete m_constraintSolver;
}

void Ragdoll::collectParts(std::vector<std::vector<int>> &parts)
{
    std::set<int> visited;
    for (const auto &joint: m_ragdollJointNodeTree.joints()) {
        if (SkeletonBoneMarkIsStart(joint.boneMark)) {
            std::vector<int> part;
            collectPartFrom(joint.jointIndex, part, visited);
            parts.push_back(part);
        }
    }
    std::vector<int> remaining;
    for (const auto &joint: m_ragdollJointNodeTree.joints()) {
        if (visited.find(joint.jointIndex) == visited.end())
            remaining.push_back(joint.jointIndex);
    }
    m_hasMainPart = !remaining.empty();
    if (!remaining.empty())
        parts.push_back(remaining);
}

void Ragdoll::collectPartFrom(int jointIndex, std::vector<int> &part, std::set<int> &visited)
{
    if (visited.find(jointIndex) != visited.end())
        return;
    visited.insert(jointIndex);
    part.push_back(jointIndex);
    for (const auto &child: m_ragdollJointNodeTree.joints()[jointIndex].children)
        collectPartFrom(child, part, visited);
}

#define SIMD_PI_4 ((M_PI) * 0.25f)

void Ragdoll::prepare()
{
    if (m_prepared)
        return;
    
    m_prepared = true;
    
    release();
    
    if (m_ragdollJointNodeTree.joints().empty())
        return;
    
    m_collisionConfiguration = new btDefaultCollisionConfiguration();
    m_collisionDispather = new btCollisionDispatcher(m_collisionConfiguration);
    m_broadphase = new btDbvtBroadphase();
    m_constraintSolver = new btSequentialImpulseConstraintSolver();
    
    m_world = new btDiscreteDynamicsWorld(m_collisionDispather, m_broadphase, m_constraintSolver, m_collisionConfiguration);
    m_world->setGravity(btVector3(0, -100, 0));
    
    m_groundShape = new btStaticPlaneShape(btVector3(0, 1, 0), 1);
    
    btTransform defaultMotionTransform;
    defaultMotionTransform.setIdentity();
    defaultMotionTransform.setOrigin(btVector3(0, m_groundPlaneY, 0));
    m_groundMotionState = new btDefaultMotionState(defaultMotionTransform);
    btRigidBody::btRigidBodyConstructionInfo groundRigidBodyInfo(0, m_groundMotionState, m_groundShape, btVector3(0, 0, 0));
    m_groundRigidBody = new btRigidBody(groundRigidBodyInfo);
    m_groundRigidBody->setFriction(1.0);
    m_groundRigidBody->setRollingFriction(1.0);
    m_world->addRigidBody(m_groundRigidBody);
    
    m_parts.clear();
    collectParts(m_parts);
    
    for (const auto &part: m_parts) {
        float mass = 1.0;
        btCompoundShape *shape = new btCompoundShape();
        for (int i = 0; i < (int)part.size(); i++) {
            const auto &joint = m_ragdollJointNodeTree.joints()[part[i]];

            btSphereShape *sphereShape = new btSphereShape(joint.radius);
            sphereShape->setUserIndex(joint.jointIndex);

            btTransform btSphereTransform;
            btSphereTransform.setIdentity();
            btSphereTransform.setOrigin(btVector3(joint.position.x(), joint.position.y(), joint.position.z()));
            shape->addChildShape(btSphereTransform, sphereShape);
            
            mass += 10000 * joint.radius;
            
            m_childShapes.push_back(sphereShape);
        }
        
        btVector3 localInertia(0, 0, 0);
        shape->calculateLocalInertia(mass, localInertia);
        
        btTransform defaultMotionTransform;
        defaultMotionTransform.setIdentity();
        
        btDefaultMotionState *motionState = new btDefaultMotionState(defaultMotionTransform);

        btRigidBody::btRigidBodyConstructionInfo bodyInfo(mass, motionState, shape, localInertia);
        btRigidBody *body = new btRigidBody(bodyInfo);
        body->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);
        body->setFriction(1.0);
        
        m_world->addRigidBody(body);
        
        m_shapes.push_back(shape);
        m_motionStates.push_back(motionState);
        m_bodies.push_back(body);
    }
    
    if (m_hasMainPart) {
        const auto &mainBody = m_bodies[m_parts.size() - 1];
        for (int i = 0; i < (int)m_parts.size() - 1; i++) {
            btConeTwistConstraint *constraint = nullptr;
            const auto &body = m_bodies[i];
            const auto &part = m_parts[i];
            const auto &joint = m_ragdollJointNodeTree.joints()[part[0]];
            const auto btLinkLocation = btVector3(joint.position.x(), joint.position.y(), joint.position.z());
            
            btTransform localA;
            btTransform localB;

            localA.setIdentity();
            localA.setOrigin(btLinkLocation);
            
            localB.setIdentity();
            localB.setOrigin(btLinkLocation);
            constraint = new btConeTwistConstraint(*mainBody,
                *body,
                localA,
                localB);
            constraint->setLimit(btScalar(SIMD_PI_4 * 0.1f), btScalar(SIMD_PI_4 * 0.1f), btScalar(SIMD_PI * 0.1f), 0.5f);
            m_constraints.push_back(constraint);
            
            const bool disableCollisionsBetweenLinkedBodies = true;
            m_world->addConstraint(constraint, disableCollisionsBetweenLinkedBodies);
        }
    }
}

void Ragdoll::stepSimulation(float amount)
{
    if (nullptr == m_world)
        return;
    
    m_world->stepSimulation(btScalar(amount));

    m_frame = RigFrame(m_ragdollJointNodeTree.joints().size());
    
    JointNodeTree outputNodeTree = m_poseJointNodeTree;
    for (int partIndex = 0; partIndex < (int)m_parts.size(); partIndex++) {
        const auto *body = m_bodies[partIndex];

        btTransform btWorldTransform;
        if (body->getMotionState())
            body->getMotionState()->getWorldTransform(btWorldTransform);
        else
            btWorldTransform = body->getWorldTransform();
        
        const btCompoundShape *shape = (const btCompoundShape *)m_shapes[partIndex];
        for (int i = 0; i < shape->getNumChildShapes(); i++) {
            const btSphereShape *childShape = (const btSphereShape *)shape->getChildShape(i);
            int jointIndex = childShape->getUserIndex();
            btTransform btChildTransform = btWorldTransform * shape->getChildTransform(i);
            const auto &btOrigin = btChildTransform.getOrigin();
            outputNodeTree.joints()[jointIndex].position = QVector3D(btOrigin.x(), btOrigin.y(), btOrigin.z());
        }
    }
    
    outputNodeTree.recalculateMatricesAfterPositionUpdated();
    for (const auto &joint: outputNodeTree.joints()) {
        m_frame.updateRotation(joint.jointIndex, joint.rotation);
        m_frame.updateTranslation(joint.jointIndex, joint.translation);
    }
}

void Ragdoll::saveFrame(RigFrame &frame)
{
    frame = m_frame;
}

#endif

