#pragma once

#include "sge_core/materials/IGeometryDrawer.h"
#include "sge_core/shaders/ConstantColorShader.h"
#include "sge_core/shaders/FWDBuildShadowMapShader.h"
#include "sge_core/shaders/SkyShader.h"
#include "sge_engine/GameDrawer/GameDrawer.h"
#include "sge_engine/GameObject.h"
#include "sge_engine/TexturedPlaneDraw.h"
#include "sge_engine/actors/ALight.h"
#include "sge_engine/traits/TraitParticles.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/math/mat4f.h"

#include "sge_engine/GameDrawer/RenderItems/GeometryRenderItem.h"
#include "sge_engine/GameDrawer/RenderItems/HelperDrawRenderItem.h"
#include "sge_engine/GameDrawer/RenderItems/TraitParticlesRenderItem.h"
#include "sge_engine/GameDrawer/RenderItems/TraitSpriteRrenderItem.h"
#include "sge_engine/GameDrawer/RenderItems/TraitViewportIconRenderItem.h"

namespace sge {

struct TraitRenderGeometry;
struct TraitParticlesSimple;
struct TraitParticlesProgrammable;
struct ANavMesh;
struct AParticlesSimple;

struct LightShadowInfo {
	ShadowMapBuildInfo buildInfo;
	// GpuHandle<Texture> pointLightDepthTexture; ///< This could be a single 2D or a Cube texture depending on the
	// light source.
	GpuHandle<FrameTarget> frameTarget; ///< Regular frame target for spot and directional lights.
	bool isCorrectlyUpdated = false;
};

struct SGE_ENGINE_API DefaultGameDrawer : public IGameDrawer {
	void prepareForNewFrame() override;
	void updateShadowMaps(const GameDrawSets& drawSets) override;

	void drawItem(const GameDrawSets& drawSets, const SelectedItemDirect& item, DrawReason drawReason) override;
	void drawWorld(const GameDrawSets& drawSets, const DrawReason drawReason) override;

  private:
	/// @brief Gets all the render items needed to draw the actor for the specified draw reason.
	/// @param [in] item describes the actor (and/if its subparts that we want to render).
	void getRenderItemsForActor(const GameDrawSets& drawSets, const SelectedItemDirect& item, DrawReason drawReason);

	void drawSky(const GameDrawSets& drawSets, const DrawReason drawReason);


	/// @brief A helper function that is called in @drawItem or @drawWorld. These functions get render items
	/// form actors and the actual rendering is done by this function.
	void drawCurrentRenderItems(const GameDrawSets& drawSets, DrawReason drawReason, bool shouldDrawSky);

	void drawRenderItem_Geometry(
	    GeometryRenderItem& ri,
	    const GameDrawSets& drawSets,
	    const ObjectLighting& lighting,
	    DrawReason const drawReason);

	void drawRenderItem_TraitSprite(
	    const TraitSpriteRenderItem& ri,
	    const GameDrawSets& drawSets,
	    const ObjectLighting& lighting,
	    DrawReason const drawReason);

	void drawRenderItem_TraitViewportIcon(
	    TraitViewportIconRenderItem& viewportIcon, const GameDrawSets& drawSets, const DrawReason& drawReason);

	void drawRenderItem_TraitParticlesSimple(
	    TraitParticlesSimpleRenderItem& ri,
	    const GameDrawSets& drawSets,
	    DrawReason drawReason,
	    const ObjectLighting& generalMods);

	void drawRenderItem_TraitParticlesProgrammable(
	    TraitParticlesProgrammableRenderItem& ri, const GameDrawSets& drawSets, const ObjectLighting& lighting);

	/// @brief A function usually called as a result of HelperDrawRenderItem.
	/// intented be used for objects that need to be visible in the editor and not during gameplay.
	void drawHelperActor(
	    Actor* actor,
	    const GameDrawSets& drawSets,
	    EditMode const editMode,
	    int const itemIndex,
	    const ObjectLighting& lighting,
	    DrawReason const drawReason);

	void drawHelperActor_drawANavMesh(
	    ANavMesh& navMesh,
	    const GameDrawSets& drawSets,
	    const ObjectLighting& lighting,
	    const DrawReason drawReason,
	    const vec4f wireframeColor);

	void drawHelperActor_drawAParticlesSimple(
	    AParticlesSimple& particles,
	    const GameDrawSets& drawSets,
	    const ObjectLighting& lighting,
	    const DrawReason drawReason,
	    const vec4f wireframeColor);

  private:
	/// @brief Returns true if the bounding box of the actor (Actor::getBBoxOs) is in the specified
	/// draw camera frustum.
	bool isInFrustum(const GameDrawSets& drawSets, Actor* actor) const;
	void getActorObjectLighting(Actor* actor, ObjectLighting& lighting);
	void getLightingForLocation(const Box3f& bboxWs, ObjectLighting& lighting);

	void clearRenderItems()
	{
		m_RIs_opaque.clear();
		m_RIs_alphaSorted.clear();

		m_RIs_geometry.clear();
		m_RIs_traitSprite.clear();
		m_RIs_traitViewportIcon.clear();
		m_RIs_traitParticles.clear();
		m_RIs_traitParticlesProgrammable.clear();
		m_RIs_helpers.clear();
	}

  public:
	FWDBuildShadowMapShader m_shadowMapBuilder;
	ConstantColorWireShader m_constantColorShader;
	SkyShader m_skyShader;
	TexturedPlaneDraw m_texturedPlaneDraw;
	ParticleRenderDataGen m_partRendDataGen;

	std::vector<ShadingLightData> m_shadingLights;
	std::vector<const ShadingLightData*> m_shadingLightPerObject;

	// TODO: find a proper place for this
	std::map<ObjectId, LightShadowInfo> m_perLightShadowFrameTarget;

	/// Render items cache variables.
	/// These could safely be a member variable in @drawWorld or @drawItem
	/// However in order to save up from alocating them again and again for each frame
	/// We store the containers here.

	std::vector<IRenderItem*> m_RIs_opaque; ///< Pointers to all opaque render items. Pointers are not owned.
	std::vector<IRenderItem*>
	    m_RIs_alphaSorted; ///< Pointers to all semi-transparent render items. Pointers are not owned.

	// Render items cache to save up on memory.
	std::vector<GeometryRenderItem> m_RIs_geometry;
	std::vector<TraitSpriteRenderItem> m_RIs_traitSprite;
	std::vector<TraitViewportIconRenderItem> m_RIs_traitViewportIcon;
	std::vector<TraitParticlesSimpleRenderItem> m_RIs_traitParticles;
	std::vector<TraitParticlesProgrammableRenderItem> m_RIs_traitParticlesProgrammable;
	std::vector<HelperDrawRenderItem> m_RIs_helpers;
};

} // namespace sge
