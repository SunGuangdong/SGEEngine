#include "DefaultPBRMtl.h"
#include "sge_core/AssetLibrary/AssetLibrary.h"
#include "sge_core/AssetLibrary/AssetTexture2D.h"
#include "sge_core/ICore.h"
#include "sge_core/typelib/typeLib.h"
#include "sge_utils/utils/json.h"

namespace sge {

// clang-format off
ReflAddTypeId(DefaultPBRMtl, 10'11'21'0001);
ReflBlock() {
	ReflAddType(DefaultPBRMtl)
		ReflMember(DefaultPBRMtl, needsAlphaSorting)
		ReflMember(DefaultPBRMtl, alphaMultiplier).uiRange(0.f, 1.f, 0.01f)
		ReflMember(DefaultPBRMtl, texDiffuse)
		ReflMember(DefaultPBRMtl, texEmission)
		ReflMember(DefaultPBRMtl, texMetallic)
		ReflMember(DefaultPBRMtl, texRoughness)
		ReflMember(DefaultPBRMtl, texNormalMap)
		ReflMember(DefaultPBRMtl, diffuseColor).addMemberFlag(MFF_Vec4fAsColor)
		ReflMember(DefaultPBRMtl, emissionColor).addMemberFlag(MFF_Vec4fAsColor)
		ReflMember(DefaultPBRMtl, metallic).uiRange(0.f, 1.f, 0.01f)
		ReflMember(DefaultPBRMtl, roughness).uiRange(0.f, 1.f, 0.01f)
		ReflMember(DefaultPBRMtl, disableCulling)
		ReflMember(DefaultPBRMtl, uvShift).uiRange(-FLT_MAX, FLT_MAX, 0.01f)
		ReflMember(DefaultPBRMtl, uvScale).uiRange(-FLT_MAX, FLT_MAX, 0.01f)
		ReflMember(DefaultPBRMtl, uvRotation).addMemberFlag(MFF_FloatAsDegrees)
	;
}
// clang-format on

TypeId DefaultPBRMtl::getTypeId() const {
	return sgeTypeId(DefaultPBRMtl);
}

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
	mdlData.texNormalMap = getTexFromAsset(texNormalMap);

	// TODO: this needs to be an option in the material.
	if (mdlData.diffuseTexture != nullptr) {
		mdlData.diffuseColorSrc = DefaultPBRMtlData::diffuseColorSource_diffuseMap;
	} else {
		mdlData.diffuseColorSrc = DefaultPBRMtlData::diffuseColorSource_vertexColor;
	}

	mdlData.alphaMultipler = alphaMultiplier;

	mdlData.needsAlphaSorting = needsAlphaSorting || isDiffuseSemiTransp || (alphaMultiplier < 1.f);


	// clang-format off
	mdlData.uvwTransform =
	      mat4f::getRotationZ(uvRotation) 
		* mat4f::getScaling(vec3f(uvScale, 0.f)) 
		* mat4f::getTranslation(vec3f(uvShift, 0.f));
	// clang-format on

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
