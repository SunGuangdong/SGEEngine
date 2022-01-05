#pragma once

#include "sge_engine/sge_engine_api.h"

namespace sge {

struct SGEContext;
struct InputState;
struct GameInspector;

struct SGE_ENGINE_API IImGuiWindow {
	IImGuiWindow() = default;
	virtual ~IImGuiWindow() = default;

	virtual bool isClosed() = 0;
	virtual void update(SGEContext* const sgecon, struct GameInspector* inspector, const InputState& is) = 0;
	virtual const char* getWindowName() const = 0;
};

} // namespace sge
