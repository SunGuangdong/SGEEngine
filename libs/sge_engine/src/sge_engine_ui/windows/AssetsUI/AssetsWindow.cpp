#include <filesystem>

#include "AssetsWindow.h"
#include "IconsForkAwesome/IconsForkAwesome.h"
#include "MaterialEditWindow.h"
#include "ModelParseSettings.h"
#include "sge_core/AssetLibrary/AssetLibrary.h"
#include "sge_core/ICore.h"
#include "sge_core/SGEImGui.h"
#include "sge_core/materials/MaterialFamilyList.h"
#include "sge_core/model/ModelReader.h"
#include "sge_core/model/ModelWriter.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine_ui/ui/ImGuiDragDrop.h"
#include "sge_engine_ui/ui/UIAssetPicker.h"
#include "sge_log/Log.h"
#include "sge_utils/io/FileStream.h"
#include "sge_utils/other/FileOpenDialog.h"
#include "sge_utils/text/Path.h"
#include "sge_utils/text/format.h"

#ifdef WIN32
	#define WIN32_LEAN_AND_MEAN
	#define NOMINAMX
	#include <Shlobj.h>
	#include <Windows.h>
	#include <shellapi.h>
#endif

namespace sge {

JsonValue* createDefaultPBRFromImportedMtl(ExternalPBRMaterialSettings& externalMaterial, JsonValueBuffer& jvb)
{
	JsonValue* jMaterial = jvb(JID_MAP);

	jMaterial->setMember("family", jvb("DefaultPBR"));
	jMaterial->setMember("alphaMultiplier", jvb(1.f));
	jMaterial->setMember("needsAlphaSorting", jvb(false));

	jMaterial->setMember("diffuseColor", jvb(externalMaterial.diffuseColor.data, 4));
	jMaterial->setMember("emissionColor", jvb(externalMaterial.emissionColor.data, 4));
	jMaterial->setMember("metallic", jvb(externalMaterial.metallic));
	jMaterial->setMember("roughness", jvb(externalMaterial.roughness));

	if (!externalMaterial.diffuseTextureName.empty()) {
		jMaterial->setMember("texDiffuse", jvb(externalMaterial.diffuseTextureName));
	}

	if (!externalMaterial.emissionTextureName.empty()) {
		jMaterial->setMember("texEmission", jvb(externalMaterial.emissionTextureName));
	}

	if (!externalMaterial.diffuseTextureName.empty()) {
		jMaterial->setMember("texMetallic", jvb(externalMaterial.metallicTextureName));
	}

	if (!externalMaterial.roughnessTextureName.empty()) {
		jMaterial->setMember("texRoughness", jvb(externalMaterial.roughnessTextureName));
	}

	if (!externalMaterial.normalTextureName.empty()) {
		jMaterial->setMember("texNormalMap", jvb(externalMaterial.normalTextureName));
	}

	return jMaterial;
}

AssetsWindow::AssetsWindow(std::string windowName)
    : m_windowName(std::move(windowName))
{
	// Try to load the functions that will be used for importing 3D models.
	if (mdlconvlibHandler.loadNoExt("mdlconvlib")) {
		m_sgeImportFBXFile =
		    reinterpret_cast<sgeImportModel3DFileFn>(mdlconvlibHandler.getProcAdress("sgeImportModel3DFile"));
		m_sgeImportFBXFileAsMultiple = reinterpret_cast<sgeImportModel3DFileAsMultipleFn>(
		    mdlconvlibHandler.getProcAdress("sgeImportModel3DFileAsMultiple"));
	}

	if (m_sgeImportFBXFile == nullptr || m_sgeImportFBXFileAsMultiple == nullptr) {
		sgeLogWarn("Failed to load dynamic library mdlconvlib. Importing FBX files would not be possible without it!");
	}
}

void AssetsWindow::openAssetImportPopUp()
{
	shouldOpenImportPopup = true;

	importPopUpInitState = AssetImportPopupInitState();
}

void AssetsWindow::openAssetImportPopUp_importFile(const std::string& filename)
{
	shouldOpenImportPopup = true;

	importPopUpInitState = AssetImportPopupInitState();
	importPopUpInitState.initialFilePathToImport = filename;
}

void AssetsWindow::openAssetImportPopUp_importOverFile(const std::string& importOverFilename)
{
	shouldOpenImportPopup = true;

	importPopUpInitState = AssetImportPopupInitState();
	importPopUpInitState.forcedImportedFilename = importOverFilename;
}

bool AssetsWindow::importAsset(AssetImportData& aid)
{
	AssetLibrary* const assetLib = getCore()->getAssetLib();

	std::string fullAssetPath = aid.outputDir + "/" + aid.outputFilename;

	if (aid.assetType == assetIface_model3d && aid.importModelsAsMultipleFiles == false) {
		Model importedModel;

		if (m_sgeImportFBXFile == nullptr) {
			sgeLogError("mdlconvlib dynamic library is not loaded. We cannot import FBX files without it!");
		}

		ModelImportAdditionalResult modelImportAddRes;
		if (m_sgeImportFBXFile && m_sgeImportFBXFile(importedModel, modelImportAddRes, aid.fileToImportPath.c_str())) {
			// Make sure the import directory exists.
			createDirectory(extractFileDir(aid.outputDir.c_str(), false).c_str());

			// Convert the 3d model to our internal type.
			ModelWriter modelWriter;
			[[maybe_unused]] const bool succeeded = modelWriter.write(importedModel, fullAssetPath.c_str());

			std::string notificationMsg = string_format("Imported %s", fullAssetPath.c_str());
			sgeLogInfo(notificationMsg.c_str());
			getEngineGlobal()->showNotification(notificationMsg);

			// Create the needed materials.
			JsonValueBuffer jvb;
			for (auto mtlToCreate : modelImportAddRes.mtlsToCreate) {
				JsonValue* jMaterial = createDefaultPBRFromImportedMtl(mtlToCreate.second, jvb);
				if (jMaterial) {
					const std::string mtlFilePath = aid.outputDir + "/" + mtlToCreate.first;
					JsonWriter jw;
					jw.WriteInFile(mtlFilePath.c_str(), jMaterial, true);
				}
			}

			// Copy the referenced texture files.
			const std::string modelInputDir = extractFileDir(aid.fileToImportPath.c_str(), true);
			for (const std::string& texturePathLocal : modelImportAddRes.textureToCopy) {
				const std::string textureDestDir = aid.outputDir + "/" + extractFileDir(texturePathLocal.c_str(), true);
				const std::string textureFilename = extractFileNameWithExt(texturePathLocal.c_str());

				const std::string textureSrcPath = canonizePathRespectOS(modelInputDir + texturePathLocal);
				const std::string textureDstPath = canonizePathRespectOS(textureDestDir + textureFilename);

				createDirectory(textureDestDir.c_str());
				copyFile(textureSrcPath.c_str(), textureDstPath.c_str());

				AssetPtr assetTexture = assetLib->getAssetFromFile(textureDstPath.c_str());
				assetLib->reloadAssetModified(assetTexture);
			}

			// Create the embbedded textures in the 3D model (if any).
			for (auto& texToCreatePair : modelImportAddRes.texturesToCreate) {
				std::string textureFilepath = aid.outputDir + "/" + texToCreatePair.first;

				FileWriteStream fws;
				if (fws.open(textureFilepath.c_str())) {
					fws.write(
					    texToCreatePair.second.textureFileData.data(), texToCreatePair.second.textureFileData.size());
				}
			}

			// Create the model.
			AssetPtr assetModel = assetLib->getAssetFromFile(fullAssetPath.c_str());
			assetLib->reloadAssetModified(assetModel);

			return true;
		}
		else {
			std::string notificationMsg = string_format("Failed to import %s", fullAssetPath.c_str());
			sgeLogError(notificationMsg.c_str());
			getEngineGlobal()->showNotification(notificationMsg);

			return false;
		}
	}
	else if (aid.assetType == assetIface_model3d && aid.importModelsAsMultipleFiles == true) {
		if (m_sgeImportFBXFileAsMultiple == nullptr) {
			sgeLogError("mdlconvlib dynamic library is not loaded. We cannot import FBX files without it!");
		}

		std::vector<std::string> referencedTextures;
		std::vector<MultiModelImportResult> importedModels;

		ModelImportAdditionalResult modelImportAddRes;
		if (m_sgeImportFBXFileAsMultiple &&
		    m_sgeImportFBXFileAsMultiple(importedModels, modelImportAddRes, aid.fileToImportPath.c_str())) {
			createDirectory(extractFileDir(aid.outputDir.c_str(), false).c_str());

			// Create the needed materials.
			JsonValueBuffer jvb;
			for (auto mtlToCreate : modelImportAddRes.mtlsToCreate) {
				JsonValue* jMaterial = createDefaultPBRFromImportedMtl(mtlToCreate.second, jvb);
				if (jMaterial) {
					const std::string mtlFilePath = aid.outputDir + "/" + mtlToCreate.first;
					JsonWriter jw;
					jw.WriteInFile(mtlFilePath.c_str(), jMaterial, true);
				}
			}

			// Copy the referenced textures.
			const std::string modelInputDir = extractFileDir(aid.fileToImportPath.c_str(), true);
			for (const std::string& texturePathLocal : modelImportAddRes.textureToCopy) {
				const std::string textureDestDir = aid.outputDir + "/" + extractFileDir(texturePathLocal.c_str(), true);
				const std::string textureFilename = extractFileNameWithExt(texturePathLocal.c_str());

				const std::string textureSrcPath = canonizePathRespectOS(modelInputDir + texturePathLocal);
				const std::string textureDstPath = canonizePathRespectOS(textureDestDir + textureFilename);

				createDirectory(textureDestDir.c_str());
				copyFile(textureSrcPath.c_str(), textureDstPath.c_str());

				AssetPtr assetTexture = assetLib->getAssetFromFile(textureDstPath.c_str());
				assetLib->reloadAssetModified(assetTexture);
			}

			// Create the embbedded textures in the 3D model (if any).
			for (auto& texToCreatePair : modelImportAddRes.texturesToCreate) {
				std::string textureFilepath = aid.outputDir + "/" + texToCreatePair.first;

				FileWriteStream fws;
				if (fws.open(textureFilepath.c_str())) {
					fws.write(
					    texToCreatePair.second.textureFileData.data(), texToCreatePair.second.textureFileData.size());
				}
			}

			// Create the models.
			for (MultiModelImportResult& model : importedModels) {
				if (model.propsedFilename.empty())
					continue;

				std::string path = aid.outputDir + "/" + model.propsedFilename;

				// Convert the 3d model to our internal type.
				ModelWriter modelWriter;
				[[maybe_unused]] const bool succeeded = modelWriter.write(model.importedModel, path.c_str());

				AssetPtr assetModel = assetLib->getAssetFromFile(path.c_str());
				assetLib->reloadAssetModified(assetModel);

				std::string notificationMsg = string_format("Imported %s", path.c_str());
				sgeLogInfo(notificationMsg.c_str());
				getEngineGlobal()->showNotification(notificationMsg);
			}

			return true;
		}
		else {
			std::string notificationMsg = string_format("Failed to import %s", fullAssetPath.c_str());
			sgeLogError(notificationMsg.c_str());
			getEngineGlobal()->showNotification(notificationMsg);

			return false;
		}
	}
	else if (aid.assetType == assetIface_texture2d) {
		// TODO: DDS conversion.
		createDirectory(extractFileDir(aid.outputDir.c_str(), false).c_str());
		copyFile(aid.fileToImportPath.c_str(), fullAssetPath.c_str());

		AssetPtr assetTexture = assetLib->getAssetFromFile(fullAssetPath.c_str());
		assetLib->reloadAssetModified(assetTexture);

		return true;
	}
	else if (aid.assetType == assetIface_spriteAnim) {
		SpriteAnimation tempSpriteAnimation;
		if (SpriteAnimation::importFromAsepriteSpriteSheetJsonFile(tempSpriteAnimation, aid.fileToImportPath.c_str())) {
			// There is a texture associated with this sprite, import it as well.
			std::string importTextureSource =
			    extractFileDir(aid.fileToImportPath.c_str(), true) + tempSpriteAnimation.texturePath;
			std::string importTextureDest = aid.outputDir + "/" + tempSpriteAnimation.texturePath;
			createDirectory(extractFileDir(importTextureDest.c_str(), false).c_str());
			copyFile(importTextureSource.c_str(), importTextureDest.c_str());

			// Finally make the path to the texture relative to the .sprite file.
			tempSpriteAnimation.texturePath =
			    relativePathTo(aid.outputDir.c_str(), "./") + "/" + tempSpriteAnimation.texturePath;

			return tempSpriteAnimation.saveSpriteToFile(fullAssetPath.c_str());
		}
		else {
			sgeLogError("Failed to import %s as a Sprite!", aid.fileToImportPath.c_str());
		}
	}
	else if (aid.assetType == assetIface_text) {
		createDirectory(extractFileDir(aid.outputDir.c_str(), false).c_str());
		copyFile(aid.fileToImportPath.c_str(), fullAssetPath.c_str());

		AssetPtr asset = assetLib->getAssetFromFile(fullAssetPath.c_str());
		assetLib->reloadAssetModified(asset);

		return true;
	}
	else {
		createDirectory(extractFileDir(aid.outputDir.c_str(), false).c_str());
		copyFile(aid.fileToImportPath.c_str(), fullAssetPath.c_str());
		sgeLogWarn("Imported a files by just copying it as it is not recognized asset type!") return true;
	}

	return false;
}

Texture* AssetsWindow::getThumbnailForModel3D(const std::string& localAssetPath)
{
	if (assetPreviewTex[localAssetPath].IsResourceValid()) {
		return assetPreviewTex[localAssetPath]->getRenderTarget(0);
	}
	else {
		AssetPtr thumbnailAsset = getCore()->getAssetLib()->getAssetFromFile(localAssetPath.c_str(), nullptr, true);
		if (isAssetLoaded(thumbnailAsset, assetIface_model3d)) {
			Box3f bboxModel = getLoadedAssetIface<AssetIface_Model3D>(thumbnailAsset)->getStaticEval().aabox;
			if (bboxModel.isEmpty() == false) {
				orbit_camera camera;

				camera.orbitPoint = bboxModel.center();
				camera.radius = bboxModel.diagonal().length() * 1.25f;
				camera.yaw = deg2rad(45.f);
				camera.pitch = deg2rad(45.f);

				RawCamera rawCamera = RawCamera(
				    camera.eyePosition(),
				    camera.GetViewMatrix(),
				    mat4f::getPerspectiveFovRH(deg2rad(60.f), 1.f, 0.1f, 10000.f, 0.f, kIsTexcoordStyleD3D));

				GpuHandle<FrameTarget> ft = getCore()->getDevice()->requestResource<FrameTarget>();
				ft->create2D(64, 64);
				getCore()->getDevice()->getContext()->clearDepth(ft, 1.f);

				RenderDestination rdest;
				rdest.frameTarget = ft;
				rdest.viewport = ft->getViewport();
				rdest.sgecon = getCore()->getDevice()->getContext();

				drawEvalModel(
				    rdest,
				    rawCamera,
				    mat4f::getIdentity(),
				    ObjectLighting::makeAmbientLightOnly(),
				    getLoadedAssetIface<AssetIface_Model3D>(thumbnailAsset)->getStaticEval(),
				    InstanceDrawMods());

				assetPreviewTex[localAssetPath] = ft;
				return assetPreviewTex[localAssetPath]->getRenderTarget(0);
			}
		}
	}

	return nullptr;
}

Texture* AssetsWindow::getThumbnailForAsset(const std::string& localAssetPath)
{
	AssetPtr thumbnailAsset = getCore()->getAssetLib()->getAssetFromFile(localAssetPath.c_str(), nullptr, true);
	if (isAssetLoaded(thumbnailAsset, assetIface_model3d)) {
		return getThumbnailForModel3D(localAssetPath);
	}
	else if (AssetIface_Texture2D* texIface = getLoadedAssetIface<AssetIface_Texture2D>(thumbnailAsset)) {
		return texIface->getTexture();
	}

	return nullptr;
}

void AssetsWindow::update(SGEContext* const UNUSED(sgecon), GameInspector* UNUSED(inspector), const InputState& is)
{
	if (isClosed()) {
		return;
	}

	AssetLibrary* const assetLib = getCore()->getAssetLib();

	if (ImGui::Begin(m_windowName.c_str(), &m_isOpened)) {
		doImportPopUp();

		namespace fs = std::filesystem;

		if (ImGui::Button(ICON_FK_BACKWARD)) {
			if (directoryTree.empty() == false) {
				directoryTree.pop_back();
			}
		}

		ImGui::SameLine();
		ImGui::Separator();
		ImGui::SameLine();
		ImGui::Text("Path: ");

		{
			int eraseAfter = -1;
			for (int t = 0; t < int(directoryTree.size()); ++t) {
				ImGui::SameLine();

				if (ImGui::Button(directoryTree[t].c_str())) {
					eraseAfter = t + 1;
				}
			}

			if (eraseAfter > -1) {
				while (directoryTree.size() > eraseAfter) {
					directoryTree.pop_back();
				}
			}
		}

		ImGui::NewLine();

		bool explorePreviewAssetChanged = false;
		if (explorePreviewAsset) {
			ImGui::Columns(2);
		}

		ImGui::Text(ICON_FK_SEARCH " File Filter");
		ImGui::SameLine();
		m_exploreFilter.Draw("##File Filter");
		if (ImGui::IsItemClicked(2)) {
			ImGui::ClearActiveID(); // Hack: (if we do not make this call ImGui::InputText will set it's cached value.
			m_exploreFilter.Clear();
		}

		// List all files in the currently selected directory in the interfance.
		if (ImGui::BeginChildFrame(ImGui::GetID("FilesChildFrameID"), ImVec2(-1.f, -1.f))) {
			try {
				if (!directoryTree.empty() && ImGui::Selectable(ICON_FK_BACKWARD " [/..]")) {
					directoryTree.pop_back();
				}

				bool shouldOpenNewFolderPopup = false;
				bool shouldOpenNewMaterialPopup = false;

				std::string label;
				fs::path pathToAssets = assetLib->getAssetsDirAbs();
				for (auto& p : directoryTree) {
					pathToAssets.append(p);
				}

				if (ImGui::Selectable(ICON_FK_FOLDER_O " [New Folder]")) {
					shouldOpenNewFolderPopup = true;
				}

				Optional<fs::path> rightClickedPath;

				// Show All sub directories of the one we currenly explore.
				std::string dirToAdd;
				for (const fs::directory_entry& entry : fs::directory_iterator(pathToAssets)) {
					if (entry.is_directory() && m_exploreFilter.PassFilter(entry.path().filename().string().c_str())) {
						string_format(label, "%s %s", ICON_FK_FOLDER, entry.path().filename().string().c_str());
						if (ImGui::Selectable(label.c_str())) {
							dirToAdd = entry.path().filename().string();
						}

						if (ImGui::IsItemClicked(1)) {
							rightClickedPath = entry.path();
						}
					}
				}

				// Show every file in the current directory with an icon next to it.
				int i = 0;
				for (const fs::directory_entry& entry : fs::directory_iterator(pathToAssets)) {
					i++;
					const std::string localAssetPath = relativePathToCwdCanoize(entry.path().string());

					if (entry.is_regular_file() &&
					    m_exploreFilter.PassFilter(entry.path().filename().string().c_str())) {
						AssetIfaceType assetType = assetIface_guessFromExtension(
						    extractFileExtension(entry.path().string().c_str()).c_str(), false);
						if (assetType == assetIface_model3d) {
							string_format(label, "%s %s", ICON_FK_CUBE, entry.path().filename().string().c_str());
						}
						else if (assetType == assetIface_texture2d) {
							string_format(label, "%s %s", ICON_FK_PICTURE_O, entry.path().filename().string().c_str());
						}
						else if (assetType == assetIface_text) {
							string_format(label, "%s %s", ICON_FK_FILE, entry.path().filename().string().c_str());
						}
						else if (assetType == assetIface_audio) {
							string_format(
							    label, "%s %s", ICON_FK_FILE_AUDIO_O, entry.path().filename().string().c_str());
						}
						else if (assetType == assetIface_mtl) {
							string_format(
							    label, "%s %s", ICON_FK_PAINT_BRUSH, entry.path().filename().string().c_str());
						}
						else {
							// Not implemented asset interface.
							string_format(
							    label, "%s %s", ICON_FK_QUESTION_CIRCLE, entry.path().filename().string().c_str());
						}

						if (assetType != assetIface_unknown) {
							bool showAsSelected =
							    isAssetLoaded(explorePreviewAsset) && explorePreviewAsset->getPath() == localAssetPath;

#if 1
							Texture* thumbnailTex = getThumbnailForAsset(localAssetPath);
							if (thumbnailTex) {
								ImGui::Image(thumbnailTex, ImVec2(64.f, 64.f));
								ImGui::SameLine();
							}
#endif

							if (ImGui::Selectable(label.c_str(), &showAsSelected)) {
								explorePreviewAssetChanged = true;
								explorePreviewAsset = assetLib->getAssetFromFile(localAssetPath.c_str());

								if (auto mtlIface =
								        std::dynamic_pointer_cast<AssetIface_Material>(explorePreviewAsset)) {
									openMaterialEditWindow(mtlIface);
								}
							}

							if (ImGui::IsItemClicked(1)) {
								rightClickedPath = entry.path();
							}

							if (ImGui::BeginDragDropSource()) {
								DragDropPayloadAsset::setPayload(localAssetPath);
								ImGui::Text(localAssetPath.c_str());
								ImGui::EndDragDropSource();
							}
						}
						else {
							ImGui::Selectable(label.c_str());
						}
					}
				}

				// Handle right-clicking over an asset in the explorer.
				if (rightClickedPath.hasValue()) {
					ImGui::OpenPopup("RightClickMenuAssets");
					m_rightClickedPath = rightClickedPath.get();
				}
				else if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(1)) {
					ImGui::OpenPopup("RightClickMenuAssets");
					m_rightClickedPath.clear();
				}

				// Right click menu.
				fs::path importOverAsset;
				if (ImGui::BeginPopup("RightClickMenuAssets")) {
					if (ImGui::MenuItem(ICON_FK_DOWNLOAD " Import here...")) {
						openAssetImportPopUp();
					}

					if (ImGui::MenuItem(ICON_FK_FOLDER " New Folder")) {
						shouldOpenNewFolderPopup = true;
					}

					if (ImGui::BeginMenu(ICON_FK_PLUS " Create")) {
						if (ImGui::MenuItem(ICON_FK_PAINT_BRUSH " Material")) {
							shouldOpenNewMaterialPopup = true;
						}
						ImGui::EndMenu();
					}

					ImGui::Separator();

					if (!m_rightClickedPath.empty()) {
						ImGui::Separator();

						if (ImGui::MenuItem(ICON_FK_CLIPBOARD " Copy Path")) {
							ImGui::SetClipboardText(m_rightClickedPath.string().c_str());
						}

						if (ImGui::MenuItem(ICON_FK_REFRESH " Import Over")) {
							openAssetImportPopUp_importOverFile(m_rightClickedPath.string());
						}

						if (assetIface_guessFromExtension(
						        extractFileExtension(m_rightClickedPath.string().c_str()).c_str(), false) ==
						    assetIface_model3d) {
							if (ImGui::MenuItem(ICON_FK_SEARCH " Preview 3D Model")) {
								ModelPreviewWindow* modelPreview =
								    new ModelPreviewWindow((m_rightClickedPath.string() + " Moder Previwer").c_str());

								modelPreview->setPreviewModel(
								    getCore()->getAssetLib()->getAssetFromFile(m_rightClickedPath.string().c_str()));

								getEngineGlobal()->addWindow(modelPreview);
							}
						}
#ifdef WIN32
						if (ImGui::MenuItem(ICON_FK_WINDOWS " Open in Explorer")) {
							PIDLIST_ABSOLUTE pItem = ILCreateFromPathA(m_rightClickedPath.string().c_str());
							if (pItem) {
								SHOpenFolderAndSelectItems(pItem, 0, NULL, 0);
								ILFree(pItem);
							}
						}
#endif
					}

					ImGui::EndPopup();
				}

				// Create Directory Popup.
				{
					if (shouldOpenNewFolderPopup) {
						ImGui::OpenPopup("SGE Assets Window Create Dir");
					}

					static char createDirFileName[1024] = {0};
					if (ImGui::BeginPopup("SGE Assets Window Create Dir")) {
						ImGui::InputText(
						    ICON_FK_FOLDER " Folder Name", createDirFileName, SGE_ARRSZ(createDirFileName));
						if (ImGui::Button(ICON_FK_CHECK " Create")) {
							createDirectory((pathToAssets.string() + "/" + std::string(createDirFileName)).c_str());
							createDirFileName[0] = '\0';
							ImGui::CloseCurrentPopup();
						}

						if (ImGui::Button("Cancel")) {
							ImGui::CloseCurrentPopup();
						}

						ImGui::EndPopup();
					}

					if (dirToAdd.empty() == false) {
						directoryTree.emplace_back(std::move(dirToAdd));
					}
				}

				// Create Material Popup.
				{
					if (shouldOpenNewMaterialPopup) {
						ImGui::OpenPopup("SGE Assets Window Create Material");
					}

					static char newMtlName[1024] = {0};
					static std::string newMtlFamily = "DefaultPBR";
					if (ImGui::BeginPopup("SGE Assets Window Create Material")) {
						auto& allFamilies = getCore()->getMaterialLib()->getAllFamilies();

						if (ImGui::BeginCombo("Family", newMtlFamily.c_str())) {
							for (auto& family : allFamilies) {
								if (ImGui::Selectable(family.second.familyDesc.displayName.c_str())) {
									newMtlFamily = family.second.familyDesc.displayName;
								}
							}

							ImGui::EndCombo();
						}

						ImGui::InputText(ICON_FK_FILE " Name", newMtlName, SGE_ARRSZ(newMtlName));

						if (ImGui::Button(ICON_FK_CHECK " Create")) {
							const MaterialFamilyLibrary::MaterialFamilyData* mtlFamData =
							    getCore()->getMaterialLib()->findFamilyByName(newMtlFamily.c_str());

							if (mtlFamData) {
								std::shared_ptr<IMaterial> newMtl = mtlFamData->familyDesc.mtlAllocFn();

								JsonValueBuffer jvb;
								JsonValue* jMtlRoot = newMtl->toJson(jvb, pathToAssets.string().c_str());

								JsonWriter jw;
								std::string mtlFilename =
								    (pathToAssets.string() + "/" + std::string(newMtlName) + ".mtl").c_str();
								jw.WriteInFile(mtlFilename.c_str(), jMtlRoot, true);
								newMtlName[0] = '\0';
							}

							ImGui::CloseCurrentPopup();
						}


						if (ImGui::Button("Cancel")) {
							ImGui::CloseCurrentPopup();
						}

						ImGui::EndPopup();
					}
				}
			}


			catch (...) {
			}
		}
		ImGui::EndChildFrame();

		if (isAssetLoaded(explorePreviewAsset)) {
			ImGui::NextColumn();

			if (ImGui::BeginChild("Asset Quick Info and Inspect")) {
				if (AssetIface_Model3D* modelIface = getLoadedAssetIface<AssetIface_Model3D>(explorePreviewAsset)) {
					doPreviewAssetModel(is, explorePreviewAssetChanged);
				}
				else if (
				    AssetIface_Texture2D* texIface = getLoadedAssetIface<AssetIface_Texture2D>(explorePreviewAsset)) {
					doPreviewAssetTexture2D(texIface);
				}
				else if (
				    AssetIface_SpriteAnim* spriteIface =
				        getLoadedAssetIface<AssetIface_SpriteAnim>(explorePreviewAsset)) {
					if (AssetIface_Texture2D* spriteTexIface =
					        getLoadedAssetIface<AssetIface_Texture2D>(spriteIface->getSpriteAnimation().textureAsset)) {
						auto desc = spriteTexIface->getTexture()->getDesc().texture2D;
						ImVec2 sz = ImGui::GetContentRegionAvail();
						ImGui::Image(spriteTexIface->getTexture(), sz);
					}
				}
				else if (auto mtlIface = std::dynamic_pointer_cast<AssetIface_Material>(explorePreviewAsset)) {
					if (ImGui::Button(ICON_FK_PICTURE_O " Edit Material")) {
						openMaterialEditWindow(mtlIface);
					}
				}
				else if (
				    IAssetInterface_Audio* audioIface =
				        getLoadedAssetIface<IAssetInterface_Audio>(explorePreviewAsset)) {
					ImGui::Text("No Preview");
					if (auto audioDataPtr = audioIface->getAudioData()) {
						// auto audioData = explorePreviewAsset->asAudio();
						// ImGui::Text("Vorbis encoded Audio file");
						// ImGui::Text("Sample Rate: %.1f kHZ", (float)(*track)->info.samplesPerSecond / 1000.0f);
						// ImGui::Text("Number of channels: %d", (*track)->info.numChannels);
						// ImGui::Text("Length: %.2f s", (float)(*track)->info.numSamples /
						// (float)(*track)->info.samplesPerSecond);
					}
				}
				else {
					ImGui::Text("No Preview");
				}
			}
			ImGui::EndChild();
		}
	}
	ImGui::End();
}

