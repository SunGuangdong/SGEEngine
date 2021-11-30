#pragma once

#include "sge_core/model/Model.h"
#include <set>
#include <string>
#include <vector>
#include "sge_utils/math/vec4.h"

extern "C" {

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

struct ModelImportAdditionalResult {
	std::set<std::string> textureToCopy;
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

struct MultiModelImportResult {
	sge::Model importedModel;
	std::string propsedFilename;
	std::set<std::string> referencedTextures;
};

typedef bool (*sgeImportFBXFileAsMultipleFn)(std::vector<MultiModelImportResult>& result,
                                             ModelImportAdditionalResult& additionalResult,
                                             const char* fbxFilename);

} // extern "C"
