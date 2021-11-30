#include "DefaultPBRMtl.h"
#include "sge_core/AssetLibrary/AssetLibrary.h"
#include "sge_core/AssetLibrary/AssetTexture2D.h"
#include "sge_core/ICore.h"
#include "sge_core/typelib/typeLib.h"
#include "sge_utils/utils/Path.h"
#include "sge_utils/utils/json.h"

namespace sge {

// clang-format off
ReflAddTypeId(DefaultPBRMtl, 10'11'21'0001);
ReflBlock() {
	ReflAddType(DefaultPBRMtl)
		ReflMember(DefaultPBRMtl, forceNoLighting)
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

	const auto getTexFromAsset = [](const std::shared_ptr<AssetIface_Texture2D>& texIface, bool* pIsSemiTransparent = nullptr) -> Texture* {
		if (texIface) {
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


JsonValue* DefaultPBRMtl::toJson(JsonValueBuffer& jvb, const char* localDir) {
	JsonValue* jMaterial = jvb(JID_MAP);

	jMaterial->setMember("family", jvb("DefaultPBR"));
	jMaterial->setMember("forceNoLighting", jvb(forceNoLighting));
	jMaterial->setMember("disableCulling", jvb(disableCulling));
	jMaterial->setMember("alphaMultiplier", jvb(alphaMultiplier));
	jMaterial->setMember("needsAlphaSorting", jvb(needsAlphaSorting));

	static_assert(sizeof(diffuseColor) == sizeof(vec4f));
	jMaterial->setMember("diffuseColor", jvb(diffuseColor.data, 4)); // An array of 4 floats rgba.
	static_assert(sizeof(emissionColor) == sizeof(vec4f));
	jMaterial->setMember("emissionColor", jvb(emissionColor.data, 4)); // An array of 4 floats rgba.
	jMaterial->setMember("metallic", jvb(metallic));
	jMaterial->setMember("roughness", jvb(roughness));


	auto setMemberFromTexIface = [&jvb, &jMaterial, &localDir](const char* memberName,
	                                                           std::shared_ptr<AssetIface_Texture2D>& texIface) -> void {
		sgeAssert(memberName);
		if (texIface) {
			AssetPtr asset = std::dynamic_pointer_cast<Asset>(texIface);
			if (asset && isAssetLoaded(asset)) {
				jMaterial->setMember(memberName, jvb(relativePathTo(asset->getPath().c_str(), localDir)));
			}
		}
	};

	setMemberFromTexIface("texDiffuse", texDiffuse);
	setMemberFromTexIface("texEmission", texEmission);
	setMemberFromTexIface("texMetallic", texMetallic);
	setMemberFromTexIface("texRoughness", texRoughness);
	setMemberFromTexIface("texNormalMap", texNormalMap);

	return jMaterial;
}

bool DefaultPBRMtl::fromJson(const JsonValue* jMaterial, const char* localDir) {
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

	if (const JsonValue* jNeedsAlphaSorting = jMaterial->getMember("forceNoLighting")) {
		forceNoLighting = jNeedsAlphaSorting->getAsBool();
	}

	if (const JsonValue* jNeedsAlphaSorting = jMaterial->getMember("disableCulling")) {
		forceNoLighting = jNeedsAlphaSorting->getAsBool();
	}

	jMaterial->getMember("diffuseColor")->getNumberArrayAs<float>(diffuseColor.data, 4);
	jMaterial->getMember("emissionColor")->getNumberArrayAs<float>(emissionColor.data, 4);
	metallic = jMaterial->getMember("metallic")->getNumberAs<float>();
	roughness = jMaterial->getMember("roughness")->getNumberAs<float>();


	if (const JsonValue* jTex = jMaterial->getMember("texDiffuse")) {
		texDiffuse =
		    std::dynamic_pointer_cast<AssetIface_Texture2D>(getCore()->getAssetLib()->getAssetFromFile(jTex->GetString(), localDir));
	}

	if (const JsonValue* jTex = jMaterial->getMember("texEmission")) {
		texEmission =
		    std::dynamic_pointer_cast<AssetIface_Texture2D>(getCore()->getAssetLib()->getAssetFromFile(jTex->GetString(), localDir));
	}

	if (const JsonValue* jTex = jMaterial->getMember("texMetallic")) {
		texMetallic =
		    std::dynamic_pointer_cast<AssetIface_Texture2D>(getCore()->getAssetLib()->getAssetFromFile(jTex->GetString(), localDir));
	}

	if (const JsonValue* jTex = jMaterial->getMember("texRoughness")) {
		texRoughness =
		    std::dynamic_pointer_cast<AssetIface_Texture2D>(getCore()->getAssetLib()->getAssetFromFile(jTex->GetString(), localDir));
	}

	if (const JsonValue* jTex = jMaterial->getMember("texNormalMap")) {
		texNormalMap =
		    std::dynamic_pointer_cast<AssetIface_Texture2D>(getCore()->getAssetLib()->getAssetFromFile(jTex->GetString(), localDir));
	}

	return true;
}

} // namespace sge
