#pragma once

#include "sge_renderer/renderer/renderer.h"

#include "sge_core/materials/SimpleTriplanar/SimpleTriplanarMtl.h"
#include "sge_engine/Actor.h"
#include "sge_engine/RigidBodyEditorConfig.h"
#include "sge_engine/TerrainGenerator.h"
#include "sge_engine/traits/TraitCustomAE.h"
#include "sge_engine/traits/TraitModel.h"
#include "sge_engine/traits/TraitRenderGeometry.h"
#include "sge_engine/traits/TraitRigidBody.h"

namespace sge {

struct GameInspector;

/// ABlockingObstacle provides a way to block out levels using boxes, stairs and slopes.
/// The obstacle is textured with triplaner mapped textures.
struct SGE_ENGINE_API ABlockingObstacle final : public Actor, public IActorCustomAttributeEditorTrait {
	ABlockingObstacle()
	    : m_textureX(assetIface_texture2d)
	    , m_textureY(assetIface_texture2d)
	{
	}

	void create() final;
	void onPlayStateChanged(bool const isStartingToPlay) override;
	void postUpdate(const GameUpdateSets& updateSets) final;
	Box3f getBBoxOS() const final;

	void doAttributeEditor(GameInspector* inspector) override;

  public:
	TraitRenderGeometry tt_rendGeom;
	TraitRigidBody m_traitRB;

	SimpleObstacleDesc targetDesc;
	SimpleObstacleDesc currentDesc;
	RigidBodyPropertiesConfigurator m_rbPropConfig;

	AssetProperty m_textureX;
	float m_textureXScale = 1.f;
	AssetProperty m_textureY;
	float m_textureYScale = 1.f;

	// Properties of the geometry and the material.
	Box3f boundingBox;
	GpuHandle<Buffer> vertexBuffer;
	GpuHandle<Buffer> indexBuffer;
	Geometry geometry;
	SimpleTriplanarMtlData material;
	int numVerts = 0;
	int numIndices = 0;
};

} // namespace sge
