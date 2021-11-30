#include "IGeometryDrawer.h"
#include "IMaterial.h"
#include "MaterialFamilyList.h"
#include "sge_core/AssetLibrary/AssetMaterial.h"
#include "sge_core/ICore.h"
#include "sge_core/model/EvaluatedModel.h"
#include "sge_core/shaders/LightDesc.h"

namespace sge {

SGE_CORE_API void drawGeometry(const RenderDestination& rdest,
                               const ICamera& camera,
                               const mat4f& geomWorldTransfrom,
                               const ObjectLighting& lighting,
                               const Geometry& geometry,
                               const IMaterialData* mtlDataBase,
                               const InstanceDrawMods& instDrawMods) {
	if_checked(mtlDataBase) {
		const MaterialFamilyLibrary::MaterialFamilyData* family =
		    getCore()->getMaterialLib()->findFamilyById(mtlDataBase->materialFamilyId);

		// TODO: here we need to find the material family and the approriate IGeometryDrawer for that material family.
		// This means that this function should not be here
		if_checked(family && family->geometryDrawer) {
			family->geometryDrawer->drawGeometry(rdest, camera, geomWorldTransfrom, lighting, geometry, mtlDataBase, instDrawMods);
		}
	}
}

SGE_CORE_API void drawEvalModel(const RenderDestination& rdest,
                                const ICamera& camera,
                                const mat4f& geomWorldTransfrom,
                                const ObjectLighting& lighting,
                                const EvaluatedModel& evalModel,
                                const InstanceDrawMods& instDrawMods) {
	for (int iNode = 0; iNode < evalModel.getNumEvalNodes(); ++iNode) {
		const EvaluatedNode& evalNode = evalModel.getEvalNode(iNode);
		const ModelNode* rawNode = evalModel.m_model->nodeAt(iNode);

		for (int iMesh = 0; iMesh < rawNode->meshAttachments.size(); ++iMesh) {
			const MeshAttachment& meshAttachment = rawNode->meshAttachments[iMesh];
			const EvaluatedMesh& evalMesh = evalModel.getEvalMesh(meshAttachment.attachedMeshIndex);
			mat4f const finalTrasform =
			    (evalMesh.geometry.hasVertexSkinning()) ? geomWorldTransfrom : geomWorldTransfrom * evalNode.evalGlobalTransform;

			const std::shared_ptr<AssetIface_Material>& mtlIface = evalModel.getEvalMaterial(meshAttachment.attachedMaterialIndex);
			IMaterialData* mdlData = mtlIface && mtlIface->getMaterial() ? mtlIface->getMaterial()->getMaterialDataLocalStorage() : nullptr;

			if_checked(mdlData) {
				const MaterialFamilyLibrary::MaterialFamilyData* family =
				    getCore()->getMaterialLib()->findFamilyById(mdlData->materialFamilyId);

				// TODO: here we need to find the material family and the approriate IGeometryDrawer for that material family.
				// This means that this function should not be here
				if_checked(family && family->geometryDrawer) {
					family->geometryDrawer->drawGeometry(rdest, camera, finalTrasform, lighting, evalMesh.geometry, mdlData, instDrawMods);
				}
			}
		}
	}
}
} // namespace sge
