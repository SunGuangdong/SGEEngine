#include "CharacterController.h"
#include "sge_core/ICore.h"
#include "sge_engine/Actor.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/physics/BulletHelper.h"
#include "sge_engine/physics/RigidBody.h"
#include "sge_engine/traits/TraitRigidBody.h"
#include "sge_utils/containers/StaticArray.h"

namespace sge {

inline float speedLerp2(const float& a, const float& b, const float speed, const float epsilon = 1e-6f)
{
	float const diff = b - a;
	float const fabsfDiff = fabsf(diff);

	// if the two points are too close together just return the target point.
	if (fabsfDiff < epsilon) {
		return b;
	}

	// TODO: handle double here.
	float k = float(speed) / fabsfDiff;

	if (k >= 1.f) {
		return b;
	}

	return (1.f - k) * a + k * b;
}

struct AInvisibleRigidObstacle;

CharaterCtrlOutcome CharacterCtrlDynamic::update(const GameUpdateSets& updateSets, const CharacterCtrlInput& input)
{
	const float kAllowedJumpTimeDelay = 0.1f; // The time we allow the player jump even if he is no longer on a walkable land.

	GameWorld* const world = m_actor->getWorld();

	CharaterCtrlOutcome outcome;

	// Compute the facing direction.
	if (input.walkDir.length() > 1e-5f) {
		m_targetFacingDir = input.walkDir;
	}

	if (true) {
		float k = (dot(input.facingDir, m_targetFacingDir) + 1.f) * 0.5f;
		float rotationSpeed = deg2rad(480.f) + deg2rad(480.f) * (1.f - k);
		outcome.facingDir =
		    rotateTowards(input.facingDir, m_targetFacingDir, updateSets.dt * rotationSpeed, vec3f(0.f, 1.f, 0.f)).normalized();
	}
	else {
		outcome.facingDir = m_targetFacingDir;
	}

	const float walkDirInputMagnitude = input.walkDir.length();

	if (walkDirInputMagnitude > 1e-3f) {
		float k = pow((dot(outcome.facingDir, input.walkDir / walkDirInputMagnitude) + 1.f) * 0.5f, 5.f);
		m_walkDirSmoothAccumulator = normalized(k * outcome.facingDir + (1.f - k) * input.walkDir);
	}
	else {
		m_walkDirSmoothAccumulator = vec3f(0.f);
	}

	// isLogicallyGrounded - this for the logic for all other game object and animations.
	// isGroundSlopeClimbable - The physics should use this to check if the player can walk normally or if it is falling.
	bool isLogicallyGrounded = false;
	bool isGroundSlopeClimbable = false;
	float groundCosine = 1.f; // The least extreme cosine between the ground and the up vector.
	vec3f groundNormal = vec3f(0, 1.f, 0);

	if (input.walkDir != vec3f(0.f)) {
		m_lastNonZeroWalkDir = input.walkDir;
	}

	TraitRigidBody* const myRigidBody = getTrait<TraitRigidBody>(m_actor);

	vec3f const initalLinearVelocty = myRigidBody->getRigidBody()->getLinearVel();
	vec3f const initalHorizontalVel(initalLinearVelocty.x, 0.f, initalLinearVelocty.z);

	// The velocity that is going to be applied.
	vec3f velocityToApply(0.f);

	const std::vector<const btPersistentManifold*>& manifolds = world->m_physicsManifoldList[myRigidBody->getRigidBody()];

	vec3f correctedWalkDir = m_walkDirSmoothAccumulator;

	for (int iManifold = 0; iManifold < manifolds.size(); ++iManifold) {
		const btPersistentManifold* const manifold = manifolds[iManifold];

		int usIdx = -1; // Which of the bodies on the manifold is "us" our character.
		const btCollisionObject* const other = getOtherBodyFromManifold(manifold, m_actor, &usIdx);

		if (usIdx != -1 && other && !(other->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE)) {
			for (int iCP = 0; iCP < manifold->getNumContacts(); ++iCP) {
				const btManifoldPoint& cp = manifold->getContactPoint(iCP);

				const vec3f localContantPoint = fromBullet(usIdx == 0 ? cp.m_localPointA : cp.m_localPointB);
				const vec3f hitNormal = fromBullet(usIdx == 0 ? cp.m_normalWorldOnB : -cp.m_normalWorldOnB);

				// Check for being grounded.
				const bool isContantOnFeet = localContantPoint.y < m_cfg.feetLevel;
				if (isContantOnFeet) {
					const float currentDotWithGround = hitNormal.dot(vec3f::axis_y());

					isLogicallyGrounded = true;
					if (currentDotWithGround > groundCosine) {
						groundCosine = currentDotWithGround;
						groundNormal = hitNormal;
					}
				}

				if (hitNormal.y < 0.99f && m_walkDirSmoothAccumulator.length() > 0.f) {
					vec3f b = quat_mul_pos(m_actor->getOrientation(), localContantPoint).x0z();
					if (dot(m_walkDirSmoothAccumulator, b) > 0.f) {
						if (hitNormal.y <= 1e-3f) {
							vec3f retargetInputDir = cross(hitNormal, vec3f::axis_y()).normalized0();
							correctedWalkDir = retargetInputDir * retargetInputDir.dot(m_walkDirSmoothAccumulator);
						}
					}
				}
			}
		}
	} // end for each manifold

	if (isLogicallyGrounded && groundCosine >= m_cfg.minClimbableIncline) {
		isGroundSlopeClimbable = true;
	}

	// Determine if the player is standing on climbable ground.
	const bool justStepOnGround = !m_wasGroundSlopeClimbable && isGroundSlopeClimbable;

	// If the character stepped on the ground re-enable jumping.
	if (m_jumpCounter != 0 && justStepOnGround) {
		m_jumpCounter = 0;
	}

	if (isGroundSlopeClimbable) {
		m_timeInAir = 0.f;
		// m_jumpCounter = 0;
	}
	else {
		m_timeInAir += updateSets.dt;
	}

	// Check if we can actually jump.
	if (input.isJumpButtonPressed && (m_jumpCounter < 2) && (m_timeInAir <= kAllowedJumpTimeDelay)) {
		outcome.didJustJumped = true;
		if (m_jumpCounter == 0) {
			velocityToApply += initalHorizontalVel * 0.25f / m_cfg.walkSpeed;
			sgeAssert(!velocityToApply.hasNan());
			m_jumpingHorizontalVelocity = initalHorizontalVel.length();
		}

		{
			float jumpVelocityY = m_cfg.computeJumpAcceleration();
			velocityToApply.y += jumpVelocityY - initalLinearVelocty.y; // +gravityY * updateSets.dt;
			sgeAssert(!velocityToApply.hasNan());

			m_jumpCounter++;
		}
	}

#if 1
	// GROUND MOVEMENT WALKING.
	if (isGroundSlopeClimbable && m_jumpCounter <= 0) {
		// velocityToApply += vec3f(0.f, -0.01f, 0.f);

		vec3f walkDirProjectedOnGround = correctedWalkDir;

		if (correctedWalkDir != vec3f(0.f) && !correctedWalkDir.hasNan()) {
			// Project the walking direction down on the ground in order to make walking on slopes smoother.
			walkDirProjectedOnGround = cross(cross(groundNormal, correctedWalkDir), groundNormal).normalized0();
		}

		sgeAssert(walkDirProjectedOnGround.hasNan() == false);

	#if 1
		walkDirProjectedOnGround *= walkDirInputMagnitude;

		// TODO: move the a cfg variable.
		float velocityChangeSpeed = 200.f;
		if (input.walkDir == vec3f(0.f)) {
			velocityChangeSpeed = 200.f;
		}

		// if (justStepOnGround) {
		//	velocityChangeSpeed = 2000.f;
		//}

		const float initialSpeed = initalLinearVelocty.length();
		float targetSpeed = m_cfg.walkSpeed * correctedWalkDir.length();

		float initialSpeedAlongMovementVector = 0.f;
		if (initialSpeed > 1e-3f) {
			initialSpeedAlongMovementVector = dot(walkDirProjectedOnGround.normalized0(), initalLinearVelocty);
			// Discard negative values, as they will make the character slide backwards.
			initialSpeedAlongMovementVector = maxOf(0.f, initialSpeedAlongMovementVector);
		}

		velocityToApply -= initalLinearVelocty;
		velocityToApply +=
		    walkDirProjectedOnGround * speedLerp2(initialSpeedAlongMovementVector, targetSpeed, velocityChangeSpeed * updateSets.dt);
		sgeAssert(!velocityToApply.hasNan());
	#else
		// float initialSpeedAlongMovementVector = maxOf(0.f,  dot(walkDirProjectedOnGround.normalized0(), initalLinearVelocty));
		float maxSpeed = 8.f;
		float timeToMaxSpeed = 0.5f;
		float acceleration = maxSpeed / timeToMaxSpeed;
		float drag = 2.f * acceleration / sqr(maxSpeed);

		// float targetSpeed = m_isRolling ? m_cfg.rollingSpeed : m_cfg.walkSpeed * walkDirInputMagnitude;
		velocityToApply += walkDirProjectedOnGround * acceleration * updateSets.dt;

		velocityToApply.x -= sign(initalLinearVelocty.x) * sqr(initalLinearVelocty.x) * drag * updateSets.dt * 0.5f;
		velocityToApply.z -= sign(initalLinearVelocty.z) * sqr(initalLinearVelocty.z) * drag * updateSets.dt * 0.5f;


	#endif
	}
	else {
		// Handle minimal jump height.
		if (isJumping() && input.isJumpBtnReleased) {
			if (initalLinearVelocty.y > m_cfg.computeMinJumpAcceleration()) {
				velocityToApply.y -= initalLinearVelocty.y;
				velocityToApply.y += m_cfg.computeMinJumpAcceleration();
				sgeAssert(!velocityToApply.hasNan());
			}
		}

		// Air horizontal movement:
		float const airVelocityChangeSpeed = 16.6f;
		vec3f targetHorizontalVel = input.walkDir * maxOf(m_cfg.walkSpeed * 1.f, m_jumpingHorizontalVelocity);

		velocityToApply +=
		    speedLerp(initalHorizontalVel, targetHorizontalVel, airVelocityChangeSpeed * updateSets.dt) - initalHorizontalVel;
		sgeAssert(!velocityToApply.hasNan());
	}
#endif

	// Handle gravity:
	{
		// Use lower gravity when the player is sliding on a wall in order to give them time to react.
		// However apply it only if the sliding is downwards, becuse when the player tries to jump
		// to a platform while touching the wall they will get frustratingly slowed down.
		float gravity = m_cfg.computeGravity(initalLinearVelocty.y < -1e-1f);

		if (!isGroundSlopeClimbable || isJumping()) {
			myRigidBody->getRigidBody()->setGravity(vec3f(0, -gravity, 0));
		}
		else {
			if (isGroundSlopeClimbable) {
				myRigidBody->getRigidBody()->setGravity(groundNormal * -3.f * m_cfg.computeGravity(false));
			}
		}
	}

	// Apply the gameplay veloctiy.
	velocityToApply += gamplayAppliedLinearImpuse;
	gamplayAppliedLinearImpuse = vec3f(0.f);

	// Apply the computed velocity to the rigid body.
	sgeAssert(!velocityToApply.hasNan());
	myRigidBody->getRigidBody()->applyLinearVelocity(velocityToApply);
	myRigidBody->getRigidBody()->setAngularVelocity(vec3f(0.f));

	m_wasGroundSlopeClimbable = isGroundSlopeClimbable;

	outcome.isGroundSlopeClimbable = isGroundSlopeClimbable;
	outcome.isLogicallyGrounded = isLogicallyGrounded;

	return outcome;
}
} // namespace sge
