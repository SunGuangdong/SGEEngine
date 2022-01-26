#pragma once

#include "../IImGuiWindow.h"
#include "sge_core/AssetLibrary/AssetLibrary.h"

namespace sge {

struct GameInspector;

struct SGE_ENGINE_API MaterialEditWindow : public IImGuiWindow {
	MaterialEditWindow(std::string windowName);
	bool isClosed() override {
		return !m_isOpened;
	}

	const char* getWindowName() const override {
		return m_windowName.c_str();
	}

	void update(SGEContext* const sgecon, GameInspector* inspector, const InputState& is) override;

	void setAsset(std::shared_ptr<AssetIface_Material> newMtlAsset) {
		mtlProvider = newMtlAsset;
	}

  private:
	bool m_isOpened = true;
	std::string m_windowName;
	std::shared_ptr<AssetIface_Material> mtlProvider;
};

} // namespace sge
