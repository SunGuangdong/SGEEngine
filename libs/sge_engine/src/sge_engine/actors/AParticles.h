#pragma once

#include "sge_engine/Actor.h"
#include "sge_engine/traits/TraitParticles.h"
#include "sge_engine/traits/TraitViewportIcon.h"

namespace sge {

ReflAddTypeIdExists(AParticlesSimple);
struct SGE_ENGINE_API AParticlesSimple : public Actor {
	Box3f getBBoxOS() const final;
	void create() final;
	void postUpdate(const GameUpdateSets& u) final;

  public:
	TraitParticlesSimple m_particles;
	TraitViewportIcon m_traitViewportIcon;
};

} // namespace sge
