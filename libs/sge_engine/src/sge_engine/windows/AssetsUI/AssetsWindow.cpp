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
#include "sge_engine/GameInspector.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/ui/ImGuiDragDrop.h"
#include "sge_engine/ui/UIAssetPicker.h"
#include "sge_log/Log.h"
#include "sge_utils/tiny/FileOpenDialog.h"
#include "sge_utils/utils/Path.h"
#include "sge_utils/utils/strings.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINAMX
#include <Shlobj.h>
#include <Windows.h>
#include <shellapi.h>
#endif

namespace sge {

///
JsonValue* createDefaultPBRFromImportedMtl(ExternalPBRMaterialSettings& externalMaterial, JsonValueBuffer& jvb) {
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

AssetsWindow::AssetsWindow(std::string windowName, GameInspector& inspector)
    : m_windowName(std::move(windowName))
    , m_inspector(inspector) {
	// Try to load the functions that will be used for importing 3D models.
	if (mdlconvlibHandler.loadNoExt("mdlconvlib")) {
		m_sgeImportFBXFile = reinterpret_cast<sgeImportFBXFileFn>(mdlconvlibHandler.getProcAdress("sgeImportFBXFile"));
		m_sgeImportFBXFileAsMultiple =
		    reinterpret_cast<sgeImportFBXFileAsMultipleFn>(mdlconvlibHandler.getProcAdress("sgeImportFBXFileAsMultiple"));
	}

	if (m_sgeImportFBXFile == nullptr || m_sgeImportFBXFileAsMultiple == nullptr) {
		sgeLogWarn("Failed to load dynamic library mdlconvlib. Importing FBX files would not be possible without it!");
	}
}

void AssetsWindow::openAssetImport(const std::string& filename) {
	shouldOpenImportPopup = true;
	openAssetImport_filename = filename;
}

bool AssetsWindow::importAsset(AssetImportData& aid) {
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
			const bool succeeded = modelWriter.write(importedModel, fullAssetPath.c_str());

			

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

			AssetPtr assetModel = assetLib->getAssetFromFile(fullAssetPath.c_str());
			assetLib->reloadAssetModified(assetModel);

			return true;
		} else {
			std::string notificationMsg = string_format("Failed to import %s", fullAssetPath.c_str());
			sgeLogError(notificationMsg.c_str());
			getEngineGlobal()->showNotification(notificationMsg);

			return false;
		}
	} else if (aid.assetType == assetIface_model3d && aid.importModelsAsMultipleFiles == true) {
		if (m_sgeImportFBXFileAsMultiple == nullptr) {
			sgeLogError("mdlconvlib dynamic library is not loaded. We cannot import FBX files without it!");
		}

		std::vector<std::string> referencedTextures;
		std::vector<MultiModelImportResult> importedModels;

		ModelImportAdditionalResult modelImportAddRes;
		if (m_sgeImportFBXFileAsMultiple && m_sgeImportFBXFileAsMultiple(importedModels, modelImportAddRes, aid.fileToImportPath.c_str())) {
			createDirectory(extractFileDir(aid.outputDir.c_str(), false).c_str());

			for (MultiModelImportResult& model : importedModels) {
				if (model.propsedFilename.empty())
					continue;

				std::string path = aid.outputDir + "/" + model.propsedFilename;

				// Convert the 3d model to our internal type.
				ModelWriter modelWriter;
				const bool succeeded = modelWriter.write(model.importedModel, path.c_str());

				AssetPtr assetModel = assetLib->getAssetFromFile(path.c_str());
				assetLib->reloadAssetModified(assetModel);

				std::string notificationMsg = string_format("Imported %s", path.c_str());
				sgeLogInfo(notificationMsg.c_str());
				getEngineGlobal()->showNotification(notificationMsg);
			}

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

			return true;
		} else {
			std::string notificationMsg = string_format("Failed to import %s", fullAssetPath.c_str());
			sgeLogError(notificationMsg.c_str());
			getEngineGlobal()->showNotification(notificationMsg);

			return false;
		}
	} else if (aid.assetType == assetIface_texture2d) {
		// TODO: DDS conversion.
		createDirectory(extractFileDir(aid.outputDir.c_str(), false).c_str());
		copyFile(aid.fileToImportPath.c_str(), fullAssetPath.c_str());

		AssetPtr assetTexture = assetLib->getAssetFromFile(fullAssetPath.c_str());
		assetLib->reloadAssetModified(assetTexture);

		return true;
	} else if (aid.assetType == assetIface_spriteAnim) {
		SpriteAnimation tempSpriteAnimation;
		if (SpriteAnimation::importFromAsepriteSpriteSheetJsonFile(tempSpriteAnimation, aid.fileToImportPath.c_str())) {
			// There is a texture associated with this sprite, import it as well.
			std::string importTextureSource = extractFileDir(aid.fileToImportPath.c_str(), true) + tempSpriteAnimation.texturePath;
			std::string importTextureDest = aid.outputDir + "/" + tempSpriteAnimation.texturePath;
			createDirectory(extractFileDir(importTextureDest.c_str(), false).c_str());
			copyFile(importTextureSource.c_str(), importTextureDest.c_str());

			// Finally make the path to the texture relative to the .sprite file.
			tempSpriteAnimation.texturePath = relativePathTo(aid.outputDir.c_str(), "./") + "/" + tempSpriteAnimation.texturePath;

			return tempSpriteAnimation.saveSpriteToFile(fullAssetPath.c_str());
		} else {
			sgeLogError("Failed to import %s as a Sprite!", aid.fileToImportPath.c_str());
		}
	} else if (aid.assetType == assetIface_text) {
		createDirectory(extractFileDir(aid.outputDir.c_str(), false).c_str());
		copyFile(aid.fileToImportPath.c_str(), fullAssetPath.c_str());

		AssetPtr asset = assetLib->getAssetFromFile(fullAssetPath.c_str());
		assetLib->reloadAssetModified(asset);

		return true;
	} else {
		createDirectory(extractFileDir(aid.outputDir.c_str(), false).c_str());
		copyFile(aid.fileToImportPath.c_str(), fullAssetPath.c_str());
		sgeLogWarn("Imported a files by just copying it as it is not recognized asset type!") return true;
	}

