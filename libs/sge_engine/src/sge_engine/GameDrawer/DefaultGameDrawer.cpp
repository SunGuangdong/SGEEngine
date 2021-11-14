#include "sge_engine/GameDrawer/DefaultGameDrawer.h"
#include "sge_core/Camera.h"
#include "sge_core/DebugDraw.h"
#include "sge_core/ICore.h"
#include "sge_core/QuickDraw.h"
#include "sge_core/shaders/LightDesc.h"
#include "sge_engine/GameInspector.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/IWorldScript.h"
#include "sge_engine/Physics.h"
#include "sge_engine/actors/ABlockingObstacle.h"
#include "sge_engine/actors/ACRSpline.h"
#include "sge_engine/actors/ACamera.h"
#include "sge_engine/actors/AInfinitePlaneObstacle.h"
#include "sge_engine/actors/AInvisibleRigidObstacle.h"
#include "sge_engine/actors/ALight.h"
#include "sge_engine/actors/ALine.h"
#include "sge_engine/actors/ALocator.h"
#include "sge_engine/actors/ANavMesh.h"
#include "sge_engine/actors/ASky.h"
#include "sge_engine/materials/Material.h"
#include "sge_engine/traits/TraitModel.h"
#include "sge_engine/traits/TraitParticles.h"
#include "sge_engine/traits/TraitSprite.h"
#include "sge_engine/traits/TraitViewportIcon.h"
#include "sge_utils/math/Frustum.h"
#include "sge_utils/math/color.h"
#include "sge_utils/utils/FileStream.h"

// Caution:
// this include is an exception do not include anything else like it.
#include "../core_shaders/ShadeCommon.h"
//#include "../shaders/FWDDefault_buildShadowMaps.h"

namespace sge {

struct LegacyRenderItem : IRenderItem {
	LegacyRenderItem(std::function<void()> drawFn)
	    : drawFn(drawFn) {
	}

