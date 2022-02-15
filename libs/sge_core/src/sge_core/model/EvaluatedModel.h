#pragma once

#include <memory>
#include <vector>

#include "sge_core/AssetLibrary/IAsset.h"
#include "sge_core/Geometry.h"
#include "sge_core/model/Model.h"
#include "sge_core/sgecore_api.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/containers/ArrayView.h"
#include "sge_utils/containers/vector_map.h"
#include "sge_utils/math/Box3f.h"
#include "sge_utils/math/mat4f.h"
#include "sge_utils/math/primitives.h"

namespace sge {

struct AssetLibrary;
struct Asset;

struct EvaluatedNode {
	mat4f evalGlobalTransform = mat4f::getIdentity();
	Box3f aabbGlobalSpace; // untransformed contents bounding box.
};

struct EvaluatedMesh {
	Geometry geometry;
};

/// Usually when we render a model, we do not care about the hierarchy
/// we just wanna draw the meshes in it at correct locations.
/// This struct represents a single mesh instance in a model and its location
/// so we could just draw it with any nodes traversal.
struct EvaluatedMeshInstance {
	/// An assembled set of geometry ready for rendering.
	Geometry geometry;

	/// The index of the material in the owning Model.
	int iMaterial = -1;

	/// The transform of the geometry(mesh) with all nodes hierarchy applied.
	mat4f modelSpaceTransform = mat4f::getIdentity();

	Box3f modelSpaceBBox;
};

struct SGE_CORE_API EvaluatedModel {
	EvaluatedModel()
	    : m_model(nullptr)
	{
	}

	void initialize(Model* model);
	bool isInitialized() const { return m_model; }

	/// Evaluates the model with the specified transforms for each node (in model global space, not local).
	/// Useful for ragdolls or inverse kinematics.
	/// @param [in] nodesGlobalTransform an array for each node matched by the
	/// index in the array specifying the location in model space to be used for that node.
	bool evaluate(const mat4f* nodesGlobalTransform, const int nodesGlobalTransformCount);

	/// Evaluates the model with no animation applied.
	/// If you want an animation to play use @ModelAnimator.
	void evaluateStatic();

	int getNumEvalMeshes() const { return int(m_evaluatedMeshes.size()); }

	const EvaluatedMesh& getEvalMesh(const int iMesh) const { return m_evaluatedMeshes[iMesh]; }

	int getNumEvalNodes() const { return int(m_evaluatedNodes.size()); }

	const EvaluatedNode& getEvalNode(const int iNode) const { return m_evaluatedNodes[iNode]; }

	Texture* getSkinningBonesTexture() const { return m_skinningBoneTransfsTex.GetPtr(); }

	int getMeshBonesOffsetInSkinningBonesTexture(const int iMesh) const
	{
		if (iMesh >= 0 && iMesh < m_perMeshSkinningBonesTransformOFfsetInTex.size()) {
			return m_perMeshSkinningBonesTransformOFfsetInTex[iMesh];
		}

		sgeAssert(false && "it is expected that this array was allocated");
		return -1;
	}

	const std::vector<EvaluatedMeshInstance>& getEvalMeshInstances() const { return m_evalAllMeshInstances; }

  private:
	bool evaluate_ApplyNodeGlobalTransforms(const ArrayView<const mat4f>& boneGlobalTrasnformOverrides);
	bool evaluate_Skinning();
	bool evaluate_getAllGeometries();

  public:
	/// The model being animated.
	Model* m_model = nullptr;

	/// Stores the state of each node, the index in this array corresponts to the index of the node in the
	/// @Model::m_nodes array.
	std::vector<EvaluatedNode> m_evaluatedNodes;

	/// @brief Since meshes can be animated only by bone transforms, which only modify their bones.
	/// We can patically evalute them only once.
	std::vector<EvaluatedMesh> m_evaluatedMeshes;

	/// A texture holding the bones for every model.
	/// The texture should be of size 4x<number-of-bones-in-all-meshes> and it must be
	/// of type RGBA float32 with point sampler.
	GpuHandle<Texture> m_skinningBoneTransfsTex;

	/// @m_skinningBoneTransfsTex stores the bones transforms for all meshes.
	/// Each mesh esh bones are placed sequentially in that matrix.
	/// This vector hold the row index of the 1st bone for the specified mesh.
	/// -1 if there are no bones for that mesh.
	std::vector<int> m_perMeshSkinningBonesTransformOFfsetInTex;

	/// A list of all instances and their transforms to be rendered.
	std::vector<EvaluatedMeshInstance> m_evalAllMeshInstances;

	Box3f aabox;

	/// Temporaries used to avoid allocating memory again and again for each evaluation.
	std::vector<mat4f> bonesTransformTexDataForAllMeshes;
};

} // namespace sge
