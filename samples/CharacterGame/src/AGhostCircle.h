#pragma once

#include "sge_engine/Actor.h"
#include "sge_engine/traits/TraitSprite.h"
#include "sge_engine/traits/TraitRigidBody.h"

namespace sge {

struct AGhostCircle : public Actor {
	AGhostCircle() = default;

	void create();
	void update(const GameUpdateSets& UNUSED(updateSets));
	virtual AABox3f getBBoxOS() const { return ttSprite.getBBoxOS(); }

  public:
	
	TraitSprite ttSprite;
	TraitRigidBody ttRigidBody;
	bool isGoingPosZ = false;
};

} // namespace sge