	std::function<void()> drawFn;
};

vec4f getPrimarySelectionColor() {
	const vec4f kPrimarySelectionColor1 = vec4f(0.17f, 0.75f, 0.53f, 1.f);
	const vec4f kPrimarySelectionColor2 = vec4f(0.25f, 1.f, 0.63f, 1.f);

	float k = 1.f - fabsf(sinf(0.5f * Timer::now_seconds() * pi()));
	return lerp(kPrimarySelectionColor1, kPrimarySelectionColor2, k);
}
static inline const vec4f kSecondarySelectionColor = colorFromIntRgba(245, 158, 66, 255);

// Actors forward declaration
struct AInvisibleRigidObstacle;

void DefaultGameDrawer::prepareForNewFrame() {
	m_shadingLights.clear();
}

void DefaultGameDrawer::updateShadowMaps(const GameDrawSets& drawSets) {
	const std::vector<GameObject*>* const allLights = getWorld()->getObjects(sgeTypeId(ALight));
	if (allLights == nullptr) {
		return;
	}

	ICamera* const gameCamera = drawSets.gameCamera;
	const Frustum* const gameCameraFrustumWs = drawSets.gameCamera->getFrustumWS();

	// Compute All shadow maps.
	for (const GameObject* const actorLight : *allLights) {
		const ALight* const light = static_cast<const ALight*>(actorLight);
		const LightDesc& lightDesc = light->getLightDesc();

		const bool isPointLight = lightDesc.type == light_point;

		LightShadowInfo& lsi = this->m_perLightShadowFrameTarget[light->getId()];
		lsi.isCorrectlyUpdated = false;

		if (lightDesc.isOn == false) {
			continue;
		}

		// if the light is not inside the view frsutum do no update its shadow map.
		// If the camera frustum is present, try to clip the object.
		if (gameCameraFrustumWs != nullptr) {
			const AABox3f bboxOS = light->getBBoxOS();
			if (!bboxOS.IsEmpty()) {
				if (gameCameraFrustumWs->isObjectOrientedBoxOutside(bboxOS, light->getTransform().toMatrix())) {
					continue;
				}
			}
		}

		// Retrieve the ShadowMapBuildInfo which tells us the cameras to be used for rendering the shadow map.
		Optional<ShadowMapBuildInfo> shadowMapBuildInfoOpt = lightDesc.buildShadowMapInfo(light->getTransform(), *gameCameraFrustumWs);
		if (shadowMapBuildInfoOpt.isValid() == false) {
			continue;
		}

		// Sanity check.
		if (shadowMapBuildInfoOpt->isPointLight != isPointLight) {
			sgeAssert(false);
			continue;
		}

		lsi.buildInfo = shadowMapBuildInfoOpt.get();

		int shadowMapWidth = lightDesc.shadowMapRes;
		int shadowMapHegiht = lightDesc.shadowMapRes;

		if (isPointLight) {
			// Point light shadow will render a cube map to a single 2D texture.
			// The specified shadow map resolution should be the resolution of each side of the cubemap faces.
			shadowMapWidth = lightDesc.shadowMapRes * 4;
			shadowMapHegiht = lightDesc.shadowMapRes * 3;
		}

		// Create the appropriatley sized frame target for the shadow map.
		GpuHandle<FrameTarget>& shadowFrameTarget = lsi.frameTarget;
		const bool shouldCreateNewShadowMapTexture = shadowFrameTarget.IsResourceValid() == false ||
		                                             shadowFrameTarget->getWidth() != shadowMapWidth ||
		                                             shadowFrameTarget->getHeight() != shadowMapHegiht;
		if (shouldCreateNewShadowMapTexture) {
			// Caution, TODO: On Safari (the web browser) I've read that it needs a color render target.
			// Keep that in mind when testing and developing.
			shadowFrameTarget = getCore()->getDevice()->requestResource<FrameTarget>();
			shadowFrameTarget->create2D(shadowMapWidth, shadowMapHegiht, TextureFormat::Unknown, TextureFormat::D24_UNORM_S8_UINT);
		}

		// Draws the shadow map to the created frame target.
		const auto drawShadowMapFromCamera = [this, &lsi](const RenderDestination& rendDest, ICamera* gameCamera,
		                                                  ICamera* drawCamera) -> void {
			GameDrawSets drawShadowSets;

			drawShadowSets.gameCamera = gameCamera; // The gameplay camera, form where the player is going to look at the scene.
			drawShadowSets.drawCamera = drawCamera; // This is the camera that is going to be used for rendering the shadow map.
			drawShadowSets.rdest = rendDest;
			drawShadowSets.quickDraw = &getCore()->getQuickDraw();
			drawShadowSets.shadowMapBuildInfo = &lsi.buildInfo;

			drawWorld(drawShadowSets, drawReason_gameplayShadow);
		};

		// Clear the pre-existing image in the shadow map.
		getCore()->getDevice()->getContext()->clearColor(shadowFrameTarget, 0, vec4f(0.f).data);
		getCore()->getDevice()->getContext()->clearDepth(shadowFrameTarget, 1.f);

		if (shadowMapBuildInfoOpt->isPointLight) {
			// Compute the shadow map sub regions to be used a viewport for rendering each face.
			const short mapSideRes = short(lightDesc.shadowMapRes);
			Rect2s viewports[signedAxis_numElements];
			viewports[axis_x_pos] = Rect2s(mapSideRes, mapSideRes, 0, mapSideRes);
			viewports[axis_x_neg] = Rect2s(mapSideRes, mapSideRes, 2 * mapSideRes, mapSideRes);
			viewports[axis_y_pos] = Rect2s(mapSideRes, mapSideRes, mapSideRes, 0);
			viewports[axis_y_neg] = Rect2s(mapSideRes, mapSideRes, mapSideRes, 2 * mapSideRes);
			viewports[axis_z_pos] = Rect2s(mapSideRes, mapSideRes, mapSideRes, mapSideRes);
			viewports[axis_z_neg] = Rect2s(mapSideRes, mapSideRes, 3 * mapSideRes, mapSideRes);

			// Render the scene for each face of the cube map.
			// TODO: avoid rendering individual faces that would not cast shadow in the visiable area of the game camera.
			for (int iSignedAxis = 0; iSignedAxis < signedAxis_numElements; ++iSignedAxis) {
				RenderDestination rdest(getCore()->getDevice()->getContext(), shadowFrameTarget, viewports[iSignedAxis]);
				drawShadowMapFromCamera(rdest, gameCamera, &shadowMapBuildInfoOpt->pointLightShadowMapCameras[iSignedAxis]);
			}
		} else {
			// Non-point lights have only one camera that uses the whole texture for storing the shadow map.
			RenderDestination rdest(getCore()->getDevice()->getContext(), shadowFrameTarget);
			drawShadowMapFromCamera(rdest, gameCamera, &shadowMapBuildInfoOpt->shadowMapCamera);
		}

		lsi.isCorrectlyUpdated = true;
	}

	for (const GameObject* actorLight : *allLights) {
		const ALight* const light = static_cast<const ALight*>(actorLight);
		const LightDesc& lightDesc = light->getLightDesc();

		ShadingLightData shadingLight;

		if (lightDesc.isOn == false) {
			continue;
		}

		vec3f color = lightDesc.color * lightDesc.intensity;
		vec3f position(0.f);
		float spotLightCosAngle = 1.f; // Defaults to cos(0)

		if (lightDesc.type == light_point) {
			position = light->getTransform().p;
		} else if (lightDesc.type == light_directional) {
			position = -light->getTransformMtx().c0.xyz().normalized0();
		} else if (lightDesc.type == light_spot) {
			position = light->getTransform().p;
			spotLightCosAngle = cosf(lightDesc.spotLightAngle);
		} else {
			sgeAssert(false);
		}

		LightShadowInfo& lsi = m_perLightShadowFrameTarget[light->getId()];
		if (lightDesc.hasShadows && lsi.isCorrectlyUpdated) {
			if (lsi.buildInfo.isPointLight) {
				if (lsi.frameTarget.IsResourceValid()) {
					shadingLight.shadowMap = lsi.frameTarget->getDepthStencil();
					shadingLight.shadowMapProjView =
					    mat4f::getIdentity(); // Not used for points light. They use the near/far projection settings and linear depth maps
				}
			} else {
				if (lsi.frameTarget.IsResourceValid()) {
					shadingLight.shadowMap = lsi.frameTarget->getDepthStencil();
					shadingLight.shadowMapProjView = lsi.buildInfo.shadowMapCamera.getProjView();
				}
			}
		}

		shadingLight.pLightDesc = &lightDesc;
		shadingLight.lightPositionWs = position;
		shadingLight.lightDirectionWs = light->getTransformMtx().c0.xyz().normalized0();
		shadingLight.lightBoxWs = light->getBBoxOS().getTransformed(light->getTransformMtx());

		m_shadingLights.push_back(shadingLight);
	}

	m_shadingLightPerObject.reserve(m_shadingLights.size());
}

bool DefaultGameDrawer::isInFrustum(const GameDrawSets& drawSets, Actor* actor) const {
	// If the camera frustum is present, try to clip the object.
	const Frustum* const pFrustum = drawSets.drawCamera->getFrustumWS();
	const AABox3f bboxOS = actor->getBBoxOS();
	if (pFrustum != nullptr) {
		if (!bboxOS.IsEmpty()) {
			const transf3d& tr = actor->getTransform();

			// We can technically transform the box in world space and then take the bounding sphere, however
			// transforming 8 verts is a perrty costly operation (and we do not need all the data).
			// So instead of that we "manually compute the sphere here.
			// Note: is that really faster?!
			vec3f const bboxCenterOS = bboxOS.center();
			quatf const bbSpherePosQ = tr.r * quatf(bboxCenterOS * tr.s, 0.f) * conjugate(tr.r);
			vec3f const bbSpherePos = bbSpherePosQ.xyz() + tr.p;
			float const bbSphereRadius = bboxOS.halfDiagonal().length() * tr.s.componentMaxAbs();

			if (pFrustum->isSphereOutside(bbSpherePos, bbSphereRadius)) {
				return false;
			}
		}
	}

	return true;
}

void DefaultGameDrawer::getActorObjectLighting(Actor* actor, ObjectLighting& lighting) {
	// Find all the lights that can affect this object.
	m_shadingLightPerObject.clear();
	AABox3f bboxOS = actor->getBBoxOS();
	AABox3f actorBBoxWs = bboxOS.getTransformed(actor->getTransformMtx());
	for (const ShadingLightData& shadingLight : m_shadingLights) {
		if (shadingLight.lightBoxWs.IsEmpty() || shadingLight.lightBoxWs.overlaps(actorBBoxWs)) {
			m_shadingLightPerObject.emplace_back(&shadingLight);
		}
	}

	lighting.ppLightData = m_shadingLightPerObject.data();
	lighting.lightsCount = int(m_shadingLightPerObject.size());
}

void DefaultGameDrawer::getRenderItemsForActor(const GameDrawSets& drawSets, const SelectedItemDirect& item, DrawReason drawReason) {
	Actor* actor = item.gameObject ? item.gameObject->getActor() : nullptr;
	if (actor == nullptr) {
		return;
	}

	// Check if the actor is in the camera frustum.
	// If it isn't don't bother getting any render items.
	if (!isInFrustum(drawSets, actor)) {
		return;
	}

	if (TraitModel* const trait = getTrait<TraitModel>(actor); item.editMode == editMode_actors && trait != nullptr) {
		trait->getRenderItems(drawReason, m_RIs_traitModel);
	}

	if (TraitSprite* const trait = getTrait<TraitSprite>(actor); item.editMode == editMode_actors && trait != nullptr) {
		trait->getRenderItems(drawReason, drawSets, m_RIs_traitSprite);
	}

	if (TraitParticlesSimple* const trait = getTrait<TraitParticlesSimple>(actor); item.editMode == editMode_actors && trait != nullptr) {
		trait->getRenderItems(m_RIs_traitParticles);
	}

	if (TraitParticlesProgrammable* const trait = getTrait<TraitParticlesProgrammable>(actor);
	    item.editMode == editMode_actors && trait != nullptr) {
		trait->getRenderItems(m_RIs_traitParticlesProgrammable);
	}

	if (drawReason_IsEditOrSelectionTool(drawReason)) {
		if (TraitViewportIcon* const trait = getTrait<TraitViewportIcon>(actor)) {
			trait->getRenderItems(m_RIs_traitViewportIcon);
		}
	}

	const TypeId actorType = actor->getType();

	if (actorType == sgeTypeId(ACamera) || actorType == sgeTypeId(ALight) || actorType == sgeTypeId(ALine) ||
	    actorType == sgeTypeId(ACRSpline) || actorType == sgeTypeId(ALocator) || actorType == sgeTypeId(AInvisibleRigidObstacle) ||
	    actorType == sgeTypeId(AInfinitePlaneObstacle) || actorType == sgeTypeId(ANavMesh)) {
		m_RIs_helpers.push_back(HelperDrawRenderItem(item, drawReason));
	}
}

vec4f getSelectionTintColor(DrawReason drawReason) {
	const bool useWireframe = drawReason_IsVisualizeSelection(drawReason);
	const vec4f wireframeColor =
	    (drawReason == drawReason_visualizeSelectionPrimary) ? getPrimarySelectionColor() : kSecondarySelectionColor;
	const int wireframeColorInt = colorToIntRgba(wireframeColor);
	vec4f selectionTint = useWireframe ? wireframeColor : vec4f(0.f);
	selectionTint.w = useWireframe ? 1.f : 0.f;

	return selectionTint;
}

void DefaultGameDrawer::drawCurrentRenderItems(const GameDrawSets& drawSets, DrawReason drawReason, bool shouldDrawSky) {
	// ObjectLighting is overused for multiple things.
	ObjectLighting lighting;
	// const bool useWireframe = drawReason_IsVisualizeSelection(drawReason);
	// const vec4f wireframeColor =
	//    (drawReason == drawReason_visualizeSelectionPrimary) ? getPrimarySelectionColor() : kSecondarySelectionColor;
	// const int wireframeColorInt = colorToIntRgba(wireframeColor);
	// vec4f selectionTint = useWireframe ? wireframeColor : vec4f(0.f);
	// selectionTint.w = useWireframe ? 1.f : 0.f;

	lighting.ambientLightColor = getWorld()->m_ambientLight;
	lighting.uRimLightColorWWidth = vec4f(getWorld()->m_rimLight, getWorld()->m_rimCosineWidth);

	// Extract the alpha sorting plane.
	const vec3f zSortingPlanePosWs = drawSets.drawCamera->getCameraPosition();
	const vec3f zSortingPlaneNormal = drawSets.drawCamera->getCameraLookDir();
	const Plane zSortPlane = Plane::FromPosAndDir(zSortingPlanePosWs, zSortingPlaneNormal);

	// Gather and sort the render items.
	m_RIs_opaque.clear();
	m_RIs_alphaSorted.clear();

	for (auto& ri : m_RIs_traitModel) {
		ri.zSortingValue = zSortPlane.Distance(ri.zSortingPositionWs);
		if (ri.needsAlphaSorting) {
			m_RIs_alphaSorted.push_back(&ri);
		} else {
			m_RIs_opaque.push_back(&ri);
		}
	}

	for (auto& ri : m_RIs_traitSprite) {
		ri.zSortingValue = zSortPlane.Distance(ri.zSortingPositionWs);
		if (ri.needsAlphaSorting) {
			m_RIs_alphaSorted.push_back(&ri);
		} else {
			m_RIs_opaque.push_back(&ri);
		}
	}

	for (auto& ri : m_RIs_traitParticles) {
		ri.zSortingValue = zSortPlane.Distance(ri.zSortingPositionWs);
		if (ri.needsAlphaSorting) {
			m_RIs_alphaSorted.push_back(&ri);
		} else {
			m_RIs_opaque.push_back(&ri);
		}
	}

	for (auto& ri : m_RIs_traitParticlesProgrammable) {
		ri.zSortingValue = zSortPlane.Distance(ri.zSortingPositionWs);
		if (ri.needsAlphaSorting) {
			m_RIs_alphaSorted.push_back(&ri);
		} else {
			m_RIs_opaque.push_back(&ri);
		}
	}

	for (auto& ri : m_RIs_traitViewportIcon) {
		ri.zSortingValue = zSortPlane.Distance(ri.zSortingPositionWs);

		if (ri.needsAlphaSorting) {
			m_RIs_alphaSorted.push_back(&ri);
		} else {
			m_RIs_opaque.push_back(&ri);
		}
	}

	for (auto& ri : m_RIs_helpers) {
		ri.zSortingValue = zSortPlane.Distance(ri.zSortingPositionWs);

		if (ri.needsAlphaSorting) {
			m_RIs_alphaSorted.push_back(&ri);
		} else {
			m_RIs_opaque.push_back(&ri);
		}
	}

	// The opaque items don't need to be sorted in any way for the rendering to appear correct.
	// However, if we sort them fron-to-back we would reduce the pixel shader overdraw,
	// as for objects further back, there would be already a bigger z-depth value.
	std::sort(m_RIs_opaque.begin(), m_RIs_opaque.end(),
	          [](IRenderItem* a, IRenderItem* b) -> bool { return a->zSortingValue < b->zSortingValue; });

	// Sort the semi-transparent render items as they need to be draw in back-to-front order
	// so that the alpha blending and z-depth could be done correctly.
	std::sort(m_RIs_alphaSorted.rbegin(), m_RIs_alphaSorted.rend(),
	          [](IRenderItem* a, IRenderItem* b) -> bool { return a->zSortingValue < b->zSortingValue; });

	// TODO Sort the render items.

	// Draw the render items.
	auto drawRenderItems = [&](std::vector<IRenderItem*>& renderItems) -> void {
		for (auto& riRaw : renderItems) {
			if (auto riTraitModel = dynamic_cast<TraitModelRenderItem*>(riRaw)) {
				// Find all the lights that can affect this object.
				ObjectLighting reasonInfo = lighting;
				getActorObjectLighting(riTraitModel->traitModel->getActor(), reasonInfo);

				drawRenderItem_TraitModel(*riTraitModel, drawSets, reasonInfo, drawReason);
			} else if (auto riSprite = dynamic_cast<TraitSpriteRenderItem*>(riRaw)) {
				// Find all the lights that can affect this object.
				ObjectLighting reasonInfo = lighting;
				getActorObjectLighting(riSprite->actor, reasonInfo);

				drawRenderItem_TraitSprite(*riSprite, drawSets, reasonInfo, drawReason);
			} else if (auto riTraitViewportIcon = dynamic_cast<TraitViewportIconRenderItem*>(riRaw)) {
				drawRenderItem_TraitViewportIcon(*riTraitViewportIcon, drawSets, drawReason);
			} else if (auto riTraitParticles = dynamic_cast<TraitParticlesSimpleRenderItem*>(riRaw)) {
				drawRenderItem_TraitParticlesSimple(*riTraitParticles, drawSets, lighting);
			} else if (auto riTraitParticlesProgrammable = dynamic_cast<TraitParticlesProgrammableRenderItem*>(riRaw)) {
				drawRenderItem_TraitParticlesProgrammable(*riTraitParticlesProgrammable, drawSets, lighting);
			} else if (auto riHelper = dynamic_cast<HelperDrawRenderItem*>(riRaw)) {
				drawHelperActor(riHelper->item.gameObject->getActor(), drawSets, riHelper->item.editMode, 0, lighting, drawReason);
			} else {
				sgeAssert(false && "Render Item does not have a drawing implementation");
			}
		}
	};

	drawRenderItems(m_RIs_opaque);

	// Draw the sky after the opaque objects to reduce the overdraw done by its pixel shader.
	// However draw it before the transparent objects, as it might be visible trough them.
	if (shouldDrawSky) {
		drawSky(drawSets, drawReason);
	}

	drawRenderItems(m_RIs_alphaSorted);
}

void DefaultGameDrawer::drawItem(const GameDrawSets& drawSets, const SelectedItemDirect& item, DrawReason drawReason) {
	clearRenderItems();

	getRenderItemsForActor(drawSets, item, drawReason);
	drawCurrentRenderItems(drawSets, drawReason, false);
}

void DefaultGameDrawer::drawWorld(const GameDrawSets& drawSets, const DrawReason drawReason) {
	clearRenderItems();

	// Get the render items for all actors in the scene.
	getWorld()->iterateOverPlayingObjects(
	    [&](GameObject* object) -> bool {
		    // TODO: Skip this check for whole types. We know they are not actors...
		    if (Actor* actor = object->getActor()) {
			    AABox3f actorBboxOS = actor->getBBoxOS();
			    SelectedItemDirect item;
			    item.editMode = editMode_actors;
			    item.gameObject = actor;
			    getRenderItemsForActor(drawSets, item, drawReason);
		    }

		    return true;
	    },
	    false);

	// Draw the render items.
	drawCurrentRenderItems(drawSets, drawReason, true);

	if (getWorld()->inspector && getWorld()->inspector->m_physicsDebugDrawEnabled) {
		drawSets.rdest.sgecon->clearDepth(drawSets.rdest.frameTarget, 1.f);

		getWorld()->m_physicsDebugDraw.preDebugDraw(drawSets.drawCamera->getProjView(), drawSets.quickDraw, drawSets.rdest);
		getWorld()->physicsWorld.dynamicsWorld->debugDrawWorld();
		getWorld()->m_physicsDebugDraw.postDebugDraw();
	}

	if (drawReason == drawReason_gameplay || drawReason == drawReason_editing) {
		for (ObjectId scriptObj : getWorld()->m_scriptObjects) {
			if (IWorldScript* script = dynamic_cast<IWorldScript*>(getWorld()->getObjectById(scriptObj))) {
				script->onPostDraw(drawSets);
			}
		}
	}

	// Draw the debug draw commands.
	getCore()->getDebugDraw().draw(drawSets.rdest, drawSets.drawCamera->getProjView());


#if 0
	// This code here is something needed only for debugging shadow maps.
	// I ended up writing this from stach more than 10 times that is why I decided to keep it.
	if (drawReason == drawReason_editing) {
		if (m_shadingLights.size() > 0 && m_shadingLights[0].shadowMap) {
			drawSets.quickDraw->drawTexture(drawSets.rdest, 0.f, 0.f, 256.f, m_shadingLights[0].shadowMap);
		}
	}
#endif
}

void DefaultGameDrawer::drawSky(const GameDrawSets& drawSets, const DrawReason drawReason) {
	if (drawReason_IsGameOrEditNoShadowPass(drawReason)) {
		const std::vector<GameObject*>* allSkies = getWorld()->getObjects(sgeTypeId(ASky));

		ASky* const sky = (allSkies && !allSkies->empty()) ? static_cast<ASky*>(allSkies->at(0)) : nullptr;

		if (sky) {
			SkyShaderSettings skyShaderSettings = sky->getSkyShaderSetting();

			mat4f view = drawSets.drawCamera->getView();
			mat4f proj = drawSets.drawCamera->getProj();
			vec3f camPosWs = drawSets.drawCamera->getCameraPosition();

			m_skyShader.draw(drawSets.rdest, camPosWs, view, proj, skyShaderSettings);
		} else {
			mat4f view = drawSets.drawCamera->getView();
			mat4f proj = drawSets.drawCamera->getProj();
			vec3f camPosWs = drawSets.drawCamera->getCameraPosition();

			SkyShaderSettings skyShaderSettings;
			skyShaderSettings.mode = SkyShaderSettings::mode_colorGradinet;
			skyShaderSettings.topColor = vec3f(0.75f);
			skyShaderSettings.bottomColor = vec3f(0.25f);

			m_skyShader.draw(drawSets.rdest, camPosWs, view, proj, skyShaderSettings);
		}
	}
}

void DefaultGameDrawer::drawRenderItem_TraitModel(TraitModelRenderItem& ri,
                                                  const GameDrawSets& drawSets,
                                                  const ObjectLighting& lighting,
                                                  DrawReason const drawReason) {
	const vec3f camPos = drawSets.drawCamera->getCameraPosition();
	const vec3f camLookDir = drawSets.drawCamera->getCameraLookDir();

	const mat4f n2w = ri.traitModel->getActor()->getTransformMtx();

	ModelNode* node = ri.evalModel->m_model->nodeAt(ri.iEvalNode);

	const MeshAttachment& meshAttachment = node->meshAttachments[ri.iEvalNodeMechAttachmentIndex];
	const EvaluatedMesh& evalMesh = ri.evalModel->getEvalMesh(meshAttachment.attachedMeshIndex);
	const EvaluatedNode& evalNode = ri.evalModel->getEvalNode(ri.iEvalNode);

	const Geometry& geom = evalMesh.geometry;

	mat4f const finalTrasform = (evalMesh.geometry.hasVertexSkinning()) ? n2w : n2w * evalNode.evalGlobalTransform;

	if (!drawReason_IsVisualizeSelection(drawReason)) {
		if (drawReason == drawReason_gameplayShadow) {
			m_shadowMapBuilder.drawGeometry(drawSets.rdest, camPos, drawSets.drawCamera->getProjView(), finalTrasform,
			                                *drawSets.shadowMapBuildInfo, geom, ri.mtl.diffuseTexture, false);
		} else {
			m_modeldraw.drawGeometry(drawSets.rdest, camPos, camLookDir, drawSets.drawCamera->getProjView(), finalTrasform, lighting, &geom,
			                         ri.mtl, ri.traitModel->m_models[ri.iModel].instanceDrawMods);
		}
	} else {
		m_constantColorShader.drawGeometry(drawSets.rdest, drawSets.drawCamera->getProjView(), finalTrasform, geom,
		                                   getSelectionTintColor(drawReason), false);
	}
}

void DefaultGameDrawer::drawRenderItem_TraitSprite(const TraitSpriteRenderItem& ri,
                                                   const GameDrawSets& drawSets,
                                                   const ObjectLighting& lighting,
                                                   DrawReason const drawReason) {
	if (ri.spriteTexture == nullptr) {
		return;
	}

	const vec3f camPos = drawSets.drawCamera->getCameraPosition();
	const vec3f camLookDir = drawSets.drawCamera->getCameraLookDir();

	Geometry texPlaneGeom = m_texturedPlaneDraw.getGeometry(drawSets.rdest.getDevice());

	if (drawReason_IsVisualizeSelection(drawReason)) {
		m_constantColorShader.drawGeometry(drawSets.rdest, drawSets.drawCamera->getProjView(), ri.obj2world, texPlaneGeom,
		                                   getSelectionTintColor(drawReason), true);
	} else if (drawReason == drawReason_gameplayShadow) {
		// Shadow maps.
		m_shadowMapBuilder.drawGeometry(drawSets.rdest, camPos, drawSets.drawCamera->getProjView(), ri.obj2world,
		                                *drawSets.shadowMapBuildInfo, texPlaneGeom, ri.spriteTexture, ri.forceNoCulling);
	} else {
		// Gameplay shaded.
		PBRMaterial texPlaneMtl = m_texturedPlaneDraw.getMaterial(ri.spriteTexture);
		texPlaneMtl.diffuseColor = ri.colorTint;

		InstanceDrawMods mods;
		mods.gameTime = getWorld()->timeSpendPlaying;
		mods.forceNoLighting = ri.forceNoLighting;
		mods.forceNoCulling = ri.forceNoCulling;
		mods.forceAdditiveBlending = ri.forceAlphaBlending;

		m_modeldraw.drawGeometry(drawSets.rdest, camPos, camLookDir, drawSets.drawCamera->getProjView(), ri.obj2world, lighting,
		                         &texPlaneGeom, texPlaneMtl, mods);
	}
}

void DefaultGameDrawer::drawRenderItem_TraitViewportIcon(TraitViewportIconRenderItem& ri,
                                                         const GameDrawSets& drawSets,
                                                         const DrawReason& drawReason) {
	if (Texture* iconTex = ri.traitIcon->getIconTexture()) {
		vec4f tintColor = vec4f(1.f);

		if (drawReason == drawReason_visualizeSelectionPrimary)
			tintColor = getPrimarySelectionColor();
		else if (drawReason == drawReason_visualizeSelection)
			tintColor = kSecondarySelectionColor;

		const mat4f node2world = ri.traitIcon->computeNodeToWorldMtx(*drawSets.drawCamera);
		m_texturedPlaneDraw.draw(drawSets.rdest, drawSets.drawCamera->getProjView() * node2world, iconTex, tintColor);
	}
};

void DefaultGameDrawer::drawRenderItem_TraitParticlesSimple(TraitParticlesSimpleRenderItem& ri,
                                                            const GameDrawSets& drawSets,
                                                            const ObjectLighting& lighting) {
	TraitParticlesSimple* traitParticles = ri.traitParticles;

	const vec3f camPos = drawSets.drawCamera->getCameraPosition();
	const vec3f camLookDir = drawSets.drawCamera->getCameraLookDir();

	for (ParticleGroupDesc& pdesc : traitParticles->m_pgroups) {
		if (pdesc.m_visMethod == ParticleGroupDesc::vis_model3D) {
			AssetIface_Model3D* modelIface = pdesc.m_particleModel.getAssetInterface<AssetIface_Model3D>();
			if (modelIface != nullptr) {
				const auto& itr = traitParticles->m_pgroupState.find(pdesc.m_name);
				const mat4f n2w = itr->second.getParticlesToWorldMtx();
				if (itr != traitParticles->m_pgroupState.end()) {
					const ParticleGroupState& pstate = itr->second;

					for (const ParticleGroupState::ParticleState& particle : pstate.getParticles()) {
						mat4f particleTForm = n2w * mat4f::getTranslation(particle.pos) * mat4f::getScaling(particle.scale);

						m_modeldraw.draw(drawSets.rdest, camPos, camLookDir, drawSets.drawCamera->getProjView(), particleTForm, lighting,
						                 modelIface->getSharedEval(), InstanceDrawMods());
					}
				}
			}
		} else if (pdesc.m_visMethod == ParticleGroupDesc::vis_sprite) {
			const auto& itr = traitParticles->m_pgroupState.find(pdesc.m_name);
			if (itr != traitParticles->m_pgroupState.end()) {
				const mat4f n2w = mat4f::getIdentity();

				ParticleGroupState& pstate = itr->second;
				ParticleGroupState::SpriteRendData* srd =
				    pstate.computeSpriteRenderData(*drawSets.rdest.sgecon, pdesc, *drawSets.drawCamera);
				if (srd != nullptr) {
					InstanceDrawMods mods;
					mods.forceNoLighting = true;
					mods.forceAdditiveBlending = true;

					m_modeldraw.drawGeometry(drawSets.rdest, camPos, camLookDir, drawSets.drawCamera->getProjView(), n2w, lighting,
					                         &srd->geometry, srd->material, mods);
				}
			}
		}
	}
}

void DefaultGameDrawer::drawRenderItem_TraitParticlesProgrammable(TraitParticlesProgrammableRenderItem& ri,
                                                                  const GameDrawSets& drawSets,
                                                                  const ObjectLighting& lighting) {
	TraitParticlesProgrammable* particlesTrait = ri.traitParticles;

	const mat4f n2w = particlesTrait->getActor()->getTransformMtx();

	const vec3f camPos = drawSets.drawCamera->getCameraPosition();
	const vec3f camLookDir = drawSets.drawCamera->getCameraLookDir();

	const int numPGroups = particlesTrait->getNumPGroups();
	for (int iGroup = 0; iGroup < numPGroups; ++iGroup) {
		TraitParticlesProgrammable::ParticleGroup* pgrp = particlesTrait->getPGroup(iGroup);

		if (isAssetLoaded(pgrp->spriteTexture, assetIface_model3d)) {
			for (const TraitParticlesProgrammable::ParticleGroup::ParticleData& particle : pgrp->allParticles) {
				mat4f particleTForm =
				    mat4f::getTranslation(particle.position) * mat4f::getRotationQuat(particle.spin) * mat4f::getScaling(particle.scale);

				m_modeldraw.draw(drawSets.rdest, camPos, camLookDir, drawSets.drawCamera->getProjView(), particleTForm, lighting,
				                 getAssetIface<AssetIface_Model3D>(pgrp->spriteTexture)->getStaticEval(), InstanceDrawMods());
			}
		} else {
			if (m_partRendDataGen.generate(*pgrp, *drawSets.rdest.sgecon, *drawSets.drawCamera, n2w)) {
				InstanceDrawMods mods;
				mods.forceNoLighting = true;
				mods.forceAdditiveBlending = true;

				const mat4f identity = mat4f::getIdentity();
				m_modeldraw.drawGeometry(drawSets.rdest, camPos, camLookDir, drawSets.drawCamera->getProjView(), identity, lighting,
				                         &m_partRendDataGen.geometry, m_partRendDataGen.material, mods);
			}
		}
	}
}


#if 0
void DefaultGameDrawer::drawTraitModel(TraitModel* modelTrait,
                                       const GameDrawSets& drawSets,
                                       const ObjectLighting& lighting,
                                       DrawReason const drawReason) {
	if (modelTrait->isRenderable == false) {
		return;
	}

	const vec3f camPos = drawSets.drawCamera->getCameraPosition();
	const vec3f camLookDir = drawSets.drawCamera->getCameraLookDir();
	Actor* const actor = modelTrait->getActor();

	AssetPtr const asset = modelTrait->getAssetProperty().getAsset();

	if (isAssetLoaded(asset) && asset->getType() == assetIface_model3d) {
		AssetModel* const model = modelTrait->getAssetProperty().getAssetModel();

		std::vector<MaterialOverride> mtlOverrides;
		for (auto& mtlOverride : modelTrait->m_materialOverrides) {
			GameObject* const mtlProvider = getWorld()->getObjectById(mtlOverride.materialObjId);
			OMaterial* mtl = dynamic_cast<OMaterial*>(mtlProvider);
			if (mtl) {
				MaterialOverride ovr;
				ovr.name = mtlOverride.materialName;
				ovr.mtl = mtl->getMaterial();

				mtlOverrides.push_back(ovr);
			}
		}

		if (!drawReason_IsVisualizeSelection(drawReason)) {
			if (modelTrait->useSkeleton) {
				if (model && model->sharedEval.isInitialized()) {
					// Compute the overrdies.
					std::vector<mat4f> boneOverrides;
					modelTrait->computeSkeleton(boneOverrides);
					// Draw
					const mat4f n2w = actor->getTransformMtx() * modelTrait->m_additionalTransform;
					model->sharedEval.evaluateFromNodesGlobalTransform(boneOverrides);
					m_modeldraw.draw(drawSets.rdest, camPos, camLookDir, drawSets.drawCamera->getProjView(), n2w, lighting,
					                 model->sharedEval, modelTrait->instanceDrawMods, &mtlOverrides);
				}
			} else {
				if (modelTrait->m_evalModel) {
					const mat4f n2w = actor->getTransformMtx() * modelTrait->m_additionalTransform;
					m_modeldraw.draw(drawSets.rdest, camPos, camLookDir, drawSets.drawCamera->getProjView(), n2w, lighting,
					                 *modelTrait->m_evalModel, modelTrait->instanceDrawMods, &mtlOverrides);
				} else if (model && model->staticEval.isInitialized()) {
					const mat4f n2w = actor->getTransformMtx() * modelTrait->m_additionalTransform;
					m_modeldraw.draw(drawSets.rdest, camPos, camLookDir, drawSets.drawCamera->getProjView(), n2w, lighting,
					                 model->staticEval, modelTrait->instanceDrawMods, &mtlOverrides);
				}
			}
		} else {
			if (modelTrait->useSkeleton) {
				if (model && model->sharedEval.isInitialized()) {
					// Compute the overrdies.
					std::vector<mat4f> boneOverrides;
					modelTrait->computeSkeleton(boneOverrides);

					// Draw
					const mat4f n2w = actor->getTransformMtx() * modelTrait->m_additionalTransform;
					model->sharedEval.evaluateFromNodesGlobalTransform(boneOverrides);
					m_constantColorShader.draw(drawSets.rdest, drawSets.drawCamera->getProjView(), n2w, model->sharedEval,
					                           lighting.selectionTint);
				}
			} else {
				if (modelTrait->m_evalModel) {
					const mat4f n2w = actor->getTransformMtx() * modelTrait->m_additionalTransform;
					m_constantColorShader.draw(drawSets.rdest, drawSets.drawCamera->getProjView(), n2w, *modelTrait->m_evalModel,
					                           lighting.selectionTint);
				} else if (model && model->staticEval.isInitialized()) {
					const mat4f n2w = actor->getTransformMtx() * modelTrait->m_additionalTransform;
					m_constantColorShader.draw(drawSets.rdest, drawSets.drawCamera->getProjView(), n2w, model->staticEval,
					                           lighting.selectionTint);
				}
			}
		}
	}
}
#endif

void DefaultGameDrawer::drawHelperActor_drawANavMesh(ANavMesh& navMesh,
                                                     const GameDrawSets& drawSets,
                                                     const ObjectLighting& UNUSED(lighting),
                                                     const DrawReason drawReason,
                                                     const vec4f wireframeColor) {
	// Draw the bounding box of the nav-mesh. This box basically shows where is the area where the nav-mesh is going to be built.
	if (drawReason_IsEditOrSelectionTool(drawReason)) {
		drawSets.quickDraw->drawWiredAdd_Box(navMesh.getTransformMtx(), colorToIntRgba(wireframeColor));
		drawSets.quickDraw->drawWired_Execute(drawSets.rdest, drawSets.drawCamera->getProjView());
	}

	if (drawReason_IsVisualizeSelection(drawReason)) {
		vec4f wireframeColorAlphaFloat = wireframeColor;
		wireframeColorAlphaFloat.w = 0.5f;
		uint32 wireframeColorAlpha = colorToIntRgba(wireframeColorAlphaFloat);

		drawSets.quickDraw->drawWired_Clear();
		// A small value that we add to each vertex (in world space) so that the contruction and nav-mesh
		// geometry does not end-up z-fighting in the viewport.
		// The proper solution would be an actual depth bias, but since this is an editor only (and used usuually only for debug)
		// this is a good enough solution.
		const vec3f fightingPositionBias = -drawSets.drawCamera->getCameraLookDir() * 0.001f;

		for (size_t iTri = 0; iTri < navMesh.m_debugDrawNavMeshTriListWs.size() / 3; ++iTri) {
			vec3f a = navMesh.m_debugDrawNavMeshTriListWs[iTri * 3 + 0] + fightingPositionBias;
			vec3f b = navMesh.m_debugDrawNavMeshTriListWs[iTri * 3 + 1] + fightingPositionBias;
			vec3f c = navMesh.m_debugDrawNavMeshTriListWs[iTri * 3 + 2] + fightingPositionBias;

			drawSets.quickDraw->drawSolidAdd_Triangle(a, b, c, wireframeColorAlpha);
			drawSets.quickDraw->drawWiredAdd_triangle(a, b, c, colorToIntRgba(wireframeColor));
		}

		const uint32 buildMeshColorInt = colorIntFromRGBA255(246, 189, 85);
		const uint32 buildMeshColorAlphaInt = colorIntFromRGBA255(246, 189, 85, 127);
		for (size_t iTri = 0; iTri < navMesh.m_debugDrawNavMeshBuildTriListWs.size() / 3; ++iTri) {
			vec3f a = navMesh.m_debugDrawNavMeshBuildTriListWs[iTri * 3 + 0] + fightingPositionBias;
			vec3f b = navMesh.m_debugDrawNavMeshBuildTriListWs[iTri * 3 + 1] + fightingPositionBias;
			vec3f c = navMesh.m_debugDrawNavMeshBuildTriListWs[iTri * 3 + 2] + fightingPositionBias;

			drawSets.quickDraw->drawSolidAdd_Triangle(a, b, c, buildMeshColorAlphaInt);
			drawSets.quickDraw->drawWiredAdd_triangle(a, b, c, buildMeshColorInt);
		}

		drawSets.quickDraw->drawSolid_Execute(drawSets.rdest, drawSets.drawCamera->getProjView(), false,
		                                      getCore()->getGraphicsResources().BS_backToFrontAlpha);
		drawSets.quickDraw->drawWired_Execute(drawSets.rdest, drawSets.drawCamera->getProjView());
	}
}

void DefaultGameDrawer::drawHelperActor(Actor* actor,
                                        const GameDrawSets& drawSets,
                                        EditMode const editMode,
                                        int const itemIndex,
                                        const ObjectLighting& lighting,
                                        DrawReason const drawReason) {
	TypeId actorType = actor->getType();

	const vec3f camPos = drawSets.drawCamera->getCameraPosition();
	const vec3f camLookDir = drawSets.drawCamera->getCameraLookDir();

	const bool isVisualizingSelection = drawReason_IsVisualizeSelection(drawReason);
	vec4f selectionTint = getSelectionTintColor(drawReason);
	const uint32 wireframeColorInt = colorToIntRgba(selectionTint);

	// If we are rendering the actor to visualize a selection, show a wireframe representation specifying how the light is oriented, or its
	// area of effect.
	if (actorType == sgeTypeId(ALight) && drawReason_IsVisualizeSelection(drawReason)) {
		if (editMode == editMode_actors) {
			drawSets.quickDraw->drawWired_Clear();

			const ALight* const light = static_cast<const ALight*>(actor);
			const vec3f color = light->getLightDesc().color;
			const LightDesc lightDesc = light->getLightDesc();
			if (lightDesc.type == light_point) {
				const float sphereRadius = maxOf(lightDesc.range, 0.1f);
				drawSets.quickDraw->drawWiredAdd_Sphere(actor->getTransformMtx(), wireframeColorInt, sphereRadius, 6);
			} else if (lightDesc.type == light_directional) {
				const float arrowLength = maxOf(lightDesc.intensity * 10.f, 1.f);
				drawSets.quickDraw->drawWiredAdd_Arrow(
				    actor->getTransform().p, actor->getTransform().p + actor->getTransformMtx().c0.xyz().normalized() * arrowLength,
				    wireframeColorInt);
			} else if (lightDesc.type == light_spot) {
				const float coneHeight = maxOf(lightDesc.range, 2.f);
				const float coneRadius = tanf(lightDesc.spotLightAngle) * coneHeight;
				drawSets.quickDraw->drawWiredAdd_ConeBottomAligned(actor->getTransformMtx() * mat4f::getRotationZ(deg2rad(-90.f)),
				                                                   wireframeColorInt, coneHeight, coneRadius, 6);
			}

			drawSets.quickDraw->drawWired_Execute(drawSets.rdest, drawSets.drawCamera->getProjView());
		}
	} else if (actorType == sgeTypeId(ABlockingObstacle) && editMode == editMode_actors) {
		ABlockingObstacle* const simpleObstacle = static_cast<ABlockingObstacle*>(const_cast<Actor*>(actor));

		if (simpleObstacle->geometry.hasData()) {
			if (drawReason_IsVisualizeSelection(drawReason)) {
				m_constantColorShader.drawGeometry(drawSets.rdest, drawSets.drawCamera->getProjView(), simpleObstacle->getTransformMtx(),
				                                   simpleObstacle->geometry, selectionTint, false);
			} else {
				m_modeldraw.drawGeometry(drawSets.rdest, camPos, camLookDir, drawSets.drawCamera->getProjView(),
				                         simpleObstacle->getTransformMtx(), lighting, &simpleObstacle->geometry, simpleObstacle->material,
				                         InstanceDrawMods());
			}
		}
	} else if (actorType == sgeTypeId(ACamera) && drawReason_IsVisualizeSelection(drawReason)) {
		if (editMode == editMode_actors) {
			if (const TraitCamera* const traitCamera = getTrait<TraitCamera>(actor)) {
				const auto intersectPlanes = [](const Plane& p0, const Plane& p1, const Plane& p2) -> vec3f {
					// http://www.ambrsoft.com/TrigoCalc/Plan3D/3PlanesIntersection_.htm
					float const det = -triple(p0.norm(), p1.norm(), p2.norm()); // Caution: I'm not sure about that minus...

					float const x = triple(p0.v4.wyz(), p1.v4.wyz(), p2.v4.wyz()) / det;
					float const y = triple(p0.v4.xwz(), p1.v4.xwz(), p2.v4.xwz()) / det;
					float const z = triple(p0.v4.xyw(), p1.v4.xyw(), p2.v4.xyw()) / det;

					return vec3f(x, y, z);
				};


				const ICamera* const ifaceCamera = traitCamera->getCamera();
				const Frustum* const f = ifaceCamera->getFrustumWS();
				if (f) {
					const vec3f frustumVerts[8] = {
					    intersectPlanes(f->t, f->r, f->n), intersectPlanes(f->t, f->l, f->n),
					    intersectPlanes(f->b, f->l, f->n), intersectPlanes(f->b, f->r, f->n),

					    intersectPlanes(f->t, f->r, f->f), intersectPlanes(f->t, f->l, f->f),
					    intersectPlanes(f->b, f->l, f->f), intersectPlanes(f->b, f->r, f->f),
					};

					const vec3f lines[] = {
					    // Near plane.
					    frustumVerts[0],
					    frustumVerts[1],
					    frustumVerts[1],
					    frustumVerts[2],
					    frustumVerts[2],
					    frustumVerts[3],
					    frustumVerts[3],
					    frustumVerts[0],

					    // Lines that connect near and far planes.
					    frustumVerts[4 + 0],
					    frustumVerts[4 + 1],
					    frustumVerts[4 + 1],
					    frustumVerts[4 + 2],
					    frustumVerts[4 + 2],
					    frustumVerts[4 + 3],
					    frustumVerts[4 + 3],
					    frustumVerts[4 + 0],

					    // Far plane.
					    frustumVerts[0],
					    frustumVerts[4],
					    frustumVerts[1],
					    frustumVerts[5],
					    frustumVerts[2],
					    frustumVerts[6],
					    frustumVerts[3],
					    frustumVerts[7],
					};

					drawSets.quickDraw->drawWired_Clear();
					for (int t = 0; t < SGE_ARRSZ(lines); t += 2) {
						drawSets.quickDraw->drawWiredAdd_Line(lines[t], lines[t + 1], wireframeColorInt);
					}
					drawSets.quickDraw->drawWired_Execute(drawSets.rdest, drawSets.drawCamera->getProjView());
				}
			}
		}
	} else if (actorType == sgeTypeId(ALine) && drawReason_IsEditOrSelectionTool(drawReason)) {
		const ALine* const spline = static_cast<const ALine*>(actor);

		if (editMode == editMode_actors) {
			mat4f const obj2world = spline->getTransformMtx();

			const int color = drawReason_IsVisualizeSelection(drawReason) ? 0xFF0055FF : 0xFFFFFFFF;

			const int numSegments = spline->getNumSegments();
			for (int iSegment = 0; iSegment < numSegments; ++iSegment) {
				int i0, i1;
				if (spline->getSegmentVerts(iSegment, i0, i1) == false) {
					sgeAssert(false);
					break;
				}

				// TODO: cache one of the matrix multiplications.
				vec3f p0 = mat_mul_pos(obj2world, spline->points[i0]);
				vec3f p1 = mat_mul_pos(obj2world, spline->points[i1]);

				getCore()->getQuickDraw().drawWiredAdd_Line(p0, p1, color);
				getCore()->getQuickDraw().drawWiredAdd_Sphere(mat4f::getTranslation(p0), color, 0.2f, 3);

				if (iSegment == numSegments - 1) {
					getCore()->getQuickDraw().drawWiredAdd_Sphere(mat4f::getTranslation(p1), color, 0.2f, 3);
				}
			}

			getCore()->getQuickDraw().drawWired_Execute(drawSets.rdest, drawSets.drawCamera->getProjView(), nullptr);
		} else if (editMode == editMode_points) {
			mat4f const tr = spline->getTransformMtx();

			getCore()->getQuickDraw().drawWired_Clear();
			getCore()->getQuickDraw().drawWiredAdd_Sphere(tr * mat4f::getTranslation(spline->points[itemIndex]),
			                                              isVisualizingSelection ? wireframeColorInt : 0xFFFFFFFF, 0.2f, 3);
			getCore()->getQuickDraw().drawWired_Execute(drawSets.rdest, drawSets.drawCamera->getProjView(), nullptr);
		}
	} else if (actorType == sgeTypeId(ACRSpline) && drawReason_IsEditOrSelectionTool(drawReason)) {
		ACRSpline* const spline = static_cast<ACRSpline*>(actor);

		mat4f const tr = spline->getTransformMtx();
		const float pointScale = spline->getBBoxOS().getTransformed(tr).size().length() * 0.01f;

		if (editMode == editMode_actors) {
			int color = isVisualizingSelection ? wireframeColorInt : 0xFFFFFFFF;

			const float lenPerLine = 1.f;

			getCore()->getQuickDraw().drawWired_Clear();

			vec3f p0;
			spline->evaluateAtDistance(&p0, nullptr, 0.f);
			p0 = mat_mul_pos(tr, p0);
			for (float t = lenPerLine; t <= spline->totalLength; t += lenPerLine) {
				vec3f p1;
				spline->evaluateAtDistance(&p1, nullptr, t);
				p1 = mat_mul_pos(tr, p1);
				getCore()->getQuickDraw().drawWiredAdd_Line(p0, p1, color);
				p0 = p1;
			}

			for (int t = 0; t < spline->getNumPoints(); t++) {
				vec3f worldPos = mat_mul_pos(tr, spline->points[t]);
				if (t == 0)
					getCore()->getQuickDraw().drawWiredAdd_Box(mat4f::getTRS(worldPos, quatf::getIdentity(), vec3f(pointScale)), color);
				else
					getCore()->getQuickDraw().drawWiredAdd_Sphere(mat4f::getTranslation(worldPos), color, pointScale, 3);
			}

			getCore()->getQuickDraw().drawWired_Execute(drawSets.rdest, drawSets.drawCamera->getProjView(), nullptr);
		} else if (editMode == editMode_points) {
			getCore()->getQuickDraw().drawWired_Clear();
			getCore()->getQuickDraw().drawWiredAdd_Sphere(tr * mat4f::getTranslation(spline->points[itemIndex]),
			                                              isVisualizingSelection ? wireframeColorInt : 0xFFFFFFFF, pointScale, 3);
			getCore()->getQuickDraw().drawWired_Execute(drawSets.rdest, drawSets.drawCamera->getProjView(), nullptr);
		}
	} else if (actorType == sgeTypeId(ALocator) && drawReason_IsEditOrSelectionTool(drawReason)) {
		if (editMode == editMode_actors) {
			drawSets.quickDraw->drawWired_Clear();
			drawSets.quickDraw->drawWiredAdd_Basis(actor->getTransformMtx());
			drawSets.quickDraw->drawWiredAdd_Box(actor->getTransformMtx(), wireframeColorInt);
			drawSets.quickDraw->drawWired_Execute(drawSets.rdest, drawSets.drawCamera->getProjView());
		}
	} else if (actorType == sgeTypeId(ABone) && drawReason_IsEditOrSelectionTool(drawReason)) {
		if (editMode == editMode_actors) {
			drawSets.quickDraw->drawWired_Clear();

			ABone* const bone = static_cast<ABone*>(actor);
			GameWorld* world = bone->getWorld();
			float boneLength = 1.f;
			const vector_set<ObjectId>* pChildren = world->getChildensOf(bone->getId());
			if (pChildren != nullptr) {
				vec3f boneFromWs = bone->getPosition();

				const vector_set<ObjectId>& children = *pChildren;
				if (children.size() == 1) {
					Actor* child = world->getActorById(children.getNth(0));
					if (child != nullptr) {
						vec3f boneToWs = child->getPosition();
						vec3f boneDirVectorWs = (boneToWs - boneFromWs);

						if (boneDirVectorWs.lengthSqr() > 1e-6f) {
							boneLength = boneDirVectorWs.length();
							quatf rotationWs =
							    quatf::fromNormalizedVectors(vec3f(1.f, 0.f, 0.f), boneDirVectorWs.normalized0(), vec3f(0.f, 0.f, 1.f));
							vec3f translWs = boneFromWs;
							mat4f visualBoneTransformWs = mat4f::getTRS(translWs, rotationWs, vec3f(1.f));
							drawSets.quickDraw->drawWiredAdd_Bone(visualBoneTransformWs, boneDirVectorWs.length(), bone->boneLength,
							                                      selectionTint);
						}
					}
				} else {
					for (ObjectId childId : children) {
						Actor* child = world->getActorById(childId);
						if (child != nullptr) {
							vec3f boneToWs = child->getPosition();
							drawSets.quickDraw->drawWiredAdd_Line(boneFromWs, boneToWs, wireframeColorInt);
						}
					}
				}
			}

			// Dreaw the location of the bone.
			drawSets.quickDraw->drawWiredAdd_Sphere(bone->getTransformMtx(), wireframeColorInt, boneLength / 12.f, 6);
			drawSets.quickDraw->drawWired_Execute(drawSets.rdest, drawSets.drawCamera->getProjView(), nullptr,
			                                      getCore()->getGraphicsResources().DSS_always_noTest);
		}
	} else if ((actorType == sgeTypeId(AInvisibleRigidObstacle)) && drawReason_IsEditOrSelectionTool(drawReason)) {
		if (editMode == editMode_actors) {
			drawSets.quickDraw->drawWired_Clear();
			drawSets.quickDraw->drawWiredAdd_Box(actor->getTransformMtx(), actor->getBBoxOS(), wireframeColorInt);
			drawSets.quickDraw->drawWired_Execute(drawSets.rdest, drawSets.drawCamera->getProjView());
		}
	} else if (actorType == sgeTypeId(AInfinitePlaneObstacle) && drawReason_IsEditOrSelectionTool(drawReason)) {
		if (editMode == editMode_actors) {
			AInfinitePlaneObstacle* plane = static_cast<AInfinitePlaneObstacle*>(actor);
			const float scale = plane->displayScale * plane->getTransform().s.componentMaxAbs();

			drawSets.quickDraw->drawWired_Clear();
			drawSets.quickDraw->drawWiredAdd_Arrow(actor->getPosition(), actor->getPosition() + actor->getDirY() * scale,
			                                       wireframeColorInt);
			drawSets.quickDraw->drawWiredAdd_Grid(actor->getPosition(), actor->getDirX() * scale, actor->getDirZ() * scale, 1, 1,
			                                      wireframeColorInt);
			drawSets.quickDraw->drawWired_Execute(drawSets.rdest, drawSets.drawCamera->getProjView());
		}
	} else if (actorType == sgeTypeId(ANavMesh)) {
		if (ANavMesh* navMesh = static_cast<ANavMesh*>(actor)) {
			drawHelperActor_drawANavMesh(*navMesh, drawSets, lighting, drawReason, selectionTint);
		}
	} else {
		// Not implemented.
	}
}

} // namespace sge
