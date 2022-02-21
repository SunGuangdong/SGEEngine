#include "RigidBodyFromModel.h"
#include "sge_core/AssetLibrary/AssetLibrary.h"
#include "sge_core/ICore.h"
#include "sge_core/model/EvaluatedModel.h"
#include "sge_engine/physics/CollisionShape.h"
#include "sge_engine/traits/TraitModel.h"


namespace sge {

bool addCollisionShapeBasedOnModel(std::vector<CollsionShapeDesc>& shapeDescs, const EvaluatedModel& evaluatedMode)
{
	const Model* const model = evaluatedMode.m_model;
	if (model == nullptr) {
		return false;
	}

	// Attempt to use the convex hulls first.
	if (model->m_convexHulls.size() > 0) {
		for (const ModelCollisionMesh& cvxHull : model->m_convexHulls) {
			shapeDescs.emplace_back(CollsionShapeDesc::createConvexPoly(cvxHull.vertices, cvxHull.indices));
		}
	}
	// Then the concave.
	else if (model->m_concaveHulls.size() > 0) {
		for (const ModelCollisionMesh& cvxHull : model->m_concaveHulls) {
			shapeDescs.emplace_back(CollsionShapeDesc::createTriMesh(cvxHull.vertices, cvxHull.indices));
		}
	}
	// Then the collision shapes (boxes, capsules, cylinders ect.)
	else {
		for (const Model_CollisionShapeBox& box : model->m_collisionBoxes) {
			shapeDescs.emplace_back(CollsionShapeDesc::createBox(box.halfDiagonal, box.transform));
		}

		for (const Model_CollisionShapeCapsule& capsule : model->m_collisionCapsules) {
			shapeDescs.emplace_back(
			    CollsionShapeDesc::createCapsule(capsule.radius, capsule.halfHeight * 2.f, capsule.transform));
		}

		for (const Model_CollisionShapeCylinder& cylinder : model->m_collisionCylinders) {
			shapeDescs.emplace_back(CollsionShapeDesc::createCylinder(cylinder.halfDiagonal, cylinder.transform));
		}

		for (const Model_CollisionShapeSphere& sphere : model->m_collisionSpheres) {
			shapeDescs.emplace_back(CollsionShapeDesc::createSphere(sphere.radius, sphere.transform));
		}

		if (shapeDescs.empty()) {
			// Fallback to the bounding box of the whole 3D model.
			Box3f modelBBox = evaluatedMode.aabox;

			// For example if we have a single plane for obsticle,
			// the bounding box by some axis could be 0, in order
			// not to break the physics add some size in that axis.
			vec3f boxSize = modelBBox.size();
			if (boxSize.x < 1e-2f)
				modelBBox.max.x += 0.01f, modelBBox.min.x -= 1e-2f;
			if (boxSize.y < 1e-2f)
				modelBBox.max.y += 0.01f, modelBBox.min.y -= 1e-2f;
			if (boxSize.z < 1e-2f)
				modelBBox.max.z += 0.01f, modelBBox.min.z -= 1e-2f;

			if (modelBBox.isEmpty() == false) {
				shapeDescs.emplace_back(CollsionShapeDesc::createBox(modelBBox));
			}
		}
	}

	return true;
}

bool addCollisionShapeBasedOnModelRenderGeom(
    std::vector<CollsionShapeDesc>& shapeDescs, const EvaluatedModel& evaluatedModel)
{
	std::vector<vec3f> triangleList;
	std::vector<int> triangleIndices;

	int indicesOffset = 0;

	const int numNodes = evaluatedModel.getNumEvalNodes();
	for (int iNode = 0; iNode < numNodes; ++iNode) {
		const EvaluatedNode& evalNode = evaluatedModel.getEvalNode(iNode);
		ModelNode* modelNode = evaluatedModel.m_model->nodeAt(iNode);

		for (MeshAttachment& meshAttachment : modelNode->meshAttachments) {
			ModelMesh* modelMesh = evaluatedModel.m_model->meshAt(meshAttachment.attachedMeshIndex);

			const char* vertexBuffer = modelMesh->vertexBufferRaw.data();
			const char* indexBuffer = modelMesh->indexBufferRaw.data();

			const int numTriangles = modelMesh->numElements / 3;
			const int vertexStide = modelMesh->stride;
			const int posByteOffset = modelMesh->vbPositionOffsetBytes;

			triangleList.reserve(triangleList.size() + numTriangles);
			triangleIndices.reserve(triangleIndices.size() + numTriangles);

			int numVertsAddedFromCurrMesh = 0;
			if (modelMesh->primitiveTopology == PrimitiveTopology::TriangleList &&
			    modelMesh->ibFmt == UniformType::Uint) {
				for (int iTri = 0; iTri < numTriangles; ++iTri) {
					unsigned vertIdx[3];
					if (indexBuffer) {
						vertIdx[0] = ((unsigned*)(indexBuffer))[iTri * 3 + 0];
						vertIdx[1] = ((unsigned*)(indexBuffer))[iTri * 3 + 1];
						vertIdx[2] = ((unsigned*)(indexBuffer))[iTri * 3 + 2];
					}
					else {
						vertIdx[0] = iTri * 3 + 0;
						vertIdx[1] = iTri * 3 + 1;
						vertIdx[2] = iTri * 3 + 2;
					}

					// Obtain the points.
					vec3f v0 = *(vec3f*)(vertexBuffer + vertIdx[0] * vertexStide + posByteOffset);
					vec3f v1 = *(vec3f*)(vertexBuffer + vertIdx[1] * vertexStide + posByteOffset);
					vec3f v2 = *(vec3f*)(vertexBuffer + vertIdx[2] * vertexStide + posByteOffset);

					// Transform the triangle verts to object space of the mode.
					v0 = mat_mul_pos(evalNode.evalGlobalTransform, v0);
					v1 = mat_mul_pos(evalNode.evalGlobalTransform, v1);
					v2 = mat_mul_pos(evalNode.evalGlobalTransform, v2);

					// Add the result.
					triangleList.push_back(v0);
					triangleList.push_back(v1);
					triangleList.push_back(v2);

					numVertsAddedFromCurrMesh += 3;

					// TODO: Add a proper index buffer here.
					triangleIndices.push_back(iTri * 3 + 0 + indicesOffset);
					triangleIndices.push_back(iTri * 3 + 1 + indicesOffset);
					triangleIndices.push_back(iTri * 3 + 2 + indicesOffset);
				}
			}

			indicesOffset += numVertsAddedFromCurrMesh;
		}
	}

	sgeAssert(triangleList.size() % 3 == 0 && "We expect a triangle list here");
	sgeAssert(triangleIndices.size() % 3 == 0 && "We expect a triangle list here");
	if (triangleList.empty() == false) {
		CollsionShapeDesc shapeDesc =
		    CollsionShapeDesc::createTriMesh(std::move(triangleList), std::move(triangleIndices));
		shapeDescs.emplace_back(std::move(shapeDesc));
		return true;
	}

	return false;
}

bool addCollisionShapeBasedOnModel(std::vector<CollsionShapeDesc>& shapeDescs, const char* modelAssetPath)
{
	AssetPtr modelAsset = getCore()->getAssetLib()->getAssetFromFile(modelAssetPath);
	if (!isAssetLoaded(modelAsset, assetIface_model3d)) {
		return false;
	}

	return addCollisionShapeBasedOnModel(
	    shapeDescs, getLoadedAssetIface<AssetIface_Model3D>(modelAsset)->getStaticEval());
}

bool addCollisionShapeBasedOnTraitModel(std::vector<CollsionShapeDesc>& shapeDescs, TraitModel& traitModel)
{
	bool hadShapes = false;
	for (ModelEntry& mdlSets : traitModel.m_models) {
		if (AssetIface_Model3D* mdlIface = mdlSets.m_assetProperty.getAssetInterface<AssetIface_Model3D>()) {
			// TODO: Each model entry has an additional trasform in it.
			// However since it is mat4f and the physics engine works only with affine transforms
			// we cannot embed it here.
			hadShapes |= addCollisionShapeBasedOnModel(shapeDescs, mdlIface->getStaticEval());
		}
	}

	return hadShapes;
}

} // namespace sge
