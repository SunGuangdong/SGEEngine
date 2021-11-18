#include "DefaultPBRMtl.h"
#include "sge_core/AssetLibrary/AssetLibrary.h"
#include "sge_core/AssetLibrary/AssetTexture2D.h"
#include "sge_core/ICore.h"
#include "sge_utils/utils/json.h"

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
	mdlData.diffuseTexture = getTexFromAsset(texDiffuse, &isDiffuseSemiTransp);
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


JsonValue* DefaultPBRMtl::toJson(JsonValueBuffer& jvb) {
	JsonValue* jMaterial = jvb(JID_MAP);

	jMaterial->setMember("family", jvb("DefaultPBR"));
	jMaterial->setMember("alphaMultiplier", jvb(alphaMultiplier));
	jMaterial->setMember("needsAlphaSorting", jvb(needsAlphaSorting));

	static_assert(sizeof(diffuseColor) == sizeof(vec4f));
	jMaterial->setMember("diffuseColor", jvb(diffuseColor.data, 4)); // An array of 4 floats rgba.
	static_assert(sizeof(emissionColor) == sizeof(vec4f));
	jMaterial->setMember("emissionColor", jvb(emissionColor.data, 4)); // An array of 4 floats rgba.
	jMaterial->setMember("metallic", jvb(metallic));
	jMaterial->setMember("roughness", jvb(roughness));


	if (isAssetLoaded(texDiffuse))
		jMaterial->setMember("texDiffuse", jvb(texDiffuse->getPath()));

	if (isAssetLoaded(texEmission))
		jMaterial->setMember("texEmission", jvb(texEmission->getPath()));

	if (isAssetLoaded(texMetallic))
		jMaterial->setMember("texMetallic", jvb(texMetallic->getPath()));

	if (isAssetLoaded(texRoughness))
		jMaterial->setMember("texRoughness", jvb(texRoughness->getPath()));

	if (isAssetLoaded(texNormalMap))
		jMaterial->setMember("texNormalMap", jvb(texNormalMap->getPath()));

	return jMaterial;
}

bool DefaultPBRMtl::fromJson(const JsonValue* jMaterial) {
	if (jMaterial == nullptr) {
		return false;
	}

	*this = DefaultPBRMtl();

	if (const JsonValue* jNeedsAlphaSorting = jMaterial->getMember("alphaMultiplier")) {
		alphaMultiplier = jNeedsAlphaSorting->getNumberAs<float>();
	}

	if (const JsonValue* jNeedsAlphaSorting = jMaterial->getMember("needsAlphaSorting")) {
		needsAlphaSorting = jNeedsAlphaSorting->getAsBool();
	}

	jMaterial->getMember("diffuseColor")->getNumberArrayAs<float>(diffuseColor.data, 4);
	jMaterial->getMember("emissionColor")->getNumberArrayAs<float>(emissionColor.data, 4);
	metallic = jMaterial->getMember("metallic")->getNumberAs<float>();
	roughness = jMaterial->getMember("roughness")->getNumberAs<float>();


	if (const JsonValue* jTex = jMaterial->getMember("texDiffuse")) {
		texDiffuse = getCore()->getAssetLib()->getAssetFromFile(jTex->GetString());
	}

	if (const JsonValue* jTex = jMaterial->getMember("texEmission")) {
		texEmission = getCore()->getAssetLib()->getAssetFromFile(jTex->GetString());
	}

	if (const JsonValue* jTex = jMaterial->getMember("texMetallic")) {
		texMetallic = getCore()->getAssetLib()->getAssetFromFile(jTex->GetString());
	}

	if (const JsonValue* jTex = jMaterial->getMember("texRoughness")) {
		texRoughness = getCore()->getAssetLib()->getAssetFromFile(jTex->GetString());
	}

	if (const JsonValue* jTex = jMaterial->getMember("texNormalMap")) {
		texNormalMap = getCore()->getAssetLib()->getAssetFromFile(jTex->GetString());
	}

	return true;
}

} // namespace sge
