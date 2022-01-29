#pragma once

#include "sge_core/AssetLibrary/IAsset.h"
#include "sge_core/materials/IMaterial.h"
#include "sge_core/sgecore_api.h"
#include "sge_utils/math/mat4.h"
#include "sge_utils/containers/Optional.h"

namespace sge {

struct Texture;
struct AssetIface_Texture2D;

struct SGE_CORE_API SimpleTriplanarMtlData : public IMaterialData {
	SimpleTriplanarMtlData()
	    : IMaterialData(1002) {
	}

	mat4f uvwTransform = mat4f::getIdentity();

	vec4f diffuseColor = vec4f(1.f, 1.f, 1.f, 1.f);

	Texture* diffuseTextureX = nullptr;
	Texture* diffuseTextureY = nullptr;
	Texture* diffuseTextureZ = nullptr;

	float sharpness = 1.f;

	bool forceNoLighting = false;
	bool ignoreTranslation = false;
};

struct SGE_CORE_API SimpleTriplanarMtl : public IMaterial {
	virtual IMaterialData* getMaterialDataLocalStorage() override;

	TypeId getTypeId() const override;

	virtual JsonValue* toJson(JsonValueBuffer& jvb, const char* localDir) override;
	virtual bool fromJson(const JsonValue* jMtlRoot, const char* localDir) override;

  public:
	bool forceNoLighting = false;
	bool ignoreTranslation = false;

	std::shared_ptr<AssetIface_Texture2D> texDiffuseX;
	std::shared_ptr<AssetIface_Texture2D> texDiffuseY;
	std::shared_ptr<AssetIface_Texture2D> texDiffuseZ;

	vec4f diffuseColor = vec4f(1.f);
	float sharpness = 1.f;
	vec3f uvwShift = vec3f(0.f);
	vec3f uvwScale = vec3f(1.f);

	SimpleTriplanarMtlData mdlData;
};
} // namespace sge
