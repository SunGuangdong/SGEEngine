#include "DefaultPBRMtl.h"
#include "sge_core/AssetLibrary/AssetTexture2D.h"

namespace sge {
IMaterialData* DefaultPBRMtl::getMaterialDataLocalStorage() {
	mdlData = DefaultPBRMtlData();

	mdlData.diffuseColor = diffuseColor;
	mdlData.metalness = metallic;
	mdlData.roughness = roughness;

	const auto getTexFromAsset = [](const AssetPtr& asset) -> Texture* {
		if (const AssetIface_Texture2D* texIface = getAssetIface<AssetIface_Texture2D>(asset)) {
			return texIface->getTexture();
		}

		return nullptr;
	};

	mdlData.diffuseTexture = getTexFromAsset(diffuseTexture);
	mdlData.texNormalMap = getTexFromAsset(texNormalMap);
	mdlData.texMetalness = getTexFromAsset(texMetallic);
	mdlData.texRoughness = getTexFromAsset(texRoughness);

	// TODO: this needs to be an option in the material.
	if (mdlData.diffuseTexture != nullptr) {
		mdlData.diffuseColorSrc = DefaultPBRMtlData::diffuseColorSource_diffuseMap;
	} else {
		mdlData.diffuseColorSrc = DefaultPBRMtlData::diffuseColorSource_vertexColor;
	}

	return &mdlData;
}
} // namespace sge
