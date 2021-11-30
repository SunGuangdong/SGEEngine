#pragma once

#include "sge_core/model/Model.h"
#include "sge_utils/math/vec4.h"
#include <set>
#include <string>
#include <vector>

extern "C" {
/// When a material is imported from an external 3D scene file (FBX for example)
/// we need to pass its parameters to the sge_engine so it could create a material for it.
/// this structure describes these materials.
struct ExternalPBRMaterialSettings {
	std::string diffuseTextureName;
	std::string emissionTextureName;
	std::string normalTextureName;
	std::string metallicTextureName;
	std::string roughnessTextureName;

	sge::vec4f diffuseColor = sge::vec4f(1.f);
	sge::vec4f emissionColor = sge::vec4f(0.f);
	float metallic = 0.f;
	float roughness = 1.f;
};

/// When a 3D model is imported it usually has some textures and materials
/// that are, by default, used by it. For example this could be materials and their textures.
/// This structure holds description to these resources so they could be copied/imported.
struct ModelImportAdditionalResult {
	/// A list of textures needed. This strings are the file paths as they were
	/// found in the external 3D scene file.
	std::set<std::string> textureToCopy;

	/// A list of material to be created.
	/// "key" of the map holds the asset name to be used as a filename for the material.
	/// "value" of the map holds the description of the material.
	std::map<std::string, ExternalPBRMaterialSettings> mtlsToCreate;
};

/// @brief Load the fuunction symbol named "m_sgeImportFBXFile" and cast it to sgeImportFBXFileFn to call the function.
///
/// Because of the way FBX SDK is licensed we do not want the end user
/// to be forced to distribute FBX SDK with their final product(game for example).
/// As a solution the sge_engine will dynamically link with mdlconvlib.
/// If the library is missing the sge_engine would not be able to import *.fbx files.
/// Not being able to import FBX files in the final product is usually fine as
/// sge_engine uses our own internal 3D model file file format.
///
/// Imports the specified FBX file into our own internal format.
/// FBX SDK isn't very permissive in it license, as you need a written approval for distributing FBX SDK binaries from Autodesk.
/// We have our own format so we do not need FBX SDK in the working game. However if we want to import FBX files as assets we still need the
/// SDK. By having a DLL that can import FBX files the engine could check if the dll is present and invoke FBX SDK without explicity
/// depending on it at link time. If the SDK is not available we can just say to the engine user that the FBX SDK importer isn't aviable.
/// @param [out] result stores the imported model.
/// @param [in] fbxFilename is the filename to be loaded.
/// @param [out] pOutReferencedTextures A list of referenced textures in the specified filename (used for dependancy tracking).
/// @return true if the import was successful.
typedef bool (*sgeImportFBXFileFn)(sge::Model& result, ModelImportAdditionalResult& additionalResult, const char* fbxFilename);

/// A structure holding a single (ot of many) imported 3D models.
/// The structure is used when importing one 3D Scene file (FBX for example) as multiple model files.
/// For more details see the comment @sgeImportFBXFileAsMultipleFn.
struct MultiModelImportResult {
	/// The imported 3D model.
	sge::Model importedModel;
	/// A propoused name to be used for the *mdl file if the model is going to get saved.
	/// The name is based on the node used to import the model.
	std::string propsedFilename;
};

/// This function provides the option to import a single 3D scene file (FBX for example) as multiple *.mdl files,
/// by importing each node that is parented to the root node in the scene as a separate model.
/// This is useful when the artist needs to create for example tileable 3D models or others that share a lot of commonalitices
/// and it would be easier to work with them if they are in one file.
/// @param [out] result stores a list of all imported models.
/// @param [in] fbxFilename is the filename to be loaded.
/// @param [out] pOutReferencedTextures A list of referenced textures in the specified filename (used for dependancy tracking).
///              This one contains the materials and textures used across all imported models.
/// @return true if the import was successful.
typedef bool (*sgeImportFBXFileAsMultipleFn)(std::vector<MultiModelImportResult>& result,
                                             ModelImportAdditionalResult& additionalResult,
                                             const char* fbxFilename);

} // extern "C"
