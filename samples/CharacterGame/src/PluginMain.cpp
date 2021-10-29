#include "sge_core/ICore.h"
#include "sge_core/SGEImGui.h"
#include "sge_core/shaders/modeldraw.h"
#include "sge_engine/GameDrawer/DefaultGameDrawer.h"
#include "sge_engine/IPlugin.h"

#include "GlobalRandom.h"

// clang-format off
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
EM_JS(bool, sge_emscripten_isMobile, (), {
	return (/android|webos|iphone|ipad|ipod|blackberry|iemobile|opera mini/i.test(navigator.userAgent.toLowerCase()));
});
#else
bool sge_emscripten_isMobile() {
	return false;
}
#endif
// clang-format on

namespace sge {
struct PluginGame final : public IPlugin {
	virtual IGameDrawer* allocateGameDrawer() { return new DefaultGameDrawer(); }

	void onLoaded(ImGuiContext* imguiCtx, ICore* global) override {
		ImGui::SetCurrentContext(imguiCtx);
		setCore(global);

		isMobileEmscripten = sge_emscripten_isMobile();

		if (isMobileEmscripten == false) {
			bgMusic.createDecoder(getCore()->getAssetLib()->getAsset("assets/snd/song18.mp3", true)->asAudio()->audioData);

			bgMusic.state.isLooping = true;
			bgMusic.state.volume = 0.5f;

			getCore()->getAudioDevice()->play(&bgMusic, true);


			sndPickUp.createDecoder(getCore()->getAssetLib()->getAsset("assets/snd/Powerup6.wav", true)->asAudio()->audioData);

			sndPickUp.state.volume = 0.20f;

			sndHurt.createDecoder(getCore()->getAssetLib()->getAsset("assets/snd/Hit_Hurt7.wav", true)->asAudio()->audioData);
			sndHurt.state.volume = 0.30f;
		}
	}

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
