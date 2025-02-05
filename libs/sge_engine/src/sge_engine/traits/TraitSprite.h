#pragma once

#include "sge_engine/Actor.h"
#include "sge_engine/AssetProperty.h"
#include "sge_engine/GameDrawer/IRenderItem.h"
#include "sge_engine/enums2d.h"

namespace sge {

struct ICamera;
struct TraitModel;
struct TraitSpriteRenderItem;

ReflAddTypeIdExists(TraitSprite);

/// @brief A struct holding the rendering options of a sprite or a texture in 3D.
struct TraitSpriteImageSets {
	/// @brief Computes the object-to-node transformation of the sprite so i has its origin in the deisiered location.
	mat4f getAnchorAlignMtxOS(float imageWidth, float imageHeight) const
	{
		const float sz = imageWidth / m_pixelsPerUnit;
		const float sy = imageHeight / m_pixelsPerUnit;

		const mat4f anchorAlignMtx = anchor_getPlaneAlignMatrix(m_anchor, vec2f(sz, sy));
		return anchorAlignMtx;
	}

	/// @brief Computes the matrix that needs to be applied to a quad
	/// that faces +X and has corners (0,0,0) and (0,1,1), So it appears as described by this structure:
	/// its size, orientation (billboarding), aligment and so on.
	/// @param [in] asset is the asset that is going to be attached to the plane. it needs to be a texture or a sprite.
	/// @param [in] drawCamera the camera that is going to be used for rendering. If null the billboarding effect will
	/// be missing.
	/// @param [in] nodeToWorldTransform the transform that indicates the location of the object(for example, could be
	/// the transform of an actor).
	/// @param [in] additionaTransform an additional transform to be applied before before all other transforms.
	/// @return the matrix to be used as object-to-world transform form the quad described above.
	mat4f computeObjectToWorldTransform(
	    const Asset& asset,
	    const ICamera* const drawCamera,
	    const transf3d& nodeToWorldTransform,
	    const mat4f& additionaTransform) const;

	Box3f computeBBoxOS(const Asset& asset, const mat4f& additionaTransform) const;

	/// Adds a color tint to the final color and the alpha of the object.
	vec4f colorTint = vec4f(1.f);

	// Sprite and texture drawing.
	/// @brief Describes where the (0,0,0) point of the plane should be relative to the object.
	/// TODO: replace this with UV style coordinates.
	Anchor m_anchor = anchor_bottomMid;

	/// @brief Describes how much along the plane normal (which is X) should the plane be moved.
	/// This is useful when we want to place recoration on top of walls or floor objects.
	float m_localXOffset = 0.01f;

	/// @brief Describes how big is one pixel in world space.
	float m_pixelsPerUnit = 64.f;

	/// @brief Describes if any billboarding should happen for the plane.
	Billboarding m_billboarding = billboarding_none;

	/// @brief if true the image will get rendered with no lighting applied,
	/// as if we just rendered the color (with gamma correction and post processing).
	bool forceNoLighting = true;

	/// @brief if true the plane will not get any culling applied. Useful if we want the
	/// texture to be visible from both sides.
	bool forceNoCulling = true;

	/// @brief If true, the sprite plane will, by default look towards +Z, otherwise it will be facing +X.
	bool defaultFacingAxisZ = true;

	/// Flips the sprite horizontally while maintaining its origin at the same place.
	/// Useful if we are building a 2D game where the character is a sprite that can walk left and right.
	bool flipHorizontally = false;

	/// Sprite (if any) evaluation time in seconds.
	float spriteFrameTime = 0.f;

	/// @brief True if the sprite needs to be drawn always with alpha blending.
	/// Usually this is needed when there are some edges that appear rough or sharp
	/// or when the image is semi-transparent.
	bool forceAlphaBlending = false;
};

/// A single entry in the trait sprite. This entry can represent
/// a sprite or a texture 2d to get rendered and holds its render settings.
struct TraitSpriteEntry {
	TraitSpriteEntry()
	    : m_assetProperty(assetIface_texture2d, assetIface_spriteAnim)
	{
	}

	void setImage(const AssetPtr& asset);

	bool isRenderable = true;
	AssetProperty m_assetProperty;
	mat4f m_additionalTransform = mat4f::getIdentity();
	TraitSpriteImageSets imageSettings;
};

/// TraitSprite provides a way to render 2D texture and sprites as textured planes and billboards in the 3D world.
struct SGE_ENGINE_API TraitSprite : public Trait {
	SGE_TraitDecl_Full(TraitSprite);

	TraitSprite() = default;

	void addImage(const AssetPtr& imageAsset);
	void addImage(const char* const assetPath);

	/// Not called automatically see the class comment above.
	/// Updates the working sprites. The changes usually occure the the assets has been changed via the Property Editor.
	/// Returns true if the model has been changed (no matter if it is valid or not).
	/// If the game object doesn't offer any changed via the UI there is no point in calling it.
	bool postUpdate() { return updateAssetProperty(); }

	Box3f getBBoxOS() const;

	/// Generates the list of render items for this trait.
	void getRenderItems(
	    DrawReason drawReason, const GameDrawSets& drawSets, std::vector<TraitSpriteRenderItem>& renderItems);

  private:
	bool updateAssetProperty()
	{
		bool hadChange = false;
		for (TraitSpriteEntry& image : images) {
			hadChange = image.m_assetProperty.update() || hadChange;
		}
		return hadChange;
	}

  public:
	bool isRenderable = true;
	bool hasShadows = true;
	std::vector<TraitSpriteEntry> images;

	/// A transform to be applied an all entries in @images when rendered.
	mat4f additionalTransform = mat4f::getIdentity();
};

} // namespace sge
