#pragma once

#include "sge_engine/sge_engine_api.h"
#include "sge_utils/math/common.h"
#include "sge_utils/math/quatf.h"
#include "sge_utils/math/vec3f.h"

#include "CharacterCtrlCommon.h"


namespace sge {
struct Actor;
struct GameUpdateSets;
struct RigidBody;


struct SGE_ENGINE_API CharacterCtrlKinematic {
	void update(const CharacterCtrlInput& input, RigidBody& rb, float deltaTime);

  public:
	CharacterCtrlCfg m_cfg;

	// State of the character:
	bool isJumping = false;
	vec3f velocity = vec3f(0.f);
	vec3f inputVelocity = vec3f(0.f);

	vec3f facingDir = vec3f(0.f, 0.f, -1.f);
	quatf facingRotation = quatf::getIdentity();
};

} // namespace sge
