#include "sge_engine/behaviours/KinematicCharacterCtrl.h"
#include "sge_engine/Actor.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/physics/PenetrationRecovery.h"
#include "sge_engine/physics/PhysicsWorld.h"
#include "sge_engine/physics/PhysicsWorldQuery.h"
#include "sge_engine/physics/RigidBody.h"

namespace sge {


struct FindContactCallback : public btManifoldResult {
	FindContactCallback(const btCollisionObjectWrapper* body0Wrap, const btCollisionObjectWrapper* body1Wrap)
	    : btManifoldResult(body0Wrap, body1Wrap)
	{
	}

	void FindContactCallback::addContactPoint(const btVector3& normalOnBInWorld, const btVector3& pointInWorldOnB, btScalar depth)
	{
		if (m_penetration_distance > depth) {
			const bool isSwapped = m_manifoldPtr->getBody0() != m_body0Wrap->getCollisionObject();
			m_penetration_distance = depth;
			m_other_compound_shape_index = isSwapped ? m_index0 : m_index1;
			m_pointWorld = isSwapped ? (pointInWorldOnB + (normalOnBInWorld * depth)) : pointInWorldOnB;

			m_pointNormalWorld = isSwapped ? normalOnBInWorld * -1 : normalOnBInWorld;
		}
	}

	bool hasPenetration() const { return m_penetration_distance < 0; }

