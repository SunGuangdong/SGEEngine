#include <filesystem>

#include "../ActorCreateWindow.h"
#include "../AssetsUI/AssetsWindow.h"
#include "../CreditsWindow.h"
#include "../GameInspectorWindow.h"
#include "../GamePlayWindow.h"
#include "../HelpWindow.h"
#include "../InfoWindow.h"
#include "../LogWindow.h"
#include "../ModelPreviewWindow.h"
#include "../OutlinerWindow.h"
#include "../PrefabWindow.h"
#include "../ProjectSettingsWindow.h"
#include "../PropertyEditorWindow.h"
#include "../SceneWindow.h"
#include "../WorldSettingsWindow.h"
#include "EditorWindow.h"
#include "sge_core/AssetLibrary/AssetLibrary.h"
#include "sge_core/ICore.h"
#include "sge_core/QuickDraw.h"
#include "sge_core/SGEImGui.h"
#include "sge_core/application/application.h"
#include "sge_core/application/input.h"
#include "sge_core/ui/MultiCurve2DEditor.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_engine/GameDrawer/DefaultGameDrawer.h"
#include "sge_engine/GameDrawer/GameDrawer.h"
#include "sge_engine/GameSerialization.h"
#include "sge_engine/actors/ACRSpline.h"
#include "sge_engine/actors/ALight.h"
#include "sge_engine/actors/ALine.h"
#include "sge_utils/debugger/VSDebugger.h"
#include "sge_utils/io/FileStream.h"
#include "sge_utils/json/json.h"
#include "sge_utils/other/FileOpenDialog.h"
#include "sge_utils/stl_algorithm_ex.h"
#include "sge_utils/text/Path.h"
#include "sge_utils/text/format.h"

#include "IconsForkAwesome/IconsForkAwesome.h"

