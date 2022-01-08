#include "sgeImportModel3DFile.h"
#include "AssimpImporter.h"
#include "FBXSDKParser.h"
#include "IAssetRelocationPolicy.h"
#include "ModelParseSettings.h"
#include "sge_utils/utils/Path.h"
#include "sge_utils/utils/strings.h"
#include <type_traits>

using namespace sge;

#if WIN32
#ifdef SGE_MDLCONVLIB_BUILDING_DLL
#define SGE_MDLCONVLIB_API __declspec(dllexport)
#else
#define SGE_MDLCONVLIB_API __declspec(dllimport)
#endif
#else
#ifdef SGE_MDLCONVLIB_BUILDING_DLL
#define SGE_MDLCONVLIB_API __attribute__((visibility("default")))
#else
#define SGE_MDLCONVLIB_API
#endif
#endif

//-----------------------------------------------------------------------
// Assimp Import
//-----------------------------------------------------------------------
bool withAssimp_sgeImportModel3DFile(sge::Model& result, ModelImportAdditionalResult& additionalResult, const char* fbxFilename) {
	Assimp::Importer importer;

	// And have it read the given file with some example postprocessing
	// Usually - if speed is not the most important aspect for you - you'll
	// propably to request more postprocessing than we do in this example.

	unsigned flags = aiProcess_PopulateArmatureData | aiProcess_CalcTangentSpace | aiProcess_Triangulate |
	                 aiProcess_SortByPType; // aiProcess_JoinIdenticalVertices

	const aiScene* scene = importer.ReadFile(fbxFilename, flags);

	if (!scene) {
		return false;
	}

	std::string materialsAssetsNamePrefix = extractFileNameNoExt(fbxFilename);

	AssimpImporter ourImporter;

	ModelParseSettings mps;
	NoneAssetRelocationPolicy relocationPolicy = NoneAssetRelocationPolicy();
	mps.pRelocaionPolicy = &relocationPolicy;

	bool success = ourImporter.parse(&result, additionalResult, materialsAssetsNamePrefix, scene, nullptr, mps);
	return success;
}
//-----------------------------------------------------------------------
// FBX SDK Import
//-----------------------------------------------------------------------
bool withFbxSdk_sgeImportModel3DFile(sge::Model& result, ModelImportAdditionalResult& additionalResult, const char* fbxFilename) {
	static fbxsdk::FbxManager* const fbxMngr = fbxsdk::FbxManager::Create();
	FbxImporter* const fbxImporter = FbxImporter::Create(fbxMngr, ""); // todo: do we need a name here?
	const bool fbxImporterInitialized = fbxImporter->Initialize(fbxFilename, -1, fbxMngr->GetIOSettings());

	if (!fbxImporterInitialized) {
		return false;
	}

	FbxScene* const fbxScene = FbxScene::Create(fbxMngr, "fbxsdkImportedScene");
	if (!fbxImporter->Import(fbxScene)) {
		return false;
	}

	ModelParseSettings mps;
	NoneAssetRelocationPolicy relocationPolicy = NoneAssetRelocationPolicy();
	mps.pRelocaionPolicy = &relocationPolicy;

	std::string materialsAssetsNamePrefix = extractFileNameNoExt(fbxFilename);

	FBXSDKParser fbxsdkParser;
	if (!fbxsdkParser.parse(&result, additionalResult, materialsAssetsNamePrefix, fbxScene, nullptr, mps)) {
		return false;
	}

	return true;
}

bool withFbxSdk_sgeImportModel3DFileAsMultiple(std::vector<MultiModelImportResult>& result,
                                               ModelImportAdditionalResult& additionalResult,
                                               const char* fbxFilename) {
	static fbxsdk::FbxManager* const fbxMngr = fbxsdk::FbxManager::Create();
	FbxImporter* const fbxImporter = FbxImporter::Create(fbxMngr, ""); // todo: do we need a name here?
	const bool fbxImporterInitialized = fbxImporter->Initialize(fbxFilename, -1, fbxMngr->GetIOSettings());

	if (!fbxImporterInitialized) {
		return false;
	}

	FbxScene* const fbxScene = FbxScene::Create(fbxMngr, "fbxsdkImportedScene");
	if (!fbxImporter->Import(fbxScene)) {
		return false;
	}

	ModelParseSettings mps;
	NoneAssetRelocationPolicy relocationPolicy = NoneAssetRelocationPolicy();
	mps.pRelocaionPolicy = &relocationPolicy;

	// Find all nodes that are immediate children of the root and import them sepretly.
	FbxNode* const fbxRoot = fbxScene->GetRootNode();

	result.clear();
	result.resize(fbxRoot->GetChildCount());

	std::string materialsAssetsNamePrefix = extractFileNameNoExt(fbxFilename);

	for (int t = 0; t < fbxRoot->GetChildCount(); ++t) {
		FbxNode* const fbxChild = fbxRoot->GetChild(t);

		// Append the name of the node in the result.
		const std::string newExt = string_format("%s.mdl", fbxChild->GetName());
		const std::string newOutName = extractFileNameWithExt(replaceExtension(fbxFilename, newExt.c_str()).c_str());

		FBXSDKParser fbxsdkParser;

		if (fbxsdkParser.parse(&result[t].importedModel, additionalResult, materialsAssetsNamePrefix, fbxScene, fbxChild, mps)) {
			result[t].propsedFilename = newOutName;
		} else {
			printf("Something went wrong when converting %s!", fbxFilename);
		}
	}

	return true;
}


extern "C" {

SGE_MDLCONVLIB_API bool sgeImportModel3DFile(sge::Model& result, ModelImportAdditionalResult& additionalResult, const char* fbxFilename) {
	// Ensure that the typedef we provide in the header matches the actual function type.
	static_assert(std::is_same<decltype(&sgeImportModel3DFile), sgeImportModel3DFileFn>::value);

	return withAssimp_sgeImportModel3DFile(result, additionalResult, fbxFilename);
	// return withFbxSdk_sgeImportModel3DFile(result, additionalResult, fbxFilename);
}

SGE_MDLCONVLIB_API bool sgeImportModel3DFileAsMultiple(std::vector<MultiModelImportResult>& result,
                                                       ModelImportAdditionalResult& additionalResult,
                                                       const char* fbxFilename) {
	// Ensure that the typedef we provide in the header matches the actual function type.
	static_assert(std::is_same<decltype(&sgeImportModel3DFileAsMultiple), sgeImportModel3DFileAsMultipleFn>::value);

	return withFbxSdk_sgeImportModel3DFileAsMultiple(result, additionalResult, fbxFilename);
}

} // extern "C"
