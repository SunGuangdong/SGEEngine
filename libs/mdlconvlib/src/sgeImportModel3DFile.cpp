#include "sgeImportModel3DFile.h"
#include "AssimpImporter.h"
#include "FBXSDKParser.h"
#include "IAssetRelocationPolicy.h"
#include "ModelParseSettings.h"
#include "sge_utils/utils/Path.h"
#include "sge_utils/utils/strings.h"
#include <type_traits>

// The macros to expose the symbols (functions) that will perform the import.
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

using namespace sge;

//-----------------------------------------------------------------------
// Assimp Import
//-----------------------------------------------------------------------
enum : unsigned {
	// clang-format off
	sgeAssimpImportFlags =
	  aiProcess_PopulateArmatureData
	| aiProcess_CalcTangentSpace
	| aiProcess_Triangulate
	| aiProcess_SortByPType
	| aiProcess_FlipUVs // Assimp prefers OpenGL stile UVs, but in SGE we use D3D style.
	// | aiProcess_JoinIdenticalVertices
	// clang-format on
};

bool withAssimp_sgeImportModel3DFile(sge::Model& result, ModelImportAdditionalResult& additionalResult, const char* fbxFilename) {
	Assimp::Importer importer;
	const aiScene* asmpScene = importer.ReadFile(fbxFilename, sgeAssimpImportFlags);
	if (!asmpScene) {
		return false;
	}


	ModelParseSettings mps;
	NoneAssetRelocationPolicy relocationPolicy = NoneAssetRelocationPolicy();
	mps.pRelocaionPolicy = &relocationPolicy;
	std::string materialsAssetsNamePrefix = extractFileNameNoExt(fbxFilename);

	AssimpImporter ourAssimpImporter;
	bool success = ourAssimpImporter.parse(&result, additionalResult, materialsAssetsNamePrefix, asmpScene, nullptr, mps);
	return success;
}

bool withAssimp_sgeImportModel3DFileAsMultiple(std::vector<MultiModelImportResult>& result,
                                               ModelImportAdditionalResult& additionalResult,
                                               const char* fbxFilename) {
	Assimp::Importer importer;
	const aiScene* asmpScene = importer.ReadFile(fbxFilename, sgeAssimpImportFlags);
	if (!asmpScene) {
		return false;
	}

	ModelParseSettings mps;
	NoneAssetRelocationPolicy relocationPolicy = NoneAssetRelocationPolicy();
	mps.pRelocaionPolicy = &relocationPolicy;
	std::string materialsAssetsNamePrefix = extractFileNameNoExt(fbxFilename);

	// Allocate the result model count.
	result.clear();
	result.resize(size_t(asmpScene->mRootNode->mNumChildren));

	// Find each node that is a child of the root node and import it as a separate model.
	for (int iChild = 0; iChild < int(asmpScene->mRootNode->mNumChildren); ++iChild) {
		const aiNode* asmpChildNode = asmpScene->mRootNode->mChildren[iChild];

		AssimpImporter ourAssimpImporter;
		bool success =
		    ourAssimpImporter.parse(&result[iChild].importedModel, additionalResult, materialsAssetsNamePrefix, asmpScene, nullptr, mps);

		if (success) {
			// Append the name of the node in the result.
			const std::string newExt = string_format("%s.mdl", asmpChildNode->mName.C_Str());
			const std::string newOutName = extractFileNameWithExt(replaceExtension(fbxFilename, newExt.c_str()).c_str());

			result[iChild].propsedFilename = newOutName;
		} else {
			printf("Something went wrong when converting %s with Assimp!", fbxFilename);
		}
	}

	return true;
}

//-----------------------------------------------------------------------
// FBX SDK Import
//-----------------------------------------------------------------------
#ifdef SGE_FBX_SDK_AVAILABLE
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
#endif // SGE_FBX_SDK_AVAILABLE

//-----------------------------------------------------------------------
// The DLL exported public functions for exporting.
//-----------------------------------------------------------------------
extern "C" {
SGE_MDLCONVLIB_API bool sgeImportModel3DFile(sge::Model& result, ModelImportAdditionalResult& additionalResult, const char* fbxFilename) {
	// Ensure that the typedef we provide in the header matches the actual function type.
	static_assert(std::is_same<decltype(&sgeImportModel3DFile), sgeImportModel3DFileFn>::value);

	// If FBX SDK is available and the files are fbx or Collada (dae) use FBX SDK.
#ifdef SGE_FBX_SDK_AVAILABLE
	std::string fileExt = extractFileExtension(fbxFilename);
	if (fileExt == "fbx" || fileExt == "dae") {
		return withFbxSdk_sgeImportModel3DFile(result, additionalResult, fbxFilename);
	}
#endif

	// If we are here FBX SDK is not available or not used use Assimp.
	return withAssimp_sgeImportModel3DFile(result, additionalResult, fbxFilename);
}

SGE_MDLCONVLIB_API bool sgeImportModel3DFileAsMultiple(std::vector<MultiModelImportResult>& result,
                                                       ModelImportAdditionalResult& additionalResult,
                                                       const char* fbxFilename) {
	// Ensure that the typedef we provide in the header matches the actual function type.
	static_assert(std::is_same<decltype(&sgeImportModel3DFileAsMultiple), sgeImportModel3DFileAsMultipleFn>::value);

	// If FBX SDK is available and the files are fbx or Collada (dae) use FBX SDK.
#ifdef SGE_FBX_SDK_AVAILABLE
	std::string fileExt = extractFileExtension(fbxFilename);
	if (fileExt == "fbx" || fileExt == "dae") {
		return withFbxSdk_sgeImportModel3DFileAsMultiple(result, additionalResult, fbxFilename);
	}
#endif

	return withAssimp_sgeImportModel3DFileAsMultiple(result, additionalResult, fbxFilename);
}

} // extern "C"
