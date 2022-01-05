#pragma once

#include "IImGuiWindow.h"
#include "imgui/imgui.h"
#include "sge_core/DebugDraw.h"


namespace sge {

struct InputState;
struct IGameDrawer;

/// SceneWindow is the window that is used to view and edit the scene in 3D.
struct SGE_ENGINE_API SceneWindow : public IImGuiWindow {
	SceneWindow(std::string windowName, IGameDrawer* gameDrawer)
	    : m_windowName(std::move(windowName))
	    , m_gameDrawer(gameDrawer) {}

	void setGameDrawer(IGameDrawer* gd) { m_gameDrawer = gd; }

	bool isClosed() override { return false; }
	void update(SGEContext* const sgecon, struct GameInspector* UNUSED(inspector), const InputState& is) override;
	const char* getWindowName() const override { return m_windowName.c_str(); }

	InputState computeInputStateInDomainSpace(const InputState& isOriginal) const;

  private:
	bool updateToolsAndOverlay(const InputState& is, const GameDrawSets& drawSets);
	void updateRightClickMenu(bool canOpen);
	void drawOverlay(const GameDrawSets& drawSets);

  private:
	GpuHandle<FrameTarget> m_frameTarget;

	IGameDrawer* m_gameDrawer;
	std::string m_windowName;

	vec2f m_canvasPos = vec2f(0.f);
	vec2f m_canvasSize = vec2f(0.f);
};

} // namespace sge
