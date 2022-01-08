#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#include "ModelParseSettings.h"
#include "sgeImportModel3DFile.h"
#include "sge_core/AssetLibrary/AssetMaterial.h"
#include "sge_core/model/Model.h"
#include "sge_utils/math/transform.h"
#include "sge_utils/utils/optional.h"

namespace sge {

struct AssimpImporter {
	/// Prases the input FBX scene and produces a ready to save SGE model.
	/// For more details see the comments @sgeImportFBXFileFn @sgeImportFBXFileAsMultipleFn, this is called by them.
	/// @param [in] enforcedRootNode is the node to be used as a root node insted of the actual one, if null, the regular root is going
	/// to be used.
	/// @param [in] materialsPrefix is needed when additional assets should get imported alongside the model.
	///             These are materials for example. Usually passing the fbx file filename is enough.
	bool parse(Model* result,
	           ModelImportAdditionalResult& additionalResult,
	           std::string& materialsPrefix,
	           const aiScene* scene,
	           aiNode* enforcedRootNode,
	           const ModelParseSettings& parseSettings);

  private:
	/// Step 1:
	/// Find the nodes that need to be imported their models/materials that
	/// needs to be parsed because they are attached to the node.
	/// in the result model we only allocate the nodes, but we do not actually fill their data (child nodes, attachments and so on).
	int discoverNodesRecursive(const aiNode* const asmpNode);

	/// Step 2:
	/// Import the materials found in @discoverNodesRecursive.
	void importMaterials();

	/// Step 3:
	/// Import the meshes that are going to be used for rendering (not the collision ones).
	/// @param [in] importSkinningData true if the skinning data (bones and weights) need to be imported as well.
	void importMeshes(const bool importSkinningData);

	/// A helper function for @importMeshes, imports a single mesh.
	/// @param [in] importedMeshIndex the index of the mesh in the imported result.
	void importMeshes_singleMesh(unsigned asimpMeshIndex, int importedMeshIndex, const bool importSkinningData);

	/// Step 4:
	/// Finally import fully the nodes. In @discoverNodesRecursive we've just allocated them but here
	/// we load their attachments, children and so on.
	void importNodes();

	/// A helper for @importNodes.
	void importNodes_singleNode(const aiNode* fbxNode, int importNodeIndex);

	/// Step 5 (optional):
	/// Imports the animations described in the FBX scene.
	void importAnimations();

  private:
	const aiScene* asmpScene = nullptr;

	ModelParseSettings m_parseSettings;
	ModelImportAdditionalResult* m_additionalResult;
	std::string m_materialsPrefix;

	std::map<unsigned, int> m_asmpMtl2MtlIndex;
	std::map<unsigned, int> m_asmpMesh2MeshIndex;
	std::map<const aiNode*, int> m_asmpNode2NodeIndex;

	/// The model that is going to store the imported result.
	Model* m_model = nullptr;
};


} // namespace sge
