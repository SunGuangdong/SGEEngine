#pragma once

#include "sge_engine/sge_engine_api.h"
#include "sge_utils/math/common.h"
#include "sge_utils/math/vec4f.h"

#include "CharacterCtrlCommon.h"

namespace sge {

struct Actor;
struct GameUpdateSets;
struct RigidBody;


/// @brief A character controller that takes an Actor and its parameter
/// add add forces to its rigid body for the rigid body should behave as a character
/// it is expected that the the angular velocity of the rigid body is already set to 0 (to avoid tippling of the rigid body).
struct SGE_ENGINE_API CharacterCtrlDynamic {
	/// @brief Call this function to make the rigid body of the actor ot appear as a character.
	/// Use the return value to change the facing direction of the character).
	CharaterCtrlOutcome update(const GameUpdateSets& updateSets, const CharacterCtrlInput& input);

	bool isJumping() const { return m_jumpCounter > 0; }

  public:
	Actor* m_actor = nullptr;
	CharacterCtrlCfg m_cfg;

	// Since this character controller always modifies its velocity, when other objects
	// want the character to move in a certain way, they will modify this value.
	// For example this could be conveyor belts or jump pads.
	vec3f gamplayAppliedLinearImpuse = vec3f(0.f);

	vec3f m_lastNonZeroWalkDir = vec3f(0.f);
	bool m_wasGroundSlopeClimbable = false;
	int m_jumpCounter = 0;
	vec3f m_targetFacingDir = vec3f(1.f, 0.f, 0.f);
	vec3f m_walkDirSmoothAccumulator = vec3f(1.f, 0.f, 0.f);
	float m_timeInAir = 0.f;
	float m_jumpingHorizontalVelocity = 0.f;
};

} // namespace sge