	btVector3 m_pointNormalWorld;
	btVector3 m_pointWorld;
	btScalar m_penetration_distance = 0;
	int m_other_compound_shape_index = 0;
};

struct ScanCollisionsResult {
	bool hadAnyPenetration = false;
	btVector3 recoveryVector = btVector3(0.f, 0.f, 0.f);
	Optional<vec3f> groundNormalWs;
};

ScanCollisionsResult
    scanCollisions(btCollisionWorld* collisionWorld, btCollisionObject* colObj, float feetLevelWorld, float UNUSED(maxPenetrationDepth))
{
	ScanCollisionsResult result;

	// Find the bounding box of the collision object in world space.
	btVector3 bbMinWs;
	btVector3 bbMaxWs;
	colObj->getCollisionShape()->getAabb(colObj->getWorldTransform(), bbMinWs, bbMaxWs);

	PhysicsWorldQuery::BoxTestCallback aabbCallback;
	collisionWorld->getBroadphase()->aabbTest(bbMinWs, bbMaxWs, aabbCallback);

	// For each potentially found collision object that the @colObj might penetrate
	// find the actual penetration depth.
	btCollisionObjectWrapper obA(nullptr, colObj->getCollisionShape(), colObj, colObj->getWorldTransform(), -1, -1);
	for (const btBroadphaseProxy* proxy : aabbCallback.proxies) {
		btCollisionObject* const otherCo = (btCollisionObject*)(proxy->m_clientObject);

		// The aabbTest above will report the @colObj as well. Skip it.
		if (otherCo == colObj) {
			continue;
		}
		btCollisionObjectWrapper obB(nullptr, otherCo->getCollisionShape(), otherCo, otherCo->getWorldTransform(), -1, -1);

		// Find the collision algorithm that computes the actual penetration depth.
		btCollisionAlgorithm* algorithm = collisionWorld->getDispatcher()->findAlgorithm(&obA, &obB, nullptr, BT_CONTACT_POINT_ALGORITHMS);
		if (algorithm) {
			FindContactCallback manifold(&obA, &obB);
			algorithm->processCollision(&obA, &obB, collisionWorld->getDispatchInfo(), &manifold);

			if (manifold.hasPenetration()) {
				result.recoveryVector += manifold.m_pointNormalWorld * -manifold.m_penetration_distance;

				if (manifold.m_pointWorld.y() < feetLevelWorld) {
					vec3f newNrm = fromBullet(manifold.m_pointNormalWorld).normalized0();
					if (!result.groundNormalWs || newNrm.y > result.groundNormalWs->y) {
						result.groundNormalWs = newNrm;
					}
				}

				result.hadAnyPenetration = true;
			}

			// The algorithm has been created with placement new in bullet.
			// Call its destructor.
			algorithm->~btCollisionAlgorithm();
			collisionWorld->getDispatcher()->freeCollisionAlgorithm(algorithm);
		}
	}

	return result;
}

void CharacterCtrlKinematic::update(const CharacterCtrlInput& input, RigidBody& rb, float deltaTime)
{
	btCollisionObject* rbBullet = rb.getCollisionShape();

	Actor* actor = (Actor*)rb.actor;
	if (!actor) {
		return;
	}

	btCollisionWorld* collisionWorldBullet = actor->getWorld()->physicsWorld.dynamicsWorld.get();

	// Move by the velocity and then recover form the penetration.
	bool isNowOnFloor = false;

	ScanCollisionsResult scanCollision;
	{
		btTransform transformAfterMovement = rbBullet->getWorldTransform();
		transformAfterMovement.setOrigin(transformAfterMovement.getOrigin() + toBullet((velocity + inputVelocity) * deltaTime));
		rbBullet->setWorldTransform(transformAfterMovement);
		collisionWorldBullet->updateSingleAabb(rbBullet);

		scanCollision = scanCollisions(collisionWorldBullet, rbBullet, transformAfterMovement.getOrigin().y() + m_cfg.feetLevel, 0.02f);
		if (scanCollision.hadAnyPenetration) {
			btTransform newTransform = rbBullet->getWorldTransform();
			newTransform.setOrigin(newTransform.getOrigin() + scanCollision.recoveryVector);
			rbBullet->setWorldTransform(newTransform);
			collisionWorldBullet->updateSingleAabb(rbBullet);

			if (scanCollision.recoveryVector.y() > 0.f) {
				isNowOnFloor = true;
			}
		}
	}

	inputVelocity = vec3f(0.f);

	if (scanCollision.groundNormalWs) {
		// Reproject the input on the groundPlane.
		Plane groundPlane(scanCollision.groundNormalWs.get(), 0.f);
		vec3f projectedInput = groundPlane.Project(input.walkDir).normalized0();

		inputVelocity = projectedInput * m_cfg.walkSpeed;
	}
	else {
		inputVelocity.x = input.walkDir.x * m_cfg.walkSpeed;
		inputVelocity.z = input.walkDir.z * m_cfg.walkSpeed;
	}



	if (!isNowOnFloor) {
		velocity.y -= m_cfg.computeGravity(velocity.y < 0.f) * deltaTime;
	}
	else {
		isJumping = false;

		if (isJumping == false && input.isJumpButtonPressed) {
			isJumping = true;
			velocity.y += m_cfg.computeJumpAcceleration();
		}

		if (!isJumping && scanCollision.groundNormalWs) {
			velocity = -scanCollision.groundNormalWs.get() * deltaTime;
		}
	}

	if (input.isJumpBtnReleased) {
		if (velocity.y > m_cfg.computeMinJumpAcceleration())
			velocity.y = m_cfg.computeMinJumpAcceleration();
	}

	{
		transf3d newActorTrasnform = fromBullet(rbBullet->getWorldTransform());
		newActorTrasnform.s = actor->getTransform().s;
		actor->setTransformEx(newActorTrasnform, false, false, false);
	}

	CharaterCtrlOutcome outcome;

	vec3f targetFacingDir = facingDir;
	if (input.walkDir.lengthSqr()) {
		targetFacingDir = input.walkDir.normalized0();
	}

	facingDir = rotateTowards(facingDir, targetFacingDir, deltaTime * deg2rad(720.f), vec3f(0.f, 1.f, 0.f));


	facingRotation = quatf::fromNormalizedVectors(m_cfg.defaultFacingDir, facingDir, vec3f::axis_y());
}

} // namespace sge
