#pragma once

#include "sge_core/AssetLibrary/IAsset.h"
#include "sge_core/materials/IMaterial.h"
#include "sge_core/sgecore_api.h"
#include "sge_utils/containers/Optional.h"
#include "sge_utils/math/mat4f.h"

namespace sge {

struct Texture;
struct AssetIface_Texture2D;

struct SGE_CORE_API DefaultPBRMtlData : public IMaterialData {
	enum DiffuseColorSource {
		diffuseColorSource_vertexColor,
		diffuseColorSource_constantColor,
		diffuseColorSource_diffuseMap,
	};

	DefaultPBRMtlData()
	    : IMaterialData(1001)
	{
	}

	mat4f uvwTransform = mat4f::getIdentity();

	vec4f diffuseColor = vec4f(1.f, 1.f, 1.f, 1.f);

	Texture* diffuseTexture = nullptr;
	Texture* texMetalness = nullptr;
	Texture* texRoughness = nullptr;

	Texture* texNormalMap = nullptr;

	vec3f diffuseTexXYZScaling = vec3f(1.f); // Triplanar scaling for each axis.

	DiffuseColorSource diffuseColorSrc = diffuseColorSource_constantColor;
	bool tintByVertexColor = false;

	float metalness = 0.f;
	float roughness = 1.f;

	bool disableCulling = false;
	bool forceNoLighting = false;
};

struct SGE_CORE_API DefaultPBRMtl : public IMaterial {
	virtual IMaterialData* getMaterialDataLocalStorage() override;

	TypeId getTypeId() const override;

	virtual JsonValue* toJson(JsonValueBuffer& jvb, const char* localDir) override;
	virtual bool fromJson(const JsonValue* jMtlRoot, const char* localDir) override;

  public:
	bool forceNoLighting = false;
	bool needsAlphaSorting = false;
	float alphaMultiplier = 1.f;

	std::shared_ptr<AssetIface_Texture2D> texDiffuse;
	std::shared_ptr<AssetIface_Texture2D> texEmission;
	std::shared_ptr<AssetIface_Texture2D> texMetallic;
	std::shared_ptr<AssetIface_Texture2D> texRoughness;
	std::shared_ptr<AssetIface_Texture2D> texNormalMap;

	vec4f diffuseColor = vec4f(1.f);
	vec4f emissionColor = vec4f(0.f);
	float metallic = 0.f;
	float roughness = 1.f;

	bool disableCulling = false;

	vec2f uvShift = vec2f(0.f);
	vec2f uvScale = vec2f(1.f);
	float uvRotation = 0.f;

	DefaultPBRMtlData mdlData;
};
} // namespace sge
