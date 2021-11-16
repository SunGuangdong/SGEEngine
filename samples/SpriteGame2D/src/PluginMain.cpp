#include "sge_core/ICore.h"
#include "sge_core/SGEImGui.h"
#include "sge_engine/GameDrawer/DefaultGameDrawer.h"
#include "sge_engine/IPlugin.h"

namespace sge {
struct PluginGame final : public IPlugin {
	virtual IGameDrawer* allocateGameDrawer() { return new DefaultGameDrawer(); }

	void onLoaded(const SgeGlobalSingletons& sgeSingletons) override { sgeSingletons.applyGlobalState(); }

	void onUnload() {}
	void run() {}
};
} // namespace sge

#if !defined(__EMSCRIPTEN__)
extern "C" {
#ifdef WIN32
__declspec(dllexport) sge::IPlugin* getInterop() {
	return new sge::PluginGame();
}
#else
__attribute__((visibility("default"))) sge::IPlugin* getInterop() {
	return new sge::PluginGame();
}
#endif
}
#else
sge::IPlugin* getInterop() {
	return new sge::PluginGame();
}
#endif
