#include "TraitModel.h"
#include "IconsForkAwesome/IconsForkAwesome.h"
#include "sge_core/AssetLibrary/AssetMaterial.h"
#include "sge_core/SGEImGui.h"
#include "sge_core/materials/DefaultPBRMtl/DefaultPBRMtl.h"
#include "sge_core/typelib/typeLib.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_engine/GameDrawer/RenderItems/TraitModelRenderItem.h"
#include "sge_engine/GameInspector.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/actors/ALocator.h"
#include "sge_engine/ui/UIAssetPicker.h"
#include "sge_engine/windows/PropertyEditorWindow.h"
#include "sge_utils/loop.h"
#include "sge_utils/stl_algorithm_ex.h"
#include "sge_utils/utils/strings.h"
#include "sge_utils/utils/vector_set.h"

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
void ModelEntry::setModel(const char* assetPath) {
	m_assetProperty.setTargetAsset(assetPath);
	updateAssetProperty();
}

void ModelEntry::setModel(AssetPtr& asset) {
	m_assetProperty.setAsset(asset);
	m_evalModel = NullOptional();
	updateAssetProperty();
}

void ModelEntry::onAssetModelChanged() {
	m_evalModel = NullOptional();

	if (AssetIface_Model3D* modelIface = getLoadedAssetIface<AssetIface_Model3D>(m_assetProperty.getAsset())) {
		mtlOverrides.resize(modelIface->getModel3D().numMaterials());
	}
}

AABox3f ModelEntry::getBBoxOS() const {
	// If the attached asset is a model use it to compute the bounding box.
	const AssetIface_Model3D* const modelIface = m_assetProperty.getAssetInterface<AssetIface_Model3D>();
	if (modelIface) {
		AABox3f bbox = modelIface->getStaticEval().aabox.getTransformed(m_additionalTransform);
		return bbox;
	}

	return AABox3f();
}

//-----------------------------------------------------------------
// TraitModel
//-----------------------------------------------------------------
void TraitModel::addModel(const char* assetPath) {
	m_models.push_back(ModelEntry());
	m_models.back().setModel(assetPath);
}

void TraitModel::addModel(AssetPtr& asset) {
	m_models.push_back(ModelEntry());
	m_models.back().setModel(asset);
}

AABox3f TraitModel::getBBoxOS() const {
	AABox3f bbox;

	for (const ModelEntry& mdl : m_models) {
		bbox.expand(mdl.getBBoxOS());
	}

	return bbox;
}

void TraitModel::getRenderItems(DrawReason drawReason, std::vector<TraitModelRenderItem>& renderItems) {

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

		mat4f node2world = getActor()->getTransformMtx();

		const EvaluatedModel* evalModel = nullptr;
		if (modelSets.m_evalModel.hasValue()) {
			evalModel = &modelSets.m_evalModel.get();
		} else if (modelIface) {
			evalModel = &modelIface->getStaticEval();
		}

		if (evalModel) {
			int numNodes = evalModel->getNumEvalNodes();

			for (int iNode = 0; iNode < numNodes; ++iNode) {
				TraitModelRenderItem renderItem;

				const EvaluatedNode& evalNode = evalModel->getEvalNode(iNode);
				const ModelNode* node = evalModel->m_model->nodeAt(iNode);
				int numAttachments = int(node->meshAttachments.size());
				for (int iAttach = 0; iAttach < numAttachments; ++iAttach) {
					int mtlIndex = node->meshAttachments[iAttach].attachedMaterialIndex;

					IMaterial* mtl = nullptr;
					if (mtlIndex < modelSets.mtlOverrides.size()) {
						if (modelSets.mtlOverrides[mtlIndex]) {
							if (modelSets.mtlOverrides[mtlIndex]) {
								mtl = modelSets.mtlOverrides[mtlIndex]->getMaterial().get();
							}
						}
					}

					if (mtl == nullptr) {
						const std::shared_ptr<AssetIface_Material>& mtlProvider =
						    evalModel->getEvalMaterial(node->meshAttachments[iAttach].attachedMaterialIndex);
						if (mtlProvider) {
							const std::shared_ptr<IMaterial>& mtlIface = mtlProvider->getMaterial();
							if (mtlIface) {
								mtl = mtlIface.get();
							}
						}
					}

					IMaterialData* imtlData = mtl->getMaterialDataLocalStorage();

					renderItem.pMtlData = imtlData;
					renderItem.zSortingPositionWs = mat_mul_pos(node2world, evalNode.aabbGlobalSpace.center());
					renderItem.traitModel = this;
					renderItem.evalModel = evalModel;
					renderItem.iModel = iModel;
					renderItem.iEvalNode = iNode;
					renderItem.iEvalNodeMechAttachmentIndex = iAttach;
					renderItem.needsAlphaSorting = imtlData->needsAlphaSorting;

					renderItems.push_back(renderItem);
				}
			}
		}
	}
}

void TraitModel::invalidateCachedAssets() {
	for (ModelEntry& modelSets : m_models) {
		modelSets.invalidateCachedAssets();
	}
}

bool TraitModel::updateAssetProperties() {
	bool hasChange = false;
	for (ModelEntry& model : m_models) {
		hasChange |= model.updateAssetProperty();
	}

	return hasChange;
}

/// TraitModel user interface that appears in the property editor.
void editUI_for_TraitModel(GameInspector& inspector, GameObject* actor, MemberChain chain) {
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
						for (int iMtl : rng_int(loadedModelIface->getModel3D().numMaterials())) {
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

SgePluginOnLoad() {
	getEngineGlobal()->addPropertyEditorIUGeneratorForType(sgeTypeId(TraitModel), editUI_for_TraitModel);
}

} // namespace sge
