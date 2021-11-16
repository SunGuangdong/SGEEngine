#include "IGeometryDrawer.h"
#include "sge_core/model/EvaluatedModel.h"

namespace sge {


void IGeometryDrawer::drawEvalModel(const RenderDestination& rdest,
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

			const std::shared_ptr<IMaterial>& mtl = evalModel.getEvalMaterial(meshAttachment.attachedMaterialIndex);
			IMaterialData* mdlData = mtl->getMaterialDataLocalStorage();

			// TODO: here we need to find the material family and the approriate IGeometryDrawer for that material family.
			// This means that this function should not be here

			drawGeometry(rdest, camera, finalTrasform, lighting, evalMesh.geometry, mdlData, instDrawMods);
		}
	}
}
} // namespace sge
