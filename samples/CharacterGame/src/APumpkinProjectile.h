#pragma once

#include "sge_engine/Actor.h"
#include "sge_engine/traits/TraitRigidBody.h"
#include "sge_engine/traits/TraitSprite.h"

namespace sge {

struct APumpkinProjectile : public Actor {

	virtual AABox3f getBBoxOS() const { return AABox3f(); }
	void create() override;
	void postUpdate(const GameUpdateSets& u) override;

	TraitRigidBody ttRigidbody;
	TraitSprite ttSprite;

	vec3f horizontalFlightDir = vec3f(0.f);
	float passedDistance = 0.f;
	float timeSpentAlive = 0.f;
};


} // namespace sge
