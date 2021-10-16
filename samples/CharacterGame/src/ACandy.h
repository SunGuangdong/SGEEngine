#pragma once

#include "sge_engine/Actor.h"
#include "sge_engine/traits/TraitSprite.h"
#include "sge_engine/traits/TraitRigidBody.h"

namespace sge {

struct ACandy : public Actor {
	ACandy() = default;

	void create();
	void update(const GameUpdateSets& UNUSED(updateSets));
	virtual AABox3f getBBoxOS() const { return ttSprite.getBBoxOS(); }

  public:

	TraitSprite ttSprite;
	TraitRigidBody ttRigidBody;
	bool isPickedUp = false;
	float timeSpendPickedUp = 0.f;
};

} // namespace sge
