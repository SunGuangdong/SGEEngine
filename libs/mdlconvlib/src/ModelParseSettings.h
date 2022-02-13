#pragma once

#include <string>

namespace sge {

struct IAssetRelocationPolicy;

//---------------------------------------------------------------------
// Import/Export settings.
//---------------------------------------------------------------------
enum class MeshPacking {
	// The whole scene is placed in one meshData.
	PackWholeScene,

	// All meshes that belong to the same mesh are stored in 1 meshData.
	PackPerMesh,

	// Each mesh has it's own meshData.
	NoPacking,
};

struct ModelParseSettings {
	ModelParseSettings() = default;

	ModelParseSettings(bool const shouldExportAnimations,
	                   MeshPacking const meshPacking,
	                   std::string fileDirectroy,
	                   IAssetRelocationPolicy* const pRelocaionPolicy)
	    : shouldExportAnimations(shouldExportAnimations)
	    , meshPacking(meshPacking)
	    , fileDirectroy(fileDirectroy)
	    , pRelocaionPolicy(pRelocaionPolicy)
	{
	}

	//  Is animation export enabled.
	bool shouldExportAnimations = true;

	// Mesh packing rules.
	MeshPacking meshPacking = MeshPacking::NoPacking;

	// The directory that points to this file.
	// Used to resolve references to other resources like textures.
	std::string fileDirectroy;

	// Asset relocation policy is used to speficy the new location
	// of the dependand assts after the parsing has been done.
	IAssetRelocationPolicy* pRelocaionPolicy = nullptr;
};

} // namespace sge
