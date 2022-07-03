#pragma once

#include "sge_core/sgecore_api.h"
#include <string>

namespace sge {

struct JsonValueBuffer;
struct JsonValue;
struct TypeId;

struct Texture;

/// Before reading this, read the comment for @IMaterial.
///
/// This structure describes the raw material data to be passed to the @IGeometryDrawer to execute the draw call.
///
/// Why is it needed, when we have IMaterial?
/// This structure has all of the IMateial data crunched and ready do get rendered.
/// the @IMaterialData is not expected to be valid after the frame ends, on each frame it is asked to get generated
/// again. The idea is that this structure lifts the weight of the @IGeometryDrawer to load and deal with assets.
/// Usually a material has some textures in it and it is applied to multiple instances.
/// Having the IGeomDrawer asking for each draw call to resolve the material assets (witch would not change) seems like
/// are redundand and slow work (however I did not measure it).
struct SGE_CORE_API IMaterialData {
	IMaterialData(uint32 materialFamilyId)
	    : materialFamilyId(materialFamilyId)
	{
	}
	virtual ~IMaterialData() = default;

	virtual Texture* getTextureForShadowMap() { return nullptr; }

	uint32 materialFamilyId = 0;
	float alphaMultipler = 1.f;
	bool needsAlphaSorting = false;
};

/// IMaterial is a base interface class used to describe proparties of a single instance of a material.
/// Each material type needs to have one of these to describe its properties.
/// The class is directly used in the AssetLibrary as asset material are represeted by it.
/// It is used together with a matching @IGeometryDrawer (basically a shading programmer wrapper) that has
/// the code to correctly render a gemetry with this material applied.
struct SGE_CORE_API IMaterial {
	IMaterial() = default;
	virtual ~IMaterial() = default;

	virtual IMaterial* materialCreateCopy() = 0;

	virtual IMaterialData* getMaterialDataLocalStorage() = 0;

	/// Returns the type-id from the reflection system of the material.
	/// Used for serialization of assets and user interface.
	virtual TypeId getTypeId() const = 0;

	/// A function that should be used to serialize the material to json so assets could be saved.
	/// Should return nullptr if not supported.
	/// @param [in/out] jvb is a pool that allocates JsonValue, you should use that pool
	/// to allocate all needed json values.
	/// @param [in] localDir, may be nullptr. This is used when we save material as asset files.
	///             To keep the asset paths clear we prefer to have realative assets, often the
	///             the needed texture by the material is in the same directory as the material itself.
	///             if we know where the material is going to get saved, we can reference the texture with
	///             relative paths to the resulting material file location.
	/// @return the root json map value that represents the material.
	virtual JsonValue* toJson(JsonValueBuffer& jvb, const char* localDir) = 0;

	/// A function used to load a in-place a material form the specificed JsonValue.
	/// Please be aware that the json might come from everywhere.
	/// @param [in] jMtlRoot the json value that represents the material.
	/// @param [in] localDir, may be nullptr. This is used when we save material as asset files.
	///             To keep the asset paths clear we prefer to have realative assets, often the
	///             the needed texture by the material is in the same directory as the material itself.
	///             if we know where the material is going to get saved, we can reference the texture with
	///             relative paths to the resulting material file location.
	/// @ return true if the loading was successful.
	virtual bool fromJson(const JsonValue* jMtlRoot, const char* localDir) = 0;
};

} // namespace sge
