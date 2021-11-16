#pragma once

#include "sge_core/AssetLibrary/IAsset.h"
#include "sge_core/materials/IMaterial.h"
#include "sge_core/sgecore_api.h"
#include "sge_utils/math/mat4.h"
#include "sge_utils/utils/optional.h"

namespace sge {

struct Texture;

struct SGE_CORE_API DefaultPBRMtlData : public IMaterialData {
	enum DiffuseColorSource {
		diffuseColorSource_vertexColor,
		diffuseColorSource_constantColor,
		diffuseColorSource_diffuseMap,
	};

	DefaultPBRMtlData()
	    : IMaterialData(1001) {
	}

	mat4f uvwTransform = mat4f::getIdentity();

	vec4f diffuseColor = vec4f(1.f, 1.f, 1.f, 1.f);
	Texture* texNormalMap = nullptr;
	Texture* diffuseTexture = nullptr;
	Texture* diffuseTextureX = nullptr; // Triplanar x-axis texture.
	Texture* diffuseTextureY = nullptr; // Triplanar y-axis texture.
	Texture* diffuseTextureZ = nullptr; // Triplanar z-axis texture.

	Texture* texMetalness = nullptr;
	Texture* texRoughness = nullptr;

	vec3f diffuseTexXYZScaling = vec3f(1.f); // Triplanar scaling for each axis.

	DiffuseColorSource diffuseColorSrc = diffuseColorSource_constantColor;
	bool tintByVertexColor = false;

	vec4f fluidColor0 = vec4f(1.f);
	vec4f fluidColor1 = vec4f(1.f);

	float metalness = 0.f;
	float roughness = 0.30f;
	float alphaMultiplier = 1.f;

	bool disableCulling = false;
};

struct SGE_CORE_API DefaultPBRMtl : public IMaterial {
	virtual IMaterialData* getMaterialDataLocalStorage() override;

	virtual std::string toJson() override {
		return std::string();
	}

	virtual void fromJson(const char* UNUSED(json)) override {
	}


  public:
	AssetPtr diffuseTexture;
	AssetPtr texNormalMap;
	AssetPtr texMetallic;
	AssetPtr texRoughness;

	vec4f diffuseColor = vec4f(1.f, 0.f, 1.f, 1.f);
	float metallic = 1.f;
	float roughness = 1.f;
	bool needsAlphaSorting = false;
	float alphaMultiplier = 1.f;

	DefaultPBRMtlData mdlData;
};
} // namespace sge
