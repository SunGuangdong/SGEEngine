#pragma once

#include "IAsset.h"
#include "IAssetInterface.h"
#include "sge_audio/AudioDevice.h"
#include "sge_core/Sprite.h"


namespace sge {

struct SGE_CORE_API AssetIface_SpriteAnim : public IAssetInterface {
	virtual SpriteAnimationWithTextures& getSpriteAnimation() = 0;
	virtual const SpriteAnimationWithTextures& getSpriteAnimation() const = 0;
};

struct SGE_CORE_API AssetSpriteAnim : public Asset, public AssetIface_SpriteAnim {
	AssetSpriteAnim(std::string assetPath, AssetLibrary& ownerAssetLib)
	    : Asset(assetPath, ownerAssetLib) {
	}

	virtual SpriteAnimationWithTextures& getSpriteAnimation() override {
		return m_sprite;
	}

	virtual const SpriteAnimationWithTextures& getSpriteAnimation() const override {
		return m_sprite;
	}

	bool loadAssetFromFile(const char* const path) override;

  public:
	SpriteAnimationWithTextures m_sprite;
};

} // namespace sge
