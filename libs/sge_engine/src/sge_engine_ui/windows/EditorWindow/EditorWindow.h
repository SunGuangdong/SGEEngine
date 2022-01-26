#pragma once

#include <string>

#include "../IImGuiWindow.h"
#include "EditorWindowHotReloadData.h"
#include "imgui/imgui.h"
#include "sge_core/AssetLibrary/AssetLibrary.h"
#include "sge_engine/GameDrawer/GameDrawer.h"
#include "sge_engine/SceneInstance.h"
#include "sge_utils/utils/timer.h"

namespace sge {
struct WindowBase;
struct InputState;
struct SceneWindow;

/// EditorWindow is the root window of the whole application.
/// It is unique in the application it hosts all the other windows.
struct SGE_ENGINE_API EditorWindow : public IImGuiWindow {
  private:
	struct Assets {
		AssetPtr m_assetForkPlayIcon;
		AssetPtr m_assetPlayIcon;
		AssetPtr m_assetPauseIcon;
		AssetPtr m_assetOpenIcon;
		AssetPtr m_assetSaveIcon;
		AssetPtr m_assetRefreshIcon;
		AssetPtr m_assetRebuildIcon;

		AssetPtr m_assetPickingIcon;
		AssetPtr m_assetTranslationIcon;
		AssetPtr m_assetRotationIcon;
		AssetPtr m_assetScalingIcon;
		AssetPtr m_assetVolumeScaleIcon;

		AssetPtr m_assetSnapToGridOffIcon;
		AssetPtr m_assetSnapToGridOnIcon;

		AssetPtr m_showGameUIOnIcon;
		AssetPtr m_showGameUIOffIcon;

		AssetPtr m_orthoIcon;
		AssetPtr m_xIcon;
		AssetPtr m_yIcon;
		AssetPtr m_zIcon;

		void load();
	};

  public:
	EditorWindow(WindowBase& nativeWindow, std::string windowName);
	~EditorWindow();

	void onGamePluginPreUnload();
	void onGamePluginChanged();

	bool isClosed() override {
		return false;
	}

	void update(SGEContext* const sgecon, GameInspector* inspector, const InputState& is) override;
	const char* getWindowName() const override {
		return m_windowName.c_str();
	}

	void newScene(bool forceKeepSameInspector = false);
	void loadWorldFromFile(const char* const filename, const char* overrideWorkingFilename, bool loadInNewInstance);
	void loadWorldFromJson(const char* const json, bool disableAutoSepping, const char* const workingFileName, bool loadInNewInstance);
	void saveInstanceToFile(int iInstance, bool forceAskForFilename);
	void saveInstanceToSpecificFile(int iInstance, const char* filename);

	void prepareForHotReload(SceneInstanceSerializedData& sceneInstancesData);
	void recoverFromHotReload(const SceneInstanceSerializedData& sceneInstancesData);

	void loadEditorSettings();
	void saveEditorSettings();
	void addReasecentScene(const char* const filename);

	void closeWelcomeWindow() {
		m_isWelcomeWindowOpened = false;
	}

	int newEmptyInstance();
	void switchToInstance(int iInstance);
	void deleteInstance(int iInstance);

	SceneInstance* getActiveInstance();

	struct PerSceneInstanceData {
		std::string filename;
		std::string displayName;
		std::unique_ptr<SceneInstance> sceneInstace;
	};

	PerSceneInstanceData* getActiveInstanceData();
	PerSceneInstanceData* getInstanceData(int iInstance);

  private:
	Assets m_assets;

	WindowBase& m_nativeWindow;
	int iActiveInstance = -1;
	std::vector<PerSceneInstanceData> m_sceneInstances;
	std::unique_ptr<IGameDrawer> m_gameDrawer;
	std::string m_windowName;

	SceneWindow* m_sceneWindow = nullptr;

	Timer m_timer;

	bool m_isWelcomeWindowOpened = true;

	std::vector<std::string> m_rescentOpenedSceneFiles;
};
} // namespace sge