namespace sge {

void EditorWindow::Assets::load()
{
	AssetLibrary* assetLib = getCore()->getAssetLib();

	m_assetPlayIcon = assetLib->getAssetFromFile("assets/editor/textures/icons/play.png");
	m_assetForkPlayIcon = assetLib->getAssetFromFile("assets/editor/textures/icons/forkplay.png");
	m_assetPauseIcon = assetLib->getAssetFromFile("assets/editor/textures/icons/pause.png");
	m_assetOpenIcon = assetLib->getAssetFromFile("assets/editor/textures/icons/open.png");
	m_assetSaveIcon = assetLib->getAssetFromFile("assets/editor/textures/icons/save.png");
	m_assetRefreshIcon = assetLib->getAssetFromFile("assets/editor/textures/icons/refresh.png");
	m_assetRebuildIcon = assetLib->getAssetFromFile("assets/editor/textures/icons/rebuild.png");
	m_assetPickingIcon = assetLib->getAssetFromFile("assets/editor/textures/icons/pick.png");
	m_assetTranslationIcon = assetLib->getAssetFromFile("assets/editor/textures/icons/translation.png");
	m_assetRotationIcon = assetLib->getAssetFromFile("assets/editor/textures/icons/rotation.png");
	m_assetScalingIcon = assetLib->getAssetFromFile("assets/editor/textures/icons/scale.png");
	m_assetVolumeScaleIcon = assetLib->getAssetFromFile("assets/editor/textures/icons/volumeScale.png");
	m_assetSnapToGridOffIcon = assetLib->getAssetFromFile("assets/editor/textures/icons/snapToGridOff.png");
	m_assetSnapToGridOnIcon = assetLib->getAssetFromFile("assets/editor/textures/icons/snapToGridOn.png");

	m_showGameUIOnIcon = assetLib->getAssetFromFile("assets/editor/textures/icons/showGameUIOn.png");
	m_showGameUIOffIcon = assetLib->getAssetFromFile("assets/editor/textures/icons/showGameUIOff.png");

	m_orthoIcon = assetLib->getAssetFromFile("assets/editor/textures/icons/ortho.png");
	m_xIcon = assetLib->getAssetFromFile("assets/editor/textures/icons/x.png");
	m_yIcon = assetLib->getAssetFromFile("assets/editor/textures/icons/y.png");
	m_zIcon = assetLib->getAssetFromFile("assets/editor/textures/icons/z.png");
}

void EditorWindow::onGamePluginPreUnload()
{
	m_sceneInstances.clear();
	iActiveInstance = -1;
}

void EditorWindow::onGamePluginChanged()
{
	m_sceneWindow->setGameDrawer(m_gameDrawer.get());
}

EditorWindow::EditorWindow(WindowBase& nativeWindow, std::string windowName)
    : m_nativeWindow(nativeWindow)
    , m_windowName(std::move(windowName))
{
	m_gameDrawer.reset(new DefaultGameDrawer());

	getEngineGlobal()->subscribeOnPluginChange([this]() -> void { onGamePluginChanged(); }).abandon();

	sgeAssert(m_gameDrawer != nullptr);

	newScene();

	getEngineGlobal()->addWindow(new AssetsWindow(ICON_FK_FILE " Assets"));
	getEngineGlobal()->addWindow(new OutlinerWindow(ICON_FK_SEARCH " Outliner"));
	getEngineGlobal()->addWindow(new WorldSettingsWindow(ICON_FK_COGS " World Settings"));
	getEngineGlobal()->addWindow(new PropertyEditorWindow(ICON_FK_COG " Property Editor"));
	getEngineGlobal()->addWindow(new PrefabWindow("Prefabs"));
	m_sceneWindow = new SceneWindow(ICON_FK_GLOBE " Scene", m_gameDrawer.get());
	getEngineGlobal()->addWindow(m_sceneWindow);
	getEngineGlobal()->addWindow(new ActorCreateWindow("Actor Create"));

	getEngineGlobal()->addWindow(new LogWindow(ICON_FK_LIST " Log"));

	m_assets.load();

	loadEditorSettings();
}

void EditorWindow::loadEditorSettings()
{
	FileReadStream frs("editorSettings.json");
	if (frs.isOpened()) {
		m_rescentOpenedSceneFiles.clear();

		JsonParser jp;
		jp.parse(&frs);

		const JsonValue* jRoot = jp.getRoot();
		const JsonValue* jReascentFiles = jRoot->getMember("reascent_files");
		if (jReascentFiles) {
			for (int t = 0; t < jReascentFiles->arrSize(); ++t) {
				m_rescentOpenedSceneFiles.emplace_back(jReascentFiles->arrAt(t)->GetString());
			}
		}
	}
}

void EditorWindow::saveEditorSettings()
{
	JsonValueBuffer jvb;
	JsonValue* jRoot = jvb(JID_MAP);

	JsonValue* jReascentFiles = jvb(JID_ARRAY);

	for (const std::string filename : m_rescentOpenedSceneFiles) {
		jReascentFiles->arrPush(jvb(filename));
	}

	jRoot->setMember("reascent_files", jReascentFiles);

	JsonWriter jw;
	jw.WriteInFile("editorSettings.json", jRoot, true);
}

void EditorWindow::addReasecentScene(const char* const filename)
{
	if (filename == nullptr) {
		return;
	}

	// Update the list of reascently used files.
	auto itr = std::find(m_rescentOpenedSceneFiles.begin(), m_rescentOpenedSceneFiles.end(), filename);
	if (itr == m_rescentOpenedSceneFiles.end()) {
		// If the file isn't already in the list, push it to the front.
		push_front(m_rescentOpenedSceneFiles, std::string(filename));
	}
	else {
		// if the file is already in the list move it to the front.
		std::string x = std::move(*itr);
		m_rescentOpenedSceneFiles.erase(itr);
		m_rescentOpenedSceneFiles.insert(m_rescentOpenedSceneFiles.begin(), std::move(x));
	}
	saveEditorSettings();
}

int EditorWindow::newEmptyInstance()
{
	m_sceneInstances.emplace_back(PerSceneInstanceData());

	m_sceneInstances.back().sceneInstace = std::make_unique<SceneInstance>();
	m_sceneInstances.back().sceneInstace->newScene();
	m_sceneInstances.back().displayName = "New Level";

	return (int)m_sceneInstances.size() - 1;
}

void EditorWindow::switchToInstance(int iInstance)
{
	if (iInstance >= 0 && iInstance < m_sceneInstances.size()) {
		iActiveInstance = iInstance;
		m_gameDrawer->initialize(&getActiveInstance()->getWorld());
	}
}

void EditorWindow::deleteInstance(int iInstance)
{
	if (iInstance >= 0 && iInstance < m_sceneInstances.size()) {
		std::string message = string_format(
		    "Closing scene %s. All unsaved changes will be lost! Do you want to continue closing?",
		    getInstanceData(iInstance)->displayName.c_str());

		if (DialogYesNo("Closing Scene", message.c_str())) {
			m_sceneInstances.erase(m_sceneInstances.begin() + iInstance);
			int newActiveInst = (int)m_sceneInstances.size() - 1;

			if (newActiveInst >= 0) {
				switchToInstance(newActiveInst);
			}
		}
	}
}

SceneInstance* EditorWindow::getActiveInstance()
{
	if (iActiveInstance >= 0 && iActiveInstance < m_sceneInstances.size()) {
		return m_sceneInstances[iActiveInstance].sceneInstace.get();
	}

	return nullptr;
}

EditorWindow::PerSceneInstanceData* EditorWindow::getActiveInstanceData()
{
	return getInstanceData(iActiveInstance);
}

EditorWindow::PerSceneInstanceData* EditorWindow::getInstanceData(int iInstance)
{
	if (iInstance >= 0 && iInstance < m_sceneInstances.size()) {
		return &m_sceneInstances[iInstance];
	}

	return nullptr;
}

EditorWindow::~EditorWindow()
{
	sgeAssert(false);
}

void EditorWindow::newScene(bool UNUSED(forceKeepSameInspector))
{
	switchToInstance(newEmptyInstance());

	// m_sceneInstance.newScene(forceKeepSameInspector);
	// if (m_gameDrawer) {
	//	m_gameDrawer->initialize(&m_sceneInstance.getWorld());
	//}

	//// m_inspector.m_world->userProjectionSettings.aspectRatio = 1.f; // Just some default to be overriden.

	// for (auto& wnd : getEngineGlobal()->getAllWindows()) {
	//	wnd->onNewWorld();
	//}
}

void EditorWindow::loadWorldFromFile(
    const char* const filename, const char* overrideWorkingFilename, bool forceKeepSameInspector)
{
	std::vector<char> fileContents;
	if (FileReadStream::readFile(filename, fileContents)) {
		fileContents.push_back('\0');
		loadWorldFromJson(
		    fileContents.data(),
		    true,
		    overrideWorkingFilename != nullptr ? overrideWorkingFilename : filename,
		    forceKeepSameInspector);

		addReasecentScene(filename);

		return;
	}

	sgeAssert(false);
}

void EditorWindow::loadWorldFromJson(
    const char* const json, bool disableAutoSepping, const char* const workingFileName, bool loadInNewInstance)
{
	if (loadInNewInstance && getActiveInstance() == nullptr) {
		int iInstance = newEmptyInstance();
		switchToInstance(iInstance);
	}

	PerSceneInstanceData* sceneInstData = getActiveInstanceData();

	if (workingFileName) {
		sceneInstData->filename = workingFileName;
	}

	if (workingFileName) {
		sceneInstData->displayName = extractFileNameWithExt(workingFileName);
	}
	else {
		sceneInstData->displayName = "Unsaved Scene";
	}

	sceneInstData->sceneInstace->loadWorldFromJson(json, disableAutoSepping);
}

void EditorWindow::saveInstanceToFile(int iInstance, bool forceAskForFilename)
{
	std::string filename;

	PerSceneInstanceData* instData = getInstanceData(iInstance);

	if (!forceAskForFilename && instData->filename.empty() == false) {
		filename = instData->filename;
	}
	else {
		// In order for the File Save dialog to point to the default assets/levels directory
		// the directory must exist.
		std::filesystem::create_directories("./assets/levels");
		filename = FileSaveDialog("Save GameWorld to file...", "*.lvl\0*.lvl\0", "lvl", "./assets/levels");
		if (filename.empty() == false) {
			if (extractFileExtension(filename.c_str()).empty()) {
				filename += ".lvl";
			}

			// Check if the file exists if so, ask if we want to overwrite.
			FileReadStream frsCheckExist(filename.c_str());
			if (frsCheckExist.isOpened()) {
				if (DialogYesNo("Save Game World", "Are you sure you want to OVERWRITE the existing level?") == false) {
					filename.clear();
				}
			}
		}
	}

	saveInstanceToSpecificFile(iInstance, filename.c_str());
}

void EditorWindow::saveInstanceToSpecificFile(int iInstance, const char* filename)
{
	if (isStringEmpty(filename)) {
		sgeAssert(false && "saveWorldToSpecificFile expects a filename");
		return;
	}

	PerSceneInstanceData* instData = getInstanceData(iInstance);

	if (instData) {
		bool succeeded = instData->sceneInstace->saveWorldToFile(filename);
		if (succeeded) {
			instData->filename = filename;

			getEngineGlobal()->showNotification(string_format("SUCCEEDED saving '%s'", filename));
			sgeLogInfo("[SAVE_LEVEL] Saving game level succeeded. File is %s\n", filename);
			instData->filename = filename;
			instData->displayName = filename;

			addReasecentScene(filename);
		}
		else {
			std::string error = string_format("FAILED saving level '%s'", filename);
			getEngineGlobal()->showNotification(error);
			sgeLogWarn(error.c_str());
		}
	}
}


void EditorWindow::update(SGEContext* const sgecon, GameInspector* UNUSED(inspector), const InputState& is)
{
	m_timer.tick();

	const ImGuiWindowFlags mainWndFlags = ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoResize |
	                                      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_MenuBar;

	ImGui::SetNextWindowPos(ImVec2(0.f, 0.f));
	ImGui::SetNextWindowSize(ImVec2(float(m_nativeWindow.GetClientWidth()), float(m_nativeWindow.GetClientHeight())));
	ImGui::Begin(m_windowName.c_str(), nullptr, mainWndFlags);

	if (ImGui::BeginTabBar(
	        "SGEEdtorTabSceneInstances", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs)) {
		int iInstanceToDelete = -1;
		for (int iInst = 0; iInst < (int)m_sceneInstances.size(); ++iInst) {
			ImGuiEx::IDGuard idguard(iInst);

			int tabFlags = 0;

			if (iInst == iActiveInstance) {
				tabFlags |= ImGuiTabItemFlags_SetSelected;
			}

			bool isOpen = true;
			if (ImGui::BeginTabItem(m_sceneInstances[iInst].displayName.c_str(), &isOpen)) {
				if (iInst != iActiveInstance) {
					switchToInstance(iInst);
				}
				ImGui::EndTabItem();
			}

			if (isOpen == false) {
				iInstanceToDelete = iInst;
			}
		}

		if (iInstanceToDelete >= 0) {
			deleteInstance(iInstanceToDelete);
		}

		if (ImGui::TabItemButton("+ New", ImGuiTabItemFlags_Trailing)) {
			switchToInstance(newEmptyInstance());
		}

		ImGui::EndTabBar();
	}

	if (getActiveInstanceData() == nullptr) {
		ImGui::Text("Create a new scene from the tab meny above");
		ImGui::End();
		return;
	}

	// [SGE_EDITOR_MOUSE_CAPTURE_HOTKEY]
	// while editing some games might need the cursor to be relative
	// (hidden and not moving while getting move events) for games file first pesron shooters.
	// In order to test them we need to allow the game to capture the cursor, however
	// when done testing we need to be able to remove that capture and have a normal mouse cursor
	// behaviour to continue editing the game. By pressing F2 we toggle if the game is able or not
	// to capture the cursor.
	if (is.IsKeyPressed(Key::Key_F2)) {
		getEngineGlobal()->setEngineAllowingRelativeCursor(!getEngineGlobal()->getEngineAllowingRelativeCursor());
	}

	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu(ICON_FK_FILE " File")) {
			if (ImGui::MenuItem(ICON_FK_FILE " New Scene")) {
				if (DialogYesNo("Open GameWorld", "Are you sure? All unsaved data WILL BE LOST?")) {
					newScene();
				}
			}

			if (ImGui::MenuItem(ICON_FK_FOLDER_OPEN " Open Scene")) {
				if (DialogYesNo("Open GameWorld", "Are you sure? All unsaved data WILL BE LOST?")) {
					// In order for the file save dialong to be opned in the default assets/levels directory
					// the directory must exist.
					std::filesystem::create_directories("./assets/levels");
					const std::string filename =
					    FileOpenDialog("Load GameWorld form file...", true, "*.lvl\0*.lvl\0", "./assets/levels");

					if (!filename.empty()) {
						loadWorldFromFile(filename.c_str(), filename.c_str(), true);
					}
				}
			}

			if (ImGui::BeginMenu(ICON_FK_FOLDER_OPEN " Open Scene...")) {
				static std::vector<std::string> levelsList;

				if (ImGui::IsWindowAppearing()) {
					levelsList.clear();
					// Fild all filels in dir. and store them in levelsList
					if (std::filesystem::is_directory("./assets/levels")) {
						for (const auto& entry : std::filesystem::directory_iterator("./assets/levels")) {
							levelsList.emplace_back(entry.path().string());
						}
					}
				}

				if (levelsList.empty()) {
					ImGui::Text(ICON_FK_EXCLAMATION_TRIANGLE " No files in assets/levels.");
				}
				else {
					for (const auto& levelFile : levelsList) {
						if (ImGui::MenuItem(levelFile.c_str())) {
							loadWorldFromFile(levelFile.c_str(), nullptr, true);
						}
					}
				}
				ImGui::EndMenu();
			}

			if (ImGui::MenuItem(ICON_FK_FLOPPY_O " Save")) {
				saveInstanceToFile(iActiveInstance, false);
			}

			if (ImGui::MenuItem(ICON_FK_FLOPPY_O " Save As...")) {
				saveInstanceToFile(iActiveInstance, true);
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(ICON_FK_WINDOW_RESTORE " Window")) {
			if (ImGui::MenuItem(ICON_FK_FAX " Start-up Dialogue")) {
				m_isWelcomeWindowOpened = true;
			}

			ImGui::Separator();

			if (ImGui::MenuItem(ICON_FK_FILE " Assets")) {
				AssetsWindow* wnd = getEngineGlobal()->findFirstWindowOfType<AssetsWindow>();
				if (wnd) {
					ImGui::SetWindowFocus(wnd->getWindowName());
				}
				else {
					getEngineGlobal()->addWindow(new AssetsWindow(ICON_FK_FILE " Assets"));
				}
			}

			if (ImGui::MenuItem(ICON_FK_SEARCH " Outliner")) {
				OutlinerWindow* wnd = getEngineGlobal()->findFirstWindowOfType<OutlinerWindow>();
				if (wnd) {
					ImGui::SetWindowFocus(wnd->getWindowName());
				}
				else {
					getEngineGlobal()->addWindow(new OutlinerWindow(ICON_FK_SEARCH " Outliner"));
				}
			}

			if (ImGui::MenuItem(ICON_FK_COGS " World Settings")) {
				WorldSettingsWindow* wnd = getEngineGlobal()->findFirstWindowOfType<WorldSettingsWindow>();
				if (wnd) {
					ImGui::SetWindowFocus(wnd->getWindowName());
				}
				else {
					getEngineGlobal()->addWindow(new WorldSettingsWindow(ICON_FK_COGS " World Settings"));
				}
			}

			if (ImGui::MenuItem(ICON_FK_COGS " Property Editor")) {
				PropertyEditorWindow* wnd = getEngineGlobal()->findFirstWindowOfType<PropertyEditorWindow>();
				if (wnd) {
					ImGui::SetWindowFocus(wnd->getWindowName());
				}
				else {
					getEngineGlobal()->addWindow(new PropertyEditorWindow("Propery Editor"));
				}
			}

			if (ImGui::MenuItem("Actor Create")) {
				ActorCreateWindow* wnd = getEngineGlobal()->findFirstWindowOfType<ActorCreateWindow>();
				if (wnd) {
					ImGui::SetWindowFocus(wnd->getWindowName());
				}
				else {
					getEngineGlobal()->addWindow(new ActorCreateWindow("Actor Create"));
				}
			}

			if (ImGui::MenuItem("Prefabs")) {
				PrefabWindow* wnd = getEngineGlobal()->findFirstWindowOfType<PrefabWindow>();
				if (wnd) {
					ImGui::SetWindowFocus(wnd->getWindowName());
				}
				else {
					getEngineGlobal()->addWindow(new PrefabWindow("Prefabs"));
				}
			}

			if (ImGui::MenuItem("Game Play")) {
				GamePlayWindow* gameplayWindow = getEngineGlobal()->findFirstWindowOfType<GamePlayWindow>();

				if (gameplayWindow == nullptr) {
					JsonValueBuffer jvb;
					JsonValue* jWorld = serializeGameWorld(&getActiveInstance()->getWorld(), jvb);
					JsonWriter jw;
					WriteStdStringStream stringStream;
					jw.write(&stringStream, jWorld);

					gameplayWindow = new GamePlayWindow("Game Play", stringStream.serializedString.c_str());
					getEngineGlobal()->addWindow(gameplayWindow);
				}
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(ICON_FK_BUG " Debug")) {
			if (ImGui::MenuItem(ICON_FK_LIST " Log")) {
				LogWindow* wnd = getEngineGlobal()->findFirstWindowOfType<LogWindow>();
				if (wnd) {
					ImGui::SetWindowFocus(wnd->getWindowName());
				}
				else {
					getEngineGlobal()->addWindow(new LogWindow(ICON_FK_LIST " Log"));
				}
			}

			if (ImGui::MenuItem("Model Preview")) {
				getEngineGlobal()->addWindow(new ModelPreviewWindow("Model Preview"));
			}

			if (ImGui::MenuItem("Info")) {
				getEngineGlobal()->addWindow(new InfoWindow("Info"));
			}

			if (ImGui::MenuItem("Game Inspector Window")) {
				getEngineGlobal()->addWindow(new InfoWindow("Info"));
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(ICON_FK_BOOK " Help")) {
			if (ImGui::MenuItem(ICON_FK_BOOK " Keyboard Shortucts and Docs")) {
				HelpWindow* wnd = getEngineGlobal()->findFirstWindowOfType<HelpWindow>();
				if (wnd) {
					ImGui::SetWindowFocus(wnd->getWindowName());
				}
				else {
					getEngineGlobal()->addWindow(new HelpWindow(ICON_FK_BOOK " Keyboard Shortucts and Docs"));
				}
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Credits")) {
				CreditsWindow* wnd = getEngineGlobal()->findFirstWindowOfType<CreditsWindow>();
				if (wnd) {
					ImGui::SetWindowFocus(wnd->getWindowName());
				}
				else {
					getEngineGlobal()->addWindow(new CreditsWindow("Credits"));
				}
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(ICON_FK_DOWNLOAD " Run & Export")) {
			ProjectSettingsWindow* wnd = getEngineGlobal()->findFirstWindowOfType<ProjectSettingsWindow>();
			if (wnd) {
				ImGui::SetWindowFocus(wnd->getWindowName());
			}
			else {
				getEngineGlobal()->addWindow(new ProjectSettingsWindow(ICON_FK_PUZZLE_PIECE " Run & Export"));
			}

			ImGui::EndMenu();
		}

		if (isVSDebuggerPresent()) {
			ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), "Debugging");
		}

		ImGui::EndMenuBar();
	}

	// Show the notifications in a small window at the bottom right corner.
	if (getEngineGlobal()->getNotificationCount() != 0) {
		const int notificationWndFlags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar |
		                                 ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
		                                 ImGuiWindowFlags_NoFocusOnAppearing;

		if (ImGui::Begin("SGENotificationWindow", nullptr, notificationWndFlags)) {
			for (int t = 0; t < getEngineGlobal()->getNotificationCount(); ++t) {
				ImGui::TextUnformatted(getEngineGlobal()->getNotificationText(t));
			}

			const ImGuiIO& io = ImGui::GetIO();
			ImVec2 const windowSize = ImGui::GetWindowSize();
			ImVec2 pos;
			pos.x = io.DisplaySize.x - windowSize.x - io.DisplaySize.x * 0.01f;
			pos.y = io.DisplaySize.y - windowSize.y - io.DisplaySize.y * 0.01f;

			ImGui::SetWindowPos(pos);
		}
		ImGui::End();
	}

	// Update the game world.
	InputState isSceneWindowDomain = m_sceneWindow->computeInputStateInDomainSpace(is);
	getActiveInstance()->update(m_timer.diff_seconds(), isSceneWindowDomain);

	// Rendering.
	sgecon->clearDepth(getCore()->getDevice()->getWindowFrameTarget(), 1.f);

	const auto imageButton = [](const AssetPtr& asset) -> bool {
		const AssetIface_Texture2D* texIface = getLoadedAssetIface<AssetIface_Texture2D>(asset);
		if (texIface && texIface->getTexture()) {
			bool isPressed = ImGui::ImageButton(texIface->getTexture(), ImVec2(24, 24));
			return isPressed;
		}
		return false;
	};


	if (ImGui::BeginChild(
	        "Toolbar", ImVec2(0, 48), true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar)) {
		if (m_assets.m_assetPlayIcon && getActiveInstance()->getInspector().m_disableAutoStepping) {
			if (imageButton(m_assets.m_assetPlayIcon))
				getActiveInstance()->getInspector().m_disableAutoStepping = false;
		}
		else if (imageButton(m_assets.m_assetPauseIcon)) {
			getActiveInstance()->getInspector().m_disableAutoStepping = true;
		}


		ImGui::SameLine();

		if (imageButton(m_assets.m_assetPickingIcon)) {
			getActiveInstance()->getInspector().setTool(&getActiveInstance()->getInspector().m_selectionTool);
		}
		ImGuiEx::TextTooltip("Enables the scene selection tool.\nShortcut: Q");

		ImGui::SameLine();

		if (imageButton(m_assets.m_assetTranslationIcon)) {
			getActiveInstance()->getInspector().m_transformTool.m_mode = Gizmo3D::Mode_Translation;
			getActiveInstance()->getInspector().setTool(&getActiveInstance()->getInspector().m_transformTool);
		}
		ImGuiEx::TextTooltip("Enables the actor move tool.\nShortcut: W");

		ImGui::SameLine();

		if (imageButton(m_assets.m_assetRotationIcon)) {
			getActiveInstance()->getInspector().m_transformTool.m_mode = Gizmo3D::Mode_Rotation;
			getActiveInstance()->getInspector().setTool(&getActiveInstance()->getInspector().m_transformTool);
		}
		ImGuiEx::TextTooltip("Enables the actor rotation tool.\nShortcut: E");

		ImGui::SameLine();

		if (imageButton(m_assets.m_assetScalingIcon)) {
			getActiveInstance()->getInspector().m_transformTool.m_mode = Gizmo3D::Mode_Scaling;
			getActiveInstance()->getInspector().setTool(&getActiveInstance()->getInspector().m_transformTool);
		}
		ImGuiEx::TextTooltip("Enables the actor scaling tool\nShortcut: R.");

		ImGui::SameLine();

		if (imageButton(m_assets.m_assetVolumeScaleIcon)) {
			getActiveInstance()->getInspector().m_transformTool.m_mode = Gizmo3D::Mode_ScaleVolume;
			getActiveInstance()->getInspector().setTool(&getActiveInstance()->getInspector().m_transformTool);
		}
		ImGuiEx::TextTooltip("Enables the actor box-scaling tool.\nShortcut: Ctrl + R");

		ImGui::SameLine();

		if (imageButton(m_assets.m_assetForkPlayIcon)) {
			GamePlayWindow* oldGameplayWindow = getEngineGlobal()->findFirstWindowOfType<GamePlayWindow>();
			if (oldGameplayWindow != nullptr) {
				getEngineGlobal()->removeWindow(oldGameplayWindow);
			}

			JsonValueBuffer jvb;
			JsonValue* jWorld = serializeGameWorld(&getActiveInstance()->getWorld(), jvb);
			JsonWriter jw;
			WriteStdStringStream stringStream;
			jw.write(&stringStream, jWorld);
			getEngineGlobal()->addWindow(new GamePlayWindow("Game Play", stringStream.serializedString.c_str()));
		}
		ImGuiEx::TextTooltip("Play the level in isolation.");

		ImGui::SameLine();
		ImGui::Separator();
		ImGui::SameLine();

		if (getActiveInstance()->getInspector().m_transformTool.m_useSnapSettings) {
			if (imageButton(m_assets.m_assetSnapToGridOnIcon)) {
				getActiveInstance()->getInspector().m_transformTool.m_useSnapSettings = false;
			}
		}
		else {
			if (imageButton(m_assets.m_assetSnapToGridOffIcon)) {
				getActiveInstance()->getInspector().m_transformTool.m_useSnapSettings = true;
			}
		}
		ImGuiEx::TextTooltip("Toggle on/off snapping for tools.");

		ImGui::SameLine();

		ImGui::SameLine();
		ImGui::Separator();
		ImGui::SameLine();

		if (imageButton(m_assets.m_orthoIcon)) {
			getActiveInstance()->getWorld().m_editorCamera.isOrthograhpic =
			    !getActiveInstance()->getWorld().m_editorCamera.isOrthograhpic;
		}
		ImGuiEx::TextTooltip("Toggle the orthographic/perspective mode of the preview camera.");

		ImGui::SameLine();
		if (imageButton(m_assets.m_xIcon)) {
			getActiveInstance()->getWorld().m_editorCamera.m_orbitCamera.yaw = 0.f;
			getActiveInstance()->getWorld().m_editorCamera.m_orbitCamera.pitch = 0.f;
		}
		ImGuiEx::TextTooltip("Align the preview camera to +X axis.");

		ImGui::SameLine();
		if (imageButton(m_assets.m_yIcon)) {
			getActiveInstance()->getWorld().m_editorCamera.m_orbitCamera.yaw =
			    deg2rad(90.f) *
			    float(int(getActiveInstance()->getWorld().m_editorCamera.m_orbitCamera.yaw / deg2rad(90.f)));
			getActiveInstance()->getWorld().m_editorCamera.m_orbitCamera.pitch = deg2rad(90.f);
		}
		ImGuiEx::TextTooltip("Align the preview camera to +Y axis.");

		ImGui::SameLine();
		if (imageButton(m_assets.m_zIcon)) {
			getActiveInstance()->getWorld().m_editorCamera.m_orbitCamera.yaw = deg2rad(-90.f);
			getActiveInstance()->getWorld().m_editorCamera.m_orbitCamera.pitch = 0.f;
		}
		ImGuiEx::TextTooltip("Align the preview camera to +Z axis.");

		ImGui::SameLine();
		ImGui::Separator();
		ImGui::SameLine();
#if 1
		if (getActiveInstance()->getInspector().editMode == editMode_points) {
			// Todo: Reenable this!
			if (ImGui::Button("Tessalate")) {
				std::map<ALine*, std::vector<int>> tessalateBetweenIndices;
				std::map<ACRSpline*, std::vector<int>> tessalateBetweenIndicesCRSpline;

				for (int t = 0; t < getActiveInstance()->getInspector().m_selection.size(); ++t) {
					ALine* const spline = getActiveInstance()->getWorld().getActor<ALine>(
					    getActiveInstance()->getInspector().m_selection[t].objectId);
					if (spline) {
						tessalateBetweenIndices[spline].push_back(
						    getActiveInstance()->getInspector().m_selection[t].index);
					}

					ACRSpline* const crSpline = getActiveInstance()->getWorld().getActor<ACRSpline>(
					    getActiveInstance()->getInspector().m_selection[t].objectId);
					if (crSpline) {
						tessalateBetweenIndicesCRSpline[crSpline].push_back(
						    getActiveInstance()->getInspector().m_selection[t].index);
					}
				}

				for (auto& itr : tessalateBetweenIndices) {
					if (itr.second.size() > 1) {
						ASplineAddPoints* cmd = new ASplineAddPoints();
						cmd->setup(itr.first, itr.second);
						getActiveInstance()->getInspector().appendCommand(cmd, true);
					}
				}

				for (auto& itr : tessalateBetweenIndicesCRSpline) {
					if (itr.second.size() > 1) {
						ACRSplineAddPoints* cmd = new ACRSplineAddPoints();
						cmd->setup(itr.first, itr.second);
						getActiveInstance()->getInspector().appendCommand(cmd, true);
					}
				}
			}
		}
#endif
	}
	ImGui::EndChild();

	ImGuiID dockSpaceID = ImGui::GetID("SGE_CentralDock");

	// Check if ImGui loaded any layout from the imgui_layout_cache.ini file.
	// If so we assume that this is not the 1st run of the editor and the user has configured the windows
	// the way they like. Otherwise, we assume this is first use and load the default layout.
	if (ImGui::DockBuilderGetNode(dockSpaceID) == NULL) {
		const ImVec2 avilableSpace = ImGui::GetContentRegionAvail();

		ImGui::DockBuilderRemoveNode(dockSpaceID); // clear any previous layout
		ImGui::DockBuilderAddNode(dockSpaceID, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockSpaceID, avilableSpace);

		auto dock_id_left = ImGui::DockBuilderSplitNode(dockSpaceID, ImGuiDir_Left, 0.2f, nullptr, &dockSpaceID);
		auto dock_id_left_down = ImGui::DockBuilderSplitNode(dock_id_left, ImGuiDir_Down, 0.2f, nullptr, &dock_id_left);
		auto dock_id_down = ImGui::DockBuilderSplitNode(dockSpaceID, ImGuiDir_Down, 0.25f, nullptr, &dockSpaceID);
		auto dock_id_right = ImGui::DockBuilderSplitNode(dockSpaceID, ImGuiDir_Right, 0.25f, nullptr, &dockSpaceID);

		ImGui::DockBuilderDockWindow(ICON_FK_GLOBE " Scene", dockSpaceID);

		ImGui::DockBuilderDockWindow(ICON_FK_FILE " Assets", dock_id_down);
		ImGui::DockBuilderDockWindow("Prefabs", dock_id_down);
		ImGui::DockBuilderDockWindow("Actor Create", dock_id_down);
		ImGui::DockBuilderDockWindow(ICON_FK_LIST " Log", dock_id_down);

		ImGui::DockBuilderDockWindow(ICON_FK_SEARCH " Outliner", dock_id_left);
		ImGui::DockBuilderDockWindow("Tool Settings", dock_id_left_down);

		ImGui::DockBuilderDockWindow(ICON_FK_COGS " World Settings", dock_id_right);
		ImGui::DockBuilderDockWindow(ICON_FK_COG " Property Editor", dock_id_right);

		ImGui::DockBuilderFinish(dockSpaceID);
	}
	else {
		ImGui::DockSpace(
		    dockSpaceID, ImVec2(0.f, 0.f), ImGuiDockNodeFlags_None | ImGuiDockNodeFlags_PassthruCentralNode);
	}

	ImGui::End();

	if (m_isWelcomeWindowOpened && ImGui::Begin(
	                                   "Welcome Window",
	                                   &m_isWelcomeWindowOpened,
	                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
	                                       ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::SetWindowFontScale(2.f);
		ImGui::TextColored(ImVec4(0.25f, 1.f, 0.63f, 1.f), "Welcome to SGEEditor");
		ImGui::SetWindowFontScale(1.f);
		ImGui::NewLine();

		if (m_rescentOpenedSceneFiles.empty()) {
			ImGui::Text("Your reascenlty opened scene files will show here!");
		}

		for (int t = 0; t < minOf(int(m_rescentOpenedSceneFiles.size()), 7); ++t) {
			const std::string& levelFile = m_rescentOpenedSceneFiles[t];
			bool isSelected = false;

			if (ImGui::Selectable(levelFile.c_str(), &isSelected)) {
				std::string filenameToOpen = levelFile;
				loadWorldFromFile(filenameToOpen.c_str(), filenameToOpen.c_str(), false);
				m_isWelcomeWindowOpened = false;

				// Caution: we need to break the loop here as the loadWorldFromFile() would modify
				// m_rescentOpenedSceneFiles.
				break;
			}

			if (t == 0) {
				ImGui::NewLine();
				ImGui::Text("Other reascent Files:");
			}
		}

		ImGui::NewLine();

		if (ImGui::Button(ICON_FK_BOOK " Keyboard shortcuts and Help")) {
			HelpWindow* const wnd = getEngineGlobal()->findFirstWindowOfType<HelpWindow>();
			if (wnd) {
				ImGui::SetWindowFocus(wnd->getWindowName());
			}
			else {
				getEngineGlobal()->addWindow(new HelpWindow(ICON_FK_BOOK " Keyboard Shortucts and Docs"));
			}

			m_isWelcomeWindowOpened = false;
		}

		ImGui::SameLine();

		if (ImGui::Button("Close")) {
			m_isWelcomeWindowOpened = false;
		}

		// If the user clicks somewhere outside of the start-up window, close it.
		const bool isWindowOrItsChildsHovered =
		    ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) ||
		    ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
		if (!isWindowOrItsChildsHovered && ImGui::IsMouseClicked(0)) {
			m_isWelcomeWindowOpened = false;
		}

		const ImGuiIO& io = ImGui::GetIO();
		ImVec2 const windowSize = ImGui::GetWindowSize();
		ImVec2 pos;
		pos.x = io.DisplaySize.x * 0.5f - windowSize.x * 0.5f;
		pos.y = io.DisplaySize.y * 0.5f - windowSize.y * 0.5f;

		ImGui::SetWindowPos(pos);
		ImGui::End();
	}

	if (is.isKeyCombo(Key::Key_LCtrl, Key::Key_Z)) {
		getActiveInstance()->getInspector().undoCommand();
	}

	if (is.isKeyCombo(Key::Key_LCtrl, Key::Key_Y)) {
		getActiveInstance()->getInspector().redoCommand();
	}

	// Update the existing windows. if on the previous update the window was closed remove it.
	// Note that we do not remove the window on the frame it was closed, as ImGui could have generated
	// command which use data that the window provided to its draw calls (textures for example).
	// If the window is delete and imgui tries to access such data, the executable will crash (or worse).
	for (int iWnd = 0; iWnd < getEngineGlobal()->getAllWindows().size(); ++iWnd) {
		IImGuiWindow* const wnd = getEngineGlobal()->getAllWindows()[iWnd].get();
		sgeAssert(wnd && "This pointer should never be null");

		if (wnd->isClosed()) {
			getEngineGlobal()->removeWindow(wnd);
			iWnd -= 1;
			continue;
		}

		wnd->update(sgecon, &getActiveInstance()->getInspector(), is);
	}
}
void EditorWindow::prepareForHotReload(SceneInstanceSerializedData& sceneInstancesData)
{
	sceneInstancesData = SceneInstanceSerializedData();

	sceneInstancesData.iActiveInstance = iActiveInstance;

	for (PerSceneInstanceData& instace : m_sceneInstances) {
		SceneInstanceSerializedData::PerInstanceSerializedData instanceData;

		instanceData.displayName = instace.displayName;
		instanceData.filename = instace.filename;
		instanceData.levelJson = serializeGameWorld(&instace.sceneInstace->getWorld());

		sceneInstancesData.instances.emplace_back(std::move(instanceData));
	}

	for (std::unique_ptr<IImGuiWindow>& wnd : getEngineGlobal()->getAllWindows()) {
		if (dynamic_cast<GamePlayWindow*>(wnd.get())) {
			wnd->close();
		}
	}

	// Clear all the instances, deallocated and so on, the game plugin is going to get unloaded
	// and all function pointers will be invalidated.
	iActiveInstance = -1;
	m_sceneInstances.clear();

	// TODO: Close all gameplay windows.
}

void EditorWindow::recoverFromHotReload(const SceneInstanceSerializedData& sceneInstancesData)
{
	for (int iInst = 0; iInst < sceneInstancesData.instances.size(); ++iInst) {
		const SceneInstanceSerializedData::PerInstanceSerializedData& instData = sceneInstancesData.instances[iInst];

		int newInstIndex = newEmptyInstance();
		sgeAssert(newInstIndex == iInst);

		PerSceneInstanceData* inst = getInstanceData(newInstIndex);

		inst->displayName = instData.displayName;
		inst->filename = instData.filename;
		loadGameWorldFromString(&inst->sceneInstace->getWorld(), instData.levelJson.c_str());
	}

	switchToInstance(sceneInstancesData.iActiveInstance);
}


} // namespace sge
