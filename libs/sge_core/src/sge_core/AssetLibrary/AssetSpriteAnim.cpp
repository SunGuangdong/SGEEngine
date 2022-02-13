#include "AssetSpriteAnim.h"

namespace sge {

bool AssetSpriteAnim::loadAssetFromFile(const char* const path)
{
	m_status = AssetStatus_LoadFailed;
	if (SpriteAnimationWithTextures::importSprite(m_sprite, path, m_ownerAssetLib)) {
		m_status = AssetStatus_Loaded;
	}

	return m_status == AssetStatus_Loaded;
}
} // namespace sge
