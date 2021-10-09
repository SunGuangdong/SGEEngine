#pragma once

#include "sge_engine/Actor.h"
#include "sge_engine/traits/TraitSprite.h"
#include "sge_engine/traits/TraitRigidBody.h"

namespace sge {

struct ABat : public Actor {
	ABat() = default;

	void create();
	void update(const GameUpdateSets& UNUSED(updateSets));
	virtual AABox3f getBBoxOS() const { return ttSprite.getBBoxOS(); }

  public:

	std::shared_ptr<Asset> frames[2];

	int batAnimationIndex = 0;
	TraitSprite ttSprite;
	TraitRigidBody ttRigidBody;
};

} // namespace sge
