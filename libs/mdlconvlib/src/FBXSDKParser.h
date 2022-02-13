#pragma once

#ifdef SGE_FBX_SDK_AVAILABLE

	#include <fbxsdk.h>
	#include <memory>

	#include "ModelParseSettings.h"
	#include "sgeImportModel3DFile.h"
	#include "sge_core/AssetLibrary/AssetMaterial.h"
	#include "sge_core/model/Model.h"
	#include "sge_utils/containers/Optional.h"
	#include "sge_utils/math/transform.h"

namespace sge {

struct IAssetRelocationPolicy;

bool InitializeFBXSDK();

/// @brief FBXSDKParser convers FBX/DAE/OBJ files to our own internal format by using FBX SDK.s
struct FBXSDKParser {
	/// Prases the input FBX scene and produces a ready to save SGE model.
	/// For more details see the comments @sgeImportFBXFileFn @sgeImportFBXFileAsMultipleFn, this is called by them.
	/// @param [in] enforcedRootNode is the node to be used as a root node insted of the actual one, if null, the regular root is going to
	/// be used.
	/// @param [in] materialsPrefix is needed when additional assets should get imported alongside the model.
	///             These are materials for example. Usually passing the fbx file filename is enough.
	bool parse(Model* result,
	           ModelImportAdditionalResult& additionalResult,
	           std::string& materialsPrefix,
	           fbxsdk::FbxScene* scene,
	           FbxNode* enforcedRootNode,
	           const ModelParseSettings& parseSettings);

  private:
	/// Step 1:
	/// Find the nodes that need to be imported their models/materials that
	/// needs to be parsed because they are attached to the node.
	/// in the result model we only allocate the nodes, but we do not actually fill their data (child nodes, attachments and so on).
	int discoverNodesRecursive(fbxsdk::FbxNode* const fbxNode);

	/// Step 2:
	/// Import the materials found in @discoverNodesRecursive.
	void importMaterials();

	/// Step 3:
	/// Import the meshes that are going to be used for rendering (not the collision ones).
	/// @param [in] importSkinningData true if the skinning data (bones and weights) need to be imported as well.
	void importMeshes(const bool importSkinningData);

	/// A helper function for @importMeshes, imports a single mesh.
	/// @param [in] importedMeshIndex the index of the mesh in the imported result.
	void importMeshes_singleMesh(FbxMesh* fbxMesh, int importedMeshIndex, const bool importSkinningData);
	int importMeshes_getDefaultMaterialIndex();

	/// Step 4:
	/// Finally import fully the nodes. In @discoverNodesRecursive we've just allocated them but here
	/// we load their attachments, children and so on.
	void importNodes();

	/// A helper for @importNodes.
	void importNodes_singleNode(FbxNode* fbxNode, int importNodeIndex);

	/// Step 5 (optional):
	/// Imports the animations described in the FBX scene.
	void importAnimations();

	/// Step 6: Import the collision geometry described by meshes attached to nodes with specific names.
	void importCollisionGeometry();

  private:
	fbxsdk::FbxScene* m_fbxScene = nullptr; ///< The FBX scene to be imported.
	ModelParseSettings m_parseSettings;
	ModelImportAdditionalResult* m_additionalResult;
	std::string m_materialsPrefix;

	std::map<FbxSurfaceMaterial*, int> m_fbxMtl2MtlIndex;
	std::map<FbxMesh*, int> m_fbxMesh2MeshIndex;
	std::map<FbxNode*, int> m_fbxNode2NodeIndex;

	// Some DCC (like Blender) might export a geometry with no materials attached to it.
	// In that case we create a material (one per scene) to be used as a default.
	Optional<int> m_defaultMaterialIndex;

	/// Meshes to be used for collision geometry (if any). These do not participate in the m_fbxMesh2MeshIndex
	/// as they aren't going to be used for rendering.
	std::map<FbxMesh*, std::vector<transf3d>> m_collision_ConvexHullMeshes;
	std::map<FbxMesh*, std::vector<transf3d>> m_collision_BvhTriMeshes;
	std::map<FbxMesh*, std::vector<transf3d>> m_collision_BoxMeshes;
	std::map<FbxMesh*, std::vector<transf3d>> m_collision_CaplsuleMeshes;
	std::map<FbxMesh*, std::vector<transf3d>> m_collision_CylinderMeshes;
	std::map<FbxMesh*, std::vector<transf3d>> m_collision_SphereMeshes;

	// When enforcing a root node we need to remove it's transfrom from the collsion objects.
	transf3d m_collision_transfromCorrection = transf3d::getIdentity();

	/// The model that is going to store the imported result.
	Model* m_model = nullptr;
};

} // namespace sge
#endif // SGE_FBX_SDK_AVAILABLE
