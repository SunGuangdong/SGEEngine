#pragma once

#include "sge_engine/IPlugin.h"
#include "sge_utils/sge_utils.h"

namespace sge {
struct DummyPlugin final : public IPlugin {
	IGameDrawer* allocateGameDrawer() override;
	void onLoaded(const SgeGlobalSingletons& UNUSED(sgeSingletons)) override {}
	void onUnload() override {}
	void run() override {}
};

} // namespace sge