	return false;
}

void AssetsWindow::update_assetImport(SGEContext* const sgecon, const InputState& is) {
	if (ImGui::Button(ICON_FK_PLUS " Add Asset")) {
		std::string filename = FileOpenDialog("Import 3D Model", true, "*.fbx\0*.fbx\0*.dae\0*.dae\0*.obj\0*.obj\0*.*\0*.*\0", nullptr);
		if (filename.empty() == false) {
			openAssetImport(filename);
		}
	}

	if (ImGui::BeginChild("Child Assets To Import")) {
		std::string groupPanelName;
		for (int iAsset = 0; iAsset < m_assetsToImport.size(); ++iAsset) {
			AssetImportData& aid = m_assetsToImport[iAsset];
			ImGui::PushID(&aid);

			if (aid.importFailed) {
				ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "Failed to Import");
			}

			string_format(groupPanelName, "3D Model %s", aid.fileToImportPath.c_str());
			ImGuiEx::BeginGroupPanel(groupPanelName.c_str());
			{
				if (ImGui::Button("Import As")) {
					aid.outputFilename = FileSaveDialog("Import 3D Model As", "*.mdl\0*.mdl", "mdl", nullptr);
				}
				ImGui::SameLine();
				char importAs[1024] = {0};
				sge_strcpy(importAs, aid.outputFilename.c_str());
				if (ImGui::InputText("##Import As", importAs, SGE_ARRSZ(importAs))) {
					aid.outputFilename = importAs;
				}

				ImGui::Checkbox("Preview", &aid.preview);
				if (aid.preview) {
					if (aid.assetType == assetIface_model3d) {
						aid.modelPreviewWidget.doWidget(sgecon, is, getAssetIface<AssetIface_Model3D>(aid.tempAsset)->getStaticEval(),
						                                vec2f(-1.f, 256.f));
					}
				}

				if (ImGui::Button(ICON_FK_DOWNLOAD "Import")) {
					if (importAsset(aid)) {
						m_assetsToImport.erase(m_assetsToImport.begin() + iAsset);
						iAsset--;
					}
				}

				ImGui::SameLine();
				if (ImGui::Button(ICON_FK_TRASH " Remove")) {
					m_assetsToImport.erase(m_assetsToImport.begin() + iAsset);
					iAsset--;
				}
			}
			ImGuiEx::EndGroupPanel();

			ImGui::PopID();
		}

		if (ImGui::Button("Import All")) {
		}

		ImGui::EndChild();
	}
}

