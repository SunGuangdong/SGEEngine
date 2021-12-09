#pragma once

#include "sge_engine/GameDrawer/IRenderItem.h"

namespace sge {

struct TraitModel;
struct EvaluatedModel;
struct IMaterialData;

struct TraitModelRenderItem : public IRenderItem {
	TraitModel* traitModel = nullptr;
	int iModel = -1;
	const EvaluatedModel* evalModel = nullptr;
	int iEvalNode = -1; // The mesh to be rendered from the model.
	int iEvalNodeMechAttachmentIndex = -1;

	IMaterialData* pMtlData = nullptr;
};

struct TraitModelRenderItem2 : public IRenderItem {
	Geometry geometry;
	std::vector<mat4f> bonesTransforms;
	IMaterialData* pMtlData = nullptr;

	/// Transform the mesh to world space.
	/// Includes the actorToWorld transform 
	/// and the additional transofrm of the TraitModel.
	mat4f mesh2world;
};

} // namespace sge
