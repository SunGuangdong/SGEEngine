#pragma once

#include "sge_engine/sge_engine_api.h"
#include "sge_utils/math/common.h"
#include "sge_utils/math/vec4.h"

namespace sge {

struct Actor;
struct GameUpdateSets;
struct RigidBody;

/// @brief CharacterCtrlCfg describes how the character control should move, its speed, jump height and other parameters.
struct SGE_ENGINE_API CharacterCtrlCfg {
	/// Everything below this height in actor space is concidered feet.
	// Used to detect if the player is grounded. This is in object space.
	float feetLevel = 0.1f;

	/// Walk speed in units per second. This is in object space.
	float walkSpeed = 8.87f;

	/// Cosine of the climbable slope angle.
	float minClimbableIncline = cosf(deg2rad(60.f));

	float jumpHeight = 5.f;               ///< The regular jump height of the cracter.
	float jumpTimeApex = 1.f;             ///< The time (in seconds) that we wish the character to rach @jumpHeight.
	float minJumpHeight = 2.f;            ///< The minium jump hight that we want if the player released the jump button.
	float fallingGravityMultiplier = 2.f; ///< How much stronger we want the gravity to be if the character is falling. In mario games it is
	                                      ///< used to make the jump feel better.

	/// The size of the gravity force.
	/// If the @isFallingGravity == true, than this gravity should be aplied when the
	/// character is going upwards, so its jump height and time to jump apex could be used correctly.
	/// If @isFallingGravity == false, than this gravity should be applied when the character is moving downwards.
	/// This is used usually for platformers for better 'feel'.
	float computeGravity(bool isFallingGravity) const
	{
		float gravity = 2.f * jumpHeight / (jumpTimeApex * jumpTimeApex);
		if (isFallingGravity) {
			gravity *= fallingGravityMultiplier;
		}

		return gravity;
	}

	/// The velocity to be applied when the player jumps.
	/// The player will reach @minJumpHeight after @jumpTimeApex seconds.
	float computeJumpAcceleration() const
	{
		float gravity = computeGravity(false);
		float vel = gravity * jumpTimeApex;
		return vel;
	}

	/// Useful for variable jump height (aka. the longer you hold the jump button the higher you jump).
	/// The velocity to be applied if the player release the jump button and if the character current upwards velocity
	/// is less than this value.
	/// As a result the character will jump at least @minJumpHeight high.
	float computeMinJumpAcceleration() const
	{
		float gravity = computeGravity(false);
		float vel = sqrtf(2.f * gravity * minJumpHeight);
		return vel;
	}
};

/// @brief Hold the result after a single update of the rigid body.
struct SGE_ENGINE_API CharaterCtrlOutcome {
	bool isLogicallyGrounded = false;
	bool isGroundSlopeClimbable = false;
	bool didJustJumped = false;
	vec3f wallNormal = vec3f(0.f);
	vec3f facingDir = vec3f(1.f, 0.f, 0.f);
};

struct SGE_ENGINE_API CharacterCtrlInput {
	/// A shortcut for initializing the strcture for AI input (as the class is used for AI and player character controllers).
	static CharacterCtrlInput aiInput(const vec3f& facingDir, const vec3f& walkDir)
	{
		CharacterCtrlInput in;
		in.facingDir = facingDir;
		in.walkDir = walkDir;
		return in;
	}

	vec3f facingDir = vec3f(0.f); /// The forward direction of the player in world space.
	vec3f walkDir = vec3f(0.f);   /// Is the input direction of that the player specified this update.
	bool isJumpButtonPressed = false;
	bool isJumpBtnReleased = false;
};

/// @brief A character controller that takes an Actor and its parameter
/// add add forces to its rigid body for the rigid body should behave as a character
/// it is expected that the the angular velocity of the rigid body is already set to 0 (to avoid tippling of the rigid body).
struct SGE_ENGINE_API CharacterCtrl {
	/// @brief Call this function to make the rigid body of the actor ot appear as a character.
	/// Use the return value to change the facing direction of the character).
	CharaterCtrlOutcome update(const GameUpdateSets& updateSets, const CharacterCtrlInput& input);

	bool isJumping() const
	{
		return m_jumpCounter > 0;
	}


  public:
	Actor* m_actor = nullptr;
	CharacterCtrlCfg m_cfg;

	// Since this character controller always modifies its velocity, when other objects
	// want the character to move in a certain way, they will modify this value.
	// For example this could be conveyor belts or jump pads.
	vec3f gamplayAppliedLinearVelocity = vec3f(0.f);

	vec3f m_lastNonZeroWalkDir = vec3f(0.f);
	bool m_wasGroundSlopeClimbable = false;
	int m_jumpCounter = 0;
	vec3f m_targetFacingDir = vec3f(1.f, 0.f, 0.f);
	vec3f m_walkDirSmoothAccumulator = vec3f(1.f, 0.f, 0.f);
	float m_timeInAir = 0.f;
	float m_jumpingHorizontalVelocity = 0.f;
};

} // namespace sge
