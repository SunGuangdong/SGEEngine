#include "TraitModel.h"
#include "IconsForkAwesome/IconsForkAwesome.h"
#include "sge_core/AssetLibrary/AssetMaterial.h"
#include "sge_core/SGEImGui.h"
#include "sge_core/materials/DefaultPBRMtl/DefaultPBRMtl.h"
#include "sge_core/typelib/MemberChain.h"
#include "sge_core/typelib/typeLib.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_engine/GameDrawer/RenderItems/GeometryRenderItem.h"
#include "sge_engine/GameInspector.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/InspectorCmds.h"
#include "sge_engine/actors/ALocator.h"
#include "sge_engine_ui/ui/UIAssetPicker.h"
#include "sge_engine_ui/windows/PropertyEditorWindow.h"
#include "sge_utils/containers/vector_set.h"
#include "sge_utils/containers/Range.h"
#include "sge_utils/stl_algorithm_ex.h"
#include "sge_utils/text/format.h"

namespace sge {

// clang-format off
ReflAddTypeId(TraitModel, 20'03'01'0004);
ReflAddTypeId(ModelEntry, 21'07'11'0002);
ReflAddTypeId(std::vector<ModelEntry>, 21'07'11'0003);
ReflBlock() {
	ReflAddType(ModelEntry)
		ReflMember(ModelEntry, isRenderable)
		ReflMember(ModelEntry, m_assetProperty)
		ReflMember(ModelEntry, mtlOverrides)
	;
	ReflAddType(std::vector<ModelEntry>);

	ReflAddType(TraitModel)
		ReflMember(TraitModel, isRenderable)
		ReflMember(TraitModel, m_models)
	;
}
// clang-format on

//-----------------------------------------------------------------
// TraitModel::ModelEntry
//-----------------------------------------------------------------
void ModelEntry::setModel(const char* assetPath, bool setupCustomEvalState)
{
	AssetPtr asset = getCore()->getAssetLib()->getAssetFromFile(assetPath);
	setModel(asset, setupCustomEvalState);
}

void ModelEntry::setModel(AssetPtr& asset, bool setupCustomEvalState)
{
	customEvalModel = NullOptional();
	m_assetProperty.setAsset(asset);

	if (setupCustomEvalState) {
		AssetIface_Model3D* mdlIface = getLoadedAssetIface<AssetIface_Model3D>(asset);
		if (mdlIface) {
			customEvalModel = EvaluatedModel();
			customEvalModel->initialize(&mdlIface->getModel3D());
		}
	}

	changeIndex.markAChange();
}

void ModelEntry::onAssetModelChanged()
{
	customEvalModel = NullOptional();

	if (AssetIface_Model3D* modelIface = getLoadedAssetIface<AssetIface_Model3D>(m_assetProperty.getAsset())) {
		mtlOverrides.resize(modelIface->getModel3D().numMaterials());
	}
}

AABox3f ModelEntry::getBBoxOS(const mat4f& invWorldTrasnform) const
{
	AABox3f bbox;

	// If the attached asset is a model use it to compute the bounding box.
	if (customEvalModel) {
		bbox = customEvalModel->aabox.getTransformed(m_additionalTransform);
	}
	else {
		const AssetIface_Model3D* const modelIface = m_assetProperty.getAssetInterface<AssetIface_Model3D>();
		if (modelIface) {
			bbox = modelIface->getStaticEval().aabox.getTransformed(m_additionalTransform);
		}
	}

	// TODO: take in to concideration: ignoreActorTransform
	if (ignoreActorTransform && !bbox.IsEmpty()) {
		return bbox.getTransformed(invWorldTrasnform);
	}

	return bbox;
}

//-----------------------------------------------------------------
// TraitModel
//-----------------------------------------------------------------
void TraitModel::addModel(const char* assetPath, bool setupCustomEvalState)
{
	m_models.push_back(ModelEntry());
	m_models.back().setModel(assetPath, setupCustomEvalState);
}

void TraitModel::addModel(AssetPtr& asset, bool setupCustomEvalState)
{
	m_models.push_back(ModelEntry());
	m_models.back().setModel(asset, setupCustomEvalState);
}

AABox3f TraitModel::getBBoxOS() const
{
	AABox3f bbox;
	for (const ModelEntry& mdl : m_models) {
		bbox.expand(mdl.getBBoxOS(mat4f::getIdentity()));
	}

	return bbox;
}

void TraitModel::getRenderItems(DrawReason drawReason, std::vector<GeometryRenderItem>& renderItems)
{
	if (isRenderable == false) {
		return;
	}

	if (drawReason == drawReason_gameplayShadow && forceNoShadows) {
		return;
	}

	for (int iModel = 0; iModel < int(m_models.size()); iModel++) {
		ModelEntry& modelSets = m_models[iModel];
		if (!modelSets.isRenderable) {
			continue;
		}
		AssetIface_Model3D* modelIface = modelSets.m_assetProperty.getAssetInterface<AssetIface_Model3D>();

		mat4f actor2world = getActor()->getTransformMtx();

		const EvaluatedModel* evalModel = nullptr;
		if (modelSets.customEvalModel.hasValue()) {
			evalModel = &modelSets.customEvalModel.get();
		}
		else if (modelIface) {
			evalModel = &modelIface->getStaticEval();
		}

		if (evalModel) {
			for (const EvaluatedMeshInstance& meshInst : evalModel->getEvalMeshInstances()) {
				IMaterial* mtl = nullptr;

				// Check if there is a material override specified.
				if (meshInst.iMaterial < modelSets.mtlOverrides.size()) {
					if (modelSets.mtlOverrides[meshInst.iMaterial]) {
						if (modelSets.mtlOverrides[meshInst.iMaterial]) {
							mtl = modelSets.mtlOverrides[meshInst.iMaterial]->getMaterial().get();
						}
					}
				}

				// If there isn't material override use the default one in the model.
				if (mtl == nullptr) {
					mtl = evalModel->m_model->loadedMaterialAt(meshInst.iMaterial);
				}

				if (mtl) {
					IMaterialData* const imtlData = mtl->getMaterialDataLocalStorage();

					if (imtlData) {
						GeometryRenderItem ri;
						ri.geometry = &meshInst.geometry;
						ri.pMtlData = imtlData;
						ri.worldTransform = actor2world * modelSets.m_additionalTransform * meshInst.modelSpaceTransform;
						ri.bboxWs = meshInst.modelSpaceBBox.getTransformed(ri.worldTransform);

						ri.zSortingPositionWs = mat_mul_pos(ri.worldTransform, meshInst.modelSpaceBBox.center());
						ri.needsAlphaSorting = imtlData->needsAlphaSorting;

						renderItems.push_back(ri);
					}
				}
			}
		}
	}
}

void TraitModel::invalidateCachedAssets()
{
	for (ModelEntry& modelSets : m_models) {
		modelSets.invalidateCachedAssets();
	}
}

bool TraitModel::updateAssetProperties()
{
	bool hasChange = false;
	for (ModelEntry& model : m_models) {
		hasChange |= model.updateAssetProperty();
	}

	return hasChange;
}

/// TraitModel user interface that appears in the property editor.
void editUI_for_TraitModel(GameInspector& inspector, GameObject* actor, MemberChain chain)
{
	TraitModel& traitModel = *(TraitModel*)chain.follow(actor);

	if (ImGui::CollapsingHeader(ICON_FK_CUBE " 3D Models", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGuiEx::IndentGuard indent3DModels;

		// Set to a valid index if the user has clicked the "Delete" button on
		// any entry. If so we need to delete it once the UI has been built.
		int deleteModelIndex = -1;

		chain.add(sgeFindMember(TraitModel, isRenderable));
		ProperyEditorUIGen::doMemberUI(inspector, actor, chain);
		chain.pop();

		// Per model User Interface.
		for (int iModel = 0; iModel < traitModel.m_models.size(); ++iModel) {
			std::string label = string_format("Model %d", iModel);

			chain.add(sgeFindMember(TraitModel, m_models), iModel);
			ModelEntry& modelSets = traitModel.m_models[iModel];

			const ImGuiEx::IDGuard idGuardImGui(&modelSets);
			ImGuiEx::BeginGroupPanel(label.c_str());
			if (ImGui::CollapsingHeader(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGuiEx::IndentGuard indent3DModelsElement;
				chain.add(sgeFindMember(ModelEntry, isRenderable));
				ProperyEditorUIGen::doMemberUI(inspector, actor, chain);
				chain.pop();

				chain.add(sgeFindMember(ModelEntry, m_assetProperty));
				ProperyEditorUIGen::doMemberUI(inspector, actor, chain);
				chain.pop();

				// Material overrides interface.
				// TODO: Add undo/redo command history.
				AssetIface_Model3D* loadedModelIface = modelSets.m_assetProperty.getAssetInterface<AssetIface_Model3D>();
				if (loadedModelIface) {
					// Now do the UI for each available material slot.
					modelSets.mtlOverrides.resize(loadedModelIface->getModel3D().numMaterials());

					if (ImGui::CollapsingHeader(ICON_FK_PAINT_BRUSH " Materials")) {
						ImGuiEx::IndentGuard indentCollapsHeaderCotent;
						for (int iMtl : RangeInt(loadedModelIface->getModel3D().numMaterials())) {
							const ImGuiEx::IDGuard guard(iMtl);

							const ModelMaterial* mtl = loadedModelIface->getModel3D().materialAt(iMtl);

							AssetIfaceType types[] = {assetIface_mtl};
							AssetPtr currentAsset = std::dynamic_pointer_cast<Asset>(modelSets.mtlOverrides[iMtl]);

							if (currentAsset == nullptr) {
								ImGui::Text(ICON_FK_EXCLAMATION_TRIANGLE " %s has non-asset attached!", mtl->name.c_str());
							}
							if (assetPicker(mtl->name.c_str(), currentAsset, getCore()->getAssetLib(), types, 1)) {
								modelSets.mtlOverrides[iMtl] = std::dynamic_pointer_cast<AssetIface_Material>(currentAsset);
							}
						}
					}
				}

				// A button deleting the current model.
				if (traitModel.uiDontOfferResizingModelCount == false) {
					if (ImGui::Button(ICON_FK_TRASH " Remove Model")) {
						deleteModelIndex = iModel;
					}
				}

				chain.pop(); // Pops chain.add(sgeFindMember(TraitModel, m_models), iModel);
			}

			ImGuiEx::EndGroupPanel();
		}

		// Handle adding/removing models.
		if (traitModel.uiDontOfferResizingModelCount == false) {
			if (deleteModelIndex >= 0) {
				std::vector<ModelEntry> oldModels = traitModel.m_models;
				std::vector<ModelEntry> newModels = traitModel.m_models;
				newModels.erase(newModels.begin() + deleteModelIndex);

				chain.add(sgeFindMember(TraitModel, m_models));

				CmdMemberChange* const cmdDelElem = new CmdMemberChange();
				cmdDelElem->setup(actor, chain, &oldModels, &newModels, nullptr);
				cmdDelElem->apply(&inspector);
				inspector.appendCommand(cmdDelElem, false);

				chain.pop();
			}
		}

		if (ImGui::Button(ICON_FK_PLUS " Add Model")) {
			std::vector<ModelEntry> oldModels = traitModel.m_models;
			std::vector<ModelEntry> newModels = traitModel.m_models;
			newModels.push_back(ModelEntry());

			chain.add(sgeFindMember(TraitModel, m_models));

			CmdMemberChange* const cmdNewModelElem = new CmdMemberChange();
			cmdNewModelElem->setup(actor, chain, &oldModels, &newModels, nullptr);
			cmdNewModelElem->apply(&inspector);
			inspector.appendCommand(cmdNewModelElem, false);

			chain.pop();
		}
	}
}

SgePluginOnLoad()
{
	getEngineGlobal()->addPropertyEditorIUGeneratorForType(sgeTypeId(TraitModel), editUI_for_TraitModel);
}


} // namespace sge
