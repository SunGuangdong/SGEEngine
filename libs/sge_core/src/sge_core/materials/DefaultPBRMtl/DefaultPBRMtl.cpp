#include "DefaultPBRMtl.h"
#include "sge_core/AssetLibrary/AssetTexture2D.h"

namespace sge {
IMaterialData* DefaultPBRMtl::getMaterialDataLocalStorage() {
	mdlData = DefaultPBRMtlData();

	mdlData.diffuseColor = diffuseColor;
	mdlData.metalness = metallic;
	mdlData.roughness = roughness;

	const auto getTexFromAsset = [](const AssetPtr& asset, bool* pIsSemiTransparent = nullptr) -> Texture* {
		if (const AssetIface_Texture2D* texIface = getAssetIface<AssetIface_Texture2D>(asset)) {

			if (pIsSemiTransparent) {
				*pIsSemiTransparent |= texIface->getTextureMeta().isSemiTransparent;
			}

			return texIface->getTexture();
		}

		return nullptr;
	};

	bool isDiffuseSemiTransp = diffuseColor.w < 1.f;
	mdlData.diffuseTexture = getTexFromAsset(diffuseTexture, &isDiffuseSemiTransp);
	mdlData.texNormalMap = getTexFromAsset(texNormalMap);
	mdlData.texMetalness = getTexFromAsset(texMetallic);
	mdlData.texRoughness = getTexFromAsset(texRoughness);

	// TODO: this needs to be an option in the material.
	if (mdlData.diffuseTexture != nullptr) {
		mdlData.diffuseColorSrc = DefaultPBRMtlData::diffuseColorSource_diffuseMap;
	} else {
		mdlData.diffuseColorSrc = DefaultPBRMtlData::diffuseColorSource_vertexColor;
	}

	mdlData.needsAlphaSorting = isDiffuseSemiTransp;
	mdlData.alphaMultipler = alphaMultiplier;

	return &mdlData;
}
} // namespace sge
