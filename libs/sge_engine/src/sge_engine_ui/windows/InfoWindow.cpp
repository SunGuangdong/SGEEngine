#include "sge_utils/sge_utils.h"

#ifdef WINAPI_FAMILY_DESKTOP_APP
	#define NOMINMAX
	#define WIN32_LEAN_AND_MEAN
	#include <Psapi.h>
	#include <Windows.h>
#endif

#include "sge_core/SGEImGui.h"

#include "sge_core/AssetLibrary/AssetLibrary.h"
#include "sge_renderer/renderer/renderer.h"

#include "InfoWindow.h"

#include "sge_core/ICore.h"

namespace sge {

void InfoWindow::update(SGEContext* const UNUSED(sgecon), struct GameInspector* UNUSED(inspector), const InputState& UNUSED(is))
{
	if (isClosed()) {
		return;
	}

	if (ImGui::Begin(m_windowName.c_str(), &m_isOpened)) {
		const FrameStatistics& framestats = getCore()->getLastFrameStatistics();

#ifdef WINAPI_FAMILY_DESKTOP_APP
		PROCESS_MEMORY_COUNTERS memCounter;
		if (GetProcessMemoryInfo(GetCurrentProcess(), &memCounter, sizeof(memCounter))) {
			unsigned mb = (unsigned)((memCounter.WorkingSetSize / 1024ull) / 1024ull);
			unsigned kb = (unsigned)(memCounter.WorkingSetSize / 1024ull);
			ImGui::Value("Used Memory(MB)", mb);
			ImGui::Value("Used Memory(KB)", kb);
		}
#endif

		float fps = framestats.lastPresentDt > 1e-6f ? 1.f / framestats.lastPresentDt : 0.f;

		ImGui::DragFloat("FPS", &fps, 1.f, 0.f, 0.f, "%.1f");

		ImGui::Value("Draw Calls Count", framestats.numDrawCalls);
		ImGui::Value("Primitives Count", (int)framestats.numPrimitiveDrawn);
		ImGui::Value("VSync Enabled", getCore()->getDevice()->getVsync());

		SGEDevice* const sgedev = getCore()->getDevice();

		if (ImGui::CollapsingHeader("Vertex Declarations")) {
			for (const auto& declPair : sgedev->getVertexDeclMap()) {
				ImGui::Text("Declaration idx=%d, size=%d", declPair.second, declPair.first.size());

				for (const VertexDecl& decl : declPair.first) {
					ImGui::Text("\tslot=%d %s offset=%d fmt=%d", decl.bufferSlot, decl.semantic.c_str(), decl.byteOffset, decl.format);
				}

				ImGui::Separator();
			}
		}
	}
	ImGui::End();
}

} // namespace sge
