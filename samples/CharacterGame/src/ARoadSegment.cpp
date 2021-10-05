#pragma once

#include "sge_engine/Actor.h"
#include "sge_engine/traits/TraitModel.h"

namespace sge {

struct ARoadSegment : public Actor {
	ARoadSegment() = default;

	void create() {
		registerTrait(ttModel);

		ttModel.addModel("assets/roadSegment.mdl", true);
	}

	virtual AABox3f getBBoxOS() const { return ttModel.getBBoxOS(); }

	TraitModel ttModel;
};

RelfAddTypeId(ARoadSegment, 21'10'03'0001);
ReflBlock() {
	ReflAddActor(ARoadSegment);
}

} // namespace sge
