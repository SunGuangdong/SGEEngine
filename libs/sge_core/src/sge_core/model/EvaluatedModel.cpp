#include <functional>

#include "sge_core/AssetLibrary/AssetLibrary.h"
#include "sge_core/ICore.h"
#include "sge_utils/math/transform.h"
#include "sge_utils/utils/range_loop.h"

#include "sge_core/materials/DefaultPBRMtl/DefaultPBRMtl.h"

#include "EvaluatedModel.h"
#include "Model.h"

namespace sge {

void EvaluatedModel::initialize(Model* model) {
	*this = EvaluatedModel();

	sgeAssert(model != nullptr);
	m_model = model;
	if (m_model) {
		evaluateStatic();
	}
}

void EvaluatedModel::evaluateStatic() {
	std::vector<mat4f> globalMatrices(m_model->numNodes());
	std::function<void(int, const mat4f&)> evalGlobalTransform = [&](int iNode, const mat4f& parentGlobalTform) -> void {
		ModelNode* rawNode = m_model->nodeAt(iNode);
		mat4f ownGlobalTransform = parentGlobalTform * rawNode->staticLocalTransform.toMatrix();

		globalMatrices[iNode] = ownGlobalTransform;

		for (int childIndex : rawNode->childNodes) {
			evalGlobalTransform(childIndex, ownGlobalTransform);
		}
	};

	const int iRoot = m_model->getRootNodeIndex();
	evalGlobalTransform(iRoot, mat4f::getIdentity());

	evaluate(globalMatrices.data(), int(globalMatrices.size()));
}


bool EvaluatedModel::evaluate(const mat4f* nodesGlobalTransform, const int nodesGlobalTransformCount) {
	span<const mat4f> nodesTrasfSpan = span<const mat4f>(nodesGlobalTransform, nodesGlobalTransformCount);
	evaluate_ApplyNodeGlobalTransforms(nodesTrasfSpan);
	evaluate_Skinning();
	return true;
}

bool EvaluatedModel::evaluate_ApplyNodeGlobalTransforms(const span<const mat4f>& boneGlobalTrasnformOverrides) {
	aabox.setEmpty();
	m_evaluatedNodes.resize(m_model->numNodes());

	if (boneGlobalTrasnformOverrides.size() != m_model->numNodes()) {
		sgeAssert(false && "It seems that the provided amount of node transforms isn't the one that we expect");
		return false;
	}

	for (int iNode = 0; iNode < m_model->numNodes(); ++iNode) {
		EvaluatedNode& evalNode = m_evaluatedNodes[iNode];
		evalNode.evalGlobalTransform = boneGlobalTrasnformOverrides[iNode];
	}

	return true;
}

bool EvaluatedModel::evaluate_Skinning() {
	m_evaluatedMeshes.resize(m_model->numMeshes());
	m_perMeshSkinningBonesTransformOFfsetInTex.resize(m_model->numMeshes(), -1);

	// Clear the preovious state.
	bonesTransformTexDataForAllMeshes.clear();

	// Evaluate the meshes.
	for (int iMesh = 0; iMesh < m_model->numMeshes(); ++iMesh) {
		const ModelMesh& rawMesh = *m_model->meshAt(iMesh);

		m_perMeshSkinningBonesTransformOFfsetInTex[iMesh] = -1;

		if (rawMesh.bones.empty() == false) {
			m_perMeshSkinningBonesTransformOFfsetInTex[iMesh] = int(bonesTransformTexDataForAllMeshes.size());

			// Compute the tansform of the bone, it combines the binding offset matrix of the bone and
			// the evaluated position of the node that represents the bone in the scene.
			std::vector<mat4f> bonesTransformTexData(rawMesh.bones.size());
			for (int iBone = 0; iBone < rawMesh.bones.size(); ++iBone) {
				const ModelMeshBone& bone = rawMesh.bones[iBone];

				const mat4f boneTransformWithOffsetModelObjectSpace =
				    m_evaluatedNodes[bone.nodeIdx].evalGlobalTransform * bone.offsetMatrix;

				bonesTransformTexDataForAllMeshes.push_back(boneTransformWithOffsetModelObjectSpace);
			}
		}
	}

	// Compute the bones skinning matrix texture for the whole model.
	if (bonesTransformTexDataForAllMeshes.empty() == false) {
		int neededTexWidth = 4;
		int neededTexHeight = int(bonesTransformTexDataForAllMeshes.size());

		TextureData data =
		    TextureData(bonesTransformTexDataForAllMeshes.data(), sizeof(vec4f) * 4); // 4 pixel of 4 colors represent one matrix.

		const bool doesBigEnoughTextureExists =
		    m_skinningBoneTransfsTex.HasResource() && m_skinningBoneTransfsTex->getDesc().texture2D.height >= neededTexHeight;

		SGEContext* const context = getCore()->getDevice()->getContext();
		if (doesBigEnoughTextureExists == false) {
			TextureDesc td;
			td.textureType = UniformType::Texture2D;
			td.usage = TextureUsage::DynamicResource;
			td.format = TextureFormat::R32G32B32A32_FLOAT;
			td.texture2D.arraySize = 1;
			td.texture2D = Texture2DDesc(neededTexWidth, neededTexHeight);

			SamplerDesc sd;
			sd.filter = TextureFilter::Min_Mag_Mip_Point;

			m_skinningBoneTransfsTex = context->getDevice()->requestResource<Texture>();
			m_skinningBoneTransfsTex->create(td, &data, sd);
		} else {
			context->updateTextureData(m_skinningBoneTransfsTex.GetPtr(), data);
		}

		// Assert that if there are bones, then the bone texture must be created.
		sgeAssert(m_skinningBoneTransfsTex.HasResource() == !bonesTransformTexDataForAllMeshes.empty());
	}

	// Finally fill the geometry structure for every mesh that is going to be used for draw calls.
	for (int iMesh = 0; iMesh < m_model->numMeshes(); ++iMesh) {
		EvaluatedMesh& evalMesh = m_evaluatedMeshes[iMesh];
		const ModelMesh& rawMesh = *m_model->meshAt(iMesh);

		const int firstBoneOffset = m_perMeshSkinningBonesTransformOFfsetInTex[iMesh];

		evalMesh.geometry =
		    Geometry(rawMesh.vertexBuffer.GetPtr(), rawMesh.indexBuffer.GetPtr(), m_skinningBoneTransfsTex.GetPtr(), firstBoneOffset,
		             rawMesh.vertexDeclIndex, rawMesh.vbVertexColorOffsetBytes >= 0, rawMesh.vbUVOffsetBytes >= 0,
		             rawMesh.vbNormalOffsetBytes >= 0, rawMesh.hasUsableTangetSpace, rawMesh.primitiveTopology, rawMesh.vbByteOffset,
		             rawMesh.ibByteOffset, rawMesh.stride, rawMesh.ibFmt, rawMesh.numElements);
	}


	aabox.setEmpty();
	for (int iNode = 0; iNode < this->getNumEvalNodes(); ++iNode) {
		ModelNode* node = m_model->nodeAt(iNode);
		m_evaluatedNodes[iNode].aabbGlobalSpace.setEmpty();
		for (const MeshAttachment& att : node->meshAttachments) {
			ModelMesh* mesh = m_model->meshAt(att.attachedMeshIndex);
			m_evaluatedNodes[iNode].aabbGlobalSpace.expand(mesh->aabox.getTransformed(m_evaluatedNodes[iNode].evalGlobalTransform));
			aabox.expand(m_evaluatedNodes[iNode].aabbGlobalSpace);
		}
	}

	return true;
}


} // namespace sge