void AssetsWindow::doImportPopUp()
{
	if (shouldOpenImportPopup) {
		ImGui::OpenPopup("SGE Assets Window Import Popup");
	}

	if (ImGui::BeginPopup("SGE Assets Window Import Popup")) {
		if (shouldOpenImportPopup) {
			shouldOpenImportPopup = false;
			// If this is still true then the popup has just been opened.
			// Initialize it with the information about the asset we are about to import.
			m_importAssetToImportInPopup = AssetImportData();

			// If openAssetImport_filename is specified then we must use it, it means that the popup was somehow forced
			// externally like a drag-and-drop.
			if (importPopUpInitState.forcedImportedFilename.empty()) {
				m_importAssetToImportInPopup.fileToImportPath =
				    FileOpenDialog("Pick a file to import", true, "*.*\0*.*\0", nullptr);
			}
			else {
				m_importAssetToImportInPopup.fileToImportPath = importPopUpInitState.forcedImportedFilename;

				// Clear it so we not reset the UI on the next update.
				importPopUpInitState.forcedImportedFilename.clear();
			}

			// If the user clicked over an assed and clicked import over, use the name of already imported asset,
			// otherwise create a new name based on the input name.
			m_importAssetToImportInPopup.outputDir = getCurrentExplorePath().string();
			if (importPopUpInitState.forcedImportedFilename.empty()) {
				m_importAssetToImportInPopup.outputFilename =
				    extractFileNameWithExt(m_importAssetToImportInPopup.fileToImportPath.c_str());
			}
			else {
				m_importAssetToImportInPopup.outputFilename = importPopUpInitState.forcedImportedFilename;

				// Clear it so we not reset the UI on the next update.
				importPopUpInitState.forcedImportedFilename.clear();
			}

			// Guess the type of the inpute asset.
			const std::string inputExtension =
			    extractFileExtension(m_importAssetToImportInPopup.fileToImportPath.c_str());
			m_importAssetToImportInPopup.assetType = assetIface_guessFromExtension(inputExtension.c_str(), true);

			// If the asset type is None, maybe the asset has a commonly used extension
			// like Aseprite json sprite sheet descriptors, or they might be just generic
			// files that the user wants to copy. Try to guess the type of this generic file
			// for convinice of the user.
			if (m_importAssetToImportInPopup.assetType == assetIface_unknown) {
				if (inputExtension == "json") {
					SpriteAnimation tempSpriteAnimation;
					if (SpriteAnimation::importFromAsepriteSpriteSheetJsonFile(
					        tempSpriteAnimation, m_importAssetToImportInPopup.fileToImportPath.c_str())) {
						m_importAssetToImportInPopup.assetType = assetIface_spriteAnim;
					}
				}
			}

			// Change the extension of the imported file based on the asset type.
			if (m_importAssetToImportInPopup.assetType == assetIface_model3d) {
				m_importAssetToImportInPopup.outputFilename =
				    replaceExtension(m_importAssetToImportInPopup.outputFilename.c_str(), "mdl");
			}

			if (m_importAssetToImportInPopup.assetType == assetIface_spriteAnim) {
				m_importAssetToImportInPopup.outputFilename =
				    replaceExtension(m_importAssetToImportInPopup.outputFilename.c_str(), "sprite");
			}

			if (m_importAssetToImportInPopup.fileToImportPath.empty()) {
				ImGui::CloseCurrentPopup();
			}
		}

		// The UI of the pop-up itself:
		if (m_importAssetToImportInPopup.assetType == assetIface_model3d) {
			ImGui::Text(ICON_FK_CUBE " 3D Model");
			ImGui::Checkbox(
			    ICON_FK_CUBES " Import As Multiple Models", &m_importAssetToImportInPopup.importModelsAsMultipleFiles);
			ImGuiEx::TextTooltip(
			    "When multiple game objects are defined in one 3D model file. You can import them as a separate 3D "
			    "models using this option!");
		}
		else if (m_importAssetToImportInPopup.assetType == assetIface_texture2d) {
			ImGui::Text(ICON_FK_PICTURE_O " Texture");
		}
		else if (m_importAssetToImportInPopup.assetType == assetIface_text) {
			ImGui::Text(ICON_FK_FILE " Text");
		}
		else if (m_importAssetToImportInPopup.assetType == assetIface_spriteAnim) {
			ImGui::Text(ICON_FK_FILM " Sprite");
		}
		else if (m_importAssetToImportInPopup.assetType == assetIface_mtl) {
			ImGui::Text(ICON_FK_FLASK " Material");
		}
		else {
			ImGui::Text(ICON_FK_FILE_TEXT_O " Unknown, the file is going to be copyied!");
			ImGui::Text("If you know the type of the asset you can override it below.");

			const char* assetTypeNames[int(assetIface_count)] = {nullptr};
			assetTypeNames[int(assetIface_unknown)] = ICON_FK_FILE_TEXT_O " Unknown";
			assetTypeNames[int(assetIface_model3d)] = ICON_FK_CUBE " 3D Model";
			assetTypeNames[int(assetIface_texture2d)] = ICON_FK_PICTURE_O " Texture";
			assetTypeNames[int(assetIface_text)] = ICON_FK_FILE " Text";
			assetTypeNames[int(assetIface_spriteAnim)] = ICON_FK_FILM " Sprite";
			assetTypeNames[int(assetIface_mtl)] = ICON_FK_FLASK " Material";

			ImGuiEx::Label("Import As:");
			if (ImGui::BeginCombo("##Import As: ", assetTypeNames[int(m_importAssetToImportInPopup.assetType)])) {
				for (int t = 0; t < SGE_ARRSZ(assetTypeNames); ++t) {
					if (assetTypeNames[t] != nullptr) {
						if (ImGui::Selectable(assetTypeNames[t])) {
							m_importAssetToImportInPopup.assetType = AssetIfaceType(t);
							if (t == int(assetIface_model3d)) {
								m_importAssetToImportInPopup.outputFilename =
								    replaceExtension(m_importAssetToImportInPopup.outputFilename.c_str(), "mdl");
							}
							if (t == int(assetIface_spriteAnim)) {
								m_importAssetToImportInPopup.outputFilename =
								    replaceExtension(m_importAssetToImportInPopup.outputFilename.c_str(), "sprite");
							}
						}
					}
				}

				ImGui::EndCombo();
			}
		}

		ImGuiEx::InputText("Read From", m_importAssetToImportInPopup.fileToImportPath, ImGuiInputTextFlags_ReadOnly);

		if (m_importAssetToImportInPopup.importModelsAsMultipleFiles == false) {
			ImGuiEx::InputText("Import As", m_importAssetToImportInPopup.outputFilename);
		}

		// Show a warning that the import will fail if mdlconvlib is not loaded.
		if (m_sgeImportFBXFile == nullptr && m_importAssetToImportInPopup.assetType == assetIface_model3d) {
			ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), "Importing 3D files cannot be done! mdlconvlib is missing!");
		}


		if (ImGui::Button(ICON_FK_DOWNLOAD " Import")) {
			importAsset(m_importAssetToImportInPopup);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button(ICON_FK_DOWNLOAD " Cancel")) {
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void AssetsWindow::openMaterialEditWindow(std::shared_ptr<sge::AssetIface_Material> mtlIface)
{
	MaterialEditWindow* mtlEditWnd =
	    dynamic_cast<MaterialEditWindow*>(getEngineGlobal()->findWindowByName(ICON_FK_PICTURE_O " Material Edit"));
	if (mtlEditWnd == nullptr) {
		mtlEditWnd = new MaterialEditWindow(ICON_FK_PICTURE_O " Material Edit");
		getEngineGlobal()->addWindow(mtlEditWnd);
	}
	else {
		ImGui::SetNextWindowFocus();
	}

	mtlEditWnd->setAsset(mtlIface);
}

void AssetsWindow::doPreviewAssetModel(const InputState& is, bool explorePreviewAssetChanged)
{
	if (explorePreviewAssetChanged) {
		Box3f bboxModel = getLoadedAssetIface<AssetIface_Model3D>(explorePreviewAsset)->getStaticEval().aabox;
		if (bboxModel.isEmpty() == false) {
			m_exploreModelPreviewWidget.camera.orbitPoint = bboxModel.center();
			m_exploreModelPreviewWidget.camera.radius = bboxModel.diagonal().length() * 1.25f;
			m_exploreModelPreviewWidget.camera.yaw = deg2rad(45.f);
			m_exploreModelPreviewWidget.camera.pitch = deg2rad(45.f);
		}
	}

	AssetIface_Model3D* modelSrc = getLoadedAssetIface<AssetIface_Model3D>(explorePreviewAsset);

	const Model* model = modelSrc ? modelSrc->getModel3D() : nullptr;
	if (model) {
		EvaluatedModel& staticEval = modelSrc->getStaticEval();

		m_exploreModelPreviewWidget.doWidget(getCore()->getDevice()->getContext(), is, staticEval);

		ImGui::Text("Animation Count: %d", model->numAnimations());
		for (int iAnim = 0; iAnim < model->numAnimations(); ++iAnim) {
			ImGui::Text("\t%s", model->animationAt(iAnim)->animationName.c_str());
		}

		for (int iMesh = 0; iMesh < model->numMeshes(); ++iMesh) {
			ImGui::Text("\t%s", model->meshAt(iMesh)->name.c_str());
		}

		ImGui::Text("Material Count: %d", model->numMaterials());
		for (int iMtl = 0; iMtl < model->numMaterials(); ++iMtl) {
			ImGui::Text("\t%s", model->materialAt(iMtl)->name.c_str());
		}
	}
}

void AssetsWindow::doPreviewAssetTexture2D(AssetIface_Texture2D* texIface)
{
	const Texture2DDesc& desc = texIface->getTexture()->getDesc().texture2D;
	const ImVec2 availableContentSize = ImGui::GetContentRegionAvail();

	const float imageSizeX = availableContentSize.x;
	const float imageSizeY = desc.height * availableContentSize.x / float(desc.width);

	ImGui::Image(texIface->getTexture(), ImVec2(imageSizeX, imageSizeY));

	const auto getAddressModeName = [](TextureAddressMode::Enum mode) -> const char* {
		switch (mode) {
			case TextureAddressMode::Repeat:
				return "Repeat";
			case TextureAddressMode::ClampEdge:
				return "Edge";
			case TextureAddressMode::ClampBorder:
				return "Border";

			default:
				return "Unknown";
		}
	};

	const auto getFilterModeName = [](TextureFilter::Enum mode) -> const char* {
		switch (mode) {
			case TextureFilter::Min_Mag_Mip_Linear:
				return "Linear (with mip mapping)";
			case TextureFilter::Min_Mag_Mip_Point:
				return "Point (with mip mapping)";
			default:
				return "Unknown";
		}
	};

	const auto doAddressModeUI = [&getAddressModeName](const char* comboLabel, TextureAddressMode::Enum& mode) -> bool {
		bool hadChange = false;
		if (ImGui::BeginCombo(comboLabel, getAddressModeName(mode))) {
			if (ImGui::Selectable(getAddressModeName(TextureAddressMode::Repeat))) {
				mode = TextureAddressMode::Repeat;
				hadChange = true;
			}

			if (ImGui::Selectable(getAddressModeName(TextureAddressMode::ClampEdge))) {
				mode = TextureAddressMode::ClampEdge;
				hadChange = true;
			}

			if (ImGui::Selectable(getAddressModeName(TextureAddressMode::ClampBorder))) {
				mode = TextureAddressMode::ClampBorder;
				hadChange = true;
			}

			ImGui::EndCombo();
		}

		return hadChange;
	};

	bool hadChange = false;

	AssetTextureMeta texMeta = texIface->getTextureMeta();

	ImGuiEx::Label("Semi-Transparent");
	hadChange |= ImGui::Checkbox("##Semi Transparent", &texMeta.isSemiTransparent);

	ImGuiEx::Label("Auto Generate MipMaps");
	bool changedMipMapGen = ImGui::Checkbox("##Auto Generate MipMaps", &texMeta.shouldGenerateMips);
	hadChange |= changedMipMapGen;
	if (changedMipMapGen) {
		getLog()->writeWarning("Auto MipMap generation will take effect after a restart!");
	}

	hadChange |= doAddressModeUI("Tiling X", texMeta.assetSamplerDesc.addressModes[0]);
	hadChange |= doAddressModeUI("Tiling Y", texMeta.assetSamplerDesc.addressModes[1]);
	hadChange |= doAddressModeUI("Tiling Z", texMeta.assetSamplerDesc.addressModes[2]);

	if (ImGui::BeginCombo("Filtering", getFilterModeName(texMeta.assetSamplerDesc.filter))) {
		if (ImGui::Selectable(getFilterModeName(TextureFilter::Min_Mag_Mip_Linear))) {
			texMeta.assetSamplerDesc.filter = TextureFilter::Min_Mag_Mip_Linear;
			hadChange = true;
		}

		if (ImGui::Selectable(getFilterModeName(TextureFilter::Min_Mag_Mip_Point))) {
			texMeta.assetSamplerDesc.filter = TextureFilter::Min_Mag_Mip_Point;
			hadChange = true;
		}

		ImGui::EndCombo();
	}

	if (hadChange) {
		GpuHandle<SamplerState> sampler = getCore()->getDevice()->requestResource<SamplerState>();
		sampler->create(texMeta.assetSamplerDesc);
		texIface->setTextureMeta(texMeta);
		texIface->getTexture()->setSamplerState(sampler);

		// Save the modified settings to the *.info file of the texture.
		if (AssetTexture2d* assetTex2d = dynamic_cast<AssetTexture2d*>(explorePreviewAsset.get())) {
			assetTex2d->saveTextureSettingsToInfoFile();
			getCore()->getAssetLib()->queueAssetForReload(explorePreviewAsset);
		}
		else {
			sgeAssertFalse("It is expected that explorePreviewAsset is a AssetTexture2d");
		}
	}
}

std::filesystem::path AssetsWindow::getCurrentExplorePath() const
{
	std::filesystem::path pathToAssets = getCore()->getAssetLib()->getAssetsDirAbs();
	for (auto& p : directoryTree) {
		pathToAssets.append(p);
	}

	return pathToAssets;
}

} // namespace sge
