#if 1
    /// This thing makes the driver to auto-choose the high-end GPU instead of the "integrated" one.
	#ifdef _WIN32
extern "C" {
__declspec(dllexport) int NvOptimusEnablement = 0x00000001;
}
extern "C" {
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
	#endif
#endif

#ifdef __EMSCRIPTEN__
	#include <emscripten.h>
#endif

#include "../exe/DummyPlugin.h"
#include "IconsForkAwesome/IconsForkAwesome.h"
#include "sge_core/AssetLibrary/AssetLibrary.h"
#include "sge_core/ICore.h"
#include "sge_core/QuickDraw/QuickDraw.h"
#include "sge_core/SGEImGui.h"
#include "sge_core/application/application.h"
#include "sge_core/setImGuiContexCore.h"
#include "sge_core/typelib/typeLib.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_engine/IPlugin.h"
#include "sge_engine/setImGuiContextEngine.h"
#include "sge_engine_ui/windows/AssetsUI/AssetsWindow.h"
#include "sge_engine_ui/windows/EditorWindow/EditorWindow.h"
#include "sge_log/Log.h"
#include "sge_utils/DLL/DLLHandler.h"
#include "sge_utils/io/FileStream.h"
#include "sge_utils/json/json.h"
#include "sge_utils/other/FileOpenDialog.h"
#include "sge_utils/text/Path.h"
#include <filesystem>
#include <thread>

#include "MiniDump.h"

using namespace sge;

int g_argc = 0;
char** g_argv = nullptr;

struct SGEGameWindow : public WindowBase {
	Timer m_timer;
	std::string pluginFileName;
	DLLHandler m_dllHandler;
	sint64 m_workingDLLModTime = 0;
	IPlugin* m_pluginInst = nullptr;

	DummyPlugin dummyPlugin;

	vec2i cachedWindowSize = vec2i(0);

	void HandleEvent(const WindowEvent event, const void* const eventData) final
	{
		if (event == WE_Create) {
			OnCreate();
		}

		if (event == WE_Resize) {
			const WE_Resize_Data& resizeData = *static_cast<const WE_Resize_Data*>(eventData);
			if (resizeData.width > 0)
				cachedWindowSize.x = resizeData.width;
			if (resizeData.height > 0)
				cachedWindowSize.y = resizeData.height;

			SGEDevice* device = getCore()->getDevice();

			if (device) {
				const WE_Resize_Data& data = *(const WE_Resize_Data*)eventData;
				getCore()->getDevice()->resizeBackBuffer(data.width, data.height);
				SGEImGui::setViewport(Rect2s(short(data.width), short(data.height)));
			}
		}

		if (event == WE_Destroying) {
			JsonValueBuffer jvb;
			JsonValue* jRoot = jvb(JID_MAP);

			jRoot->setMember("window_width", jvb(cachedWindowSize.x));
			jRoot->setMember("window_height", jvb(cachedWindowSize.y));
			jRoot->setMember("is_maximized", jvb(isMaximized()));

			JsonWriter jw;
			jw.WriteInFile("appdata/applicationSettings.json", jRoot, true);

			SGEImGui::destroy();
			sgeLogInfo("Destroy Called!");
		}

		if (event == WE_FileDrop) {
			// If a file has been droped on the application, open the asset import dialogue.
			const WE_FileDrop_Data& filedropData = *static_cast<const WE_FileDrop_Data*>(eventData);

			AssetsWindow* wnd = getEngineGlobal()->findFirstWindowOfType<AssetsWindow>();
			if (wnd) {
				ImGui::SetWindowFocus(wnd->getWindowName());
			}
			else {
				wnd = new AssetsWindow(ICON_FK_FILE " Assets");
				getEngineGlobal()->addWindow(wnd);
			}

			wnd->openAssetImportPopUp_importFile(filedropData.filename);
		}
	}

	void OnCreate()
	{
		int const backBufferWidth = this->GetClientWidth();
		int const backBufferHeight = this->GetClientHeight();

		MainFrameTargetDesc mainTargetDesc;
		mainTargetDesc.width = backBufferWidth;
		mainTargetDesc.height = backBufferHeight;
		mainTargetDesc.numBuffers = 2;
		mainTargetDesc.vSync = true;
		mainTargetDesc.sampleDesc = SampleDesc(1);
#ifdef _WIN32
		mainTargetDesc.hWindow = GetNativeHandle();
		mainTargetDesc.bWindowed = true;
#endif

		// Obtain the backbuffer render target initialize the device and the immediate context.
		SGEDevice* const device = SGEDevice::create(mainTargetDesc);

		SGEImGui::initialize(
		    device->getContext(), device->getWindowFrameTarget(), device->getWindowFrameTarget()->getViewport());
		ImGui::SetCurrentContext(getImGuiContextCore());
		setImGuiContextEngine(getImGuiContextCore());

		// Setup Audio device.
		AudioDevice* const audioDevice = new AudioDevice();
		audioDevice->createAudioDevice();
		audioDevice->startAudioDevice();

		getCore()->setup(device, audioDevice);
		getCore()->getAssetLib()->scanForAvailableAssets("assets");

		// Find the game plugin dll filename.
		for (auto const& entry : std::filesystem::directory_iterator("./")) {
			if (std::filesystem::is_regular_file(entry) && entry.path().extension() == ".gll") {
				pluginFileName = entry.path().string();
			}
		}
		typeLib().performRegistration();
		createAndInitializeEngineGlobal();

		if (pluginFileName.empty()) {
			getEngineGlobal()->changeActivePlugin(&dummyPlugin);
		}
		else {
			loadPlugin();
		}

		getEngineGlobal()->addWindow(new EditorWindow(*this, "Editor Window Main"));
	}

	void loadPlugin()
	{
		const sint64 modtime = FileReadStream::getFileModTime(pluginFileName.c_str());

		if (!pluginFileName.empty() && (modtime > m_workingDLLModTime || m_workingDLLModTime == 0)) {
#if 1
			 if (m_workingDLLModTime != 0) {
				DialogYesNo("Realod DLL", "Game DLL is about to be reloaded!");
			}
#endif

			// Save the editor world as we are going to invaludated all the game function pointers
			//  by unloading the game DLL.
			// Save the editor state into a file in case the hot-reload-crashes.
			bool isDoingHotReload = false;
			SceneInstanceSerializedData hotReloadEditorData;
			std::string workingFilename;
			if (m_pluginInst) {
				isDoingHotReload = true;
				// Notify that we are about to unload the plugin.
				getEngineGlobal()->notifyOnPluginPreUnload();

				getEngineGlobal()->getEditorWindow()->prepareForHotReload(hotReloadEditorData);
				hotReloadEditorData.saveInDirectory("hot_reaload_backup");

				m_pluginInst->onUnload();
				delete m_pluginInst;
				m_pluginInst = nullptr;
			}

			// Unload the old plugin DLL and load the new one.
			m_dllHandler.unload();

			// Hack: Sleep for a bit for the OS to have time to register that nobody references the old
			// working_plugin.dll.
			std::this_thread::sleep_for(std::chrono::milliseconds(100));

			copyFile(pluginFileName.c_str(), "working_plugin.dll");
			m_dllHandler.load("working_plugin.dll");
			m_workingDLLModTime = modtime;

			// Obain the new plugin interop interface.
			GetInpteropFnPtr interopGetter =
			    reinterpret_cast<GetInpteropFnPtr>(m_dllHandler.getProcAdress("getInterop"));
			sgeAssert(interopGetter != nullptr && "The loaded game dll does not have a getInterop()!\n");
			if (interopGetter) {
				m_pluginInst = interopGetter();
			}
			else {
				sgeAssert(false);
				sgeLogError("The loaded game dll does not have a getInterop()!");
			}
			getEngineGlobal()->changeActivePlugin(m_pluginInst);

			if (m_pluginInst) {
				m_pluginInst->onLoaded(SgeGlobalSingletons());

				typeLib().performRegistration();
				if (isDoingHotReload) {
					getEngineGlobal()->getEditorWindow()->recoverFromHotReload(hotReloadEditorData);
				}
				sgeLogCheck("Reloaded %s\n", pluginFileName.c_str());
			}
		}
	}

	void run()
	{
		m_timer.tick();
		getEngineGlobal()->update(m_timer.diff_seconds());

		loadPlugin();

		if (getEngineGlobal()->getEngineAllowingRelativeCursor() &&
		    getEngineGlobal()->doesAnyoneNeedForRelativeCursorThisFrame()) {
			setMouseCursorRelative(true);
		}
		else {
			setMouseCursorRelative(false);
		}
		getEngineGlobal()->clearAnyoneNeedForRelativeCursorThisFrame();

		SGEImGui::newFrame(GetInputState());

		SGEContext* const sgecon = getCore()->getDevice()->getContext();
		float const bgColor[] = {0.f, 0.f, 0.f, 1.f};
		sgecon->clearColor(getCore()->getDevice()->getWindowFrameTarget(), -1, bgColor);
		sgecon->clearDepth(getCore()->getDevice()->getWindowFrameTarget(), 1.f);

		if (m_pluginInst) {
			m_pluginInst->run();
		}

		getEngineGlobal()->getEditorWindow()->update(sgecon, nullptr, GetInputState());

		// Render the ImGui User Interface.
		SGEImGui::render();

		// Finally display everyting to the screen.
		getCore()->setLastFrameStatistics(getCore()->getDevice()->getFrameStatistics());
		getCore()->getDevice()->present();

		getCore()->getAssetLib()->reloadAssets();

		return;
	}
};

void sgeMainLoop()
{
	sge::ApplicationHandler::get()->PollEvents();
	for (WindowBase* wnd : sge::ApplicationHandler::get()->getAllWindows()) {
		SGEGameWindow* gameWindow = dynamic_cast<SGEGameWindow*>(wnd);
		if (gameWindow) {
			gameWindow->run();
		}
	}
}

int sge_main(int argc, char** argv)
{
	sgeLogInfo("sge_main()\n");

	// Force the numeric locale to use "." for demical numbers as
	// we have calls like atof scanf that so on that rely on this.
	setlocale(LC_NUMERIC, "C");

	g_argc = argc;
	g_argv = argv;

	FileReadStream frs("appdata/applicationSettings.json");

	vec2i windowSize(1300, 700);
	bool isMaximized = true;

	JsonParser jp;
	if (frs.isOpened() && jp.parse(&frs)) {
		windowSize.x = jp.getRoot()->getMember("window_width")->getNumberAs<int>();
		windowSize.y = jp.getRoot()->getMember("window_height")->getNumberAs<int>();
		isMaximized = jp.getRoot()->getMember("is_maximized")->getAsBool();
		frs.close();
	}

	sge::ApplicationHandler::get()->NewWindow<SGEGameWindow>(
	    "SGEEngine Editor", windowSize.x, windowSize.y, isMaximized);

#ifdef __EMSCRIPTEN__
	// In web browsers we cannot loop ourselves as we are going to make the page freeze.
	// Emscripten offer us a way to get a function being called in a loop by the browser
	// so it doesn't freeze.
	emscripten_set_main_loop(sgeMainLoop, 0, true);
#else
	while (sge::ApplicationHandler::get()->shouldStopRunning() == false) {
		sgeMainLoop();
	};
#endif

	return 0;
}

#ifdef __EMSCRIPTEN__
	#include <SDL2/SDL.h>
	#include <emscripten.h>
//#include <GLES3/gl3.h> // WebGL2 + GLES 3 emulation.
#else
	#include <SDL.h>
	#include <SDL_syswm.h>
#endif

// Caution:
// SDL2 might have a macro (depending on the target platform) for the main function!
int main(int argc, char* argv[])
{
	sgeLogInfo("main()\n");
	sgeRegisterMiniDumpHandler();

	// Disable the mouse from generating fake touch events.
	SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");

	SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "1");

#ifdef __EMSCRIPTEN__
	SDL_Init(SDL_INIT_VIDEO);
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
	SDL_Init(SDL_INIT_EVERYTHING);
	#ifdef SGE_RENDERER_D3D11
		// SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
	#else if SGE_RENDERER_GL
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	float ddpi = 0.f;
	SDL_GetDisplayDPI(0, &ddpi, nullptr, nullptr);

	// SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
	// SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
	// SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
	// SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	// SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
		// SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	#endif
#endif

	return sge_main(argc, argv);
}
