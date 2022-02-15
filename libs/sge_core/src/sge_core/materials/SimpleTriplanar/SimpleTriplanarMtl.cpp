#include "SimpleTriplanarMtl.h"
#include "sge_core/AssetLibrary/AssetLibrary.h"
#include "sge_core/AssetLibrary/AssetTexture2D.h"
#include "sge_core/ICore.h"
#include "sge_core/typelib/typeLib.h"
#include "sge_utils/json/json.h"
#include "sge_utils/text/Path.h"

namespace sge {

// clang-format off
ReflAddTypeId(SimpleTriplanarMtl, 10'11'21'0001);
ReflBlock() {
	ReflAddType(SimpleTriplanarMtl)
		ReflMember(SimpleTriplanarMtl, texDiffuseX)
		ReflMember(SimpleTriplanarMtl, texDiffuseY)
		ReflMember(SimpleTriplanarMtl, texDiffuseZ)
		ReflMember(SimpleTriplanarMtl, diffuseColor).addMemberFlag(MFF_Vec4fAsColor)
		ReflMember(SimpleTriplanarMtl, uvwShift).uiRange(-FLT_MAX, FLT_MAX, 0.01f)
		ReflMember(SimpleTriplanarMtl, uvwScale).uiRange(0.01f, FLT_MAX, 0.01f)
		ReflMember(SimpleTriplanarMtl, sharpness).uiRange(1.f, 64.f, 0.1f)
		ReflMember(SimpleTriplanarMtl, ignoreTranslation)
		ReflMember(SimpleTriplanarMtl, forceNoLighting)
	;
}
// clang-format on

TypeId SimpleTriplanarMtl::getTypeId() const
{
	return sgeTypeId(SimpleTriplanarMtl);
}

IMaterialData* SimpleTriplanarMtl::getMaterialDataLocalStorage()
{
	mdlData = SimpleTriplanarMtlData();

	mdlData.forceNoLighting = forceNoLighting;

	mdlData.diffuseColor = diffuseColor;

	const auto getTexFromAsset = [](const std::shared_ptr<AssetIface_Texture2D>& texIface) -> Texture* {
		if (texIface) {
			return texIface->getTexture();
		}

		return nullptr;
	};


	mdlData.diffuseTextureX = getTexFromAsset(texDiffuseX);
	mdlData.diffuseTextureY = getTexFromAsset(texDiffuseY);
	mdlData.diffuseTextureZ = getTexFromAsset(texDiffuseZ);

	mdlData.alphaMultipler = 1.f;
	mdlData.needsAlphaSorting = false;

	vec3f scale = uvwScale;
	scale.x = (scale.x != 0.f) ? 1.f / scale.x : 0.f;
	scale.y = (scale.y != 0.f) ? 1.f / scale.y : 0.f;
	scale.z = (scale.z != 0.f) ? 1.f / scale.z : 0.f;


	// clang-format off
	mdlData.uvwTransform =
		  mat4f::getScaling(scale) 
		* mat4f::getTranslation(uvwShift);
	// clang-format on

	mdlData.ignoreTranslation = ignoreTranslation;
	mdlData.sharpness = sharpness;

	return &mdlData;
}

JsonValue* SimpleTriplanarMtl::toJson(JsonValueBuffer& jvb, const char* localDir)
{
	JsonValue* jMaterial = jvb(JID_MAP);

	jMaterial->setMember("family", jvb("SimpleTriplanar"));
	jMaterial->setMember("forceNoLighting", jvb(forceNoLighting));
	jMaterial->setMember("ignoreTranslation", jvb(ignoreTranslation));


	auto setMemberFromTexIface =
	    [&jvb, &jMaterial, &localDir](const char* memberName, std::shared_ptr<AssetIface_Texture2D>& texIface) -> void {
		sgeAssert(memberName);
		if (texIface) {
			AssetPtr asset = std::dynamic_pointer_cast<Asset>(texIface);
			if (asset && isAssetLoaded(asset)) {
				jMaterial->setMember(memberName, jvb(relativePathTo(asset->getPath().c_str(), localDir)));
			}
		}
	};

	setMemberFromTexIface("texDiffuseX", texDiffuseX);
	setMemberFromTexIface("texDiffuseY", texDiffuseY);
	setMemberFromTexIface("texDiffuseZ", texDiffuseZ);

	static_assert(sizeof(diffuseColor) == sizeof(vec4f));
	static_assert(sizeof(uvwShift) == sizeof(vec3f));
	static_assert(sizeof(uvwScale) == sizeof(vec3f));

	jMaterial->setMember("diffuseColor", jvb(diffuseColor.data, 4)); // An array of 4 floats rgba.
	jMaterial->setMember("sharpness", jvb(sharpness));
	jMaterial->setMember("uvwShift", jvb(uvwShift.data, 3)); // An array of 3 floats rgba.
	jMaterial->setMember("uvwScale", jvb(uvwScale.data, 3)); // An array of 3 floats rgba.

	return jMaterial;
}

bool SimpleTriplanarMtl::fromJson(const JsonValue* jMaterial, const char* localDir)
{
	if (jMaterial == nullptr) {
		return false;
	}

	*this = SimpleTriplanarMtl();

	if (const JsonValue* jNeedsAlphaSorting = jMaterial->getMember("forceNoLighting")) {
		forceNoLighting = jNeedsAlphaSorting->getAsBool();
	}

	if (const JsonValue* jIgnoreTranslation = jMaterial->getMember("ignoreTranslation")) {
		ignoreTranslation = jIgnoreTranslation->getAsBool();
	}

	if (const JsonValue* jVal = jMaterial->getMember("sharpness")) {
		sharpness = jVal->getNumberAs<float>();
	}

	if (const JsonValue* jTex = jMaterial->getMember("texDiffuseX")) {
		texDiffuseX = std::dynamic_pointer_cast<AssetIface_Texture2D>(
		    getCore()->getAssetLib()->getAssetFromFile(jTex->GetString(), localDir));
	}

	if (const JsonValue* jTex = jMaterial->getMember("texDiffuseY")) {
		texDiffuseY = std::dynamic_pointer_cast<AssetIface_Texture2D>(
		    getCore()->getAssetLib()->getAssetFromFile(jTex->GetString(), localDir));
	}


	if (const JsonValue* jTex = jMaterial->getMember("texDiffuseZ")) {
		texDiffuseZ = std::dynamic_pointer_cast<AssetIface_Texture2D>(
		    getCore()->getAssetLib()->getAssetFromFile(jTex->GetString(), localDir));
	}


	if (const JsonValue* j = jMaterial->getMember("diffuseColor")) {
		j->getNumberArrayAs<float>(diffuseColor.data, 4);
	}

	if (const JsonValue* j = jMaterial->getMember("sharpness")) {
		sharpness = j->getNumberAs<float>();
	}

	if (const JsonValue* j = jMaterial->getMember("diffuseColor")) {
		j->getNumberArrayAs<float>(diffuseColor.data, 4);
	}

	if (const JsonValue* j = jMaterial->getMember("uvwShift")) {
		j->getNumberArrayAs<float>(uvwShift.data, 3);
	}

	if (const JsonValue* j = jMaterial->getMember("uvwScale")) {
		j->getNumberArrayAs<float>(uvwScale.data, 3);
	}

	return true;
}

} // namespace sge
