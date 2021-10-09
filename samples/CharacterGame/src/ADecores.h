#pragma once

#include "sge_engine/Actor.h"
#include "sge_engine/traits/TraitSprite.h"
#include "sge_utils/math/Random.h"

namespace sge {

struct ADecoreGhost : public Actor {
	ADecoreGhost() = default;

	void create();
	virtual AABox3f getBBoxOS() const { return ttSprite.getBBoxOS(); }
	void update(const GameUpdateSets& u);

  public:
	Random rnd;
	TraitSprite ttSprite;

	float curvingAngle = 0.f;
	float speed = 0.f;
	float lifeSpan = 0.f;

	float lifeAccum = 0.f;
};

} // namespace sge