void AssetsWindow::update(SGEContext* const UNUSED(sgecon), const InputState& is) {
	if (isClosed()) {
		return;
	}

	AssetLibrary* const assetLib = getCore()->getAssetLib();

	if (ImGui::Begin(m_windowName.c_str(), &m_isOpened)) {
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
				for (const fs::directory_entry& entry : fs::directory_iterator(pathToAssets)) {
					const std::string localAssetPath = relativePathToCwdCanoize(entry.path().string());

					if (entry.is_regular_file() && m_exploreFilter.PassFilter(entry.path().filename().string().c_str())) {
						AssetIfaceType assetType =
						    assetIface_guessFromExtension(extractFileExtension(entry.path().string().c_str()).c_str(), false);
						if (assetType == assetIface_model3d) {
							string_format(label, "%s %s", ICON_FK_CUBE, entry.path().filename().string().c_str());

						} else if (assetType == assetIface_texture2d) {
							string_format(label, "%s %s", ICON_FK_PICTURE_O, entry.path().filename().string().c_str());
						} else if (assetType == assetIface_text) {
							string_format(label, "%s %s", ICON_FK_FILE, entry.path().filename().string().c_str());
						} else if (assetType == assetIface_audio) {
							string_format(label, "%s %s", ICON_FK_FILE_AUDIO_O, entry.path().filename().string().c_str());
						} else {
							// Not implemented asset interface.
							string_format(label, "%s %s", ICON_FK_QUESTION_CIRCLE, entry.path().filename().string().c_str());
						}

						if (assetType != assetIface_unknown) {
							if (ImGui::Selectable(label.c_str())) {
								explorePreviewAssetChanged = true;
								explorePreviewAsset = assetLib->getAssetFromFile(localAssetPath.c_str());
							}
							if (ImGui::IsItemClicked(1)) {
								rightClickedPath = entry.path();
							}

							if (ImGui::BeginDragDropSource()) {
								DragDropPayloadAsset::setPayload(localAssetPath);
								ImGui::Text(localAssetPath.c_str());
								ImGui::EndDragDropSource();
							}
#if 0
							if (assetPreviewTex[localAssetPath].IsResourceValid()) {
								ImGui::Image(assetPreviewTex[localAssetPath]->getRenderTarget(0), ImVec2(64.f, 64.f));
							} else {
								explorePreviewAsset = assetLib->getAssetFromFile(localAssetPath.c_str(), nullptr, true);
								if (isAssetLoaded(explorePreviewAsset, assetIface_model3d)) {
									AABox3f bboxModel = getAssetIface<AssetIface_Model3D>(explorePreviewAsset)->getStaticEval().aabox;
									if (bboxModel.IsEmpty() == false) {
										orbit_camera camera;

										camera.orbitPoint = bboxModel.center();
										camera.radius = bboxModel.diagonal().length() * 1.25f;
										camera.yaw = deg2rad(45.f);
										camera.pitch = deg2rad(45.f);

										RawCamera rawCamera = RawCamera(
										    camera.eyePosition(), camera.GetViewMatrix(),
										    mat4f::getPerspectiveFovRH(deg2rad(60.f), 1.f, 0.1f, 10000.f, 0.f, kIsTexcoordStyleD3D));

										GpuHandle<FrameTarget> ft = getCore()->getDevice()->requestResource<FrameTarget>();
										ft->create2D(64, 64);
										getCore()->getDevice()->getContext()->clearDepth(ft, 1.f);

										RenderDestination rdest;
										rdest.frameTarget = ft;
										rdest.viewport = ft->getViewport();
										rdest.sgecon = getCore()->getDevice()->getContext();

										imods.forceNoLighting = true;
										drawEvalModel(rdest, rawCamera, mat4f::getIdentity(), ObjectLighting(),
										              getAssetIface<AssetIface_Model3D>(explorePreviewAsset)->getStaticEval(), InstanceDrawMods());

										assetPreviewTex[localAssetPath] = ft;
									}
								}
							}
#endif


						} else {
							ImGui::Selectable(label.c_str());
						}
					}
				}

				// Handle right-clicking over an asset in the explorer.
				if (rightClickedPath.hasValue()) {
					ImGui::OpenPopup("RightClickMenuAssets");
					m_rightClickedPath = rightClickedPath.get();
				} else if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(1)) {
					ImGui::OpenPopup("RightClickMenuAssets");
					m_rightClickedPath.clear();
				}

				// Right click menu.
				fs::path importOverAsset;
				if (ImGui::BeginPopup("RightClickMenuAssets")) {
					if (ImGui::MenuItem(ICON_FK_DOWNLOAD " Import here...")) {
						shouldOpenImportPopup = true;
						openAssetImport_filename.clear();
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

					if (!m_rightClickedPath.empty()) {
						ImGui::Separator();

						if (ImGui::MenuItem(ICON_FK_CLIPBOARD " Copy Path")) {
							ImGui::SetClipboardText(m_rightClickedPath.string().c_str());
						}

						if (ImGui::MenuItem(ICON_FK_REFRESH " Import Over")) {
							importOverAsset = m_rightClickedPath;
							shouldOpenImportPopup = true;
							openAssetImport_filename.clear();
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

				// Import Popup
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
						if (openAssetImport_filename.empty()) {
							m_importAssetToImportInPopup.fileToImportPath =
							    FileOpenDialog("Pick a file to import", true, "*.*\0*.*\0", nullptr);
						} else {
							m_importAssetToImportInPopup.fileToImportPath = openAssetImport_filename;
							openAssetImport_filename.clear();
						}

						// If the user clicked over an assed and clicked import over, use the name of already imported asset,
						// otherwise create a new name based on the input name.
						m_importAssetToImportInPopup.outputDir = pathToAssets.string();
						if (importOverAsset.empty()) {
							m_importAssetToImportInPopup.outputFilename =
							    extractFileNameWithExt(m_importAssetToImportInPopup.fileToImportPath.c_str());
						} else {
							m_importAssetToImportInPopup.outputFilename = importOverAsset.filename().string();
							importOverAsset.clear();
						}

						// Guess the type of the inpute asset.
						const std::string inputExtension = extractFileExtension(m_importAssetToImportInPopup.fileToImportPath.c_str());
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
						ImGui::Checkbox(ICON_FK_CUBES " Import As Multiple Models",
						                &m_importAssetToImportInPopup.importModelsAsMultipleFiles);
						ImGuiEx::TextTooltip(
						    "When multiple game objects are defined in one 3D model file. You can import them as a separate 3D "
						    "models using this option!");
					} else if (m_importAssetToImportInPopup.assetType == assetIface_texture2d) {
						ImGui::Text(ICON_FK_PICTURE_O " Texture");
					} else if (m_importAssetToImportInPopup.assetType == assetIface_text) {
						ImGui::Text(ICON_FK_FILE " Text");
					} else if (m_importAssetToImportInPopup.assetType == assetIface_spriteAnim) {
						ImGui::Text(ICON_FK_FILM " Sprite");
					} else if (m_importAssetToImportInPopup.assetType == assetIface_mtl) {
						ImGui::Text(ICON_FK_FLASK " Material");
					} else {
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

				// Create Directory Popup.
				{
					if (shouldOpenNewFolderPopup) {
						ImGui::OpenPopup("SGE Assets Window Create Dir");
					}

					static char createDirFileName[1024] = {0};
					if (ImGui::BeginPopup("SGE Assets Window Create Dir")) {
						ImGui::InputText(ICON_FK_FOLDER " Folder Name", createDirFileName, SGE_ARRSZ(createDirFileName));
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
								std::string mtlFilename = (pathToAssets.string() + "/" + std::string(newMtlName) + ".mtl").c_str();
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

			if (AssetIface_Model3D* modelIface = getAssetIface<AssetIface_Model3D>(explorePreviewAsset)) {
				doPreviewAssetModel(is, explorePreviewAssetChanged);
			} else if (AssetIface_Texture2D* texIface = getAssetIface<AssetIface_Texture2D>(explorePreviewAsset)) {
				doPreviewAssetTexture2D(texIface);
			} else if (AssetIface_SpriteAnim* spriteIface = getAssetIface<AssetIface_SpriteAnim>(explorePreviewAsset)) {
				if (AssetIface_Texture2D* spriteTexIface =
				        getAssetIface<AssetIface_Texture2D>(spriteIface->getSpriteAnimation().textureAsset)) {
					auto desc = spriteTexIface->getTexture()->getDesc().texture2D;
					ImVec2 sz = ImGui::GetContentRegionAvail();
					ImGui::Image(spriteTexIface->getTexture(), sz);
				}
			} else if (AssetIface_Material* mtlIface = getAssetIface<AssetIface_Material>(explorePreviewAsset)) {
				if (ImGui::Button(ICON_FK_PICTURE_O " Edit Material")) {
					std::shared_ptr<IMaterial> mtl = mtlIface->getMaterial();

					MaterialEditWindow* mtlEditWnd =
					    dynamic_cast<MaterialEditWindow*>(getEngineGlobal()->findWindowByName(ICON_FK_PICTURE_O " Material Edit"));
					if (mtlEditWnd == nullptr) {
						mtlEditWnd = new MaterialEditWindow(ICON_FK_PICTURE_O " Material Edit", m_inspector);
						getEngineGlobal()->addWindow(mtlEditWnd);
					}

					mtlEditWnd->setAsset(std::dynamic_pointer_cast<AssetIface_Material>(explorePreviewAsset));
				}
			} else if (IAssetInterface_Audio* audioIface = getAssetIface<IAssetInterface_Audio>(explorePreviewAsset)) {
				ImGui::Text("No Preview");
				if (auto audioDataPtr = audioIface->getAudioData()) {
					// auto audioData = explorePreviewAsset->asAudio();
					// ImGui::Text("Vorbis encoded Audio file");
					// ImGui::Text("Sample Rate: %.1f kHZ", (float)(*track)->info.samplesPerSecond / 1000.0f);
					// ImGui::Text("Number of channels: %d", (*track)->info.numChannels);
					// ImGui::Text("Length: %.2f s", (float)(*track)->info.numSamples / (float)(*track)->info.samplesPerSecond);
				}
			} else {
				ImGui::Text("No Preview");
			}
		}
	}
	ImGui::End();
}

void AssetsWindow::doPreviewAssetModel(const InputState& is, bool explorePreviewAssetChanged) {
	if (explorePreviewAssetChanged) {
		AABox3f bboxModel = getAssetIface<AssetIface_Model3D>(explorePreviewAsset)->getStaticEval().aabox;
		if (bboxModel.IsEmpty() == false) {
			m_exploreModelPreviewWidget.camera.orbitPoint = bboxModel.center();
			m_exploreModelPreviewWidget.camera.radius = bboxModel.diagonal().length() * 1.25f;
			m_exploreModelPreviewWidget.camera.yaw = deg2rad(45.f);
			m_exploreModelPreviewWidget.camera.pitch = deg2rad(45.f);
		}
	}

	const Model& model = getAssetIface<AssetIface_Model3D>(explorePreviewAsset)->getModel3D();
	EvaluatedModel& staticEval = getAssetIface<AssetIface_Model3D>(explorePreviewAsset)->getStaticEval();

	m_exploreModelPreviewWidget.doWidget(getCore()->getDevice()->getContext(), is, staticEval);

	ImGui::Text("Animation Count: %d", model.numAnimations());
	for (int iAnim = 0; iAnim < model.numAnimations(); ++iAnim) {
		ImGui::Text("\t%s", model.animationAt(iAnim)->animationName.c_str());
	}

	ImGui::Text("Node Count: %d", model.numNodes());
	ImGui::Text("Mesh Count: %d", model.numMeshes());

	for (int iMesh = 0; iMesh < model.numMeshes(); ++iMesh) {
		ImGui::Text("\t%s", model.meshAt(iMesh)->name.c_str());
	}

	ImGui::Text("Material Count: %d", model.numMaterials());
	for (int iMtl = 0; iMtl < model.numMaterials(); ++iMtl) {
		ImGui::Text("\t%s", model.materialAt(iMtl)->name.c_str());
	}
}

void AssetsWindow::doPreviewAssetTexture2D(AssetIface_Texture2D* texIface) {
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
		} else {
			sgeAssertFalse("It is expected that explorePreviewAsset is a AssetTexture2d");
		}
	}
}

} // namespace sge
