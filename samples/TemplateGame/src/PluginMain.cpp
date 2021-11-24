#include "sge_core/ICore.h"
#include "sge_core/SGEImGui.h"
#include "sge_engine/GameDrawer/DefaultGameDrawer.h"
#include "sge_engine/IPlugin.h"

namespace sge {
struct PluginGame final : public IPlugin {
	void onLoaded(const SgeGlobalSingletons& sgeSingletons) override { sgeSingletons.applyGlobalState(); }

	void onUnload() {}
	void run() {}
};
} // namespace sge

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
