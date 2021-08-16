#pragma once

#include "sge_core/shaders/LightDesc.h"
#include "sge_engine/Actor.h"
#include "sge_engine/traits/TraitCamera.h"
#include "sge_engine/traits/TraitViewportIcon.h"
#include "sge_utils/utils/optional.h"

namespace sge {

struct SGE_ENGINE_API ALight : public Actor {
	ALight() = default;

	void create() final;
	AABox3f getBBoxOS() const final;
	void update(const GameUpdateSets& updateSets) final;
	LightDesc getLightDesc() const { return m_lightDesc; }

  public:
	LightDesc m_lightDesc;
	TraitViewportIcon m_traitViewportIcon;
};

} // namespace sge
