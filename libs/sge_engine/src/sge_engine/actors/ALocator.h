#pragma once

#include "sge_engine/Actor.h"

namespace sge {

//--------------------------------------------------------------------
// ALocator
//--------------------------------------------------------------------
struct SGE_ENGINE_API ALocator : public Actor {
	Box3f getBBoxOS() const final {
		Box3f result;
		result.expand(vec3f(-1.f));
		result.expand(vec3f(1.f));

		return result;
	}

	void create() final {}
};

//--------------------------------------------------------------------
// ABone
//--------------------------------------------------------------------
struct SGE_ENGINE_API ABone : public Actor {
	Box3f getBBoxOS() const final {
		Box3f result;
		result.expand(vec3f(-1.f));
		result.expand(vec3f(1.f));

		return result;
	}

	float boneLength = 0.1f;

	void create() final {}
};
} // namespace sge
