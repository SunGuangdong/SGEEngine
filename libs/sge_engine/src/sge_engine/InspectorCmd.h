#pragma once

#include <string>

namespace sge {

struct GameInspector;

struct SGE_ENGINE_API InspectorCmd {
	InspectorCmd() = default;
	virtual ~InspectorCmd() = default;

	virtual void apply(GameInspector* inspector) = 0;
	virtual void redo(GameInspector* inspector) = 0;
	virtual void undo(GameInspector* inspector) = 0;

	virtual void getText(std::string& text) { text = "<command>"; }
};

} // namespace sge
