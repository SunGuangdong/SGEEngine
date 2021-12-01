#include "TraitSprite.h"
#include "IconsForkAwesome/IconsForkAwesome.h"
#include "sge_core/Camera.h"
#include "sge_core/SGEImGui.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_engine/GameDrawer/RenderItems/TraitSpriteRrenderItem.h"
#include "sge_engine/windows/PropertyEditorWindow.h"

namespace sge {
// clang-format off
ReflAddTypeId(TraitSprite, 20'07'11'0001);
ReflAddTypeId(TraitSpriteImageSets, 21'04'04'0001);
ReflAddTypeId(TraitSpriteEntry, 21'10'11'0001);
ReflAddTypeId(std::vector<TraitSpriteEntry>, 21'10'11'0002);

ReflBlock() {
	ReflAddType(TraitSpriteImageSets)
		ReflMember(TraitSpriteImageSets, m_anchor)
		ReflMember(TraitSpriteImageSets, colorTint).addMemberFlag(MFF_Vec4fAsColor)
		ReflMember(TraitSpriteImageSets, defaultFacingAxisZ)
		ReflMember(TraitSpriteImageSets, m_localXOffset).uiRange(-FLT_MAX, FLT_MAX, 0.01f)
		ReflMember(TraitSpriteImageSets, m_pixelsPerUnit).uiRange(0.00001f, 100000.f, 0.1f)
		ReflMember(TraitSpriteImageSets, m_billboarding)
		ReflMember(TraitSpriteImageSets, forceNoLighting)
		ReflMember(TraitSpriteImageSets, forceNoCulling)
		ReflMember(TraitSpriteImageSets, flipHorizontally)
		ReflMember(TraitSpriteImageSets, spriteFrameTime).uiRange(0.f, 100000.f, 0.01f)
		ReflMember(TraitSpriteImageSets, forceAlphaBlending)
	;

	ReflAddType(TraitSpriteEntry)
		ReflMember(TraitSpriteEntry, isRenderable)
		ReflMember(TraitSpriteEntry, m_assetProperty)
		ReflMember(TraitSpriteEntry, m_additionalTransform)
		ReflMember(TraitSpriteEntry, imageSettings)
	;

	ReflAddType(std::vector<TraitSpriteEntry>);

	ReflAddType(TraitSprite)
		ReflMember(TraitSprite, isRenderable)
		ReflMember(TraitSprite, hasShadows)
		ReflMember(TraitSprite, images)
		ReflMember(TraitSprite, additionalTransform)
	;
}
// clang-format on

//------------------------------------------------------------------------------
// TraitSpriteImageSets
//------------------------------------------------------------------------------
mat4f TraitSpriteImageSets::computeObjectToWorldTransform(const Asset& asset,
                                                          const ICamera* const drawCamera,
                                                          const transf3d& nodeToWorldTransform,
                                                          const mat4f& additionaTransform) const {
	vec2f imageSize = vec2f(0.f);
	bool hasValidImageToRender = false;
	if (const AssetIface_SpriteAnim* spriteIface = getAssetIface<AssetIface_SpriteAnim>(asset)) {
		const SpriteAnimationWithTextures* sprite = &spriteIface->getSpriteAnimation();
		const SpriteAnimation::Frame* frame = sprite->spriteAnimation.getFrameForTime(spriteFrameTime);
		imageSize = vec2f(float(frame->wh.x), float(frame->wh.y));
		hasValidImageToRender = true;
	} else if (const AssetIface_Texture2D* texIface = getAssetIface<AssetIface_Texture2D>(asset)) {
		if (Texture* tex = texIface->getTexture()) {
			imageSize = vec2f(float(tex->getDesc().texture2D.width), float(tex->getDesc().texture2D.height));
			hasValidImageToRender = true;
		}
	}

	if (hasValidImageToRender) {
		const mat4f localOffsetmtx = mat4f::getTranslation(m_localXOffset, 0.f, 0.f);
		const mat4f anchorAlignMtx = getAnchorAlignMtxOS(imageSize.x, imageSize.y);

		mat4f objToWorld;
		if (drawCamera != nullptr) {
			const mat4f billboardFacingMtx = billboarding_getOrentationMtx(
			    m_billboarding, nodeToWorldTransform, drawCamera->getCameraPosition(), drawCamera->getView(), defaultFacingAxisZ);
			objToWorld = billboardFacingMtx * additionaTransform * anchorAlignMtx * localOffsetmtx;
		} else {
			objToWorld = anchorAlignMtx * localOffsetmtx * additionaTransform;

			// Hack:
			if (m_billboarding == billboarding_none && defaultFacingAxisZ) {
				const mat4f billboardFacingMtx = mat4f::getRotationY(deg2rad(-90.f));
				objToWorld = billboardFacingMtx * objToWorld;
			}
		}

		if (flipHorizontally) {
			objToWorld = objToWorld * mat4f::getTranslation(0.f, 0.f, 1.f) * mat4f::getScaling(1.f, 1.f, -1.f);
		}

		return objToWorld;
	}

	return mat4f::getIdentity();
}

AABox3f TraitSpriteImageSets::computeBBoxOS(const Asset& asset, const mat4f& additionaTransform) const {
	// What is happening here is the following:
	// When there is no billboarding applied the bounding box computation is pretty strait forward:
	// Transform the verticies of of quad by object-to-node space transfrom and we are done.
	//
	// However when there is billboarding the plane is allowed to rotate (based on the type of the bilboarding).
	// it might be sweeping around an axis (Up only billboarding) or facing the camera (meaning int can rotate freely around its center
	// point).
	// In order to compute the bounding box for those cases we take the un-billboarded transformation, obtain its size around that
	// center point and then use that vector to extend the bounding box in each cardinal direction that the quad could rotate. If we do
	// not apply the @additionaTransform the point (0,0,0) in quad space is going to lay on the axis that the quad rotates about. So to
	// compute the bounding box we take the furthest point of the quard form (0,0,0) and extend the initial (without billboarding
	// bounding box based on it. After that we can safely apply @additionaTransform to move the bounding box in node space safely
	// together with the plane. Note: it seems that this function is too complicated for what it does. The bounding box doesn't really
	// need to be the smallest fit, just good enough so if it is a problem a simpler function might be written.
	const transf3d fakeNodeToWorld;
	const mat4f noAdditionalTransform = mat4f::getIdentity();
	const mat4f planeTransfromObjectToNode = computeObjectToWorldTransform(asset, nullptr, fakeNodeToWorld, noAdditionalTransform);

	AABox3f bboxOs;

	vec3f p0 = mat_mul_pos(planeTransfromObjectToNode, vec3f(0.f));
	vec3f p1 = mat_mul_pos(planeTransfromObjectToNode, vec3f(0.f, 1.f, 1.f));

	bboxOs.expand(p0);
	bboxOs.expand(p1);

	switch (m_billboarding) {
		case billboarding_none: {
			return bboxOs;
		} break;

		case billboarding_yOnly: {
			vec3f pExtreme = p0.getAbs().pickMax(p1.getAbs());
			bboxOs.expand(vec3f(0.f, 0.f, pExtreme.z));
			bboxOs.expand(vec3f(0.f, 0.f, -pExtreme.z));
			bboxOs.expand(vec3f(pExtreme.z, 0.f, 0.f));
			bboxOs.expand(vec3f(-pExtreme.z, 0.f, 0.f));
		} break;
		case billboarding_xOnly: {
			vec3f pExtreme = p0.getAbs().pickMax(p1.getAbs());
			bboxOs.expand(vec3f(0.f, 0.f, pExtreme.x));
			bboxOs.expand(vec3f(0.f, 0.f, -pExtreme.x));
			bboxOs.expand(vec3f(0.f, pExtreme.x, 0.f));
			bboxOs.expand(vec3f(0.f, -pExtreme.x, 0.f));
		} break;
		case billboarding_faceCamera: {
			vec3f pExtreme = p0.getAbs().pickMax(p1.getAbs());
			float d = pExtreme.length();
			bboxOs.expand(vec3f(-d));
			bboxOs.expand(vec3f(d));
		} break;
		default:
			sgeAssertFalse("Unimplemented billboarding");
	}

	if (additionaTransform != mat4f::getIdentity()) {
		bboxOs.getTransformed(additionaTransform);
	}

	return bboxOs;
}
//------------------------------------------------------------------------------
// TraitSpriteEntry
//------------------------------------------------------------------------------
void TraitSpriteEntry::setImage(const AssetPtr& asset) {
	m_assetProperty.setAsset(asset);
}

//------------------------------------------------------------------------------
// TraitSprite
//------------------------------------------------------------------------------
void TraitSprite::addImage(const AssetPtr& imageAsset) {
	TraitSpriteEntry newImage;
	newImage.setImage(imageAsset);
	images.push_back(newImage);
}

void TraitSprite::addImage(const char* const assetPath) {
	addImage(getCore()->getAssetLib()->getAssetFromFile(assetPath));
}

AABox3f TraitSprite::getBBoxOS() const {
	AABox3f bbox;

	for (const TraitSpriteEntry& image : images) {
		const AssetIface_SpriteAnim* const spriteIface = image.m_assetProperty.getAssetInterface<AssetIface_SpriteAnim>();
		const AssetIface_Texture2D* const texIface = image.m_assetProperty.getAssetInterface<AssetIface_Texture2D>();

		if (spriteIface || texIface) {
			return image.imageSettings.computeBBoxOS(*image.m_assetProperty.getAsset().get(),
			                                         additionalTransform * image.m_additionalTransform);
		} else {
			// Not implemented or no asset is loaded.
		}
	}

	return bbox;
}

void TraitSprite::getRenderItems(DrawReason drawReason, const GameDrawSets& drawSets, std::vector<TraitSpriteRenderItem>& renderItems) {
	if (isRenderable == false) {
		return;
	}

	if (!hasShadows && drawReason == drawReason_gameplayShadow) {
		return;
	}

	for (const TraitSpriteEntry& image : images) {
		if (image.isRenderable == false) {
			continue;
		}

		TraitSpriteRenderItem renderItem;

		renderItem.actor = getActor();
		renderItem.forceNoCulling = image.imageSettings.forceNoCulling;
		renderItem.forceNoLighting = image.imageSettings.forceNoLighting;
		renderItem.forceAlphaBlending = image.imageSettings.forceAlphaBlending;
		renderItem.colorTint = image.imageSettings.colorTint;

		const vec3f camPos = drawSets.drawCamera->getCameraPosition();
		const vec3f camLookDir = drawSets.drawCamera->getCameraLookDir();
		Actor* const actor = getActor();

		const AssetPtr& asset = image.m_assetProperty.getAsset();

		if (const AssetIface_Texture2D* texIface = getAssetIface<AssetIface_Texture2D>(asset)) {
			if (Texture* const texture = texIface->getTexture()) {
				renderItem.spriteTexture = texture;

				mat4f obj2world = image.imageSettings.computeObjectToWorldTransform(
				    *asset.get(), drawSets.drawCamera, actor->getTransform(), additionalTransform * image.m_additionalTransform);

				renderItem.obj2world = obj2world;

				renderItem.zSortingPositionWs = obj2world.c3.xyz();
				renderItem.needsAlphaSorting = texIface->getTextureMeta().isSemiTransparent || renderItem.colorTint.w < 0.999f ||
				                               image.imageSettings.forceAlphaBlending;
			}
		} else if (const AssetIface_SpriteAnim* spriteIface = getAssetIface<AssetIface_SpriteAnim>(asset)) {
			if (const AssetIface_Texture2D* texIfaceSprite =
			        getAssetIface<AssetIface_Texture2D>(spriteIface->getSpriteAnimation().textureAsset)) {
				// Get the frame of the sprite to be rendered.
				const SpriteAnimation::Frame* const frame =
				    spriteIface->getSpriteAnimation().spriteAnimation.getFrameForTime(image.imageSettings.spriteFrameTime);
				if (frame) {
					renderItem.spriteTexture = texIfaceSprite->getTexture();

					mat4f obj2world = image.imageSettings.computeObjectToWorldTransform(
					    *asset.get(), drawSets.drawCamera, actor->getTransform(), additionalTransform * image.m_additionalTransform);
					renderItem.obj2world = obj2world;

					// Compute the UVW transform so we get only this frame portion of the texture to be displayed.
					renderItem.uvwTransform =
					    mat4f::getTranslation(frame->uvRegion.x, frame->uvRegion.y, 0.f) *
					    mat4f::getScaling(frame->uvRegion.z - frame->uvRegion.x, frame->uvRegion.w - frame->uvRegion.y, 0.f);

					renderItem.zSortingPositionWs = obj2world.c3.xyz();
					renderItem.needsAlphaSorting = texIface->getTextureMeta().isSemiTransparent || renderItem.colorTint.w < 0.999f ||
					                               image.imageSettings.forceAlphaBlending;
				}
			}
		} else {
			// Not implemented for this asset type.
			continue;
		}

		if (renderItem.spriteTexture != nullptr) {
			renderItems.emplace_back(renderItem);
		}
	}
}

// TraitSprite user interface to be displayed in the propery editor.
void editUI_for_TraitSprite(GameInspector& inspector, GameObject* actor, MemberChain chain) {
	if (ImGui::CollapsingHeader(ICON_FK_PICTURE_O " Sprte Trait")) {
		chain.add(sgeFindMember(TraitSprite, images));
		ProperyEditorUIGen::doMemberUI(inspector, actor, chain);
		chain.pop();
	}
}

// TraitSpriteEntry user interface to be displayed in the propery editor.
void editUI_for_TraitSpriteEntry(GameInspector& inspector, GameObject* actor, MemberChain chain) {
	TraitSpriteEntry& image = *(TraitSpriteEntry*)chain.follow(actor);

	if (ImGui::CollapsingHeader(ICON_FK_PICTURE_O " Sprites")) {
		chain.add(sgeFindMember(TraitSpriteEntry, m_assetProperty));
		ProperyEditorUIGen::doMemberUI(inspector, actor, chain);
		chain.pop();

		if (image.m_assetProperty.getAssetInterface<AssetIface_SpriteAnim>()) {
			chain.add(sgeFindMember(TraitSpriteEntry, imageSettings));
			ProperyEditorUIGen::doMemberUI(inspector, actor, chain);
			chain.pop();
		}

		if (image.m_assetProperty.getAssetInterface<AssetIface_Texture2D>()) {
			chain.add(sgeFindMember(TraitSpriteEntry, imageSettings));
			ProperyEditorUIGen::doMemberUI(inspector, actor, chain);
			chain.pop();
		}
	}
}

SgePluginOnLoad() {
	getEngineGlobal()->addPropertyEditorIUGeneratorForType(sgeTypeId(TraitSprite), editUI_for_TraitSprite);
	getEngineGlobal()->addPropertyEditorIUGeneratorForType(sgeTypeId(TraitSpriteEntry), editUI_for_TraitSpriteEntry);
}

} // namespace sge
