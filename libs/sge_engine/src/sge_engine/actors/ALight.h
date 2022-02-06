#pragma once

#include "sge_core/shaders/LightDesc.h"
#include "sge_engine/Actor.h"
#include "sge_engine/traits/TraitCamera.h"
#include "sge_engine/traits/TraitViewportIcon.h"
#include "sge_utils/containers/Optional.h"

namespace sge {

struct SGE_ENGINE_API ALight : public Actor {
	ALight() = default;

	void create() final;
	AABox3f getBBoxOS() const final;
	void update(const GameUpdateSets& updateSets) final;
	const LightDesc& getLightDesc() const { return m_lightDesc; }

	/// Get the light forward direction. This is the -Z axis.
	vec3f getLightDirection() const { return -getTransformMtx().c2.xyz().normalized(); }

  public:
	LightDesc m_lightDesc;
	TraitViewportIcon m_traitViewportIcon;
};

} // namespace sge
