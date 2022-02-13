#include "setImGuiContexCore.h"

ImGuiContext* getImGuiContextCore()
{
	return ImGui::GetCurrentContext();
}

void setImGuiContextCore(ImGuiContext* g)
{
	ImGui::SetCurrentContext(g);
}
