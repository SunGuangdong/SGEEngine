#pragma once

#include "../IImGuiWindow.h"
#include "../ModelPreviewWindow.h"
#include "imgui/imgui.h"
#include "sgeImportModel3DFile.h"
#include "sge_core/AssetLibrary/AssetLibrary.h"
#include "sge_utils/DLL/DLLHandler.h"
#include <filesystem>
#include <string>

namespace sge {
struct InputState;
struct FrameTarget;

/// AssetsWindow is a window in the engine interface for exploring and importing assets.
struct SGE_ENGINE_API AssetsWindow : public IImGuiWindow {
  private:
	struct AssetImportData {
		bool importFailed = false;
		/// The full path to the file we are about to import.
		std::string fileToImportPath;
		AssetIfaceType assetType;

		/// The directory where the imported file(s) will be.
		std::string outputDir;
		/// The filename with extension, of the output file.
		std::string outputFilename;

		bool importModelsAsMultipleFiles = false;
		bool preview = true;
		ModelPreviewWidget modelPreviewWidget;

		AssetPtr tempAsset;
	};

  public:
	AssetsWindow(std::string windowName);

	void close() override
	{
		m_isOpened = false;
	}

	bool isClosed() override
	{
		return !m_isOpened;
	}

	Texture* getThumbnailForAsset(const std::string& localAssetPath);
	Texture* getThumbnailForModel3D(const std::string& localAssetPath);
	void update(SGEContext* const sgecon, GameInspector* inspector, const InputState& is) override;
	void openMaterialEditWindow(std::shared_ptr<sge::AssetIface_Material> mtlIface);
	const char* getWindowName() const override
	{
		return m_windowName.c_str();
	}

	//
	void openAssetImportPopUp();
	void openAssetImportPopUp_importFile(const std::string& fileNameToImport);
	void openAssetImportPopUp_importOverFile(const std::string& importOverFilename);

  private:
	void doImportPopUp();

	/// @brief Imports the specified asset with the specified settings.
	bool importAsset(AssetImportData& aid);

	void doPreviewAssetModel(const InputState& is, bool explorePreviewAssetChanged);
	void doPreviewAssetTexture2D(AssetIface_Texture2D* texIface);
	void doPreviewAssetMaterial(AssetIface_Material* texIface);

	/// Returns the current path that is being explored in the editor.
	std::filesystem::path getCurrentExplorePath() const;

  private:
	/// True if the Import Asset pop up should appear.
	bool shouldOpenImportPopup = false;

	struct AssetImportPopupInitState {
		/// If specified, when 1st opened, this will be the default filename
		/// that will be shown as a file being imported.
		/// Otherwise the textfield will be empty.
		std::string initialFilePathToImport;

		/// May be empty. if specified the, imported filename will be this one.
		std::string forcedImportedFilename;
	};

	/// When opening the Asset Import popup sometimes we want to specify some
	/// inital state to it.
	/// Maybe the user has drag-dropped a file over the window and wants to import it
	/// Maybe the user wants to override an existing asset.
	AssetImportPopupInitState importPopUpInitState;

	bool m_isOpened = true;
	std::string m_windowName;

	/// The path to the currently right clicked object. If the path is empty there is no right clicked object and we should not display the
	/// right click menu.
	std::filesystem::path m_rightClickedPath;

	// A pointer to the asset that currently has a preview.
	AssetPtr explorePreviewAsset;

	/// When in explorer the user has selected a 3d model this widget is used to draw the preview.
	ModelPreviewWidget m_exploreModelPreviewWidget;
	/// The path to the current directory that we show in the explore.
	std::vector<std::string> directoryTree;
	/// A filter for searching in the current directory in the explorer.
	ImGuiTextFilter m_exploreFilter;

	AssetImportData m_importAssetToImportInPopup;
	std::vector<AssetImportData> m_assetsToImport;

	/// A reference to the dll that holds the mdlconvlib that is used to convert 3d models.
	/// it might be missing the the user hasn't specified the FBX SDK path.
	DLLHandler mdlconvlibHandler;

	/// A pointer to the function from mdlconvlib (if available) for importing 3D models (fbx, obj, dae).
	sgeImportModel3DFileFn m_sgeImportFBXFile = nullptr;
	/// A pointer to the function from mdlconvlib (if available) for importing 3D files as multiple models (fbx, dae).
	sgeImportModel3DFileAsMultipleFn m_sgeImportFBXFileAsMultiple = nullptr;


	std::map<std::string, GpuHandle<FrameTarget>> assetPreviewTex;
};
} // namespace sge
