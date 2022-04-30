#include "PhysicsWorld.h"
#include "BulletHelper.h"
#include "PhysicsAction.h"
#include "RigidBody.h"
#include "SgeCollisionDispatcher.h"

namespace sge {

//-------------------------------------------------------------------------
// PhysicsWorld
//-------------------------------------------------------------------------
void PhysicsWorld::create()
{
	destroy();

	broadphase.reset(new btDbvtBroadphase());
	collisionConfiguration.reset(new btDefaultCollisionConfiguration());
	dispatcher.reset(new SgeCollisionDispatcher(collisionConfiguration.get()));
	solver.reset(new btSequentialImpulseConstraintSolver());
	dynamicsWorld.reset(
	    new btDiscreteDynamicsWorld(dispatcher.get(), broadphase.get(), solver.get(), collisionConfiguration.get()));
	dynamicsWorld->setForceUpdateAllAabbs(false);

	// [SGE_BULLET_GHOSTS]
	// Needed for ghost(kinematic objects) to be able to record collision manifolds.
	dynamicsWorld->getBroadphase()->getOverlappingPairCache()->setInternalGhostPairCallback(&m_ghostPairCallback);
}

void PhysicsWorld::destroy()
{
	dynamicsWorld.reset();
	solver.reset();
	dispatcher.reset();
	collisionConfiguration.reset();
	broadphase.reset();
}

void PhysicsWorld::addPhysicsObject(RigidBody& obj)
{
	sgeAssert(obj.getIsInWorldInternal() == false);

	obj.setIsInWorldInternal(true);

	if (obj.getBulletRigidBody()) {
		dynamicsWorld->addRigidBody(obj.getBulletRigidBody());
	}
	else {
		// Caution: [SGE_BULLET_GHOSTS]
		// By default ghost (kinematic) objects generate contacts with dynamic objects only.
		// No static objects. In order to generate contacts with them
		// we need to change their bullet filter mask.
		// We wanna do this as for example kinematic characters
		// need to be able to recover form collision with the level.
		dynamicsWorld->addCollisionObject(
		    obj.m_collisionObject.get(), btBroadphaseProxy::AllFilter, btBroadphaseProxy::AllFilter);
	}
}

void PhysicsWorld::removePhysicsObject(RigidBody& obj)
{
	if (obj.getBulletRigidBody()) {
		if (obj.isInWorld()) {
			dynamicsWorld->removeRigidBody(obj.getBulletRigidBody());
			obj.setIsInWorldInternal(false);
		}
	}
	else {
		if (obj.isInWorld()) {
			// For ghost objects bullet has no way of telling if a body is in
			// world or not, this is why we have out own way of checking.
			dynamicsWorld->removeCollisionObject(obj.m_collisionObject.get());
			obj.setIsInWorldInternal(false);
		}
	}
}

void PhysicsWorld::addAction(PhysicsAction* action)
{
	dynamicsWorld->addAction(action);
}
void PhysicsWorld::removeAction(PhysicsAction* action)
{
	dynamicsWorld->removeAction(action);
}

void PhysicsWorld::setGravity(const vec3f& gravity)
{
	if (dynamicsWorld) {
		dynamicsWorld->setGravity(toBullet(gravity));
	}
}

vec3f PhysicsWorld::getGravity() const
{
	if (dynamicsWorld) {
		return fromBullet(dynamicsWorld->getGravity());
	}

	return vec3f(0.f);
}

} // namespace sge
