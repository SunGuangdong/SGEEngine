#pragma once

#include "sge_engine_api.h"
#include <string>

#include "sge_core/ICore.h"
#include "sge_core/SGEImGui.h"
#include "sge_log/Log.h"

struct ImGuiContext;

namespace sge {

struct ICore;
struct IGameDrawer;
struct Log;

/// SgeGlobalSingletons as a lot of SGE libaries can be built with as DLLs
/// and some of them provide a global statate that we want to be unique and as
/// the game could be loaded dynamically (for hot reloading) we need a way
/// to share these global state between all DLLs.
/// This structure provides a way for the sge_editor or the sge_player (the standlaone final game)
/// to tell the game dll these singleton state owners so the games could use them.
///
/// Caution: this fuction needs to be implemented in a header so the caller game plugin
/// could see it and apply it on its own global state pointers.
struct SgeGlobalSingletons {
	ImGuiContext* imguiCtx = nullptr;
	ICore* global = nullptr;
	Log* log = nullptr;

	/// Captures global state of SGE in the current DLL (that calls this constructor).
	SgeGlobalSingletons() {
		imguiCtx = ImGui::GetCurrentContext();
		global = getCore();
		log = getLog();
	}

	/// Applies the global state of SGE captured in some DLL (usually the sge_editor or sge_player)
	/// and applies is in the current one (usually the game).
	void applyGlobalState() const {
		sgeAssert(imguiCtx);
		ImGui::SetCurrentContext(imguiCtx);

		setCore(global);
		setLog(log);
	}
};

struct SGE_ENGINE_API IPlugin {
	virtual ~IPlugin() {}

	virtual void onLoaded(const SgeGlobalSingletons& sgeSingletons) = 0;
	virtual void onUnload() = 0;
	virtual void run() = 0;
};

typedef IPlugin* (*GetInpteropFnPtr)();

} // namespace sge
