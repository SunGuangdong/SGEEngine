#pragma once

#include "sge_engine/sge_engine_api.h"
#include "sge_utils/math/common.h"
#include "sge_utils/math/vec3f.h"

namespace sge {

/// @brief CharacterCtrlCfg describes how the character control should move, its speed, jump height and other parameters.
struct SGE_ENGINE_API CharacterCtrlCfg {

	vec3f defaultFacingDir = vec3f(0.f, 0.f, -1.f);

	/// Everything below this height in actor space is concidered feet.
	// Used to detect if the player is grounded. This is in object space.
	float feetLevel = 0.1f;

	/// Walk speed in units per second. This is in object space.
	float walkSpeed = 10.f;

	/// Cosine of the climbable slope angle.
	float minClimbableIncline = cosf(deg2rad(60.f));

	float jumpHeight = 5.f;               ///< The regular jump height of the cracter.
	float jumpTimeApex = 0.75f;             ///< The time (in seconds) that we wish the character to rach @jumpHeight.
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

struct SGE_ENGINE_API CharacterCtrlInput {
	/// A shortcut for initializing the strcture for AI input (as the class is used for AI and player character controllers).
	static CharacterCtrlInput aiInput(const vec3f& facingDir, const vec3f& walkDir)
	{
		CharacterCtrlInput in;
		in.facingDir = facingDir;
		in.walkDir = walkDir;
		return in;
	}

	/// The forward direction of the player in world space.
	vec3f facingDir = vec3f(0.f);

	/// Is the input direction of that the player specified this update.
	vec3f walkDir = vec3f(0.f);
	bool isJumpButtonPressed = false;
	bool isJumpBtnReleased = false;
};

/// @brief Hold the result after a single update of the rigid body.
struct SGE_ENGINE_API CharaterCtrlOutcome {
	bool isLogicallyGrounded = false;
	bool isGroundSlopeClimbable = false;
	bool didJustJumped = false;
	vec3f wallNormal = vec3f(0.f);
	vec3f facingDir = vec3f(1.f, 0.f, 0.f);
};

} // namespace sge
