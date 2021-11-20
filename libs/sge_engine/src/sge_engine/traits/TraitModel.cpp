#include "TraitModel.h"
#include "IconsForkAwesome/IconsForkAwesome.h"
#include "sge_core/AssetLibrary/AssetMaterial.h"
#include "sge_core/SGEImGui.h"
#include "sge_core/materials/DefaultPBRMtl/DefaultPBRMtl.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_engine/GameDrawer/RenderItems/TraitModelRenderItem.h"
#include "sge_engine/GameInspector.h"
#include "sge_engine/GameWorld.h"
#include "sge_core/typelib/typeLib.h"
#include "sge_engine/actors/ALocator.h"
#include "sge_engine/ui/UIAssetPicker.h"
#include "sge_engine/windows/PropertyEditorWindow.h"
#include "sge_utils/loop.h"
#include "sge_utils/stl_algorithm_ex.h"
#include "sge_utils/utils/strings.h"
#include "sge_utils/utils/vector_set.h"

namespace sge {

// struct MDiffuseMaterial; // User for creating material overrides.

// clang-format off
RelfAddTypeId(TraitModel, 20'03'01'0004);
RelfAddTypeId(TraitModel::PerModelSettings, 21'07'11'0002);
RelfAddTypeId(std::vector<TraitModel::PerModelSettings>, 21'07'11'0003);
ReflBlock() {
	ReflAddType(TraitModel::PerModelSettings)
		ReflMember(TraitModel::PerModelSettings, isRenderable)
		ReflMember(TraitModel::PerModelSettings, alphaMultiplier).uiRange(0.f, 1.f, 0.01f)
		ReflMember(TraitModel::PerModelSettings, m_assetProperty)
		ReflMember(TraitModel::PerModelSettings, useSkeleton)
		ReflMember(TraitModel::PerModelSettings, rootSkeletonId)
	;
	ReflAddType(std::vector<TraitModel::PerModelSettings>);

	ReflAddType(TraitModel)
		ReflMember(TraitModel, isRenderable)
		ReflMember(TraitModel, m_models)
	;
}
// clang-format on

void TraitModel::PerModelSettings::setModel(const char* assetPath, bool updateNow) {
	m_assetProperty.setTargetAsset(assetPath);
	if (updateNow) {
		updateAssetProperty();
	}
}

void TraitModel::PerModelSettings::setModel(AssetPtr& asset, bool updateNow) {
	m_assetProperty.setAsset(asset);
	m_evalModel = NullOptional();
	if (updateNow) {
		updateAssetProperty();
	}
}

//-----------------------------------------------------------------
// TraitModel::PerModelSettings
//-----------------------------------------------------------------
AABox3f TraitModel::PerModelSettings::getBBoxOS() const {
	// If the attached asset is a model use it to compute the bounding box.
	const AssetIface_Model3D* const modelIface = m_assetProperty.getAssetInterface<AssetIface_Model3D>();
	if (modelIface) {
		AABox3f bbox = modelIface->getStaticEval().aabox.getTransformed(m_additionalTransform);
		return bbox;
	}

	return AABox3f();
}

void TraitModel::PerModelSettings::computeNodeToBoneIds(TraitModel& ownerTraitModel) {
	if (nodeToBoneId.empty() == false) {
		return;
	}

	nodeToBoneId.clear();

	if (useSkeleton == false) {
		return;
	}

	AssetIface_Model3D* modelIface = m_assetProperty.getAssetInterface<AssetIface_Model3D>();
	if (modelIface == nullptr) {
		return;
	}

	GameWorld* world = ownerTraitModel.getWorld();
	vector_set<ObjectId> boneActorIds;
	world->getAllChildren(boneActorIds, rootSkeletonId);
	boneActorIds.insert(rootSkeletonId);

	vector_set<Actor*> boneActors;
	for (ObjectId boneId : boneActorIds) {
		Actor* bone = world->getActorById(boneId);
		if (bone == nullptr) {
			nodeToBoneId.clear();
			sgeAssert("Could not find a bone for the actor!");
			return;
		}

		boneActors.insert(bone);
	}

	for (const int iNode : rng_int(modelIface->getModel3D().numNodes())) {
		const ModelNode* node = modelIface->getModel3D().nodeAt(iNode);
		for (Actor* boneActor : boneActors) {
			if (node->name == boneActor->getDisplayName()) {
				nodeToBoneId[iNode] = boneActor->getId();
				break;
			}
		}
	}
}

void TraitModel::PerModelSettings::computeSkeleton(TraitModel& ownerTraitModel, std::vector<mat4f>& boneOverrides) {
	AssetIface_Model3D* modelIface = m_assetProperty.getAssetInterface<AssetIface_Model3D>();
	if (modelIface == nullptr) {
		return;
	}

	boneOverrides.resize(modelIface->getModel3D().numNodes());

	if (useSkeleton == false) {
		return;
	}

	computeNodeToBoneIds(ownerTraitModel);
	Actor* root = ownerTraitModel.getWorld()->getActorById(rootSkeletonId);
	if (root == nullptr) {
		return;
	}

	mat4f rootInv = root ? root->getTransformMtx().inverse() : mat4f::getIdentity();

	for (auto pair : nodeToBoneId) {
		mat4f& boneGobalTForm = boneOverrides[pair.first];

		Actor* const boneActor = ownerTraitModel.getWorld()->getActorById(pair.second);
		if (boneActor) {
			boneGobalTForm = rootInv * boneActor->getTransformMtx();
		} else {
			sgeAssert(false && "Expected alive object");
			boneGobalTForm = mat4f::getIdentity();
		}
	}
}

void TraitModel::setModel(const char* assetPath, bool updateNow) {
	m_models.resize(1);

	m_models[0].m_evalModel = NullOptional();
	m_models[0].m_assetProperty.setTargetAsset(assetPath);
	if (updateNow) {
		postUpdate();
	}
}

void TraitModel::setModel(AssetPtr& asset, bool updateNow) {
	m_models.resize(1);

	m_models[0].m_assetProperty.setAsset(asset);
	m_models[0].m_evalModel = NullOptional();
	if (updateNow) {
		postUpdate();
	}
}

void TraitModel::addModel(const char* assetPath, bool updateNow) {
	m_models.push_back(PerModelSettings());
	m_models.back().setModel(assetPath, updateNow);
}

void TraitModel::addModel(AssetPtr& asset, bool updateNow) {
	m_models.push_back(PerModelSettings());
	m_models.back().setModel(asset, updateNow);
}

//-----------------------------------------------------------------
// TraitModel
//-----------------------------------------------------------------
AABox3f TraitModel::getBBoxOS() const {
	AABox3f bbox;

	for (const PerModelSettings& mdl : m_models) {
		bbox.expand(mdl.getBBoxOS());
	}

	return bbox;
}

void TraitModel::getRenderItems(DrawReason drawReason, std::vector<TraitModelRenderItem>& renderItems) {
	if (drawReason == drawReason_gameplayShadow && forceNoShadows) {
		return;
	}

	for (int iModel = 0; iModel < int(m_models.size()); iModel++) {
		PerModelSettings& modelSets = m_models[iModel];
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
							const AssetIface_Material* mtlIfaceAsset = getAssetIface<AssetIface_Material>(modelSets.mtlOverrides[mtlIndex]);
							if (mtlIfaceAsset) {
								mtl = mtlIfaceAsset->getMaterial().get();
							}
						}
					}

					if (mtl == nullptr) {
						mtl = evalModel->getEvalMaterial(node->meshAttachments[iAttach].attachedMaterialIndex).get();
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
	for (PerModelSettings& modelSets : m_models) {
		modelSets.invalidateCachedAssets();
	}
}

bool TraitModel::updateAssetProperties() {
	bool hasChange = false;
	for (PerModelSettings& model : m_models) {
		hasChange |= model.updateAssetProperty();
	}

	return hasChange;
}

/// TraitModel Attribute Editor.
void editTraitModel(GameInspector& inspector, GameObject* actor, MemberChain chain) {
	TraitModel& traitModel = *(TraitModel*)chain.follow(actor);

	if (ImGui::CollapsingHeader(ICON_FK_CUBE " Model Trait", ImGuiTreeNodeFlags_DefaultOpen)) {
		int deleteModelIndex = -1;

		chain.add(sgeFindMember(TraitModel, isRenderable));
		ProperyEditorUIGen::doMemberUI(inspector, actor, chain);
		chain.pop();

		// Per model User Interface.
		for (int iModel = 0; iModel < traitModel.m_models.size(); ++iModel) {
			std::string label = string_format("Model %d", iModel);

			chain.add(sgeFindMember(TraitModel, m_models), iModel);
			TraitModel::PerModelSettings& modelSets = traitModel.m_models[iModel];

			const ImGuiEx::IDGuard idGuardImGui(&modelSets);
			ImGuiEx::BeginGroupPanel(label.c_str());
			if (ImGui::CollapsingHeader(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
				chain.add(sgeFindMember(TraitModel::PerModelSettings, isRenderable));
				ProperyEditorUIGen::doMemberUI(inspector, actor, chain);
				chain.pop();

				chain.add(sgeFindMember(TraitModel::PerModelSettings, alphaMultiplier));
				ProperyEditorUIGen::doMemberUI(inspector, actor, chain);
				chain.pop();

				chain.add(sgeFindMember(TraitModel::PerModelSettings, m_assetProperty));
				ProperyEditorUIGen::doMemberUI(inspector, actor, chain);
				chain.pop();

				// Material and Skeleton overrides interface.
				AssetIface_Model3D* loadedModelIface = modelSets.m_assetProperty.getAssetInterface<AssetIface_Model3D>();
				if (loadedModelIface) {
#if 0
				 for (ModelMaterial* mtl : loadedModelIface->getModel3D().getMatrials()) {
					// Check if override for this material already exists.
					auto itrExisting = find_if(modelSets.m_materialOverrides, [&](const TraitModel::MaterialOverride& ovr) -> bool {
						return ovr.materialName == mtl->name;
					});

					if (itrExisting == std::end(modelSets.m_materialOverrides)) {
						TraitModel::MaterialOverride ovr;
						ovr.materialName = mtl->name;
						modelSets.m_materialOverrides.emplace_back(ovr);
					}
				}
#endif

// Now do the UI for each available slot (keep in mind that we are keeping old slots (from previous models still
// available).
#if 1
					modelSets.mtlOverrides.resize(loadedModelIface->getModel3D().numMaterials());

					if (ImGui::CollapsingHeader(ICON_FK_PAINT_BRUSH " Material Override")) {
						for (int iMtl : rng_int(loadedModelIface->getModel3D().numMaterials())) {
							const ImGuiEx::IDGuard guard(iMtl);

							const ModelMaterial* mtl = loadedModelIface->getModel3D().materialAt(iMtl);

							AssetIfaceType types[] = {assetIface_mtl};
							assetPicker(mtl->name.c_str(), modelSets.mtlOverrides[iMtl], getCore()->getAssetLib(), types, 1);


#if 0
							// ImGui::Text(loadedAsset->model.m_materials[t]->name.c_str());
							// Check if override for this material already exists.
							auto itrExisting = find_if(modelSets.m_materialOverrides, [&](const TraitModel::MaterialOverride& ovr) -> bool {
								return ovr.materialName == mtl->name;
							});

							if (itrExisting != std::end(modelSets.m_materialOverrides)) {
								ImGuiEx::BeginGroupPanel(itrExisting->materialName.c_str(), ImVec2(-1.f, 0.f));
								{
									const int slotIndex = int(itrExisting - modelSets.m_materialOverrides.begin());
									chain.add(sgeFindMember(TraitModel::PerModelSettings, m_materialOverrides), slotIndex);
									chain.add(sgeFindMember(TraitModel::MaterialOverride, materialObjId));
									ProperyEditorUIGen::doMemberUI(inspector, actor, chain);

									if (modelSets.m_materialOverrides[slotIndex].materialObjId.isNull()) {
										if (ImGui::Button("Create New Material")) {
											CmdObjectCreation* cmdMtlCreate = new CmdObjectCreation();
											// cmdMtlCreate->setup(sgeTypeId(MDiffuseMaterial));
											cmdMtlCreate->apply(&inspector);

											CmdMemberChange* const cmdAssignMaterial = new CmdMemberChange();
											const ObjectId originalValue = ObjectId();
											const ObjectId createdMaterialId = cmdMtlCreate->getCreatedObjectId();
											cmdAssignMaterial->setup(actor, chain, &originalValue, &createdMaterialId, nullptr);
											cmdAssignMaterial->apply(&inspector);

											CmdCompound* cmdCompound = new CmdCompound();
											cmdCompound->addCommand(cmdMtlCreate);
											cmdCompound->addCommand(cmdAssignMaterial);
											inspector.appendCommand(cmdCompound, false);
										}
									}

									chain.pop();
									chain.pop();

									if (GameObject* const materialObject =
									        inspector.getWorld()->getObjectById(itrExisting->materialObjId)) {
										ImGui::PushID(materialObject);
										ImGuiEx::BeginGroupPanel("", ImVec2(-1.f, 0.f));
										if (ImGui::CollapsingHeader("Material")) {
											ProperyEditorUIGen::doGameObjectUI(inspector, materialObject);
										}
										ImGuiEx::EndGroupPanel();
										ImGui::PopID();
									}
								}
								ImGuiEx::EndGroupPanel();
							} else {
								sgeAssert(false && "The slot should have been created above.");
							}
#endif
						}
					}
#endif
					// Skeletion UI
					if (ImGui::CollapsingHeader(ICON_FK_WRENCH " Skeleton (WIP)")) {
						if (ImGui::Button("Extract Skeleton")) {
							GameWorld* world = inspector.getWorld();
							ALocator* allBonesParent = world->allocObjectT<ALocator>();
							allBonesParent->setTransform(traitModel.getActor()->getTransform());

							struct NodeRemapEl {
								transf3d localTransform;
								ABone* boneActor = nullptr;
							};
							vector_map<int, NodeRemapEl> nodeRemap;

							transf3d n2w = actor->getActor()->getTransform();

							// AssetIface_Model3D* loadedModelIface = modelSets.m_assetProperty.getAssetInterface<AssetIface_Model3D>();
							Model& loadedModel = loadedModelIface->getModel3D();
							for (int iNode : rng_int(loadedModel.numNodes())) {
								const ModelNode* node = loadedModel.nodeAt(iNode);

								const float boneLengthAuto = 0.1f;
								const float boneLength = node->limbLength > 0.f ? node->limbLength : boneLengthAuto;

								nodeRemap[iNode].localTransform = node->staticLocalTransform;
								nodeRemap[iNode].boneActor = inspector.getWorld()->allocObjectT<ABone>(ObjectId(), node->name.c_str());
								nodeRemap[iNode].boneActor->boneLength = boneLength;
							}

							std::function<void(int nodeIndex, NodeRemapEl* parent)> traverseGlobalTransform;
							traverseGlobalTransform = [&](int nodeIndex, NodeRemapEl* parent) -> void {
								const ModelNode* node = loadedModel.nodeAt(nodeIndex);

								NodeRemapEl* const remapNode = nodeRemap.find_element(nodeIndex);
								if (parent) {
									remapNode->boneActor->setTransform(parent->boneActor->getTransform() * remapNode->localTransform);
									inspector.getWorld()->setParentOf(remapNode->boneActor->getId(), parent->boneActor->getId());
								} else {
									remapNode->localTransform = allBonesParent->getTransform() * remapNode->localTransform;
									remapNode->boneActor->setTransform(remapNode->localTransform);
									world->setParentOf(remapNode->boneActor->getId(), allBonesParent->getId());
								}

								for (const int childNode : node->childNodes) {
									traverseGlobalTransform(childNode, remapNode);
								}
							};

							traverseGlobalTransform(loadedModel.getRootNodeIndex(), nullptr);
						}


						chain.add(sgeFindMember(TraitModel::PerModelSettings, useSkeleton));
						ProperyEditorUIGen::doMemberUI(inspector, actor, chain);
						chain.pop();

						chain.add(sgeFindMember(TraitModel::PerModelSettings, rootSkeletonId));
						ProperyEditorUIGen::doMemberUI(inspector, actor, chain);
						chain.pop();
					}
				}

				// A button deleting the current model.
				if (traitModel.isFixedModelsSize == false) {
					if (ImGui::Button(ICON_FK_TRASH " Remove Model")) {
						deleteModelIndex = iModel;
					}
				}

				chain.pop(); // Pops chain.add(sgeFindMember(TraitModel, m_models), iModel);
			}

			ImGuiEx::EndGroupPanel();
		}

		// Handle adding/removing models.
		if (traitModel.isFixedModelsSize == false) {
			if (deleteModelIndex >= 0) {
				std::vector<TraitModel::PerModelSettings> oldModels = traitModel.m_models;
				std::vector<TraitModel::PerModelSettings> newModels = traitModel.m_models;
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
			std::vector<TraitModel::PerModelSettings> oldModels = traitModel.m_models;
			std::vector<TraitModel::PerModelSettings> newModels = traitModel.m_models;
			newModels.push_back(TraitModel::PerModelSettings());

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
	getEngineGlobal()->addPropertyEditorIUGeneratorForType(sgeTypeId(TraitModel), editTraitModel);
}

} // namespace sge
